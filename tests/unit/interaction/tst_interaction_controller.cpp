#include <gtest/gtest.h>

#include "interaction/interaction_controller.h"
#include "test_helpers.h"
#include "vision_cursor_test_fixture.h"

using namespace VisionCursor;

TEST(InteractionControllerTest, InitWithBuiltinPresetAndNoneFilter)
{
    Interaction::InteractionController controller;
    ASSERT_TRUE(controller.init("easy", "none", {}, HandJoint::IndexTIP));
    EXPECT_TRUE(controller.isInited());

    const HandLandmarkArray landmarks = Tests::makeLandmarksForControl(0.62f, 0.41f);
    Action action;
    ASSERT_TRUE(controller.process(landmarks, action));

    EXPECT_NEAR(action.x, 0.62f, 1e-5f);
    EXPECT_NEAR(action.y, 0.41f, 1e-5f);
}

TEST(InteractionControllerTest, InitWithUnknownSchemeShouldFail)
{
    Interaction::InteractionController controller;

    EXPECT_FALSE(controller.init("__unknown_scheme__", "none", {}, HandJoint::IndexTIP));
    EXPECT_FALSE(controller.isInited());
}

TEST(InteractionControllerTest, InitWithUnknownFilterShouldFail)
{
    Interaction::InteractionController controller;

    EXPECT_FALSE(controller.init("easy", "__unknown_filter__", {}, HandJoint::IndexTIP));
    EXPECT_FALSE(controller.isInited());
}

TEST(InteractionControllerTest, InitWithInvalidFilterParamsShouldFail)
{
    Interaction::InteractionController controller;

    EXPECT_FALSE(controller.init("easy", "ema", {2.0f}, HandJoint::IndexTIP));
    EXPECT_FALSE(controller.init("easy", "sma", {0.0f}, HandJoint::IndexTIP));
    EXPECT_FALSE(controller.init("easy", "one_euro", {0.0f, 0.2f, 1.0f, 60.0f}, HandJoint::IndexTIP));
}

TEST(InteractionControllerTest, ProcessBeforeInitShouldFail)
{
    Interaction::InteractionController controller;

    Action action;
    EXPECT_FALSE(controller.process(Tests::makeLandmarksForControl(), action));
}

TEST(InteractionControllerTest, ProcessInvalidLandmarksShouldSucceedWithZeroMotion)
{
    Interaction::InteractionController controller;
    ASSERT_TRUE(controller.init("easy", "none", {}, HandJoint::IndexTIP));

    Action action;
    EXPECT_TRUE(controller.process(Tests::makeInvalidLandmarks(), action));
    EXPECT_EQ(action.type, ActionType::None);
    EXPECT_FALSE(action.isMoving);
    EXPECT_FLOAT_EQ(action.dx, 0.0f);
    EXPECT_FLOAT_EQ(action.dy, 0.0f);
}

TEST(InteractionControllerTest, ProcessComputesDeltaAcrossFrames)
{
    Interaction::InteractionController controller;
    ASSERT_TRUE(controller.init("easy", "none", {}, HandJoint::IndexTIP));

    Action first;
    ASSERT_TRUE(controller.process(Tests::makeLandmarksForControl(0.20f, 0.30f), first));
    EXPECT_NEAR(first.x, 0.20f, 1e-5f);
    EXPECT_NEAR(first.y, 0.30f, 1e-5f);
    EXPECT_FLOAT_EQ(first.dx, 0.0f);
    EXPECT_FLOAT_EQ(first.dy, 0.0f);

    Action second;
    ASSERT_TRUE(controller.process(Tests::makeLandmarksForControl(0.35f, 0.50f), second));
    EXPECT_NEAR(second.x, 0.35f, 1e-5f);
    EXPECT_NEAR(second.y, 0.50f, 1e-5f);
    EXPECT_NEAR(second.dx, 0.15f, 1e-5f);
    EXPECT_NEAR(second.dy, 0.20f, 1e-5f);
}

TEST(InteractionControllerTest, ResetAndClearKeepControllerStable)
{
    Interaction::InteractionController controller;
    ASSERT_TRUE(controller.init("easy", "none", {}, HandJoint::IndexTIP));

    EXPECT_TRUE(controller.reset());

    Action action;
    EXPECT_TRUE(controller.process(Tests::makeLandmarksForControl(0.4f, 0.4f), action));

    EXPECT_TRUE(controller.clear());
    EXPECT_FALSE(controller.isInited());

    EXPECT_FALSE(controller.process(Tests::makeLandmarksForControl(), action));
}

TEST_F(VisionCursorTestFixture, InteractionControllerEasyRealtimeSmoke)
{
    Interaction::InteractionController controller;
    ASSERT_TRUE(controller.init("easy", "one_euro", {12.0f, 30.0f, 1.0f, 60.0f}, HandJoint::IndexTIP));
    ASSERT_TRUE(controller.isInited());

    int processed_count = 0;
    for (int i = 0; i < 60; ++i)
    {
        FramePtr frame;
        if (!p_camera_device->capture(frame))
        {
            continue;
        }

        HandLandmarkArray raw_landmarks;
        if (!mp_hand_detector->process(frame, raw_landmarks))
        {
            continue;
        }

        Action action;
        EXPECT_TRUE(controller.process(raw_landmarks, action));
        ++processed_count;

        if (processed_count >= 10)
        {
            break;
        }
    }

    EXPECT_GT(processed_count, 0);
}

