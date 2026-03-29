#include <gtest/gtest.h>

#include <filesystem>

#include <opencv2/opencv.hpp>

#include "mediapipe/mediapipe_hand_detector.h"
#include "test_helpers.h"
#include "vision/opencv_frame.h"

using namespace VisionCursor;

namespace
{

FramePtr makeDummyFrame()
{
    cv::Mat mat(16, 16, CV_8UC3, cv::Scalar(0, 0, 0));
    return std::make_shared<Vision::OpenCVFrame>(mat);
}

} // namespace

TEST(MediaPipeHandDetectorTest, CloseBeforeOpenShouldFail)
{
    MediaPipe::MediaPipeHandDetector detector;
    EXPECT_FALSE(detector.close());
}

TEST(MediaPipeHandDetectorTest, ProcessBeforeOpenShouldFail)
{
    MediaPipe::MediaPipeHandDetector detector;

    HandLandmarkArray landmarks;
    FramePtr frame = makeDummyFrame();

    EXPECT_FALSE(detector.process(frame, landmarks));
}

TEST(MediaPipeHandDetectorTest, OpenWithMissingDataShouldFail)
{
    MediaPipe::MediaPipeHandDetector detector;
    EXPECT_FALSE(detector.open("__not_a_real_training_data_dir__"));
}

TEST(MediaPipeHandDetectorTest, OpenAndProcessDummyFrameSmokeWhenDataExists)
{
    if (!Tests::hasTrainingData())
    {
        GTEST_SKIP() << "training_data not found";
    }

    MediaPipe::MediaPipeHandDetector detector;
    if (!detector.open(Tests::findTrainingDataDir()))
    {
        GTEST_SKIP() << "detector open failed in current environment";
    }

    FramePtr frame = makeDummyFrame();
    HandLandmarkArray landmarks;

    EXPECT_TRUE(detector.process(frame, landmarks));
    EXPECT_TRUE(detector.close());
}

TEST(MediaPipeHandDetectorTest, OpenAndProcessStaticImageSmokeWhenAssetsExist)
{
    if (!Tests::hasTrainingData())
    {
        GTEST_SKIP() << "training_data not found";
    }

    const std::string image_path = "test_data/image.png";
    if (!std::filesystem::exists(image_path))
    {
        GTEST_SKIP() << "test image not found: " << image_path;
    }

    cv::Mat bgr = cv::imread(image_path);
    if (bgr.empty())
    {
        GTEST_SKIP() << "failed to read test image";
    }

    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    FramePtr frame = std::make_shared<Vision::OpenCVFrame>(rgb);

    MediaPipe::MediaPipeHandDetector detector;
    if (!detector.open(Tests::findTrainingDataDir()))
    {
        GTEST_SKIP() << "detector open failed in current environment";
    }

    HandLandmarkArray landmarks;
    EXPECT_TRUE(detector.process(frame, landmarks));
    EXPECT_TRUE(detector.close());
}

