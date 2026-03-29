#pragma once

#include <gtest/gtest.h>

#include <QCoreApplication>

#include <memory>
#include <string>

#include "mediapipe/mediapipe_hand_detector.h"
#include "vision/camera_manager.h"

class VisionCursorTestFixture : public ::testing::Test
{
protected:
    static std::shared_ptr<VisionCursor::Vision::CameraManager> camera_manager;
    static VisionCursor::Vision::CameraDevice* p_camera_device;
    static std::shared_ptr<VisionCursor::MediaPipe::MediaPipeHandDetector> mp_hand_detector;

    static QCoreApplication* app;

    static bool initialized_;
    static std::string init_error_;

    static void SetUpTestSuite();
    static void TearDownTestSuite();

    void SetUp() override;
    void TearDown() override;

    static bool isInitialized();
};
