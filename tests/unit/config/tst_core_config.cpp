#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTextStream>

#include "config/core_config.h"
#include "test_helpers.h"

using namespace VisionCursor;

namespace
{

QString coreConfigTestDir()
{
    const QString dir = QDir::currentPath() + "/test_output/core_config";
    QDir().mkpath(dir);
    return dir;
}

QString coreConfigFile(const QString& name)
{
    return coreConfigTestDir() + "/" + name;
}

} // namespace

TEST(CoreConfigTest, CreateDefaultHasExpectedValues)
{
    Tests::ensureCoreApp();

    const Config::OrchestratorConfig config = Config::CoreConfig::createDefault();

    EXPECT_EQ(config.camera.camera_resolution.width, 1280);
    EXPECT_EQ(config.camera.camera_resolution.height, 720);
    EXPECT_EQ(config.camera.camera_fps, 60);
    EXPECT_TRUE(config.camera.is_mirror);

    EXPECT_FLOAT_EQ(config.mediapipe.min_score_thresh, 0.6f);
    EXPECT_DOUBLE_EQ(config.mediapipe.threshold, 0.2);
    EXPECT_EQ(config.mediapipe.model_complexity, 1);
    EXPECT_TRUE(config.mediapipe.use_prev_landmarks);

    EXPECT_EQ(config.interaction.scheme_name, "easy");
    EXPECT_EQ(config.interaction.filter_name, "none");
    EXPECT_EQ(config.interaction.control_joint, HandJoint::IndexTIP);

    EXPECT_FLOAT_EQ(config.mapper.x_sensitivity, 1.0f);
    EXPECT_FLOAT_EQ(config.mapper.y_sensitivity, 1.0f);
    EXPECT_FLOAT_EQ(config.mapper.scroll_sensitivity, 1500.0f);
    EXPECT_EQ(config.mapper.move_mode, MoveMode::Absolute);
    EXPECT_FLOAT_EQ(config.mapper.region.left, DEFAULT_MAPPING_REGION.left);
    EXPECT_FLOAT_EQ(config.mapper.region.top, DEFAULT_MAPPING_REGION.top);
    EXPECT_FLOAT_EQ(config.mapper.region.right, DEFAULT_MAPPING_REGION.right);
    EXPECT_FLOAT_EQ(config.mapper.region.bottom, DEFAULT_MAPPING_REGION.bottom);
}

TEST(CoreConfigTest, SaveAndLoadRoundTrip)
{
    Tests::ensureCoreApp();

    const QString path = coreConfigFile("round_trip.json");
    QFile::remove(path);

    Config::OrchestratorConfig saved = Config::CoreConfig::createDefault();
    saved.camera.camera_device_name = "UnitTest Camera";
    saved.camera.camera_resolution = {1920, 1080};
    saved.interaction.scheme_name = "advanced";
    saved.interaction.filter_name = "one_euro";
    saved.interaction.vec_filter_param = {12.0f, 30.0f, 1.0f, 60.0f};
    saved.mapper.move_mode = MoveMode::Relative;

    ASSERT_TRUE(Config::CoreConfig::save(saved, path));

    Config::OrchestratorConfig loaded;
    ASSERT_TRUE(Config::CoreConfig::load(loaded, path));

    EXPECT_EQ(loaded.camera.camera_device_name, saved.camera.camera_device_name);
    EXPECT_EQ(loaded.camera.camera_resolution.width, saved.camera.camera_resolution.width);
    EXPECT_EQ(loaded.camera.camera_resolution.height, saved.camera.camera_resolution.height);
    EXPECT_EQ(loaded.interaction.scheme_name, saved.interaction.scheme_name);
    EXPECT_EQ(loaded.interaction.filter_name, saved.interaction.filter_name);
    EXPECT_EQ(loaded.mapper.move_mode, saved.mapper.move_mode);
    EXPECT_EQ(loaded.interaction.vec_filter_param.size(), saved.interaction.vec_filter_param.size());
}

TEST(CoreConfigTest, LoadMissingFileCreatesDefault)
{
    Tests::ensureCoreApp();

    const QString path = coreConfigFile("auto_create.json");
    QFile::remove(path);

    Config::OrchestratorConfig loaded;
    ASSERT_TRUE(Config::CoreConfig::load(loaded, path));
    EXPECT_TRUE(QFile::exists(path));

    const Config::OrchestratorConfig def = Config::CoreConfig::createDefault();
    EXPECT_EQ(loaded.camera.camera_fps, def.camera.camera_fps);
    EXPECT_EQ(loaded.interaction.scheme_name, def.interaction.scheme_name);
    EXPECT_EQ(loaded.mapper.move_mode, def.mapper.move_mode);
}

TEST(CoreConfigTest, InvalidJsonFails)
{
    Tests::ensureCoreApp();

    const QString path = coreConfigFile("invalid_json.json");

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QTextStream out(&file);
    out << "{ invalid json";
    file.close();

    Config::OrchestratorConfig loaded;
    EXPECT_FALSE(Config::CoreConfig::load(loaded, path));
}

TEST(CoreConfigTest, PartialFieldsFallbackToDefault)
{
    Tests::ensureCoreApp();

    const QString path = coreConfigFile("partial_fields.json");

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));

    QTextStream out(&file);
    out << R"json(
{
  "camera": {
    "camera_device_name": "Partial Camera"
  },
  "interaction": {
    "scheme_name": "partial_scheme"
  }
}
)json";
    file.close();

    Config::OrchestratorConfig loaded;
    ASSERT_TRUE(Config::CoreConfig::load(loaded, path));

    const Config::OrchestratorConfig def = Config::CoreConfig::createDefault();

    EXPECT_EQ(loaded.camera.camera_device_name, "Partial Camera");
    EXPECT_EQ(loaded.camera.camera_resolution.width, def.camera.camera_resolution.width);
    EXPECT_EQ(loaded.camera.camera_resolution.height, def.camera.camera_resolution.height);
    EXPECT_EQ(loaded.camera.camera_fps, def.camera.camera_fps);

    EXPECT_EQ(loaded.interaction.scheme_name, "partial_scheme");
    EXPECT_EQ(loaded.interaction.filter_name, def.interaction.filter_name);
    EXPECT_EQ(loaded.interaction.control_joint, def.interaction.control_joint);

    EXPECT_FLOAT_EQ(loaded.mapper.x_sensitivity, def.mapper.x_sensitivity);
    EXPECT_FLOAT_EQ(loaded.mapper.y_sensitivity, def.mapper.y_sensitivity);
    EXPECT_EQ(loaded.mapper.move_mode, def.mapper.move_mode);
}

TEST(CoreConfigTest, InvalidEnumFails)
{
    Tests::ensureCoreApp();

    const QString path = coreConfigFile("invalid_enum.json");

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));

    QTextStream out(&file);
    out << R"json(
{
  "interaction": {
    "control_joint": "NotAHandJoint"
  }
}
)json";
    file.close();

    Config::OrchestratorConfig loaded;
    EXPECT_FALSE(Config::CoreConfig::load(loaded, path));
}
