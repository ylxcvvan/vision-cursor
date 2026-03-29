#include "device/mouse_controller.h"

#if !defined(_WIN32)
#error "mouse_windows.cpp should only be built on Windows."
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "log/logging.h"

namespace VisionCursor
{
namespace Device
{
namespace
{

bool sendMouseInput(DWORD flags, DWORD mouse_data = 0, LONG dx = 0, LONG dy = 0)
{
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.mouseData = mouse_data;
    input.mi.dwFlags = flags;

    const UINT sent = SendInput(1, &input, sizeof(INPUT));
    if (sent != 1)
    {
        LOG(ERROR) << "SendInput failed. flags=" << flags << " error=" << GetLastError();
        return false;
    }

    return true;
}

bool getButtonFlags(MouseButton button, DWORD& down_flag, DWORD& up_flag)
{
    switch (button)
    {
        case MouseButton::Left:
            down_flag = MOUSEEVENTF_LEFTDOWN;
            up_flag = MOUSEEVENTF_LEFTUP;
            return true;
        case MouseButton::Right:
            down_flag = MOUSEEVENTF_RIGHTDOWN;
            up_flag = MOUSEEVENTF_RIGHTUP;
            return true;
        case MouseButton::Middle:
            down_flag = MOUSEEVENTF_MIDDLEDOWN;
            up_flag = MOUSEEVENTF_MIDDLEUP;
            return true;
        default:
            return false;
    }
}

} // namespace

bool MouseController::isAvailable() const
{
    return true;
}

bool MouseController::getPosition(MousePosition& pos) const
{
    POINT point{};
    if (!GetCursorPos(&point))
    {
        LOG(ERROR) << "GetCursorPos failed. error=" << GetLastError();
        return false;
    }

    pos.x = static_cast<int>(point.x);
    pos.y = static_cast<int>(point.y);
    return true;
}

bool MouseController::moveTo(int x, int y)
{
    if (!SetCursorPos(x, y))
    {
        LOG(ERROR) << "SetCursorPos failed. x=" << x << " y=" << y << " error=" << GetLastError();
        return false;
    }

    return true;
}

bool MouseController::moveBy(int dx, int dy)
{
    return sendMouseInput(MOUSEEVENTF_MOVE, 0, dx, dy);
}

bool MouseController::buttonDown(MouseButton button)
{
    DWORD down_flag = 0;
    DWORD up_flag = 0;
    if (!getButtonFlags(button, down_flag, up_flag))
    {
        LOG(ERROR) << "Unsupported mouse button in buttonDown.";
        return false;
    }

    if (!sendMouseInput(down_flag))
    {
        return false;
    }

    return setButtonState(button, true);
}

bool MouseController::buttonUp(MouseButton button)
{
    DWORD down_flag = 0;
    DWORD up_flag = 0;
    if (!getButtonFlags(button, down_flag, up_flag))
    {
        LOG(ERROR) << "Unsupported mouse button in buttonUp.";
        return false;
    }

    if (!sendMouseInput(up_flag))
    {
        return false;
    }

    return setButtonState(button, false);
}

bool MouseController::click(MouseButton button)
{
    return buttonDown(button) && buttonUp(button);
}

bool MouseController::doubleClick(MouseButton button)
{
    if (!click(button))
    {
        return false;
    }

    Sleep(GetDoubleClickTime() / 4);
    return click(button);
}

bool MouseController::leftDown()
{
    return buttonDown(MouseButton::Left);
}

bool MouseController::leftUp()
{
    return buttonUp(MouseButton::Left);
}

bool MouseController::leftClick()
{
    return click(MouseButton::Left);
}

bool MouseController::leftDoubleClick()
{
    return doubleClick(MouseButton::Left);
}

bool MouseController::rightDown()
{
    return buttonDown(MouseButton::Right);
}

bool MouseController::rightUp()
{
    return buttonUp(MouseButton::Right);
}

bool MouseController::rightClick()
{
    return click(MouseButton::Right);
}

bool MouseController::rightDoubleClick()
{
    return doubleClick(MouseButton::Right);
}

bool MouseController::middleDown()
{
    return buttonDown(MouseButton::Middle);
}

bool MouseController::middleUp()
{
    return buttonUp(MouseButton::Middle);
}

bool MouseController::middleClick()
{
    return click(MouseButton::Middle);
}

bool MouseController::scroll(int delta)
{
    return scroll(ScrollAxis::Vertical, delta);
}

bool MouseController::scroll(ScrollAxis axis, int delta)
{
    if (delta == 0)
    {
        return true;
    }

    DWORD flag = 0;
    switch (axis)
    {
        case ScrollAxis::Vertical:
            flag = MOUSEEVENTF_WHEEL;
            break;
        case ScrollAxis::Horizontal:
            flag = MOUSEEVENTF_HWHEEL;
            break;
        default:
            LOG(ERROR) << "Unsupported scroll axis.";
            return false;
    }

    return sendMouseInput(flag, static_cast<DWORD>(delta));
}

bool MouseController::scrollSteps(int steps)
{
    return scroll(ScrollAxis::Vertical, steps * WHEEL_DELTA);
}

bool MouseController::releaseAllButtons()
{
    bool ok = true;

    if (left_pressed_)
    {
        ok = leftUp() && ok;
    }
    if (right_pressed_)
    {
        ok = rightUp() && ok;
    }
    if (middle_pressed_)
    {
        ok = middleUp() && ok;
    }

    return ok;
}

bool MouseController::setButtonState(MouseButton button, bool pressed)
{
    switch (button)
    {
        case MouseButton::Left:
            left_pressed_ = pressed;
            return true;
        case MouseButton::Right:
            right_pressed_ = pressed;
            return true;
        case MouseButton::Middle:
            middle_pressed_ = pressed;
            return true;
        default:
            LOG(ERROR) << "Unsupported mouse button in setButtonState.";
            return false;
    }
}

} // namespace Device
} // namespace VisionCursor
