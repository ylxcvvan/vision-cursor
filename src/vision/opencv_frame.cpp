#include "vision/opencv_frame.h"

namespace VisionCursor
{
namespace Vision
{

FramePtr OpenCVFrame::clone() const
{
    return std::make_shared<OpenCVFrame>(mat_.clone());
}

uint8_t* OpenCVFrame::getData() const
{
    return mat_.data;
}

int OpenCVFrame::getWidth() const
{
    return mat_.cols;
}

int OpenCVFrame::getHeight() const
{
    return mat_.rows;
}

void OpenCVFrame::setWidth(int width)
{
    if (width <= 0 || mat_.empty())
    {
        return;
    }

    cv::resize(mat_, mat_, cv::Size(width, mat_.rows));
}

void OpenCVFrame::setHeight(int height)
{
    if (height <= 0 || mat_.empty())
    {
        return;
    }

    cv::resize(mat_, mat_, cv::Size(mat_.cols, height));
}

cv::Mat& OpenCVFrame::Mat()
{
    return mat_;
}

const cv::Mat& OpenCVFrame::Mat() const
{
    return mat_;
}

cv::Mat OpenCVFrame::CloneMat() const
{
    return mat_.clone();
}

} // namespace Vision
} // namespace VisionCursor