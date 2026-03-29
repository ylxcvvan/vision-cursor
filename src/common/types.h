#pragma once

#include <cstdint>

namespace VisionCursor
{

enum class HandJoint : uint8_t
{
    Wrist = 0,

    ThumbCMC,
    ThumbMCP,
    ThumbIP,
    ThumbTIP,

    IndexMCP,
    IndexPIP,
    IndexDIP,
    IndexTIP,

    MiddleMCP,
    MiddlePIP,
    MiddleDIP,
    MiddleTIP,

    RingMCP,
    RingPIP,
    RingDIP,
    RingTIP,

    PinkyMCP,
    PinkyPIP,
    PinkyDIP,
    PinkyTIP
};

enum class MouseButton
{
    Left,
    Right,
    Middle
};

enum class ScrollAxis
{
    Vertical,
    Horizontal
};

enum class MotionState : uint8_t
{
    Idle = 0,
    Ready,
    Moving
};

enum class ControlState : uint8_t
{
    None = 0,
    LeftPressed,
    RightPressed,
    VerticalScrolling,
    HorizontalScrolling
};

enum class ControlPreset
{
    Easy = 0,
    Advanced,
    Tiktok
};

enum class RuleType
{
    Distance,
    Angle,
    Orientation,
    Composite,
    Unknown
};

enum class CompareOp
{
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual
};

enum class LogicOp
{
    And,
    Or
};

enum class MoveMode
{
    Absolute = 0,
    Relative
};

} // namespace VisionCursor