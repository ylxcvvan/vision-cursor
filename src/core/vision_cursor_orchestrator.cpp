#include "core/vision_cursor_orchestrator.h"

#include <chrono>
#include <exception>

#include "log/logging.h"

namespace VisionCursor
{
namespace Core
{

VisionCursorOrchestrator::~VisionCursorOrchestrator()
{
    clear();
}

bool VisionCursorOrchestrator::configure(const Config::OrchestratorConfig& config)
{
    if (running_.load())
    {
        LOG(ERROR) << "Cannot configure orchestrator while running.";
        return false;
    }

    config_ = config;

    if (!updateCameraDevices())
    {
        return false;
    }

    const std::vector<std::string> names = allCameraDeviceNames();
    if (names.empty())
    {
        LOG(ERROR) << "No available camera devices.";
        return false;
    }

    std::string selected_name = config_.camera.camera_device_name;
    if (selected_name.empty())
    {
        selected_name = names.front();
    }

    Vision::CameraDevice* selected = camera_manager_.getPointerToDevice(selected_name);
    if (selected == nullptr)
    {
        selected_name = names.front();
        selected = camera_manager_.getPointerToDevice(selected_name);
        if (selected == nullptr)
        {
            LOG(ERROR) << "Failed to select camera device.";
            return false;
        }
    }

    camera_device_ = selected;
    config_.camera.camera_device_name = selected_name;

    if (!camera_device_->refreshCameraInfo())
    {
        LOG(WARNING) << "Failed to refresh camera info before configure. device=" << selected_name;
    }

    if (camera_device_->configure(config_.camera.camera_resolution,
                                  config_.camera.camera_fps,
                                  config_.camera.is_mirror))
    {
        LOG(INFO) << "Orchestrator configured camera: " << selected_name << " "
                  << config_.camera.camera_resolution.width << "x" << config_.camera.camera_resolution.height
                  << " @" << config_.camera.camera_fps << "fps";
        return true;
    }

    CameraInfo info;
    if (!camera_device_->getCameraInfo(info))
    {
        LOG(ERROR) << "Failed to get camera info for fallback configure.";
        return false;
    }

    if (info.set_supported_resolution.empty())
    {
        LOG(ERROR) << "Camera reports no supported resolutions for fallback configure.";
        return false;
    }

    config_.camera.camera_resolution = *info.set_supported_resolution.begin();
    config_.camera.camera_fps = DEFAULT_CAMERA_FPS;

    if (!camera_device_->configure(config_.camera.camera_resolution,
                                   config_.camera.camera_fps,
                                   config_.camera.is_mirror))
    {
        LOG(ERROR) << "Fallback camera configure failed."
                   << " resolution=" << config_.camera.camera_resolution.width << "x"
                   << config_.camera.camera_resolution.height << " fps=" << config_.camera.camera_fps;
        return false;
    }

    LOG(INFO) << "Orchestrator configured camera with fallback: " << selected_name << " "
              << config_.camera.camera_resolution.width << "x" << config_.camera.camera_resolution.height
              << " @" << config_.camera.camera_fps << "fps";
    return true;
}

bool VisionCursorOrchestrator::init()
{
    if (inited_.load())
    {
        return true;
    }

    if (!initCamera())
    {
        return false;
    }

    if (!initDetector())
    {
        return false;
    }

    if (!initInteraction())
    {
        return false;
    }

    if (!initMapper())
    {
        return false;
    }

    resetOutput();
    inited_.store(true);
    LOG(INFO) << "Orchestrator initialized successfully.";
    return true;
}

bool VisionCursorOrchestrator::clear()
{
    bool ok = true;

    ok = stop() && ok;
    ok = closeCamera() && ok;

    if (detector_opened_.load())
    {
        ok = hand_detector_.close() && ok;
        detector_opened_.store(false);
    }

    ok = interaction_controller_.clear() && ok;

    inited_.store(false);
    camera_device_ = nullptr;

    resetOutput();
    LOG(INFO) << "Orchestrator cleared.";
    return ok;
}

bool VisionCursorOrchestrator::start()
{
    if (!inited_.load())
    {
        LOG(ERROR) << "Cannot start orchestrator before init.";
        return false;
    }

    if (running_.load())
    {
        LOG(ERROR) << "Orchestrator is already running.";
        return false;
    }

    if (worker_thread_.joinable())
    {
        worker_thread_.join();
    }

    if (!openCamera())
    {
        return false;
    }

    running_.store(true);

    try
    {
        worker_thread_ = std::thread(&VisionCursorOrchestrator::runLoop, this);
    }
    catch (const std::exception& ex)
    {
        running_.store(false);
        closeCamera();
        LOG(ERROR) << "Failed to create orchestrator worker thread: " << ex.what();
        return false;
    }
    catch (...)
    {
        running_.store(false);
        closeCamera();
        LOG(ERROR) << "Failed to create orchestrator worker thread: unknown exception.";
        return false;
    }

    LOG(INFO) << "Orchestrator started.";
    return true;
}

bool VisionCursorOrchestrator::stop()
{
    running_.store(false);
    output_cv_.notify_all();

    if (worker_thread_.joinable())
    {
        worker_thread_.join();
    }

    LOG(INFO) << "Orchestrator stopped.";
    return true;
}

OrchestratorOutput VisionCursorOrchestrator::lastOutput() const
{
    std::lock_guard<std::mutex> lock(output_mutex_);
    return last_output_;
}

OrchestratorOutput VisionCursorOrchestrator::getOutput(int timeout_ms)
{
    std::unique_lock<std::mutex> lock(output_mutex_);
    const uint64_t base_seq = output_seq_;

    auto has_new_output_or_stopped = [this, base_seq]() {
        return output_seq_ != base_seq || !running_.load();
    };

    if (timeout_ms < 0)
    {
        output_cv_.wait(lock, has_new_output_or_stopped);
    }
    else
    {
        output_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), has_new_output_or_stopped);
    }

