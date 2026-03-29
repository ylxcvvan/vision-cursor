#pragma once


#include <utility>

#include "interaction/rule/rule.h"

namespace VisionCursor
{
namespace Interaction
{
namespace FSM
{

class MotionTransition
{
public:
    MotionTransition(MotionState from, MotionState to, Rule::RulePtr rule, int priority = 0)
        : from_(from), to_(to), rule_(std::move(rule)), priority_(priority)
    {
    }

    MotionState from() const { return from_; }
    MotionState to() const { return to_; }

    bool evaluate(const HandLandmarkArray& landmarks) { return rule_ ? rule_->evaluate(landmarks) : false; }

    void reset()
    {
        if (rule_)
        {
            rule_->reset();
        }
    }

    int priority() const { return priority_; }

    const Rule::RulePtr& rule() const { return rule_; }

    bool isValid() const { return rule_ != nullptr; }

private:
    MotionState from_;
    MotionState to_;
    Rule::RulePtr rule_;
    int priority_ = 0;
};


class ButtonTransition
{
public:
    ButtonTransition(ControlState from, ControlState to, Rule::RulePtr rule, int priority = 0)
        : from_(from), to_(to), rule_(std::move(rule)), priority_(priority)
    {
    }

    ControlState from() const { return from_; }
    ControlState to() const { return to_; }

    bool evaluate(const HandLandmarkArray& landmarks) { return rule_ ? rule_->evaluate(landmarks) : false; }

    void reset()
    {
        if (rule_)
        {
            rule_->reset();
        }
    }

    int priority() const { return priority_; }

    const Rule::RulePtr& rule() const { return rule_; }

    bool isValid() const { return rule_ != nullptr; }

private:
    ControlState from_;
    ControlState to_;
    Rule::RulePtr rule_;
    int priority_ = 0;
};

}      // namespace FSM
}      // namespace Interaction
}      // namespace VisionCursor