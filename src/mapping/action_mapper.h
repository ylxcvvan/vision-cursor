#pragma once

#include "common/common.h"
#include "device/mouse_controller.h"
#include "interaction/action/action.h"

namespace VisionCursor
{
namespace Mapping
{

class ActionMapper
{
public:
    ActionMapper();

    bool open(float x_sensitivity,
              float y_sensitivity,
              float scroll_sensitivity,
              MoveMode move_mode,
              const MappingRegion& region = DEFAULT_MAPPING_REGION);

    bool mapAndExecute(const Action& action);

    bool setMoveMode(MoveMode move_mode);
    MoveMode moveMode() const { return move_mode_; }

    bool setScreenSize(int width, int height);
    int screenWidth() const { return screen_w_; }
    int screenHeight() const { return screen_h_; }

    bool setMoveSensitivity(float x_sensitivity, float y_sensitivity);
    float moveSensitivityX() const { return move_sens_x_; }
    float moveSensitivityY() const { return move_sens_y_; }

    bool setScrollSensitivity(float scroll_sensitivity);
    float scrollSensitivity() const { return scroll_sens_; }

    bool setMappingRegion(const MappingRegion& region);
    const MappingRegion& mappingRegion() const { return map_region_; }

    bool isInited() const { return inited_; }

private:
    bool executeButtonAction(const Action& action);
    bool executeMoveAction(const Action& action);
    bool executeScrollAction(const Action& action);

    bool isInsideMappingRegion(float normalized_x, float normalized_y) const;

    int toScreenX(float normalized_x) const;
    int toScreenY(float normalized_y) const;

    int toRelativeDX(float normalized_dx) const;
    int toRelativeDY(float normalized_dy) const;

private:
    Device::MouseController mouse_;

    MoveMode move_mode_ = MoveMode::Relative;

    int screen_w_ = 2560;
    int screen_h_ = 1440;

    float move_sens_x_ = 1.0f;
    float move_sens_y_ = 1.0f;

    float scroll_sens_ = 1500.0f;

    MappingRegion map_region_ = DEFAULT_MAPPING_REGION;

    bool inited_ = false;
};

} // namespace Mapping
} // namespace VisionCursor