    return last_output_;
}

bool VisionCursorOrchestrator::updateCameraDevices()
{
    camera_manager_.updateAllDevice();
    return true;
}

std::vector<std::string> VisionCursorOrchestrator::allCameraDeviceNames() const
{
    return camera_manager_.getAllDeviceName();
}

bool VisionCursorOrchestrator::setCameraDevice(const std::string& device_name)
{
    if (running_.load())
    {
        LOG(ERROR) << "Cannot switch camera while orchestrator is running.";
        return false;
    }

    Vision::CameraDevice* device = camera_manager_.getPointerToDevice(device_name);
    if (device == nullptr)
    {
        LOG(ERROR) << "Failed to select camera device: " << device_name;
        return false;
    }

    camera_device_ = device;
    config_.camera.camera_device_name = device_name;
    camera_device_->refreshCameraInfo();
    LOG(INFO) << "Switched camera device to: " << device_name;

    return true;
}

bool VisionCursorOrchestrator::getCameraInfo(CameraInfo& camera_info)
{
    if (camera_device_ == nullptr)
    {
        LOG(ERROR) << "Camera device is null.";
        return false;
    }

    if (!camera_device_->refreshCameraInfo())
    {
        LOG(WARNING) << "Failed to refresh current camera info before query.";
    }

    return camera_device_->getCameraInfo(camera_info);
}

CameraInfo VisionCursorOrchestrator::getCameraInfo(const std::string& device_name)
{
    CameraInfo info{};

    Vision::CameraDevice* device = camera_manager_.getPointerToDevice(device_name);
    if (device == nullptr)
    {
        LOG(ERROR) << "Failed to query camera info, unknown device: " << device_name;
        return info;
    }

    // Ensure latest supported resolution list before reading camera info for UI.
    if (!device->refreshCameraInfo())
    {
        LOG(WARNING) << "Failed to refresh camera info before query. device=" << device_name;
    }

    if (!device->getCameraInfo(info))
    {
        LOG(ERROR) << "Failed to get camera info for device: " << device_name;
    }

    return info;
}

bool VisionCursorOrchestrator::initCamera()
{
    if (camera_device_ == nullptr)
    {
        LOG(ERROR) << "Camera device is null. Call configure first.";
        return false;
    }

    return camera_device_->configure(config_.camera.camera_resolution,
                                     config_.camera.camera_fps,
                                     config_.camera.is_mirror);
}

bool VisionCursorOrchestrator::initDetector()
{
    if (detector_opened_.load())
    {
        hand_detector_.close();
        detector_opened_.store(false);
    }

    if (!hand_detector_.open(Paths::mediapipeDir(),
                             config_.mediapipe.min_score_thresh,
                             config_.mediapipe.threshold,
                             config_.mediapipe.model_complexity,
                             config_.mediapipe.use_prev_landmarks))
    {
        return false;
    }

    detector_opened_.store(true);
    LOG(INFO) << "Mediapipe detector initialized.";
    return true;
}

