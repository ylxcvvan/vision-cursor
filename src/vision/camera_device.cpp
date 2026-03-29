#include "vision/camera_device.h"

#include <QCameraDevice>
#include <QMediaDevices>
#include <QVideoFrameFormat>

#include "log/logging.h"

namespace VisionCursor
{
namespace Vision
{

namespace
{
std::string makeCameraName(const std::string& display_name, int opencv_index)
{
    return std::to_string(opencv_index) + ": " + display_name;
}

std::string toDeviceIdHex(const QCameraDevice& camera)
{
    return camera.id().toHex().toStdString();
}
} // namespace

CameraDevice::CameraDevice(int opencv_index) noexcept
{
    camera_info_.opencv_device_index = opencv_index;
}

CameraDevice::CameraDevice(const std::string& device_id, int opencv_index, const std::string& display_name) noexcept
{
    camera_info_.device_id = device_id;
    camera_info_.display_name = display_name;
    camera_info_.opencv_device_index = opencv_index;
    camera_info_.name = makeCameraName(display_name, opencv_index);
}

CameraDevice::~CameraDevice()
{
    if (is_running_)
    {
        close();
    }
}

CameraDevice::CameraDevice(CameraDevice&& other) noexcept
{
    moveFrom(other);
}

CameraDevice& CameraDevice::operator=(CameraDevice&& other) noexcept
{
    if (this != &other)
    {
        if (is_running_)
        {
            close();
        }
        moveFrom(other);
    }
    return *this;
}

bool CameraDevice::open()
{
    if (is_running_)
    {
        LOG(ERROR) << "Unable to open camera while already running. device=" << camera_info_.name;
        return false;
    }

    if (camera_info_.opencv_device_index < 0)
    {
        LOG(ERROR) << "Unable to open camera with invalid index. device_id=" << camera_info_.device_id;
        return false;
    }

    if (!cv_capture_.open(camera_info_.opencv_device_index))
    {
        LOG(ERROR) << "Unable to open camera device. name=" << camera_info_.name
                   << " index=" << camera_info_.opencv_device_index;
        return false;
    }

    is_running_ = true;
    LOG(INFO) << "Camera opened: " << camera_info_.name;
    return true;
}

bool CameraDevice::close()
{
    if (!cv_capture_.isOpened())
    {
        LOG(ERROR) << "Failed to close camera: device is not opened. name=" << camera_info_.name;
        return false;
    }

    cv_capture_.release();
    is_running_ = false;
    LOG(INFO) << "Camera closed: " << camera_info_.name;
    return true;
}

bool CameraDevice::refreshCameraInfo()
{
    if (camera_info_.opencv_device_index < 0)
    {
        LOG(ERROR) << "Failed to refresh camera info with invalid index. device_id=" << camera_info_.device_id;
        return false;
    }

    const auto cameras = QMediaDevices::videoInputs();
    if (cameras.empty())
    {
        LOG(ERROR) << "No camera devices reported by Qt.";
        return false;
    }

    int matched_index = -1;
    QCameraDevice matched_camera;

    if (!camera_info_.device_id.empty())
    {
        for (int i = 0; i < cameras.size(); ++i)
        {
            if (toDeviceIdHex(cameras[i]) == camera_info_.device_id)
            {
                matched_camera = cameras[i];
                matched_index = i;
                break;
            }
        }
    }

    if (matched_index < 0)
    {
        if (camera_info_.opencv_device_index >= cameras.size())
        {
            LOG(ERROR) << "Target camera index is out of range. index=" << camera_info_.opencv_device_index;
            return false;
        }

        matched_index = camera_info_.opencv_device_index;
        matched_camera = cameras[matched_index];
    }

    camera_info_.opencv_device_index = matched_index;
    camera_info_.device_id = toDeviceIdHex(matched_camera);
    camera_info_.display_name = matched_camera.description().toStdString();
    camera_info_.name = makeCameraName(camera_info_.display_name, matched_index);

    camera_info_.set_supported_resolution.clear();
    const auto formats = matched_camera.videoFormats();
    for (const auto& fmt : formats)
    {
        const QSize resolution = fmt.resolution();
        camera_info_.set_supported_resolution.insert({resolution.width(), resolution.height()});
    }

    if (camera_info_.set_supported_resolution.empty())
    {
        LOG(WARNING) << "Camera reports no supported resolutions. name=" << camera_info_.name;
    }

    return true;
}

bool CameraDevice::getCameraInfo(CameraInfo& camera_info) const
{
    camera_info = camera_info_;
    return true;
}

bool CameraDevice::configure(const CameraResolution& camera_resolution, int fps, bool mirror)
{
    if (fps <= 0)
    {
        LOG(ERROR) << "Invalid FPS when configuring camera. fps=" << fps;
        return false;
    }

    if (camera_info_.set_supported_resolution.empty() && !refreshCameraInfo())
    {
        LOG(ERROR) << "Failed to configure camera because camera info refresh failed. name=" << camera_info_.name;
        return false;
    }

    if (!isConfiguredResolutionSupported(camera_resolution))
    {
        LOG(ERROR) << "Requested resolution is not supported. name=" << camera_info_.name
                   << " resolution=" << camera_resolution.width << "x" << camera_resolution.height;
        return false;
    }

    target_camera_resolution_ = camera_resolution;
    fps_ = fps;
    mirror_ = mirror;
    has_pending_config_ = true;

    if (is_running_)
    {
        const bool ok = applyConfiguration();
        if (ok)
        {
            LOG(INFO) << "Camera reconfigured while running: " << camera_info_.name;
        }
        return ok;
    }

    LOG(INFO) << "Camera configuration staged: " << camera_info_.name << " "
              << target_camera_resolution_.width << "x" << target_camera_resolution_.height
              << " @" << fps_ << "fps";
    return true;
}

bool CameraDevice::applyConfiguration()
{
    if (!is_running_)
    {
        LOG(ERROR) << "Cannot apply camera configuration while device is not running. name=" << camera_info_.name;
        return false;
    }

    if (!has_pending_config_)
    {
        if (!refreshCameraInfo())
        {
            return false;
        }
        chooseBestResolution();
    }

    if (target_camera_resolution_.width <= 0 || target_camera_resolution_.height <= 0)
    {
        LOG(ERROR) << "Invalid target resolution when applying configuration. name=" << camera_info_.name;
        return false;
    }

    cv_capture_.set(cv::CAP_PROP_FRAME_WIDTH, target_camera_resolution_.width);
    cv_capture_.set(cv::CAP_PROP_FRAME_HEIGHT, target_camera_resolution_.height);
    cv_capture_.set(cv::CAP_PROP_FPS, fps_);

    has_pending_config_ = false;
    return true;
}

bool CameraDevice::capture(FramePtr& frame)
{
    if (!is_running_)
    {
        LOG(ERROR) << "Failed to capture frame: camera is not opened. name=" << camera_info_.name;
        return false;
    }

    cv::Mat raw_frame;
    if (!cv_capture_.read(raw_frame))
    {
        LOG(ERROR) << "Camera failed to read frame. name=" << camera_info_.name;
        return false;
    }

    cv::Mat rgb_frame;
    cv::cvtColor(raw_frame, rgb_frame, cv::COLOR_BGR2RGB);

    if (mirror_)
    {
        cv::flip(rgb_frame, rgb_frame, 1);
    }

    frame = std::make_shared<OpenCVFrame>(std::move(rgb_frame));
    return true;
}

void CameraDevice::moveFrom(CameraDevice& other)
{
    if (other.is_running_)
    {
        other.close();
    }

    is_running_ = false;
    mirror_ = other.mirror_;
    fps_ = other.fps_;
    camera_info_ = std::move(other.camera_info_);
    target_camera_resolution_ = other.target_camera_resolution_;
    has_pending_config_ = other.has_pending_config_;

    other.fps_ = DEFAULT_CAMERA_FPS;
    other.target_camera_resolution_ = {};
    other.has_pending_config_ = false;
}

void CameraDevice::chooseBestResolution()
{
    if (camera_info_.set_supported_resolution.empty())
    {
        return;
    }

    for (const CameraResolution& camera_resolution : ARR_SUGGEST_RESOLUTIONS)
    {
        if (camera_info_.set_supported_resolution.find(camera_resolution) != camera_info_.set_supported_resolution.end())
        {
            target_camera_resolution_ = camera_resolution;
            return;
        }
    }

    target_camera_resolution_ = *camera_info_.set_supported_resolution.begin();
}

bool CameraDevice::isConfiguredResolutionSupported(const CameraResolution& resolution) const
{
    return camera_info_.set_supported_resolution.find(resolution) != camera_info_.set_supported_resolution.end();
}

} // namespace Vision
} // namespace VisionCursor
