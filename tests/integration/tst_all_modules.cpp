#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>

#include <opencv2/opencv.hpp>

#include "interaction/interaction_controller.h"
#include "mapping/action_mapper.h"
#include "mediapipe/mediapipe_hand_detector.h"
#include "test_helpers.h"
#include "utils/utils.h"
#include "vision/camera_manager.h"

using namespace VisionCursor;

namespace
{

QGuiApplication* ensureGuiApp()
{
    if (QCoreApplication::instance() != nullptr)
    {
        return qobject_cast<QGuiApplication*>(QCoreApplication::instance());
    }

    static int argc = 1;
    static char arg0[] = "integration_all_modules";
    static char* argv[] = {arg0, nullptr};
    static QGuiApplication app(argc, argv);

    return &app;
}

int screenWidth()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr)
    {
        return 1920;
    }

    return static_cast<int>(screen->geometry().width() * screen->devicePixelRatio());
}

int screenHeight()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr)
    {
        return 1080;
    }

    return static_cast<int>(screen->geometry().height() * screen->devicePixelRatio());
}

} // namespace

TEST(IntegrationTest, AllModulesCoWorking)
{
    ASSERT_NE(ensureGuiApp(), nullptr) << "Failed to initialize QGuiApplication";

    Vision::CameraManager camera_manager;
    Vision::CameraDevice* p_camera_device = nullptr;
    MediaPipe::MediaPipeHandDetector mp_hand_detector;

    std::string reason;
    if (!Tests::tryOpenCameraAndDetector(camera_manager, p_camera_device, mp_hand_detector, &reason))
    {
        GTEST_SKIP() << "skip integration camera+detector setup: " << reason;
    }

    Interaction::InteractionController inter_controller;
    ASSERT_TRUE(inter_controller.init("advanced", "one_euro", {12.0f, 30.0f, 1.0f, 60.0f}, HandJoint::IndexTIP));

    Mapping::ActionMapper action_mapper;
    ASSERT_TRUE(action_mapper.open(1.0f, 1.0f, 1800.0f, MoveMode::Absolute));
    ASSERT_TRUE(action_mapper.setMappingRegion({0.2f, 0.2f, 0.8f, 0.8f}));

    GTEST_LOG_(INFO) << "screen width: " << screenWidth() << ", screen height: " << screenHeight();

    for (int i = 0; i < 600; ++i)
    {
        FramePtr frame;
        if (!p_camera_device->capture(frame) || frame == nullptr)
        {
            GTEST_LOG_(WARNING) << "capture failed at frame " << i;
            break;
        }

        HandLandmarkArray arr_landmark;
        if (!mp_hand_detector.process(frame, arr_landmark))
        {
            GTEST_LOG_(WARNING) << "mediapipe process failed at frame " << i;
            continue;
        }

        Action action;
        if (!inter_controller.process(arr_landmark, action))
        {
            GTEST_LOG_(WARNING) << "interaction process failed at frame " << i;
            continue;
        }

        if (!action_mapper.mapAndExecute(action))
        {
            GTEST_LOG_(WARNING) << "action mapper execute failed at frame " << i;
        }

        Utils::draw_landmarks(frame, arr_landmark);
        Tests::drawActionOverlay(frame, action);
        Utils::show_frame(frame, "All Modules CoWorking");

        if (cv::waitKey(1) == 27)
        {
            break;
        }
    }

    Tests::closeCameraAndDetector(p_camera_device, mp_hand_detector);
}

TEST(IntegrationTest, RestartAfterReleaseStillWorks)
{
    ASSERT_NE(ensureGuiApp(), nullptr) << "Failed to initialize QGuiApplication";

    auto runOnce = [](const std::string& window_name, int max_frames, std::string& error) -> bool {
        Vision::CameraManager camera_manager;
        Vision::CameraDevice* p_camera_device = nullptr;
        MediaPipe::MediaPipeHandDetector mp_hand_detector;

        if (!Tests::tryOpenCameraAndDetector(camera_manager, p_camera_device, mp_hand_detector, &error))
        {
            return false;
        }

        Interaction::InteractionController inter_controller;
        if (!inter_controller.init("advanced", "one_euro", {12.0f, 30.0f, 1.0f, 60.0f}, HandJoint::IndexTIP))
        {
            error = "failed to init interaction controller";
            Tests::closeCameraAndDetector(p_camera_device, mp_hand_detector);
            return false;
        }

        Mapping::ActionMapper action_mapper;
        if (!action_mapper.open(1.0f, 1.0f, 1500.0f, MoveMode::Relative))
        {
            error = "failed to open action mapper";
            Tests::closeCameraAndDetector(p_camera_device, mp_hand_detector);
            return false;
        }

        if (!action_mapper.setMappingRegion({0.2f, 0.2f, 0.8f, 0.8f}))
        {
            error = "failed to set action mapper region";
            Tests::closeCameraAndDetector(p_camera_device, mp_hand_detector);
            return false;
        }

        int valid_frame_count = 0;

        for (int i = 0; i < max_frames; ++i)
        {
            FramePtr frame;
            if (!p_camera_device->capture(frame) || frame == nullptr)
            {
                GTEST_LOG_(WARNING) << "capture failed at frame " << i;
                break;
            }

            HandLandmarkArray arr_landmark;
            if (!mp_hand_detector.process(frame, arr_landmark))
            {
                GTEST_LOG_(WARNING) << "mediapipe process failed at frame " << i;
            }

            Action action;
            if (!inter_controller.process(arr_landmark, action))
            {
                GTEST_LOG_(WARNING) << "interaction process failed at frame " << i;
            }

            if (!action_mapper.mapAndExecute(action))
            {
                GTEST_LOG_(WARNING) << "action mapper execute failed at frame " << i;
            }

            ++valid_frame_count;

            Utils::draw_landmarks(frame, arr_landmark);
            Tests::drawActionOverlay(frame, action);
            Utils::show_frame(frame, window_name);

            if (cv::waitKey(1) == 27)
            {
                GTEST_LOG_(INFO) << "ESC pressed, stop current run.";
                break;
            }
        }

        GTEST_LOG_(INFO) << window_name << " valid_frame_count = " << valid_frame_count;

        Tests::closeCameraAndDetector(p_camera_device, mp_hand_detector);

        cv::destroyWindow(window_name);
        cv::waitKey(30);

        if (valid_frame_count <= 0)
        {
            error = "no valid frame processed";
            return false;
        }

        return true;
    };

    std::string error;
    ASSERT_TRUE(runOnce("Restart Test - First Run", 300, error)) << "First run failed: " << error;

    cv::waitKey(200);
    GTEST_LOG_(INFO) << "First run finished, start second run.";

    ASSERT_TRUE(runOnce("Restart Test - Second Run", 300, error)) << "Second run failed: " << error;
}
