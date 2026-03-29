#include "config/core_config.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

#include "config/config_utils.h"
#include "config/fsm_config.h"
#include "log/logging.h"
#include "utils/to_string.h"

namespace VisionCursor
{
namespace Config
{

QString CoreConfig::defaultPath()
{
    return QString::fromStdString(Paths::coreConfigFile());
}

OrchestratorConfig CoreConfig::createDefault()
{
    OrchestratorConfig config;

    config.camera.camera_device_name = "";
    config.camera.camera_resolution = {1280, 720};
    config.camera.camera_fps = 60;
    config.camera.is_mirror = true;

    config.mediapipe.min_score_thresh = 0.6f;
    config.mediapipe.threshold = 0.2;
    config.mediapipe.model_complexity = 1;
    config.mediapipe.use_prev_landmarks = true;

    config.interaction.scheme_name = "easy";
    config.interaction.filter_name = "none";
    config.interaction.vec_filter_param = {};
    config.interaction.control_joint = HandJoint::IndexTIP;

    config.mapper.x_sensitivity = 1.0f;
    config.mapper.y_sensitivity = 1.0f;
    config.mapper.scroll_sensitivity = 1500.0f;
    config.mapper.move_mode = MoveMode::Absolute;
    config.mapper.region = DEFAULT_MAPPING_REGION;

    return config;
}

bool CoreConfig::load(OrchestratorConfig& config, const QString& file_path)
{
    if (!FsmConfig::ensureBuiltinPresets(false))
    {
        LOG(ERROR) << "Failed to ensure built-in FSM presets.";
        return false;
    }

    const QFileInfo file_info(file_path);

    if (!file_info.exists())
    {
        const OrchestratorConfig default_config = createDefault();
        if (!save(default_config, file_path))
        {
            LOG(ERROR) << "Failed to create default core config file: " << file_path.toStdString();
            return false;
        }
    }

    QJsonObject root_obj;
    if (!loadJsonFile(file_path, root_obj))
    {
        return false;
    }

    const bool ok = fromJsonObject(root_obj, config);
    if (ok)
    {
        LOG(INFO) << "Core config loaded: " << file_path.toStdString();
    }
    return ok;
}

bool CoreConfig::save(const OrchestratorConfig& config, const QString& file_path)
{
    const bool ok = saveJsonFile(file_path, toJsonObject(config));
    if (ok)
    {
        LOG(INFO) << "Core config saved: " << file_path.toStdString();
    }
    return ok;
}

bool CoreConfig::fromJsonObject(const QJsonObject& root_obj, OrchestratorConfig& config)
{
    config = createDefault();

    if (root_obj.contains("camera"))
    {
        if (!root_obj.value("camera").isObject())
        {
            LOG(ERROR) << "camera must be an object.";
            return false;
        }
        if (!parseCameraConfig(root_obj.value("camera").toObject(), config.camera))
        {
            return false;
        }
    }

    if (root_obj.contains("mediapipe"))
    {
        if (!root_obj.value("mediapipe").isObject())
        {
            LOG(ERROR) << "mediapipe must be an object.";
            return false;
        }
        if (!parseMediaPipeConfig(root_obj.value("mediapipe").toObject(), config.mediapipe))
        {
            return false;
        }
    }

    if (root_obj.contains("interaction"))
    {
        if (!root_obj.value("interaction").isObject())
        {
            LOG(ERROR) << "interaction must be an object.";
            return false;
        }
        if (!parseInteractionConfig(root_obj.value("interaction").toObject(), config.interaction))
        {
            return false;
        }
    }

    if (root_obj.contains("mapper"))
    {
        if (!root_obj.value("mapper").isObject())
        {
            LOG(ERROR) << "mapper must be an object.";
            return false;
        }
        if (!parseMapperConfig(root_obj.value("mapper").toObject(), config.mapper))
        {
            return false;
        }
    }

    return true;
}

QJsonObject CoreConfig::toJsonObject(const OrchestratorConfig& config)
{
    QJsonObject root_obj;

    QJsonObject camera_obj;
    camera_obj["camera_device_name"] = QString::fromStdString(config.camera.camera_device_name);
    QJsonObject resolution_obj;
    resolution_obj["width"] = config.camera.camera_resolution.width;
    resolution_obj["height"] = config.camera.camera_resolution.height;
    camera_obj["camera_resolution"] = resolution_obj;
    camera_obj["camera_fps"] = config.camera.camera_fps;
    camera_obj["is_mirror"] = config.camera.is_mirror;
    root_obj["camera"] = camera_obj;

    QJsonObject mediapipe_obj;
    mediapipe_obj["min_score_thresh"] = config.mediapipe.min_score_thresh;
    mediapipe_obj["threshold"] = config.mediapipe.threshold;
    mediapipe_obj["model_complexity"] = config.mediapipe.model_complexity;
    mediapipe_obj["use_prev_landmarks"] = config.mediapipe.use_prev_landmarks;
    root_obj["mediapipe"] = mediapipe_obj;

    QJsonObject interaction_obj;
    interaction_obj["scheme_name"] = QString::fromStdString(config.interaction.scheme_name);
    interaction_obj["filter_name"] = QString::fromStdString(config.interaction.filter_name);
    QJsonArray filter_arr;
    for (float value : config.interaction.vec_filter_param)
    {
        filter_arr.append(value);
    }
    interaction_obj["vec_filter_param"] = filter_arr;
    interaction_obj["control_joint"] = Utils::ToString(config.interaction.control_joint);
    root_obj["interaction"] = interaction_obj;

    QJsonObject mapper_obj;
    mapper_obj["x_sensitivity"] = config.mapper.x_sensitivity;
    mapper_obj["y_sensitivity"] = config.mapper.y_sensitivity;
    mapper_obj["scroll_sensitivity"] = config.mapper.scroll_sensitivity;
    mapper_obj["move_mode"] = Utils::ToString(config.mapper.move_mode);
    QJsonObject region_obj;
    region_obj["left"] = config.mapper.region.left;
    region_obj["top"] = config.mapper.region.top;
    region_obj["right"] = config.mapper.region.right;
    region_obj["bottom"] = config.mapper.region.bottom;
    mapper_obj["region"] = region_obj;
    root_obj["mapper"] = mapper_obj;

    return root_obj;
}

bool CoreConfig::parseCameraConfig(const QJsonObject& obj, OrchestratorConfig::CameraConfig& config)
{
    if (obj.contains("camera_device_name"))
    {
        config.camera_device_name = obj.value("camera_device_name").toString().toStdString();
    }
    if (obj.contains("camera_fps"))
    {
        config.camera_fps = obj.value("camera_fps").toInt(config.camera_fps);
    }
    if (obj.contains("is_mirror"))
    {
        config.is_mirror = obj.value("is_mirror").toBool(config.is_mirror);
    }
    if (obj.contains("camera_resolution"))
    {
        if (!obj.value("camera_resolution").isObject())
        {
            LOG(ERROR) << "camera_resolution must be an object.";
            return false;
        }
        return parseCameraResolution(obj.value("camera_resolution").toObject(), config.camera_resolution);
    }
    return true;
}

bool CoreConfig::parseMediaPipeConfig(const QJsonObject& obj, OrchestratorConfig::MediaPipeConfig& config)
{
    if (obj.contains("min_score_thresh"))
    {
        config.min_score_thresh = static_cast<float>(obj.value("min_score_thresh").toDouble(config.min_score_thresh));
    }
    if (obj.contains("threshold"))
    {
        config.threshold = obj.value("threshold").toDouble(config.threshold);
    }
    if (obj.contains("model_complexity"))
    {
        config.model_complexity = obj.value("model_complexity").toInt(config.model_complexity);
    }
    if (obj.contains("use_prev_landmarks"))
    {
        config.use_prev_landmarks = obj.value("use_prev_landmarks").toBool(config.use_prev_landmarks);
    }
    return true;
}

bool CoreConfig::parseInteractionConfig(const QJsonObject& obj, OrchestratorConfig::InteractionConfig& config)
{
    if (obj.contains("scheme_name"))
    {
        config.scheme_name = obj.value("scheme_name").toString().toStdString();
    }
    if (obj.contains("filter_name"))
    {
        config.filter_name = obj.value("filter_name").toString().toStdString();
    }
    if (obj.contains("vec_filter_param"))
    {
        if (!obj.value("vec_filter_param").isArray())
        {
            LOG(ERROR) << "vec_filter_param must be an array.";
            return false;
        }

        config.vec_filter_param.clear();
        const QJsonArray arr = obj.value("vec_filter_param").toArray();
        for (const QJsonValue& value : arr)
        {
            config.vec_filter_param.push_back(static_cast<float>(value.toDouble()));
        }
    }

    if (obj.contains("control_joint"))
    {
        if (!obj.value("control_joint").isString())
        {
            LOG(ERROR) << "control_joint must be a string.";
            return false;
        }

        HandJoint joint;
        if (!Utils::parseHandJoint(obj.value("control_joint").toString(), joint))
        {
            return false;
        }
        config.control_joint = joint;
    }

    return true;
}

bool CoreConfig::parseMapperConfig(const QJsonObject& obj, OrchestratorConfig::MapperConfig& config)
{
    if (obj.contains("x_sensitivity"))
    {
        config.x_sensitivity = static_cast<float>(obj.value("x_sensitivity").toDouble(config.x_sensitivity));
    }
    if (obj.contains("y_sensitivity"))
    {
        config.y_sensitivity = static_cast<float>(obj.value("y_sensitivity").toDouble(config.y_sensitivity));
    }
    if (obj.contains("scroll_sensitivity"))
    {
        config.scroll_sensitivity = static_cast<float>(obj.value("scroll_sensitivity").toDouble(config.scroll_sensitivity));
    }
    if (obj.contains("move_mode"))
    {
        if (!obj.value("move_mode").isString())
        {
            LOG(ERROR) << "move_mode must be a string.";
            return false;
        }

        MoveMode mode;
        if (!Utils::parseMoveMode(obj.value("move_mode").toString(), mode))
        {
            return false;
        }
        config.move_mode = mode;
    }
    if (obj.contains("region"))
    {
        if (!obj.value("region").isObject())
        {
            LOG(ERROR) << "region must be an object.";
            return false;
        }
        parseMappingRegion(obj.value("region").toObject(), config.region);
    }

    return true;
}

bool CoreConfig::parseCameraResolution(const QJsonObject& obj, CameraResolution& resolution)
{
    if (!obj.contains("width") || !obj.contains("height"))
    {
        LOG(ERROR) << "camera_resolution must contain width and height.";
        return false;
    }

    resolution.width = obj.value("width").toInt();
    resolution.height = obj.value("height").toInt();
    return true;
}

void CoreConfig::parseMappingRegion(const QJsonObject& obj, MappingRegion& region)
{
    if (obj.contains("left")) { region.left = static_cast<float>(obj.value("left").toDouble(region.left)); }
    if (obj.contains("top")) { region.top = static_cast<float>(obj.value("top").toDouble(region.top)); }
    if (obj.contains("right")) { region.right = static_cast<float>(obj.value("right").toDouble(region.right)); }
    if (obj.contains("bottom")) { region.bottom = static_cast<float>(obj.value("bottom").toDouble(region.bottom)); }
}

} // namespace Config
} // namespace VisionCursor
