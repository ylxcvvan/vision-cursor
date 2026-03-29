#include "interaction/rule/composite_rule.h"

namespace VisionCursor
{
namespace Interaction
{
namespace Rule
{

CompositeRule::CompositeRule(LogicOp logic, RulePtr left_rule, RulePtr right_rule)
    : logic_(logic), left_rule_(std::move(left_rule)), right_rule_(std::move(right_rule))
{
}

bool CompositeRule::evaluate(const HandLandmarkArray& landmarks)
{
    if (!left_rule_ || !right_rule_)
    {
        return false;
    }

    switch (logic_)
    {
        case LogicOp::And: {
            const bool left_result = left_rule_->evaluate(landmarks);
            if (!left_result)
            {
                return false;
            }
            return right_rule_->evaluate(landmarks);
        }

        case LogicOp::Or: {
            const bool left_result = left_rule_->evaluate(landmarks);
            if (left_result)
            {
                return true;
            }
            return right_rule_->evaluate(landmarks);
        }

        default:
            return false;
    }
}

void CompositeRule::reset()
{
    if (left_rule_)
    {
        left_rule_->reset();
    }

    if (right_rule_)
    {
        right_rule_->reset();
    }
}

}      // namespace Rule
}      // namespace Interaction
}      // namespace VisionCursor