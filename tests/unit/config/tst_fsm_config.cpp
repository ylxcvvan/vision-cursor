#include <gtest/gtest.h>

#include <QFile>

#include "config/fsm_config.h"
#include "interaction/factory/state_machine_factory.h"
#include "test_helpers.h"

using namespace VisionCursor;

TEST(FsmConfigTest, EnsureBuiltinPresetsAndLoad)
{
    Tests::ensureCoreApp();

    ASSERT_TRUE(Config::FsmConfig::ensureBuiltinPresets(true));

    Interaction::FSM::RuleSet easy_rules;
    Interaction::FSM::RuleSet advanced_rules;
    Interaction::FSM::RuleSet tiktok_rules;

    ASSERT_TRUE(Config::FsmConfig::load(easy_rules, "easy"));
    ASSERT_TRUE(Config::FsmConfig::load(advanced_rules, "advanced"));
    ASSERT_TRUE(Config::FsmConfig::load(tiktok_rules, "tiktok"));

    EXPECT_FALSE(easy_rules.motion_transitions.empty());
    EXPECT_FALSE(easy_rules.button_transitions.empty());
    EXPECT_FALSE(advanced_rules.motion_transitions.empty());
    EXPECT_FALSE(tiktok_rules.motion_transitions.empty());
}

TEST(FsmConfigTest, GenerateCustomPresetAndLoad)
{
    Tests::ensureCoreApp();

    const QString preset_name = "ut_custom_easy";
    ASSERT_TRUE(Config::FsmConfig::generatePreset(preset_name, ControlPreset::Easy, true));

    Interaction::FSM::RuleSet rules;
    ASSERT_TRUE(Config::FsmConfig::load(rules, preset_name));

    auto fsm = Interaction::FSM::StateMachineFactory::create(rules);
    ASSERT_NE(fsm, nullptr);
}

TEST(FsmConfigTest, LoadMissingPresetFails)
{
    Tests::ensureCoreApp();

    const QString missing_preset = "ut_missing_preset";
    QFile::remove(Config::FsmConfig::presetFilePath(missing_preset));

    Interaction::FSM::RuleSet rules;
    EXPECT_FALSE(Config::FsmConfig::load(rules, missing_preset));
}
