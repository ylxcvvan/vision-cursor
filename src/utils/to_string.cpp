#include "utils/to_string.h"

#include "log/logging.h"

namespace VisionCursor
{
namespace Utils
{

QString ToString(MotionState state)
{
    switch (state)
    {
        case MotionState::Idle: return "Idle";
        case MotionState::Ready: return "Ready";
        case MotionState::Moving: return "Moving";
        default: return "Idle";
    }
}

QString ToString(ControlState state)
{
    switch (state)
    {
        case ControlState::None: return "None";
        case ControlState::LeftPressed: return "LeftPressed";
        case ControlState::RightPressed: return "RightPressed";
        case ControlState::VerticalScrolling: return "VerticalScrolling";
        case ControlState::HorizontalScrolling: return "HorizontalScrolling";
        default: return "None";
    }
}

QString ToString(HandJoint joint)
{
    switch (joint)
    {
        case HandJoint::Wrist: return "Wrist";
        case HandJoint::ThumbCMC: return "ThumbCMC";
        case HandJoint::ThumbMCP: return "ThumbMCP";
        case HandJoint::ThumbIP: return "ThumbIP";
        case HandJoint::ThumbTIP: return "ThumbTIP";
        case HandJoint::IndexMCP: return "IndexMCP";
        case HandJoint::IndexPIP: return "IndexPIP";
        case HandJoint::IndexDIP: return "IndexDIP";
        case HandJoint::IndexTIP: return "IndexTIP";
        case HandJoint::MiddleMCP: return "MiddleMCP";
        case HandJoint::MiddlePIP: return "MiddlePIP";
        case HandJoint::MiddleDIP: return "MiddleDIP";
        case HandJoint::MiddleTIP: return "MiddleTIP";
        case HandJoint::RingMCP: return "RingMCP";
        case HandJoint::RingPIP: return "RingPIP";
        case HandJoint::RingDIP: return "RingDIP";
        case HandJoint::RingTIP: return "RingTIP";
        case HandJoint::PinkyMCP: return "PinkyMCP";
        case HandJoint::PinkyPIP: return "PinkyPIP";
        case HandJoint::PinkyDIP: return "PinkyDIP";
        case HandJoint::PinkyTIP: return "PinkyTIP";
        default: return "Wrist";
    }
}

QString ToString(CompareOp op)
{
    switch (op)
    {
        case CompareOp::Less: return "Less";
        case CompareOp::LessEqual: return "LessEqual";
        case CompareOp::Greater: return "Greater";
        case CompareOp::GreaterEqual: return "GreaterEqual";
        case CompareOp::Equal: return "Equal";
        case CompareOp::NotEqual: return "NotEqual";
        default: return "Less";
    }
}

QString ToString(LogicOp op)
{
    switch (op)
    {
        case LogicOp::And: return "And";
        case LogicOp::Or: return "Or";
        default: return "And";
    }
}

QString ToString(MoveMode mode)
{
    switch (mode)
    {
        case MoveMode::Absolute: return "Absolute";
        case MoveMode::Relative: return "Relative";
        default: return "Absolute";
    }
}

QString ToString(ControlPreset preset)
{
    switch (preset)
    {
        case ControlPreset::Easy: return "easy";
        case ControlPreset::Advanced: return "advanced";
        case ControlPreset::Tiktok: return "tiktok";
        default: return "easy";
    }
}

bool parseMotionState(const QString& str, MotionState& state)
{
    if (str == "Idle") { state = MotionState::Idle; return true; }
    if (str == "Ready") { state = MotionState::Ready; return true; }
    if (str == "Moving") { state = MotionState::Moving; return true; }
    LOG(ERROR) << "Unknown MotionState: " << str.toStdString();
    return false;
}

bool parseHandJoint(const QString& str, HandJoint& joint)
{
    if (str == "Wrist") { joint = HandJoint::Wrist; return true; }
    if (str == "ThumbCMC") { joint = HandJoint::ThumbCMC; return true; }
    if (str == "ThumbMCP") { joint = HandJoint::ThumbMCP; return true; }
    if (str == "ThumbIP") { joint = HandJoint::ThumbIP; return true; }
    if (str == "ThumbTIP") { joint = HandJoint::ThumbTIP; return true; }
    if (str == "IndexMCP") { joint = HandJoint::IndexMCP; return true; }
    if (str == "IndexPIP") { joint = HandJoint::IndexPIP; return true; }
    if (str == "IndexDIP") { joint = HandJoint::IndexDIP; return true; }
    if (str == "IndexTIP") { joint = HandJoint::IndexTIP; return true; }
    if (str == "MiddleMCP") { joint = HandJoint::MiddleMCP; return true; }
    if (str == "MiddlePIP") { joint = HandJoint::MiddlePIP; return true; }
    if (str == "MiddleDIP") { joint = HandJoint::MiddleDIP; return true; }
    if (str == "MiddleTIP") { joint = HandJoint::MiddleTIP; return true; }
    if (str == "RingMCP") { joint = HandJoint::RingMCP; return true; }
    if (str == "RingPIP") { joint = HandJoint::RingPIP; return true; }
    if (str == "RingDIP") { joint = HandJoint::RingDIP; return true; }
    if (str == "RingTIP") { joint = HandJoint::RingTIP; return true; }
    if (str == "PinkyMCP") { joint = HandJoint::PinkyMCP; return true; }
    if (str == "PinkyPIP") { joint = HandJoint::PinkyPIP; return true; }
    if (str == "PinkyDIP") { joint = HandJoint::PinkyDIP; return true; }
    if (str == "PinkyTIP") { joint = HandJoint::PinkyTIP; return true; }
    LOG(ERROR) << "Unknown HandJoint: " << str.toStdString();
    return false;
}

bool parseControlState(const QString& str, ControlState& state)
{
    if (str == "None") { state = ControlState::None; return true; }
    if (str == "LeftPressed") { state = ControlState::LeftPressed; return true; }
    if (str == "RightPressed") { state = ControlState::RightPressed; return true; }
    if (str == "VerticalScrolling") { state = ControlState::VerticalScrolling; return true; }
    if (str == "HorizontalScrolling") { state = ControlState::HorizontalScrolling; return true; }
    LOG(ERROR) << "Unknown ControlState: " << str.toStdString();
    return false;
}

bool parseCompareOp(const QString& str, CompareOp& op)
{
    if (str == "Less") { op = CompareOp::Less; return true; }
    if (str == "LessEqual") { op = CompareOp::LessEqual; return true; }
    if (str == "Greater") { op = CompareOp::Greater; return true; }
    if (str == "GreaterEqual") { op = CompareOp::GreaterEqual; return true; }
    if (str == "Equal") { op = CompareOp::Equal; return true; }
    if (str == "NotEqual") { op = CompareOp::NotEqual; return true; }
    LOG(ERROR) << "Unknown CompareOp: " << str.toStdString();
    return false;
}

bool parseLogicOp(const QString& str, LogicOp& op)
{
    if (str == "And") { op = LogicOp::And; return true; }
    if (str == "Or") { op = LogicOp::Or; return true; }
    LOG(ERROR) << "Unknown LogicOp: " << str.toStdString();
    return false;
}

bool parseMoveMode(const QString& str, MoveMode& mode)
{
    if (str == "Absolute") { mode = MoveMode::Absolute; return true; }
    if (str == "Relative") { mode = MoveMode::Relative; return true; }
    LOG(ERROR) << "Unknown MoveMode: " << str.toStdString();
    return false;
}

bool parseControlPreset(const QString& str, ControlPreset& preset)
{
    const QString name = str.trimmed().toLower();
    if (name == "easy") { preset = ControlPreset::Easy; return true; }
    if (name == "advanced") { preset = ControlPreset::Advanced; return true; }
    if (name == "tiktok") { preset = ControlPreset::Tiktok; return true; }
    LOG(ERROR) << "Unknown ControlPreset: " << str.toStdString();
    return false;
}

} // namespace Utils
} // namespace VisionCursor