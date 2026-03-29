#pragma once


#include <map>
#include <vector>
#include <utility>

#include "common/common.h"
#include "interaction/action/action.h"
#include "interaction/fsm/transition.h"
#include "interaction/rule/rule.h"

namespace VisionCursor
{
namespace Interaction
{
namespace FSM
{

class StateMachine
{
public:
    StateMachine()  = default;
    ~StateMachine() = default;

    void addMotionTransition(const MotionTransition& transition);
    void addMotionTransitions(const std::vector<MotionTransition>& transitions);

    void addButtonTransition(const ButtonTransition& transition);
    void addButtonTransitions(const std::vector<ButtonTransition>& transitions);

    void clearTransitions();
    void reset();

    Action update(const HandLandmarkArray& landmarks);

    MotionState currentMotionState() const { return current_motion_state_; }
    MotionState lastMotionState() const { return last_motion_state_; }

    ControlState currentControlState() const { return current_control_state_; }
    ControlState lastControlState() const { return last_control_state_; }

private:
    const MotionTransition* findTriggeredMotionTransition(const HandLandmarkArray& landmarks);
    const ButtonTransition* findTriggeredButtonTransition(const HandLandmarkArray& landmarks);

    Action makeAction(const HandLandmarkArray& landmarks);

    void transitMotionTo(MotionState new_state);
    void transitButtonTo(ControlState new_state);

    void sortMotionTransitionsOfState(MotionState state);
    void sortButtonTransitionsOfState(ControlState state);

private:
    MotionState last_motion_state_    = MotionState::Idle;
    MotionState current_motion_state_ = MotionState::Idle;

    ControlState last_control_state_    = ControlState::None;
    ControlState current_control_state_ = ControlState::None;

    std::map<MotionState, std::vector<MotionTransition>> motion_transitions_;
    std::map<ControlState, std::vector<ButtonTransition>> button_transitions_;
};

}      // namespace FSM
}      // namespace Interaction
}      // namespace VisionCursor