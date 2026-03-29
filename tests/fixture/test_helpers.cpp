#include "test_helpers.h"

#include <cmath>
#include <filesystem>

#include <opencv2/opencv.hpp>

#include "vision/opencv_frame.h"

namespace VisionCursor
{
namespace Tests
{
namespace
{
namespace fs = std::filesystem;

bool isTrainingDataRoot(const fs::path& dir)
{
    const fs::path graph_file =
        dir / "mediapipe" / "modules" / "hand_landmark" / "hand_landmark_tracking_cpu.binarypb";
    return fs::exists(graph_file);
}

void setReason(std::string* reason, const std::string& value)
{
    if (reason != nullptr)
    {
        *reason = value;
    }
}

} // namespace

QCoreApplication* ensureCoreApp()
{
    if (QCoreApplication::instance() != nullptr)
    {
        return QCoreApplication::instance();
    }

    static int argc = 1;
    static char arg0[] = "vc_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication app(argc, argv);

    return &app;
}

std::string findTrainingDataDir()
{
    fs::path current = fs::current_path();

    for (int level = 0; level < 8; ++level)
    {
        const fs::path candidate = current / "training_data";
        if (isTrainingDataRoot(candidate))
        {
            return candidate.string();
        }

        if (!current.has_parent_path())
        {
            break;
        }
        current = current.parent_path();
    }

    return {};
}

bool hasTrainingData()
{
    return !findTrainingDataDir().empty();
}

bool tryGetFirstCamera(Vision::CameraManager& manager, Vision::CameraDevice*& device, std::string* reason)
{
    ensureCoreApp();

    device = nullptr;
    manager.updateAllDevice();

    const auto names = manager.getAllDeviceName();
    if (names.empty())
    {
        setReason(reason, "no camera device found");
        return false;
    }

    device = manager.getPointerToDevice(names.front());
    if (device == nullptr)
    {
        setReason(reason, "failed to get pointer to first camera");
        return false;
    }

    return true;
}

bool tryOpenConfiguredCamera(Vision::CameraDevice& device,
                             const CameraResolution& preferred_resolution,
                             int preferred_fps,
                             std::string* reason)
{
    ensureCoreApp();

    if (!device.configure(preferred_resolution, preferred_fps))
    {
        CameraInfo info;
        if (!device.getCameraInfo(info))
        {
            setReason(reason, "configure failed and getCameraInfo failed");
            return false;
        }

        if (info.set_supported_resolution.empty() && !device.refreshCameraInfo())
        {
            setReason(reason, "configure failed and refreshCameraInfo failed");
            return false;
        }

        info.set_supported_resolution.clear();
        if (!device.getCameraInfo(info) || info.set_supported_resolution.empty())
        {
            setReason(reason, "camera has no supported resolution for fallback configure");
            return false;
        }

        const CameraResolution fallback = *info.set_supported_resolution.begin();
        if (!device.configure(fallback, preferred_fps))
        {
            setReason(reason, "fallback configure failed");
            return false;
        }
    }

    const bool was_running = device.isRunning();
    if (!was_running && !device.open())
    {
        setReason(reason, "open failed");
        return false;
    }

    if (!device.applyConfiguration())
    {
        if (!was_running)
        {
            device.close();
        }
        setReason(reason, "applyConfiguration failed");
        return false;
    }

    return true;
}

bool tryOpenCameraAndDetector(Vision::CameraManager& manager,
                              Vision::CameraDevice*& device,
                              MediaPipe::MediaPipeHandDetector& detector,
                              std::string* reason,
                              const CameraResolution& preferred_resolution,
                              int preferred_fps)
{
    if (!tryGetFirstCamera(manager, device, reason))
    {
        return false;
    }

    if (!tryOpenConfiguredCamera(*device, preferred_resolution, preferred_fps, reason))
    {
        device = nullptr;
        return false;
    }

    const std::string training_data_dir = findTrainingDataDir();
    if (training_data_dir.empty())
    {
        if (device->isRunning())
        {
            device->close();
        }
        device = nullptr;
        setReason(reason, "training_data not found");
        return false;
    }

    if (!detector.open(training_data_dir))
    {
        if (device->isRunning())
        {
            device->close();
        }
        device = nullptr;
        setReason(reason, "failed to open mediapipe detector");
        return false;
    }

    return true;
}

void closeCameraAndDetector(Vision::CameraDevice* device, MediaPipe::MediaPipeHandDetector& detector)
{
    detector.close();

    if (device != nullptr && device->isRunning())
    {
        device->close();
    }
}

void setWorldPoint(HandLandmarkArray& landmarks, HandJoint joint, float x, float y, float z)
{
    auto& point = landmarks[joint];
    point.world.x = x;
    point.world.y = y;
    point.world.z = z;
    point.has_world = true;
}

void setNormalizedPoint(HandLandmarkArray& landmarks, HandJoint joint, float x, float y, float z)
{
    auto& point = landmarks[joint];
    point.normalized.x = x;
    point.normalized.y = y;
    point.normalized.z = z;
    point.has_normalized = true;
}

HandLandmarkArray makeInvalidLandmarks()
{
    HandLandmarkArray landmarks;
    landmarks.reset();
    return landmarks;
}

HandLandmarkArray makeLandmarksForControl(float x, float y)
{
    HandLandmarkArray landmarks;
    landmarks.reset();

    // Build a stable palm reference triangle for rules that normalize by palm scale.
    setWorldPoint(landmarks, HandJoint::Wrist, 0.0f, 0.0f, 0.0f);
    setWorldPoint(landmarks, HandJoint::IndexMCP, 2.0f, 0.0f, 0.0f);
    setWorldPoint(landmarks, HandJoint::PinkyMCP, 0.0f, 2.0f, 0.0f);

    setWorldPoint(landmarks, HandJoint::IndexTIP, x, y, 0.0f);
    setNormalizedPoint(landmarks, HandJoint::IndexTIP, x, y, 0.0f);

    return landmarks;
}

std::vector<VideoTestCase> trajectoryVideoCases()
{
    return {
        {"Circle", "test_data/tst_each_filter_circle.mp4"},
        {"Rectangle", "test_data/tst_each_filter_rectangle.mp4"},
        {"Horizontal", "test_data/tst_each_filter_horizontal.mp4"},
    };
}

float distance2D(const HandLandmark& a, const HandLandmark& b)
{
    const bool use_normalized = a.has_normalized && b.has_normalized;

    const float ax = use_normalized ? a.normalized.x : a.world.x;
    const float ay = use_normalized ? a.normalized.y : a.world.y;

    const float bx = use_normalized ? b.normalized.x : b.world.x;
    const float by = use_normalized ? b.normalized.y : b.world.y;

    const float dx = ax - bx;
    const float dy = ay - by;
    return std::sqrt(dx * dx + dy * dy);
}

std::string actionTypeToString(ActionType type)
{
    switch (type)
    {
        case ActionType::None:
            return "None";
        case ActionType::LeftDown:
            return "LeftDown";
        case ActionType::LeftUp:
            return "LeftUp";
        case ActionType::LeftClick:
            return "LeftClick";
        case ActionType::RightDown:
            return "RightDown";
        case ActionType::RightUp:
            return "RightUp";
        case ActionType::RightClick:
            return "RightClick";
        case ActionType::ScrollVertical:
            return "ScrollVertical";
        case ActionType::ScrollHorizontal:
            return "ScrollHorizontal";
        default:
            return "UnknownActionType";
    }
}

void drawActionOverlay(FramePtr& frame, const Action& action)
{
    auto opencv_frame = std::dynamic_pointer_cast<Vision::OpenCVFrame>(frame);
    if (!opencv_frame)
    {
        return;
    }

    cv::Mat& mat = opencv_frame->Mat();

    const std::string line1 = "Action: " + actionTypeToString(action.type);
    const std::string line2 = "Moving: " + std::string(action.isMoving ? "true" : "false");
    const std::string line3 = "x=" + std::to_string(action.x) + " y=" + std::to_string(action.y);
    const std::string line4 = "dx=" + std::to_string(action.dx) + " dy=" + std::to_string(action.dy);

    cv::putText(mat, line1, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
    cv::putText(mat, line2, cv::Point(20, 90), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
    cv::putText(mat, line3, cv::Point(20, 130), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
    cv::putText(mat, line4, cv::Point(20, 170), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
}

} // namespace Tests
} // namespace VisionCursor
