#pragma once


#include <utility>

#include "interaction/rule/rule.h"
#include "common/common.h"
namespace VisionCursor
{
namespace Interaction
{
namespace Rule
{


class CompositeRule : public Rule
{
public:
    CompositeRule(LogicOp logic,
                  RulePtr left_rule,
                  RulePtr right_rule);

    bool evaluate(const HandLandmarkArray& landmarks) override;
    void reset() override;

    LogicOp logic() const { return logic_; }
    const RulePtr& leftRule() const { return left_rule_; }
    const RulePtr& rightRule() const { return right_rule_; }

private:
    LogicOp logic_;
    RulePtr left_rule_;
    RulePtr right_rule_;
};

} // namespace Rule
} // namespace Interaction
} // namespace VisionCursor