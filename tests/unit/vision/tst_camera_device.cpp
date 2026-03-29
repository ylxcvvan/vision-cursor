#include <gtest/gtest.h>

#include "test_helpers.h"
#include "vision/camera_device.h"
#include "vision/camera_manager.h"

using namespace VisionCursor;

TEST(CameraDeviceTest, InvalidIndexOpenFails)
{
    Vision::CameraDevice device(-1);

    EXPECT_FALSE(device.open());
}

TEST(CameraDeviceTest, CaptureWithoutOpenFails)
{
    Vision::CameraDevice device(-1);
    FramePtr frame;

    EXPECT_FALSE(device.capture(frame));
}

TEST(CameraDeviceTest, ConfigureWithInvalidIndexFails)
{
    Tests::ensureCoreApp();

    Vision::CameraDevice device(-1);
    EXPECT_FALSE(device.configure({640, 480}, 30));
}

TEST(CameraDeviceTest, OpenCaptureCloseSmokeWhenCameraAvailable)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;
    Vision::CameraDevice* device = nullptr;

    std::string reason;
    if (!Tests::tryGetFirstCamera(manager, device, &reason))
    {
        GTEST_SKIP() << "skip camera smoke: " << reason;
    }

    if (!Tests::tryOpenConfiguredCamera(*device, {1280, 720}, 60, &reason))
    {
        GTEST_SKIP() << "skip camera open/configure: " << reason;
    }

    FramePtr frame;
    EXPECT_TRUE(device->capture(frame));
    EXPECT_NE(frame, nullptr);

    EXPECT_TRUE(device->close());
}
