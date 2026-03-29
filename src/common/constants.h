#pragma once

#include <array>

#include "common/structs.h"

namespace VisionCursor
{

constexpr std::array<std::array<HandJoint, 2>, 21> ARR_HAND_CONNECTIONS = {
    std::array{HandJoint::Wrist, HandJoint::ThumbCMC},
    {HandJoint::ThumbCMC, HandJoint::ThumbMCP},
    {HandJoint::ThumbMCP, HandJoint::ThumbIP},
    {HandJoint::ThumbIP, HandJoint::ThumbTIP},

    {HandJoint::Wrist, HandJoint::IndexMCP},
    {HandJoint::IndexMCP, HandJoint::IndexPIP},
    {HandJoint::IndexPIP, HandJoint::IndexDIP},
    {HandJoint::IndexDIP, HandJoint::IndexTIP},

    {HandJoint::IndexMCP, HandJoint::MiddleMCP},
    {HandJoint::MiddleMCP, HandJoint::MiddlePIP},
    {HandJoint::MiddlePIP, HandJoint::MiddleDIP},
    {HandJoint::MiddleDIP, HandJoint::MiddleTIP},

    {HandJoint::MiddleMCP, HandJoint::RingMCP},
    {HandJoint::RingMCP, HandJoint::RingPIP},
    {HandJoint::RingPIP, HandJoint::RingDIP},
    {HandJoint::RingDIP, HandJoint::RingTIP},

    {HandJoint::RingMCP, HandJoint::PinkyMCP},
    {HandJoint::Wrist, HandJoint::PinkyMCP},
    {HandJoint::PinkyMCP, HandJoint::PinkyPIP},
    {HandJoint::PinkyPIP, HandJoint::PinkyDIP},
    {HandJoint::PinkyDIP, HandJoint::PinkyTIP}
};

inline constexpr std::array<CameraResolution, 5> ARR_SUGGEST_RESOLUTIONS = {
    CameraResolution{1280, 720},
    CameraResolution{800, 600},
    CameraResolution{640, 480},
    CameraResolution{320, 240},
    CameraResolution{160, 120}
};

inline constexpr MappingRegion DEFAULT_MAPPING_REGION = {0.3f, 0.3f, 0.7f, 0.7f};

} // namespace VisionCursor