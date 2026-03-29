#include "interaction/rule/single_rule.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "utils/utils.h"

namespace VisionCursor
{
namespace Interaction
{
namespace Rule
{
namespace
{

constexpr float kReferenceLengthUnit = 10.0f;
constexpr float kMinReferenceLength = 1e-6f;
constexpr float kCompareEpsilon = 1e-4f;

bool pickUnifiedCoordSpace(const HandLandmark& a, const HandLandmark& b, bool& use_normalized)
{
    if (a.has_normalized && b.has_normalized)
    {
        use_normalized = true;
        return true;
    }

    if (a.has_world && b.has_world)
    {
        use_normalized = false;
        return true;
    }

    return false;
}

bool pickUnifiedCoordSpace(const HandLandmark& a, const HandLandmark& b, const HandLandmark& c, bool& use_normalized)
{
    if (a.has_normalized && b.has_normalized && c.has_normalized)
    {
        use_normalized = true;
        return true;
    }

    if (a.has_world && b.has_world && c.has_world)
    {
        use_normalized = false;
        return true;
    }

    return false;
}

Landmark toCoord(const HandLandmark& point, bool use_normalized)
{
    return use_normalized ? point.normalized : point.world;
}

bool computeDistance(const HandLandmark& a, const HandLandmark& b, float& out_distance)
{
    bool use_normalized = false;
    if (!pickUnifiedCoordSpace(a, b, use_normalized))
    {
        return false;
    }

    const Landmark pa = toCoord(a, use_normalized);
    const Landmark pb = toCoord(b, use_normalized);
    out_distance = Utils::distance3D(pa, pb);
    return true;
}

bool computeAngle(const HandLandmark& a, const HandLandmark& b, const HandLandmark& c, float& out_angle_degree)
{
    bool use_normalized = false;
    if (!pickUnifiedCoordSpace(a, b, c, use_normalized))
    {
        return false;
    }

    const Landmark pa = toCoord(a, use_normalized);
    const Landmark pb = toCoord(b, use_normalized);
    const Landmark pc = toCoord(c, use_normalized);
    out_angle_degree = Utils::angle3D(pa, pb, pc);
    return true;
}

bool compareValue(float lhs, CompareOp op, float rhs)
{
    switch (op)
    {
        case CompareOp::Less:
            return lhs < rhs;
        case CompareOp::LessEqual:
            return lhs <= rhs;
        case CompareOp::Greater:
            return lhs > rhs;
        case CompareOp::GreaterEqual:
            return lhs >= rhs;
        case CompareOp::Equal:
            return Utils::floatEqual(lhs, rhs, kCompareEpsilon);
        case CompareOp::NotEqual:
            return !Utils::floatEqual(lhs, rhs, kCompareEpsilon);
        default:
            return false;
    }
}

float referenceLength(const HandLandmarkArray& landmarks)
{
    const auto& wrist = landmarks[HandJoint::Wrist];
    const auto& index_mcp = landmarks[HandJoint::IndexMCP];
    const auto& pinky_mcp = landmarks[HandJoint::PinkyMCP];

    bool use_normalized = false;
    if (!pickUnifiedCoordSpace(wrist, index_mcp, pinky_mcp, use_normalized))
    {
        return 0.0f;
    }

    const Landmark wrist_point = toCoord(wrist, use_normalized);
    const Landmark index_point = toCoord(index_mcp, use_normalized);
    const Landmark pinky_point = toCoord(pinky_mcp, use_normalized);

    return std::sqrt(std::max(Utils::triangleArea3D(wrist_point, index_point, pinky_point), 0.0f));
}

} // namespace

DistanceRule::DistanceRule(HandJoint joint_a, HandJoint joint_b, CompareOp compare_op, float threshold)
    : joint_a_(joint_a), joint_b_(joint_b), compare_op_(compare_op), threshold_(threshold)
{
}

bool DistanceRule::evaluate(const HandLandmarkArray& landmarks)
{
    if (!landmarks.isValid())
    {
        return false;
    }

    const float ref_length = referenceLength(landmarks);
    if (ref_length <= kMinReferenceLength)
    {
        return false;
    }

    const HandLandmark& a = landmarks[joint_a_];
    const HandLandmark& b = landmarks[joint_b_];

    float raw_distance = 0.0f;
    if (!computeDistance(a, b, raw_distance))
    {
        return false;
    }

    const float normalized_distance = raw_distance / ref_length * kReferenceLengthUnit;

    return compareValue(normalized_distance, compare_op_, threshold_);
}

void DistanceRule::reset()
{
}

AngleRule::AngleRule(HandJoint joint_a,
                     HandJoint joint_b,
                     HandJoint joint_c,
                     CompareOp compare_op,
                     float threshold_degree)
    : joint_a_(joint_a), joint_b_(joint_b), joint_c_(joint_c), compare_op_(compare_op),
      threshold_degree_(threshold_degree)
{
}

bool AngleRule::evaluate(const HandLandmarkArray& landmarks)
{
    if (!landmarks.isValid())
    {
        return false;
    }

    const HandLandmark& a = landmarks[joint_a_];
    const HandLandmark& b = landmarks[joint_b_];
    const HandLandmark& c = landmarks[joint_c_];

    float angle_degree = 0.0f;
    if (!computeAngle(a, b, c, angle_degree))
    {
        return false;
    }

    return compareValue(angle_degree, compare_op_, threshold_degree_);
}

void AngleRule::reset()
{
}

ConsecutiveFrameRule::ConsecutiveFrameRule(RulePtr base_rule, int required_frames)
    : base_rule_(std::move(base_rule)), required_frames_(required_frames > 0 ? required_frames : 1)
{
}

bool ConsecutiveFrameRule::evaluate(const HandLandmarkArray& landmarks)
{
    if (!base_rule_)
    {
        return false;
    }

    if (!landmarks.isValid())
    {
        current_count_ = 0;
        base_rule_->reset();
        return false;
    }

    if (base_rule_->evaluate(landmarks))
    {
        ++current_count_;
    }
    else
    {
        current_count_ = 0;
    }

    if (current_count_ >= required_frames_)
    {
        current_count_ = 0;
        return true;
    }

    return false;
}

void ConsecutiveFrameRule::reset()
{
    current_count_ = 0;

    if (base_rule_)
    {
        base_rule_->reset();
    }
}

HandDetectRule::HandDetectRule(bool should_detect) : should_detect_(should_detect) {}

bool HandDetectRule::evaluate(const HandLandmarkArray& landmarks)
{
    const bool detected = landmarks.isValid();
    return should_detect_ ? detected : !detected;
}

void HandDetectRule::reset()
{
}

MoveDistanceRule::MoveDistanceRule(HandJoint joint, CompareOp compare_op, float threshold, int frame_window)
    : joint_(joint), compare_op_(compare_op), threshold_(threshold), frame_window_(frame_window > 1 ? frame_window : 1)
{
}

bool MoveDistanceRule::evaluate(const HandLandmarkArray& landmarks)
{
    if (!landmarks.isValid())
    {
        history_.clear();
        return false;
    }

    if (frame_window_ < 1)
    {
        history_.clear();
        return false;
    }

    const float ref_length = referenceLength(landmarks);
    if (ref_length <= kMinReferenceLength)
    {
        history_.clear();
        return false;
    }

    const HandLandmark& current = landmarks[joint_];
    if (!current.isValid())
    {
        history_.clear();
        return false;
    }

    history_.push_back(current);
    while (static_cast<int>(history_.size()) > frame_window_)
    {
        history_.pop_front();
    }

    if (history_.size() < 2)
    {
        return false;
    }

    float raw_distance = 0.0f;
    if (!computeDistance(history_.front(), history_.back(), raw_distance))
    {
        history_.clear();
        return false;
    }

    const float normalized_distance = raw_distance / ref_length * kReferenceLengthUnit;

    const bool matched = compareValue(normalized_distance, compare_op_, threshold_);
    if (matched)
    {
        history_.clear();
    }

    return matched;
}

void MoveDistanceRule::reset()
{
    history_.clear();
}

BoolRule::BoolRule(bool value) : value_(value) {}

bool BoolRule::evaluate(const HandLandmarkArray&)
{
    return value_;
}

void BoolRule::reset()
{
}

} // namespace Rule
} // namespace Interaction
} // namespace VisionCursor
