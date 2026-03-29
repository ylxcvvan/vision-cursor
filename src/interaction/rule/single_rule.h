#pragma once

#include <deque>
#include <memory>

#include "common/common.h"
#include "interaction/rule/rule.h"

namespace VisionCursor
{
namespace Interaction
{
namespace Rule
{

class DistanceRule : public Rule
{
public:
    DistanceRule(HandJoint joint_a, HandJoint joint_b, CompareOp compare_op, float threshold);

    bool evaluate(const HandLandmarkArray& landmarks) override;
    void reset() override;

private:
    HandJoint joint_a_;
    HandJoint joint_b_;
    CompareOp compare_op_;
    float threshold_ = 0.0f;
};

class AngleRule : public Rule
{
public:
    AngleRule(HandJoint joint_a, HandJoint joint_b, HandJoint joint_c, CompareOp compare_op, float threshold_degree);

    bool evaluate(const HandLandmarkArray& landmarks) override;
    void reset() override;

private:
    HandJoint joint_a_;
    HandJoint joint_b_;
    HandJoint joint_c_;
    CompareOp compare_op_;
    float threshold_degree_ = 0.0f;
};

class ConsecutiveFrameRule : public Rule
{
public:
    ConsecutiveFrameRule(RulePtr base_rule, int required_frames);

    bool evaluate(const HandLandmarkArray& landmarks) override;
    void reset() override;

private:
    RulePtr base_rule_;
    int required_frames_ = 1;
    int current_count_ = 0;
};

class HandDetectRule : public Rule
{
public:
    explicit HandDetectRule(bool should_detect = true);

    bool evaluate(const HandLandmarkArray& landmarks) override;
    void reset() override;

private:
    bool should_detect_ = true;
};

class MoveDistanceRule : public Rule
{
public:
    MoveDistanceRule(HandJoint joint, CompareOp compare_op, float threshold, int frame_window);

    bool evaluate(const HandLandmarkArray& landmarks) override;
    void reset() override;

private:
    HandJoint joint_;
    CompareOp compare_op_;
    float threshold_ = 0.0f;
    int frame_window_ = 1;

    std::deque<HandLandmark> history_;
};

class BoolRule : public Rule
{
public:
    explicit BoolRule(bool value);

    bool evaluate(const HandLandmarkArray& landmarks) override;
    void reset() override;

private:
    bool value_ = true;
};

} // namespace Rule
} // namespace Interaction
} // namespace VisionCursor
