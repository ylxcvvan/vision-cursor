#pragma once

#include <QCoreApplication>

#include <string>
#include <vector>

#include "common/common.h"
#include "interaction/action/action.h"
#include "mediapipe/mediapipe_hand_detector.h"
#include "vision/camera_manager.h"

namespace VisionCursor
{
namespace Tests
{

struct VideoTestCase
{
    std::string label;
    std::string path;
};

QCoreApplication* ensureCoreApp();

std::string findTrainingDataDir();
bool hasTrainingData();

bool tryGetFirstCamera(Vision::CameraManager& manager, Vision::CameraDevice*& device, std::string* reason = nullptr);

bool tryOpenConfiguredCamera(Vision::CameraDevice& device,
                             const CameraResolution& preferred_resolution = {1280, 720},
                             int preferred_fps = 60,
                             std::string* reason = nullptr);

bool tryOpenCameraAndDetector(Vision::CameraManager& manager,
                              Vision::CameraDevice*& device,
                              MediaPipe::MediaPipeHandDetector& detector,
                              std::string* reason = nullptr,
                              const CameraResolution& preferred_resolution = {1280, 720},
                              int preferred_fps = 60);

void closeCameraAndDetector(Vision::CameraDevice* device, MediaPipe::MediaPipeHandDetector& detector);

void setWorldPoint(HandLandmarkArray& landmarks, HandJoint joint, float x, float y, float z = 0.0f);
void setNormalizedPoint(HandLandmarkArray& landmarks, HandJoint joint, float x, float y, float z = 0.0f);

HandLandmarkArray makeInvalidLandmarks();
HandLandmarkArray makeLandmarksForControl(float x = 0.5f, float y = 0.5f);

std::vector<VideoTestCase> trajectoryVideoCases();
float distance2D(const HandLandmark& a, const HandLandmark& b);

std::string actionTypeToString(ActionType type);
void drawActionOverlay(FramePtr& frame, const Action& action);

} // namespace Tests
} // namespace VisionCursor
