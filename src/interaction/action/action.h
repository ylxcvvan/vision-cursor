#pragma once


#include "common/common.h"

namespace VisionCursor
{

/// @todo 抽象鼠标的行为。被fsm使用，fsm根据rule判断状态并传出action.


enum class ActionType
{
    None,
    
    LeftDown,
    LeftUp,
    LeftClick,

    RightDown,
    RightUp,
    RightClick,

    ScrollVertical,
    ScrollHorizontal,
};

struct Action
{
    ActionType type = ActionType::None;
    bool isMoving = false;
    float x = 0;
    float y = 0;

    float dx = 0;
    float dy = 0;
};



};      // namespace VisionCursor