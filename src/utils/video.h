#pragma once

#include <string>

#include <opencv2/opencv.hpp>

#include "common/common.h"

namespace VisionCursor
{
namespace Utils
{

class VideoUtils
{
public:
    static bool openVideo(const std::string& path, cv::VideoCapture& cap);
    static bool readFrame(cv::VideoCapture& cap, FramePtr& frame);
};

} // namespace Utils
} // namespace VisionCursor