#include "mapping/action_mapper.h"

#include <algorithm>
#include <cmath>

#if !defined(_WIN32)
#error "ActionMapper currently supports Windows only."
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
namespace Mapping
{
namespace
{

int clampInt(int value, int low, int high)
{
    return std::clamp(value, low, high);
}

float clampFloat(float value, float low, float high)
{
    return std::clamp(value, low, high);
}

bool isValidRegion(const MappingRegion& region)
{
    if (region.left < 0.0f || region.left > 1.0f || region.top < 0.0f || region.top > 1.0f ||
        region.right < 0.0f || region.right > 1.0f || region.bottom < 0.0f || region.bottom > 1.0f)
    {
        return false;
    }

    return region.left < region.right && region.top < region.bottom;
}

} // namespace

ActionMapper::ActionMapper() = default;

bool ActionMapper::open(float x_sensitivity,
                        float y_sensitivity,
                        float scroll_sensitivity,
                        MoveMode move_mode,
                        const MappingRegion& region)
{
    if (!mouse_.isAvailable())
    {
        LOG(ERROR) << "MouseController is not available.";
        return false;
    }

    if (!setMoveMode(move_mode))
    {
        return false;
    }

    if (!setMoveSensitivity(x_sensitivity, y_sensitivity))
    {
        return false;
    }

    if (!setScrollSensitivity(scroll_sensitivity))
    {
        return false;
    }

    if (!setMappingRegion(region))
    {
        return false;
    }

    const int width = GetSystemMetrics(SM_CXSCREEN);
    const int height = GetSystemMetrics(SM_CYSCREEN);
    if (!setScreenSize(width, height))
    {
        LOG(ERROR) << "Failed to query valid screen size from system. width=" << width
                   << " height=" << height;
        return false;
    }

    inited_ = true;
    LOG(INFO) << "ActionMapper initialized. mode=" << (move_mode_ == MoveMode::Absolute ? "Absolute" : "Relative")
              << ", screen=" << screen_w_ << "x" << screen_h_;
    return true;
}

bool ActionMapper::mapAndExecute(const Action& action)
{
    if (!inited_)
    {
        LOG(ERROR) << "ActionMapper is not initialized.";
        return false;
    }

    if (!executeButtonAction(action))
    {
        return false;
    }

    if (!executeScrollAction(action))
    {
        return false;
    }

    if (!executeMoveAction(action))
    {
        return false;
    }

    return true;
}

bool ActionMapper::setMoveMode(MoveMode move_mode)
{
    move_mode_ = move_mode;
    return true;
}

bool ActionMapper::setScreenSize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        LOG(ERROR) << "Invalid screen size. width=" << width << " height=" << height;
        return false;
    }

    screen_w_ = width;
    screen_h_ = height;
    return true;
}

bool ActionMapper::setMoveSensitivity(float x_sensitivity, float y_sensitivity)
{
    if (x_sensitivity <= 0.0f || y_sensitivity <= 0.0f)
    {
        LOG(ERROR) << "Invalid move sensitivity. x=" << x_sensitivity << " y=" << y_sensitivity;
        return false;
    }

    move_sens_x_ = x_sensitivity;
    move_sens_y_ = y_sensitivity;
    return true;
}

bool ActionMapper::setScrollSensitivity(float scroll_sensitivity)
{
    if (scroll_sensitivity <= 0.0f)
    {
        LOG(ERROR) << "Invalid scroll sensitivity: " << scroll_sensitivity;
        return false;
    }

    scroll_sens_ = scroll_sensitivity;
    return true;
}

bool ActionMapper::setMappingRegion(const MappingRegion& region)
{
    if (!isValidRegion(region))
    {
        LOG(ERROR) << "Invalid mapping region.";
        return false;
    }

    map_region_ = region;
    LOG(INFO) << "Mapping region updated.";
    return true;
}

bool ActionMapper::executeButtonAction(const Action& action)
{
    switch (action.type)
    {
        case ActionType::LeftDown:
            return mouse_.leftDown();
        case ActionType::LeftUp:
            return mouse_.leftUp();
        case ActionType::LeftClick:
            return mouse_.leftClick();

        case ActionType::RightDown:
            return mouse_.rightDown();
        case ActionType::RightUp:
            return mouse_.rightUp();
        case ActionType::RightClick:
            return mouse_.rightClick();

        default:
            return true;
    }
}

bool ActionMapper::executeMoveAction(const Action& action)
{
    if (!action.isMoving)
    {
        return true;
    }

    if (!isInsideMappingRegion(action.x, action.y))
    {
        return true;
    }

    if (move_mode_ == MoveMode::Absolute)
    {
        const int x = toScreenX(action.x);
        const int y = toScreenY(action.y);
        return mouse_.moveTo(x, y);
    }

    const int dx = toRelativeDX(action.dx);
    const int dy = toRelativeDY(action.dy);
    return mouse_.moveBy(dx, dy);
}

bool ActionMapper::executeScrollAction(const Action& action)
{
    switch (action.type)
    {
        case ActionType::ScrollVertical: {
            const int delta = static_cast<int>(std::lround(action.dy * scroll_sens_));
            return mouse_.scroll(ScrollAxis::Vertical, delta);
        }

        case ActionType::ScrollHorizontal: {
            const int delta = static_cast<int>(std::lround(action.dx * scroll_sens_));
            return mouse_.scroll(ScrollAxis::Horizontal, delta);
        }

        default:
            return true;
    }
}

bool ActionMapper::isInsideMappingRegion(float normalized_x, float normalized_y) const
{
    return normalized_x >= map_region_.left && normalized_x <= map_region_.right &&
           normalized_y >= map_region_.top && normalized_y <= map_region_.bottom;
}

int ActionMapper::toScreenX(float normalized_x) const
{
    const float clamped_x = clampFloat(normalized_x, map_region_.left, map_region_.right);
    const float mapped_x = (clamped_x - map_region_.left) / (map_region_.right - map_region_.left);

    const int x = static_cast<int>(std::lround(mapped_x * static_cast<float>(screen_w_ - 1)));
    return clampInt(x, 0, screen_w_ - 1);
}

int ActionMapper::toScreenY(float normalized_y) const
{
    const float clamped_y = clampFloat(normalized_y, map_region_.top, map_region_.bottom);
    const float mapped_y = (clamped_y - map_region_.top) / (map_region_.bottom - map_region_.top);

    const int y = static_cast<int>(std::lround(mapped_y * static_cast<float>(screen_h_ - 1)));
    return clampInt(y, 0, screen_h_ - 1);
}

int ActionMapper::toRelativeDX(float normalized_dx) const
{
    const float scaled_dx = normalized_dx * static_cast<float>(screen_w_) * move_sens_x_;
    return static_cast<int>(std::lround(scaled_dx));
}

int ActionMapper::toRelativeDY(float normalized_dy) const
{
    const float scaled_dy = normalized_dy * static_cast<float>(screen_h_) * move_sens_y_;
    return static_cast<int>(std::lround(scaled_dy));
}

} // namespace Mapping
} // namespace VisionCursor