bool VisionCursorOrchestrator::initMapper()
{
    const bool ok = action_mapper_.open(config_.mapper.x_sensitivity,
                                        config_.mapper.y_sensitivity,
                                        config_.mapper.scroll_sensitivity,
                                        config_.mapper.move_mode,
                                        config_.mapper.region);
    if (ok)
    {
        LOG(INFO) << "Action mapper initialized.";
    }
    return ok;
}

bool VisionCursorOrchestrator::initInteraction()
{
    const bool ok = interaction_controller_.init(config_.interaction.scheme_name,
                                                 config_.interaction.filter_name,
                                                 config_.interaction.vec_filter_param,
                                                 config_.interaction.control_joint);
    if (ok)
    {
        LOG(INFO) << "Interaction controller initialized with scheme=" << config_.interaction.scheme_name;
    }
    return ok;
}

bool VisionCursorOrchestrator::openCamera()
{
    if (camera_device_ == nullptr)
    {
        LOG(ERROR) << "Camera device is null.";
        return false;
    }

    if (!camera_device_->isRunning() && !camera_device_->open())
    {
        return false;
    }

    const bool ok = camera_device_->applyConfiguration();
    if (ok)
    {
        LOG(INFO) << "Camera opened and configuration applied.";
    }
    return ok;
}

bool VisionCursorOrchestrator::closeCamera()
{
    if (camera_device_ == nullptr)
    {
        return true;
    }

    if (!camera_device_->isRunning())
    {
        return true;
    }

    return camera_device_->close();
}

bool VisionCursorOrchestrator::processOneFrame()
{
    auto publish = [this](const FramePtr& frame,
                          const HandLandmarkArray& landmarks,
                          const Action& action,
                          bool has_hand,
                          bool ok) {
        updateOutput(frame, landmarks, action, has_hand, ok);
        return ok;
    };

    if (camera_device_ == nullptr)
    {
        LOG(ERROR) << "Camera device is null in processOneFrame.";
        return publish(FramePtr{}, HandLandmarkArray{}, Action{}, false, false);
    }

    FramePtr frame;
    if (!camera_device_->capture(frame))
    {
        return publish(FramePtr{}, HandLandmarkArray{}, Action{}, false, false);
    }

    if (!frame || frame->getData() == nullptr || frame->getWidth() <= 0 || frame->getHeight() <= 0)
    {
        LOG(ERROR) << "Captured frame is invalid.";
        return publish(FramePtr{}, HandLandmarkArray{}, Action{}, false, false);
    }

    if (!mediapipe_enabled_.load())
    {
        return publish(frame, HandLandmarkArray{}, Action{}, false, true);
    }

    HandLandmarkArray landmarks;
    if (!hand_detector_.process(frame, landmarks))
    {
        return publish(frame, HandLandmarkArray{}, Action{}, false, false);
    }

    if (!landmarks.isValid())
    {
        return publish(frame, landmarks, Action{}, false, true);
    }

    Action action;
    if (!interaction_controller_.process(landmarks, action))
    {
        return publish(frame, landmarks, Action{}, true, false);
    }

    if (!mouse_control_enabled_.load())
    {
        return publish(frame, landmarks, action, true, true);
    }

    const bool map_ok = action_mapper_.mapAndExecute(action);
    return publish(frame, landmarks, action, true, map_ok);
}

void VisionCursorOrchestrator::runLoop()
{
    while (running_.load())
    {
        processOneFrame();
    }
}

void VisionCursorOrchestrator::resetOutput()
{
    std::lock_guard<std::mutex> lock(output_mutex_);
    last_output_ = OrchestratorOutput{};
    output_seq_ = 0;
}

void VisionCursorOrchestrator::updateOutput(const FramePtr& frame,
                                            const HandLandmarkArray& landmarks,
                                            const Action& action,
                                            bool has_hand,
                                            bool ok)
{
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        last_output_.frame = frame;
        last_output_.processed_landmarks = landmarks;
        last_output_.action = action;
        last_output_.has_hand = has_hand;
        last_output_.current_ok = ok;
        ++output_seq_;
    }

    output_cv_.notify_all();
}

} // namespace Core
} // namespace VisionCursor
