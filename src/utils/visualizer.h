#pragma once

#include <string>

#include <opencv2/opencv.hpp>

#include "common/common.h"

namespace VisionCursor
{
namespace Utils
{

void draw_landmarks(FramePtr& frame, const HandLandmarkArray& arr);
void drawControlJointHighlight(FramePtr& frame, const HandLandmarkArray& arr, HandJoint control_joint);
void drawMapperRegion(FramePtr& frame, const MappingRegion& region);
void cout_landmarks(const HandLandmarkArray& landmarks);
void show_frame(const FramePtr& frame, const std::string& window_name);

} // namespace Utils
} // namespace VisionCursor