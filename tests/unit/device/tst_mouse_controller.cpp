#include <gtest/gtest.h>

#include "device/mouse_controller.h"

using namespace VisionCursor;
using namespace VisionCursor::Device;

TEST(MouseControllerTest, AvailabilityAndGetPosition)
{
    MouseController mouse;

    EXPECT_TRUE(mouse.isAvailable());

    MousePosition pos{};
    EXPECT_TRUE(mouse.getPosition(pos));
}

TEST(MouseControllerTest, ZeroScrollIsNoop)
{
    MouseController mouse;
    EXPECT_TRUE(mouse.scroll(0));
    EXPECT_TRUE(mouse.scroll(ScrollAxis::Vertical, 0));
    EXPECT_TRUE(mouse.scroll(ScrollAxis::Horizontal, 0));
}

TEST(MouseControllerTest, ReleaseAllButtonsWithoutPriorPress)
{
    MouseController mouse;
    EXPECT_TRUE(mouse.releaseAllButtons());
}
