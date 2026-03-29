#pragma once

#include "common/common.h"

namespace VisionCursor
{
namespace Device
{

class MouseController
{
public:
    MouseController() = default;
    ~MouseController() = default;

    MouseController(const MouseController&) = delete;
    MouseController& operator=(const MouseController&) = delete;
    MouseController(MouseController&&) = delete;
    MouseController& operator=(MouseController&&) = delete;

public:
    bool isAvailable() const;

    bool getPosition(MousePosition& pos) const;

    bool moveTo(int x, int y);
    bool moveBy(int dx, int dy);

    bool buttonDown(MouseButton button);
    bool buttonUp(MouseButton button);

    bool click(MouseButton button);
    bool doubleClick(MouseButton button);

    bool leftDown();
    bool leftUp();
    bool leftClick();
    bool leftDoubleClick();

    bool rightDown();
    bool rightUp();
    bool rightClick();
    bool rightDoubleClick();

    bool middleDown();
    bool middleUp();
    bool middleClick();

    bool scroll(int delta);
    bool scroll(ScrollAxis axis, int delta);
    bool scrollSteps(int steps);

    bool releaseAllButtons();

private:
    bool setButtonState(MouseButton button, bool pressed);

private:
    bool left_pressed_ = false;
    bool right_pressed_ = false;
    bool middle_pressed_ = false;
};

} // namespace Device
} // namespace VisionCursor
