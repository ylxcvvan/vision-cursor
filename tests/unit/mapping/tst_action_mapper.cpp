#include <gtest/gtest.h>

#include "mapping/action_mapper.h"

using namespace VisionCursor;

namespace
{

Action makeEmptyAction()
{
    Action action;
    action.type = ActionType::None;
    action.isMoving = false;
    action.x = 0.0f;
    action.y = 0.0f;
    action.dx = 0.0f;
    action.dy = 0.0f;
    return action;
}

} // namespace

TEST(ActionMapperTest, OpenWithValidParametersInitializesMapper)
{
    Mapping::ActionMapper mapper;

    ASSERT_TRUE(mapper.open(1.0f, 1.0f, 1500.0f, MoveMode::Absolute));
    EXPECT_TRUE(mapper.isInited());
    EXPECT_EQ(mapper.moveMode(), MoveMode::Absolute);
}

TEST(ActionMapperTest, SetterValidation)
{
    Mapping::ActionMapper mapper;

    EXPECT_FALSE(mapper.setScreenSize(0, 1080));
    EXPECT_FALSE(mapper.setScreenSize(1920, 0));
    EXPECT_TRUE(mapper.setScreenSize(1920, 1080));

    EXPECT_FALSE(mapper.setMoveSensitivity(0.0f, 1.0f));
    EXPECT_FALSE(mapper.setMoveSensitivity(1.0f, 0.0f));
    EXPECT_TRUE(mapper.setMoveSensitivity(1.2f, 0.8f));

    EXPECT_FALSE(mapper.setScrollSensitivity(0.0f));
    EXPECT_TRUE(mapper.setScrollSensitivity(1200.0f));

    EXPECT_FALSE(mapper.setMappingRegion({0.9f, 0.1f, 0.2f, 0.8f}));
    EXPECT_TRUE(mapper.setMappingRegion({0.1f, 0.1f, 0.9f, 0.9f}));
}

TEST(ActionMapperTest, MapAndExecuteFailsBeforeInit)
{
    Mapping::ActionMapper mapper;

    Action action = makeEmptyAction();
    EXPECT_FALSE(mapper.mapAndExecute(action));
}

TEST(ActionMapperTest, MapAndExecuteNoopSucceedsAfterInit)
{
    Mapping::ActionMapper mapper;
    ASSERT_TRUE(mapper.open(1.0f, 1.0f, 1500.0f, MoveMode::Relative));

    Action action = makeEmptyAction();
    EXPECT_TRUE(mapper.mapAndExecute(action));
}

TEST(ActionMapperTest, RelativeMoveWithSensitivitySmoke)
{
    Mapping::ActionMapper mapper;
    ASSERT_TRUE(mapper.open(1.0f, 1.0f, 1500.0f, MoveMode::Relative));
    ASSERT_TRUE(mapper.setMoveSensitivity(2.0f, 0.5f));

    Action action = makeEmptyAction();
    action.isMoving = true;
    action.dx = 0.01f;
    action.dy = 0.01f;

    EXPECT_TRUE(mapper.mapAndExecute(action));
}

TEST(ActionMapperTest, AbsoluteMoveWithMappingRegionSmoke)
{
    Mapping::ActionMapper mapper;
    ASSERT_TRUE(mapper.open(1.0f, 1.0f, 1500.0f, MoveMode::Absolute));
    ASSERT_TRUE(mapper.setScreenSize(1920, 1080));
    ASSERT_TRUE(mapper.setMappingRegion({0.1f, 0.1f, 0.9f, 0.9f}));

    Action action = makeEmptyAction();
    action.isMoving = true;
    action.x = 0.5f;
    action.y = 0.5f;
    EXPECT_TRUE(mapper.mapAndExecute(action));

    // Outside mapping region should be ignored, but still treated as success.
    action.x = 0.05f;
    action.y = 0.5f;
    EXPECT_TRUE(mapper.mapAndExecute(action));
}

TEST(ActionMapperTest, ButtonAndScrollActionsSmoke)
{
    Mapping::ActionMapper mapper;
    ASSERT_TRUE(mapper.open(1.0f, 1.0f, 1500.0f, MoveMode::Relative));
    ASSERT_TRUE(mapper.setScrollSensitivity(0.5f));

    Action action = makeEmptyAction();

    action.type = ActionType::LeftClick;
    EXPECT_TRUE(mapper.mapAndExecute(action));

    action.type = ActionType::RightClick;
    EXPECT_TRUE(mapper.mapAndExecute(action));

    action = makeEmptyAction();
    action.type = ActionType::ScrollVertical;
    action.dy = 120.0f;
    EXPECT_TRUE(mapper.mapAndExecute(action));
}

