#include "vision_cursor_test_fixture.h"

#include "test_helpers.h"

std::shared_ptr<VisionCursor::Vision::CameraManager> VisionCursorTestFixture::camera_manager =
    std::make_shared<VisionCursor::Vision::CameraManager>();

VisionCursor::Vision::CameraDevice* VisionCursorTestFixture::p_camera_device = nullptr;

std::shared_ptr<VisionCursor::MediaPipe::MediaPipeHandDetector> VisionCursorTestFixture::mp_hand_detector =
    std::make_shared<VisionCursor::MediaPipe::MediaPipeHandDetector>();

QCoreApplication* VisionCursorTestFixture::app = nullptr;
bool VisionCursorTestFixture::initialized_ = false;
std::string VisionCursorTestFixture::init_error_;

void VisionCursorTestFixture::SetUpTestSuite()
{
    initialized_ = false;
    init_error_.clear();

    app = VisionCursor::Tests::ensureCoreApp();

    if (!camera_manager || !mp_hand_detector)
    {
        init_error_ = "fixture dependencies are null";
        return;
    }

    std::string reason;
    if (!VisionCursor::Tests::tryOpenCameraAndDetector(
            *camera_manager, p_camera_device, *mp_hand_detector, &reason, {1280, 720}, 60))
    {
        init_error_ = reason;
        return;
    }

    initialized_ = true;
}

void VisionCursorTestFixture::TearDownTestSuite()
{
    if (initialized_ && mp_hand_detector)
    {
        mp_hand_detector->close();
    }

    if (p_camera_device && p_camera_device->isRunning())
    {
        p_camera_device->close();
    }

    p_camera_device = nullptr;
    mp_hand_detector.reset();
    camera_manager.reset();
    initialized_ = false;
}

void VisionCursorTestFixture::SetUp()
{
    if (!initialized_)
    {
        GTEST_SKIP() << "Fixture initialization failed: " << init_error_;
    }
}

void VisionCursorTestFixture::TearDown()
{
}

bool VisionCursorTestFixture::isInitialized()
{
    return initialized_;
}
