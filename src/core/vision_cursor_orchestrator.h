#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "common/common.h"
#include "config/structs.h"
#include "interaction/interaction_controller.h"
#include "mapping/action_mapper.h"
#include "mediapipe/mediapipe_hand_detector.h"
#include "vision/camera_manager.h"

namespace VisionCursor
{
namespace Core
{

struct OrchestratorOutput
{
    FramePtr frame;
    HandLandmarkArray processed_landmarks;
    Action action;
    bool has_hand = false;
    bool current_ok = false;
};

class VisionCursorOrchestrator
{
public:
    VisionCursorOrchestrator() = default;
    ~VisionCursorOrchestrator();

    VisionCursorOrchestrator(const VisionCursorOrchestrator&) = delete;
    VisionCursorOrchestrator& operator=(const VisionCursorOrchestrator&) = delete;

public:
    bool configure(const Config::OrchestratorConfig& config);

    bool init();
    bool clear();

    bool start();
    bool stop();

public:
    bool isInited() const { return inited_.load(); }
    bool isRunning() const { return running_.load(); }

    const Config::OrchestratorConfig& config() const { return config_; }
    OrchestratorOutput lastOutput() const;

    // Replacement of waitAndGetFrame: wait and return the latest orchestrator output.
    OrchestratorOutput getOutput(int timeout_ms = -1);

public:
    bool updateCameraDevices();
    std::vector<std::string> allCameraDeviceNames() const;

    bool setCameraDevice(const std::string& device_name);
    bool getCameraInfo(CameraInfo& camera_info);
    CameraInfo getCameraInfo(const std::string& device_name);

public:
    void setMediapipeEnabled(bool enabled) { mediapipe_enabled_.store(enabled); }
    bool isMediapipeEnabled() const { return mediapipe_enabled_.load(); }

    void setMouseControlEnabled(bool enabled) { mouse_control_enabled_.store(enabled); }
    bool isMouseControlEnabled() const { return mouse_control_enabled_.load(); }

private:
    bool initCamera();
    bool initDetector();
    bool initMapper();
    bool initInteraction();

    bool openCamera();
    bool closeCamera();

    bool processOneFrame();
    void runLoop();

    void resetOutput();
    void updateOutput(const FramePtr& frame,
                      const HandLandmarkArray& landmarks,
                      const Action& action,
                      bool has_hand,
                      bool ok);

private:
    Config::OrchestratorConfig config_{};

    mutable std::mutex output_mutex_;
    std::condition_variable output_cv_;
    uint64_t output_seq_ = 0;
    OrchestratorOutput last_output_{};

    Vision::CameraManager camera_manager_;
    Vision::CameraDevice* camera_device_ = nullptr;

    MediaPipe::MediaPipeHandDetector hand_detector_;
    Interaction::InteractionController interaction_controller_;
    Mapping::ActionMapper action_mapper_;

    std::thread worker_thread_;

    std::atomic<bool> inited_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> detector_opened_{false};

    std::atomic<bool> mediapipe_enabled_{true};
    std::atomic<bool> mouse_control_enabled_{true};
};

} // namespace Core
} // namespace VisionCursor
