#pragma once

#include <memory>
#include <vector>

#include "interaction/fsm/state_machine.h"

namespace VisionCursor
{
namespace Interaction
{
namespace FSM
{

struct RuleSet
{
    std::vector<MotionTransition> motion_transitions;
    std::vector<ButtonTransition> button_transitions;
};

class StateMachineFactory
{
public:
    static std::unique_ptr<StateMachine> create(ControlPreset preset);
    static std::unique_ptr<StateMachine> create(const RuleSet& rules);

private:
    static void applyRuleSet(StateMachine& fsm, const RuleSet& rules);
};

} // namespace FSM
} // namespace Interaction
} // namespace VisionCursor
