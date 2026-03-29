#include <gtest/gtest.h>

#include "test_helpers.h"
#include "vision/camera_manager.h"

using namespace VisionCursor;

TEST(CameraManagerTest, UnknownQueriesReturnNull)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;

    EXPECT_EQ(manager.getPointerToDevice("__unknown_name__"), nullptr);
    EXPECT_EQ(manager.getPointerToDeviceById("__unknown_id__"), nullptr);
}

TEST(CameraManagerTest, DeviceNameAndIdOrderStayAligned)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;
    manager.updateAllDevice();

    const auto names = manager.getAllDeviceName();
    const auto ids = manager.getAllDeviceId();

    EXPECT_EQ(names.size(), ids.size());
}

TEST(CameraManagerTest, ResolveByNameAndIdWhenAvailable)
{
    Tests::ensureCoreApp();

    Vision::CameraManager manager;
    manager.updateAllDevice();

    const auto names = manager.getAllDeviceName();
    if (names.empty())
    {
        GTEST_SKIP() << "no camera available";
    }

    Vision::CameraDevice* by_name = manager.getPointerToDevice(names.front());
    ASSERT_NE(by_name, nullptr);

    const std::string id = by_name->deviceId();
    Vision::CameraDevice* by_id = manager.getPointerToDeviceById(id);
    ASSERT_NE(by_id, nullptr);

    EXPECT_EQ(by_id->deviceId(), id);
}
