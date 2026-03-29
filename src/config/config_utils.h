#pragma once

#include <QString>
#include <QJsonObject>

namespace VisionCursor
{
namespace Config
{

bool ensureParentDir(const QString& file_path);
bool loadJsonFile(const QString& file_path, QJsonObject& root_obj);
bool saveJsonFile(const QString& file_path, const QJsonObject& root_obj);

} // namespace Config
} // namespace VisionCursor