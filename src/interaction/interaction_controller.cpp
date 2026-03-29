#include "interaction/interaction_controller.h"

#include <algorithm>
#include <cctype>

#include "algorithm/filter/ema_filter.hpp"
#include "algorithm/filter/one_euro_filter.hpp"
#include "algorithm/filter/sma_filter.hpp"
#include "config/fsm_config.h"
#include "interaction/factory/state_machine_factory.h"
#include "log/logging.h"

namespace VisionCursor
{
namespace Interaction
{
namespace
{

std::string toLowerCopy(const std::string& value)
{
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool tryParseBuiltInPreset(const std::string& scheme_name, ControlPreset& preset)
{
    const std::string name = toLowerCopy(scheme_name);

    if (name == "easy")
    {
        preset = ControlPreset::Easy;
        return true;
    }
    if (name == "advanced")
    {
        preset = ControlPreset::Advanced;
        return true;
    }
    if (name == "tiktok")
    {
        preset = ControlPreset::Tiktok;
        return true;
    }

    return false;
}

float getParamOrDefault(const std::vector<float>& params, std::size_t index, float default_value)
{
    return index < params.size() ? params[index] : default_value;
}

} // namespace

InteractionController::InteractionController() = default;

bool InteractionController::init(const std::string& scheme_name,
                                 const std::string& filter_name,
                                 const std::vector<float>& filter_params,
                                 HandJoint control_joint)
{
    clear();

    scheme_name_ = scheme_name;
    filter_name_ = filter_name;
    filter_params_ = filter_params;
    control_joint_ = control_joint;

    if (!rebuildStateMachine())
    {
        clear();
        return false;
    }

    if (!rebuildFilter())
    {
        clear();
        return false;
    }

    inited_ = true;
    LOG(INFO) << "InteractionController initialized. scheme=" << scheme_name_
              << ", filter=" << filter_name_;
    return true;
}

bool InteractionController::process(const HandLandmarkArray& landmarks, Action& action)
{
    action = Action{};

    if (!inited_)
    {
        LOG(ERROR) << "InteractionController is not initialized.";
        return false;
    }

    if (!fsm_)
    {
        LOG(ERROR) << "InteractionController state machine is null.";
        return false;
    }

    HandLandmarkArray input = landmarks;
    if (filter_)
    {
        input = filter_->process(landmarks);
    }

    action = fsm_->update(input);

    if (!input.isValid())
    {
        has_last_control_point_ = false;
        action.x = 0.0f;
        action.y = 0.0f;
        action.dx = 0.0f;
        action.dy = 0.0f;
        return true;
    }

    const HandLandmark& point = input[control_joint_];
    if (!point.has_normalized)
    {
        has_last_control_point_ = false;
        action.x = 0.0f;
        action.y = 0.0f;
        action.dx = 0.0f;
        action.dy = 0.0f;
        return true;
    }

    action.x = point.normalized.x;
    action.y = point.normalized.y;

    if (!has_last_control_point_)
    {
        action.dx = 0.0f;
        action.dy = 0.0f;
        has_last_control_point_ = true;
    }
    else
    {
        action.dx = point.normalized.x - last_x_;
        action.dy = point.normalized.y - last_y_;
    }

    last_x_ = point.normalized.x;
    last_y_ = point.normalized.y;

    return true;
}

bool InteractionController::reset()
{
    if (fsm_)
    {
        fsm_->reset();
    }

    if (filter_)
    {
        filter_->reset();
    }

    has_last_control_point_ = false;
    last_x_ = 0.0f;
    last_y_ = 0.0f;

    return true;
}

bool InteractionController::clear()
{
    if (fsm_)
    {
        fsm_->reset();
        fsm_.reset();
    }

    if (filter_)
    {
        filter_->reset();
        filter_.reset();
    }

    scheme_name_.clear();
    filter_name_.clear();
    filter_params_.clear();

    control_joint_ = HandJoint::IndexTIP;

    has_last_control_point_ = false;
    last_x_ = 0.0f;
    last_y_ = 0.0f;

    inited_ = false;
    return true;
}

bool InteractionController::rebuildStateMachine()
{
    fsm_.reset();

    ControlPreset preset = ControlPreset::Easy;
    if (tryParseBuiltInPreset(scheme_name_, preset))
    {
        fsm_ = FSM::StateMachineFactory::create(preset);
        if (!fsm_)
        {
            LOG(ERROR) << "Failed to create built-in state machine for scheme: " << scheme_name_;
            return false;
        }
        LOG(INFO) << "State machine created from built-in preset: " << scheme_name_;
        return true;
    }

    FSM::RuleSet rule_set;
    if (!Config::FsmConfig::load(rule_set, QString::fromStdString(scheme_name_)))
    {
        LOG(ERROR) << "Scheme is neither built-in nor valid FSM preset file: " << scheme_name_;
        return false;
    }

    fsm_ = FSM::StateMachineFactory::create(rule_set);
    if (!fsm_)
    {
        LOG(ERROR) << "Failed to create state machine from preset file: " << scheme_name_;
        return false;
    }

    LOG(INFO) << "State machine created from preset file: " << scheme_name_;
    return true;
}

bool InteractionController::rebuildFilter()
{
    filter_.reset();

    const std::string name = toLowerCopy(filter_name_);

    if (name.empty() || name == "none")
    {
        LOG(INFO) << "Interaction filter disabled.";
        return true;
    }

    if (name == "ema")
    {
        const float alpha = getParamOrDefault(filter_params_, 0, 0.5f);
        if (alpha <= 0.0f || alpha > 1.0f)
        {
            LOG(ERROR) << "EMA alpha must be in (0, 1]. alpha=" << alpha;
            return false;
        }

        filter_ = std::make_shared<Algorithm::EMAFilter>(alpha);
        LOG(INFO) << "Interaction filter set to EMA.";
        return true;
    }

    if (name == "sma")
    {
        const int window_size = static_cast<int>(getParamOrDefault(filter_params_, 0, 3.0f));
        if (window_size <= 0)
        {
            LOG(ERROR) << "SMA window_size must be > 0. window_size=" << window_size;
            return false;
        }

        filter_ = std::make_shared<Algorithm::SMAFilter>(static_cast<std::size_t>(window_size));
        LOG(INFO) << "Interaction filter set to SMA.";
        return true;
    }

    if (name == "one_euro")
    {
        const float min_cutoff = getParamOrDefault(filter_params_, 0, 12.0f);
        const float beta = getParamOrDefault(filter_params_, 1, 30.0f);
        const float d_cutoff = getParamOrDefault(filter_params_, 2, 1.0f);
        const float frequency = getParamOrDefault(filter_params_, 3, 60.0f);

        if (min_cutoff <= 0.0f)
        {
            LOG(ERROR) << "OneEuro minCutoff must be > 0. minCutoff=" << min_cutoff;
            return false;
        }
        if (d_cutoff <= 0.0f)
        {
            LOG(ERROR) << "OneEuro dCutoff must be > 0. dCutoff=" << d_cutoff;
            return false;
        }
        if (frequency <= 0.0f)
        {
            LOG(ERROR) << "OneEuro frequency must be > 0. frequency=" << frequency;
            return false;
        }

        filter_ = std::make_shared<Algorithm::OneEuroFilter>(min_cutoff, beta, d_cutoff, frequency);
        LOG(INFO) << "Interaction filter set to OneEuro.";
        return true;
    }

    LOG(ERROR) << "Unknown filter_name: " << filter_name_;
    return false;
}

} // namespace Interaction
} // namespace VisionCursor
