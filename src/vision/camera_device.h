#pragma once

#include <string>

#include <opencv2/opencv.hpp>

#include "common/common.h"
#include "vision/opencv_frame.h"

namespace VisionCursor
{
namespace Vision
{

class CameraDevice
{
public:
    CameraDevice() = delete;
    CameraDevice(int opencv_index) noexcept;
    CameraDevice(const std::string& device_id, int opencv_index, const std::string& display_name) noexcept;
    ~CameraDevice();

    CameraDevice(const CameraDevice&) = delete;
    CameraDevice& operator=(const CameraDevice&) = delete;
    CameraDevice(CameraDevice&& other) noexcept;
    CameraDevice& operator=(CameraDevice&& other) noexcept;

public:
    bool open();
    bool close();

    bool refreshCameraInfo();
    bool getCameraInfo(CameraInfo& camera_info) const;

    bool configure(const CameraResolution& camera_resolution, int fps = DEFAULT_CAMERA_FPS, bool mirror = true);
    bool applyConfiguration();

    bool capture(FramePtr& frame);

    bool isRunning() const { return is_running_; }

    const std::string& deviceId() const { return camera_info_.device_id; }
    const std::string& displayName() const { return camera_info_.display_name; }

private:
    void moveFrom(CameraDevice& other);
    void chooseBestResolution();
    bool isConfiguredResolutionSupported(const CameraResolution& resolution) const;

private:
    bool is_running_ = false;
    bool mirror_ = true;

    int fps_ = DEFAULT_CAMERA_FPS;
    CameraInfo camera_info_{};
    CameraResolution target_camera_resolution_{};
    bool has_pending_config_ = false;

    cv::VideoCapture cv_capture_;
};

} // namespace Vision
} // namespace VisionCursor
