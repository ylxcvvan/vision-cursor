#include "utils/video.h"

#include "log/logging.h"

#include "vision/opencv_frame.h"

namespace VisionCursor
{
namespace Utils
{

bool VideoUtils::openVideo(const std::string& path, cv::VideoCapture& cap)
{
    cap.open(path);
    if (!cap.isOpened())
    {
        LOG(ERROR) << "Failed to open video: " << path;
        return false;
    }
    return true;
}

bool VideoUtils::readFrame(cv::VideoCapture& cap, FramePtr& frame)
{
    cv::Mat mat;
    if (!cap.read(mat))
    {
        LOG(ERROR) << "Video ended or failed to read frame.";
        return false;
    }

    if (mat.empty())
    {
        LOG(ERROR) << "Empty frame captured.";
        return false;
    }

    frame = std::make_shared<VisionCursor::Vision::OpenCVFrame>(mat);
    return true;
}

} // namespace Utils
} // namespace VisionCursor
