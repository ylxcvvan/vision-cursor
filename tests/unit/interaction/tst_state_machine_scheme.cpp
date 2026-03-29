#include <gtest/gtest.h>

#include <array>
#include <memory>

#include "config/fsm_config.h"
#include "interaction/factory/state_machine_factory.h"
#include "interaction/rule/single_rule.h"
#include "test_helpers.h"
#include "vision_cursor_test_fixture.h"

using namespace VisionCursor;

namespace
{

const char* presetName(ControlPreset preset)
{
    switch (preset)
    {
        case ControlPreset::Easy:
            return "easy";
        case ControlPreset::Advanced:
            return "advanced";
        case ControlPreset::Tiktok:
            return "tiktok";
        default:
            return "unknown";
    }
}

} // namespace

TEST(StateMachineSchemeTest, BuiltinEasyPresetCanBeCreated)
{
    ASSERT_TRUE(Config::FsmConfig::ensureBuiltinPresets(false));

    auto fsm = Interaction::FSM::StateMachineFactory::create(ControlPreset::Easy);
    ASSERT_NE(fsm, nullptr);

    const Action action = fsm->update(Tests::makeInvalidLandmarks());
    EXPECT_EQ(action.type, ActionType::None);
    EXPECT_EQ(fsm->currentMotionState(), MotionState::Idle);
    EXPECT_EQ(fsm->currentControlState(), ControlState::None);
}

TEST(StateMachineSchemeTest, CustomRuleSetTransitionsAndProducesAction)
{
    using Interaction::FSM::ButtonTransition;
    using Interaction::FSM::MotionTransition;
    using Interaction::FSM::RuleSet;
    using Interaction::Rule::BoolRule;

    RuleSet rules;
    rules.motion_transitions.emplace_back(
        MotionTransition(MotionState::Idle, MotionState::Moving, std::make_shared<BoolRule>(true), 10));
    rules.button_transitions.emplace_back(
        ButtonTransition(ControlState::None, ControlState::LeftPressed, std::make_shared<BoolRule>(true), 10));

    auto fsm = Interaction::FSM::StateMachineFactory::create(rules);
    ASSERT_NE(fsm, nullptr);

    const HandLandmarkArray landmarks = Tests::makeLandmarksForControl(0.5f, 0.5f);

    const Action first = fsm->update(landmarks);
    EXPECT_EQ(first.type, ActionType::LeftDown);
    EXPECT_TRUE(first.isMoving);
    EXPECT_EQ(fsm->currentMotionState(), MotionState::Moving);
    EXPECT_EQ(fsm->currentControlState(), ControlState::LeftPressed);

    const Action second = fsm->update(landmarks);
    EXPECT_EQ(second.type, ActionType::None);
    EXPECT_TRUE(second.isMoving);
}

TEST(StateMachineSchemeTest, HigherPriorityTransitionWins)
{
    using Interaction::FSM::MotionTransition;
    using Interaction::FSM::RuleSet;
    using Interaction::Rule::BoolRule;

    RuleSet rules;
    rules.motion_transitions.emplace_back(
        MotionTransition(MotionState::Idle, MotionState::Ready, std::make_shared<BoolRule>(true), 1));
    rules.motion_transitions.emplace_back(
        MotionTransition(MotionState::Idle, MotionState::Moving, std::make_shared<BoolRule>(true), 100));

    auto fsm = Interaction::FSM::StateMachineFactory::create(rules);
    ASSERT_NE(fsm, nullptr);

    (void)fsm->update(Tests::makeLandmarksForControl());
    EXPECT_EQ(fsm->currentMotionState(), MotionState::Moving);
}

TEST_F(VisionCursorTestFixture, BuiltinPresetsRealtimeSmoke)
{
    const std::array<ControlPreset, 3> presets = {
        ControlPreset::Easy,
        ControlPreset::Advanced,
        ControlPreset::Tiktok,
    };

    for (const ControlPreset preset : presets)
    {
        auto fsm = Interaction::FSM::StateMachineFactory::create(preset);
        ASSERT_NE(fsm, nullptr) << "failed to create preset " << presetName(preset);

        int update_count = 0;
        for (int i = 0; i < 50; ++i)
        {
            FramePtr frame;
            if (!p_camera_device->capture(frame))
            {
                continue;
            }

            HandLandmarkArray landmarks;
            if (!mp_hand_detector->process(frame, landmarks))
            {
                continue;
            }

            (void)fsm->update(landmarks);
            ++update_count;

            if (update_count >= 8)
            {
                break;
            }
        }

        EXPECT_GT(update_count, 0) << "no frame was processed for preset " << presetName(preset);
    }
}

