#include <gtest/gtest.h>

#include <filesystem>

#include "common/paths.hpp"
#include "config/core_config.h"
#include "core/vision_cursor_orchestrator.h"
#include "test_helpers.h"

using namespace VisionCursor;

TEST(CoreOrchestratorTest, DefaultStateAndFeatureToggles)
{
    Tests::ensureCoreApp();

    Core::VisionCursorOrchestrator orchestrator;

    EXPECT_FALSE(orchestrator.isInited());
    EXPECT_FALSE(orchestrator.isRunning());

    orchestrator.setMediapipeEnabled(false);
    EXPECT_FALSE(orchestrator.isMediapipeEnabled());
    orchestrator.setMediapipeEnabled(true);
    EXPECT_TRUE(orchestrator.isMediapipeEnabled());

    orchestrator.setMouseControlEnabled(false);
    EXPECT_FALSE(orchestrator.isMouseControlEnabled());
    orchestrator.setMouseControlEnabled(true);
    EXPECT_TRUE(orchestrator.isMouseControlEnabled());
}

TEST(CoreOrchestratorTest, StartBeforeInitShouldFail)
{
    Tests::ensureCoreApp();

    Core::VisionCursorOrchestrator orchestrator;

    EXPECT_FALSE(orchestrator.start());
    EXPECT_TRUE(orchestrator.stop());
    EXPECT_TRUE(orchestrator.clear());
}

TEST(CoreOrchestratorTest, CameraQueryAndSelectionFailGracefully)
{
    Tests::ensureCoreApp();

    Core::VisionCursorOrchestrator orchestrator;

    EXPECT_TRUE(orchestrator.updateCameraDevices());

    CameraInfo info;
    EXPECT_FALSE(orchestrator.getCameraInfo(info));

    EXPECT_FALSE(orchestrator.setCameraDevice("__definitely_not_a_real_camera__"));
}

TEST(CoreOrchestratorTest, GetOutputWhenIdleReturnsDefault)
{
    Tests::ensureCoreApp();

    Core::VisionCursorOrchestrator orchestrator;

    const Core::OrchestratorOutput out = orchestrator.getOutput(1);
    EXPECT_EQ(out.frame, nullptr);
    EXPECT_FALSE(out.has_hand);
    EXPECT_FALSE(out.current_ok);
    EXPECT_EQ(out.action.type, ActionType::None);
}

TEST(CoreOrchestratorTest, ConfigureInitStartStopSmokeWhenEnvironmentReady)
{
    Tests::ensureCoreApp();

    std::string reason;
    Vision::CameraManager camera_manager;
    Vision::CameraDevice* camera_device = nullptr;
    if (!Tests::tryGetFirstCamera(camera_manager, camera_device, &reason))
    {
        GTEST_SKIP() << "camera not ready: " << reason;
    }

    const std::string training_data_dir = Tests::findTrainingDataDir();
    if (training_data_dir.empty())
    {
        GTEST_SKIP() << "training_data not found";
    }

    // Keep orchestrator path resolution stable in different test working directories.
    Paths::init(std::filesystem::path(training_data_dir).parent_path().string());

    Core::VisionCursorOrchestrator orchestrator;
    orchestrator.setMouseControlEnabled(false);

    Config::OrchestratorConfig cfg = Config::CoreConfig::createDefault();
    cfg.camera.camera_device_name = camera_manager.getAllDeviceName().front();
    cfg.interaction.filter_name = "none";

    ASSERT_TRUE(orchestrator.configure(cfg));
    ASSERT_TRUE(orchestrator.init());
    ASSERT_TRUE(orchestrator.isInited());

    ASSERT_TRUE(orchestrator.start());
    ASSERT_TRUE(orchestrator.isRunning());

    bool received_frame_output = false;
    for (int i = 0; i < 3; ++i)
    {
        const Core::OrchestratorOutput out = orchestrator.getOutput(800);
        if (out.frame != nullptr)
        {
            received_frame_output = true;
            break;
        }
    }
    EXPECT_TRUE(received_frame_output);

    EXPECT_TRUE(orchestrator.stop());
    EXPECT_FALSE(orchestrator.isRunning());

    EXPECT_TRUE(orchestrator.clear());
    EXPECT_FALSE(orchestrator.isInited());
}

