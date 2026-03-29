#include "interaction/factory/state_machine_factory.h"

#include "config/fsm_config.h"
#include "log/logging.h"
#include "utils/to_string.h"

namespace VisionCursor
{
namespace Interaction
{
namespace FSM
{

std::unique_ptr<StateMachine> StateMachineFactory::create(ControlPreset preset)
{
    if (!Config::FsmConfig::ensureBuiltinPresets(false))
    {
        LOG(ERROR) << "Failed to ensure built-in FSM preset files.";
        return nullptr;
    }

    RuleSet rules;
    const QString preset_name = Utils::ToString(preset);
    if (!Config::FsmConfig::load(rules, preset_name))
    {
        LOG(ERROR) << "Failed to load built-in FSM preset: " << preset_name.toStdString();
        return nullptr;
    }

    return create(rules);
}

std::unique_ptr<StateMachine> StateMachineFactory::create(const RuleSet& rules)
{
    auto fsm = std::make_unique<StateMachine>();
    applyRuleSet(*fsm, rules);
    return fsm;
}

void StateMachineFactory::applyRuleSet(StateMachine& fsm, const RuleSet& rules)
{
    fsm.clearTransitions();
    fsm.reset();

    fsm.addMotionTransitions(rules.motion_transitions);
    fsm.addButtonTransitions(rules.button_transitions);
}

} // namespace FSM
} // namespace Interaction
} // namespace VisionCursor
