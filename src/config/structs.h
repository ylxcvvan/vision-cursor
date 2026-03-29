#pragma once

#include <string>
#include <vector>

#include "common/common.h"

namespace VisionCursor
{
namespace Config
{

struct OrchestratorConfig
{
    struct CameraConfig
    {
        std::string camera_device_name;
        CameraResolution camera_resolution;
        int camera_fps = 60;
        bool is_mirror = true;
    };

    struct MediaPipeConfig
    {
        float min_score_thresh = 0.6f;
        double threshold = 0.2;
        int model_complexity = 1;
        bool use_prev_landmarks = true;
    };

    struct InteractionConfig
    {
        std::string scheme_name = "easy";
        std::string filter_name = "none";
        std::vector<float> vec_filter_param;
        HandJoint control_joint = HandJoint::IndexTIP;
    };

    struct MapperConfig
    {
        float x_sensitivity = 1.0f;
        float y_sensitivity = 1.0f;
        float scroll_sensitivity = 1500.0f;
        MoveMode move_mode = MoveMode::Absolute;
        MappingRegion region = DEFAULT_MAPPING_REGION;
    };

    CameraConfig camera;
    MediaPipeConfig mediapipe;
    InteractionConfig interaction;
    MapperConfig mapper;
};

} // namespace Config
} // namespace VisionCursor
