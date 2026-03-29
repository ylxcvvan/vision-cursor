#include <gtest/gtest.h>

#include <opencv2/opencv.hpp>

#include "test_helpers.h"
#include "vision/camera_manager.h"

using namespace VisionCursor;

TEST(CameraBasicTest, OpenCVLinkageSmoke)
{
    cv::Mat img(64, 64, CV_8UC3, cv::Scalar(0, 0, 0));
    EXPECT_FALSE(img.empty());
    EXPECT_EQ(img.cols, 64);
    EXPECT_EQ(img.rows, 64);
}

TEST(CameraBasicTest, UpdateDevicesAndListSizeIsConsistent)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;
    manager.updateAllDevice();

    const auto names = manager.getAllDeviceName();
    const auto ids = manager.getAllDeviceId();

    EXPECT_EQ(names.size(), ids.size());
}

TEST(CameraBasicTest, QueryUnknownDeviceReturnsNull)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;

    EXPECT_EQ(manager.getPointerToDevice("__unknown_camera__"), nullptr);
    EXPECT_EQ(manager.getPointerToDeviceById("__unknown_camera_id__"), nullptr);
}

TEST(CameraBasicTest, FirstDevicePointerMatchesDeviceIdWhenAvailable)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;
    manager.updateAllDevice();

    const auto ids = manager.getAllDeviceId();
    if (ids.empty())
    {
        GTEST_SKIP() << "no camera available";
    }

    Vision::CameraDevice* by_id = manager.getPointerToDeviceById(ids.front());
    ASSERT_NE(by_id, nullptr);
    EXPECT_EQ(by_id->deviceId(), ids.front());
}

TEST(CameraBasicTest, CaptureFrameSmokeWhenCameraAvailable)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;
    Vision::CameraDevice* device = nullptr;
    std::string reason;
    if (!Tests::tryGetFirstCamera(manager, device, &reason))
    {
        GTEST_SKIP() << "camera not ready: " << reason;
    }

    ASSERT_NE(device, nullptr);

    if (!Tests::tryOpenConfiguredCamera(*device, {1280, 720}, 60, &reason))
    {
        GTEST_SKIP() << "camera open/configure failed: " << reason;
    }

    FramePtr frame;
    ASSERT_TRUE(device->capture(frame));
    ASSERT_NE(frame, nullptr);
    EXPECT_GT(frame->getWidth(), 0);
    EXPECT_GT(frame->getHeight(), 0);
    EXPECT_NE(frame->getData(), nullptr);

    EXPECT_TRUE(device->close());
}

