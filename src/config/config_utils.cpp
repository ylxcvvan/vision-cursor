#include "config/config_utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>

#include "log/logging.h"

namespace VisionCursor
{
namespace Config
{

bool ensureParentDir(const QString& file_path)
{
    const QFileInfo file_info(file_path);
    const QString dir_path = file_info.absolutePath();

    if (dir_path.isEmpty())
    {
        return true;
    }

    QDir dir;
    if (!dir.mkpath(dir_path))
    {
        LOG(ERROR) << "Failed to create config directory: " << dir_path.toStdString();
        return false;
    }

    return true;
}

bool loadJsonFile(const QString& file_path, QJsonObject& root_obj)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG(ERROR) << "Failed to open config file: " << file_path.toStdString();
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parse_error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);
    if (parse_error.error != QJsonParseError::NoError)
    {
        LOG(ERROR) << "Failed to parse json: " << parse_error.errorString().toStdString();
        return false;
    }

    if (!doc.isObject())
    {
        LOG(ERROR) << "Config root is not a JSON object.";
        return false;
    }

    root_obj = doc.object();
    return true;
}

bool saveJsonFile(const QString& file_path, const QJsonObject& root_obj)
{
    if (!ensureParentDir(file_path))
    {
        return false;
    }

    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        LOG(ERROR) << "Failed to open config file for writing: " << file_path.toStdString();
        return false;
    }

    const QJsonDocument doc(root_obj);
    const QByteArray json_data = doc.toJson(QJsonDocument::Indented);
    const qint64 written = file.write(json_data);
    file.close();

    if (written != json_data.size())
    {
        LOG(ERROR) << "Failed to write complete config file: " << file_path.toStdString();
        return false;
    }

    return true;
}

} // namespace Config
} // namespace VisionCursor