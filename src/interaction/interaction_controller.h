#pragma once

#include <memory>
#include <string>
#include <vector>

#include "algorithm/filter/filter.h"
#include "common/common.h"
#include "interaction/action/action.h"
#include "interaction/fsm/state_machine.h"

namespace VisionCursor
{
namespace Interaction
{

class InteractionController
{
public:
    InteractionController();

    bool init(const std::string& scheme_name,
              const std::string& filter_name,
              const std::vector<float>& filter_params,
              HandJoint control_joint);

    bool process(const HandLandmarkArray& landmarks, Action& action);

    bool reset();
    bool clear();

    bool isInited() const { return inited_; }

private:
    bool rebuildStateMachine();
    bool rebuildFilter();

private:
    std::string scheme_name_;
    std::string filter_name_;
    std::vector<float> filter_params_;
    HandJoint control_joint_ = HandJoint::IndexTIP;

    std::unique_ptr<FSM::StateMachine> fsm_;
    std::shared_ptr<Algorithm::HandFilter> filter_;

    bool has_last_control_point_ = false;
    float last_x_ = 0.0f;
    float last_y_ = 0.0f;

    bool inited_ = false;
};

} // namespace Interaction
} // namespace VisionCursor
