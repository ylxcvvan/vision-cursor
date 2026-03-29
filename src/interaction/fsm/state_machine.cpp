#include "interaction/fsm/state_machine.h"

#include <algorithm>
#include <set>

namespace VisionCursor
{
namespace Interaction
{
namespace FSM
{

void StateMachine::addMotionTransition(const MotionTransition& transition)
{
    auto& edges = motion_transitions_[transition.from()];
    edges.push_back(transition);
    sortMotionTransitionsOfState(transition.from());
}

void StateMachine::addMotionTransitions(const std::vector<MotionTransition>& transitions)
{
    std::set<MotionState> affected_states;

    for (const auto& transition : transitions)
    {
        motion_transitions_[transition.from()].push_back(transition);
        affected_states.insert(transition.from());
    }

    for (MotionState state : affected_states)
    {
        sortMotionTransitionsOfState(state);
    }
}

void StateMachine::addButtonTransition(const ButtonTransition& transition)
{
    auto& edges = button_transitions_[transition.from()];
    edges.push_back(transition);
    sortButtonTransitionsOfState(transition.from());
}

void StateMachine::addButtonTransitions(const std::vector<ButtonTransition>& transitions)
{
    std::set<ControlState> affected_states;

    for (const auto& transition : transitions)
    {
        button_transitions_[transition.from()].push_back(transition);
        affected_states.insert(transition.from());
    }

    for (ControlState state : affected_states)
    {
        sortButtonTransitionsOfState(state);
    }
}

void StateMachine::clearTransitions()
{
    motion_transitions_.clear();
    button_transitions_.clear();
}

void StateMachine::reset()
{
    last_motion_state_    = MotionState::Idle;
    current_motion_state_ = MotionState::Idle;

    last_control_state_    = ControlState::None;
    current_control_state_ = ControlState::None;

    for (auto& [_, edges] : motion_transitions_)
    {
        for (auto& transition : edges)
        {
            transition.reset();
        }
    }

    for (auto& [_, edges] : button_transitions_)
    {
        for (auto& transition : edges)
        {
            transition.reset();
        }
    }
}

Action StateMachine::update(const HandLandmarkArray& landmarks)
{
    last_motion_state_  = current_motion_state_;
    last_control_state_ = current_control_state_;

    const MotionTransition* motion_triggered = findTriggeredMotionTransition(landmarks);
    if (motion_triggered != nullptr)
    {
        transitMotionTo(motion_triggered->to());
    }

    const ButtonTransition* button_triggered = findTriggeredButtonTransition(landmarks);
    if (button_triggered != nullptr)
    {
        transitButtonTo(button_triggered->to());
    }

    return makeAction(landmarks);
}

const MotionTransition* StateMachine::findTriggeredMotionTransition(const HandLandmarkArray& landmarks)
{
    auto it = motion_transitions_.find(current_motion_state_);
    if (it == motion_transitions_.end())
    {
        return nullptr;
    }

    auto& edges = it->second;
    for (auto& transition : edges)
    {
        if (!transition.isValid())
        {
            continue;
        }

        if (transition.evaluate(landmarks))
        {
            return &transition;
        }
    }

    return nullptr;
}

const ButtonTransition* StateMachine::findTriggeredButtonTransition(const HandLandmarkArray& landmarks)
{
    auto it = button_transitions_.find(current_control_state_);
    if (it == button_transitions_.end())
    {
        return nullptr;
    }

    auto& edges = it->second;
    for (auto& transition : edges)
    {
        if (!transition.isValid())
        {
            continue;
        }

        if (transition.evaluate(landmarks))
        {
            return &transition;
        }
    }

    return nullptr;
}

void StateMachine::transitMotionTo(MotionState new_state)
{
    if (new_state == current_motion_state_)
    {
        return;
    }

    current_motion_state_ = new_state;
}

void StateMachine::transitButtonTo(ControlState new_state)
{
    if (new_state == current_control_state_)
    {
        return;
    }

    current_control_state_ = new_state;
}

void StateMachine::sortMotionTransitionsOfState(MotionState state)
{
    auto it = motion_transitions_.find(state);
    if (it == motion_transitions_.end())
    {
        return;
    }

    auto& edges = it->second;
    std::sort(edges.begin(), edges.end(), [](const MotionTransition& lhs, const MotionTransition& rhs) {
        return lhs.priority() > rhs.priority();
    });
}

void StateMachine::sortButtonTransitionsOfState(ControlState state)
{
    auto it = button_transitions_.find(state);
    if (it == button_transitions_.end())
    {
        return;
    }

    auto& edges = it->second;
    std::sort(edges.begin(), edges.end(), [](const ButtonTransition& lhs, const ButtonTransition& rhs) {
        return lhs.priority() > rhs.priority();
    });
}

Action StateMachine::makeAction(const HandLandmarkArray& landmarks)
{
    (void)landmarks;

    Action action;

    // 1. 处理交互状态变化产生的瞬时事件
    if (last_control_state_ == ControlState::None && current_control_state_ == ControlState::LeftPressed)
    {
        action.type = ActionType::LeftDown;
    }
    else if (last_control_state_ == ControlState::LeftPressed && current_control_state_ == ControlState::None)
    {
        action.type = ActionType::LeftUp;
    }
    else if (last_control_state_ == ControlState::None && current_control_state_ == ControlState::RightPressed)
    {
        action.type = ActionType::RightDown;
    }
    else if (last_control_state_ == ControlState::RightPressed && current_control_state_ == ControlState::None)
    {
        action.type = ActionType::RightUp;
    }
    else if (current_control_state_ == ControlState::VerticalScrolling)
    {
        action.type = ActionType::ScrollVertical;
    }
    else if (current_control_state_ == ControlState::HorizontalScrolling)
    {
        action.type = ActionType::ScrollHorizontal;
    }
    else
    {
        action.type = ActionType::None;
    }

    // 2. 处理指针移动
    action.isMoving = (current_motion_state_ == MotionState::Moving) &&
                      (current_control_state_ != ControlState::VerticalScrolling) &&
                      (current_control_state_ != ControlState::HorizontalScrolling);

    return action;
}

}      // namespace FSM
}      // namespace Interaction
}      // namespace VisionCursor