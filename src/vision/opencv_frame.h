#pragma once

#include <opencv2/opencv.hpp>

#include "common/frame.h"

namespace VisionCursor
{
namespace Vision
{

class OpenCVFrame : public Frame
{
public:
    OpenCVFrame() = delete;
    explicit OpenCVFrame(const cv::Mat& mat) : mat_(mat) {}
    explicit OpenCVFrame(cv::Mat&& mat) : mat_(std::move(mat)) {}
    ~OpenCVFrame() override = default;

    FramePtr clone() const override;
    uint8_t* getData() const override;

    int getWidth() const override;
    int getHeight() const override;

    void setWidth(int width) override;
    void setHeight(int height) override;

public:
    cv::Mat& Mat();
    const cv::Mat& Mat() const;
    cv::Mat CloneMat() const;

private:
    cv::Mat mat_;
};

} // namespace Vision
} // namespace VisionCursor