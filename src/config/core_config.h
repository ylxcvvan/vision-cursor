#pragma once

#include <QString>
#include <QJsonObject>

#include "config/structs.h"

namespace VisionCursor
{
namespace Config
{

class CoreConfig
{
public:
    static bool load(OrchestratorConfig& config, const QString& file_path = defaultPath());
    static bool save(const OrchestratorConfig& config, const QString& file_path = defaultPath());

    static OrchestratorConfig createDefault();
    static QString defaultPath();

private:
    static bool fromJsonObject(const QJsonObject& root_obj, OrchestratorConfig& config);
    static QJsonObject toJsonObject(const OrchestratorConfig& config);

    static bool parseCameraConfig(const QJsonObject& obj, OrchestratorConfig::CameraConfig& config);
    static bool parseMediaPipeConfig(const QJsonObject& obj, OrchestratorConfig::MediaPipeConfig& config);
    static bool parseInteractionConfig(const QJsonObject& obj, OrchestratorConfig::InteractionConfig& config);
    static bool parseMapperConfig(const QJsonObject& obj, OrchestratorConfig::MapperConfig& config);

    static bool parseCameraResolution(const QJsonObject& obj, CameraResolution& resolution);
    static void parseMappingRegion(const QJsonObject& obj, MappingRegion& region);
};

} // namespace Config
} // namespace VisionCursor