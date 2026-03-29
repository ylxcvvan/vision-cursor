#include "vision/camera_manager.h"

#include <cctype>

#include <QCameraDevice>
#include <QMediaDevices>

#include "log/logging.h"

namespace VisionCursor
{
namespace Vision
{

namespace
{
std::string toDeviceIdHex(const QCameraDevice& camera)
{
    return camera.id().toHex().toStdString();
}

std::string toDeviceName(int index, const QCameraDevice& camera)
{
    return std::to_string(index) + ": " + camera.description().toStdString();
}

std::string trimCopy(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
    {
        ++start;
    }

    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
    {
        --end;
    }

    return s.substr(start, end - start);
}

std::string normalizeDeviceDisplayName(const std::string& name)
{
    // Normalize "0: Camera Name" -> "Camera Name" to survive dynamic index changes.
    const std::string trimmed = trimCopy(name);
    size_t pos = 0;
    while (pos < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[pos])))
    {
        ++pos;
    }
    if (pos > 0 && pos < trimmed.size() && trimmed[pos] == ':')
    {
        return trimCopy(trimmed.substr(pos + 1));
    }
    return trimmed;
}
} // namespace

void CameraManager::updateAllDevice()
{
    const auto qt_devices = QMediaDevices::videoInputs();

    std::unordered_map<std::string, int> map_id_to_index;
    std::unordered_map<std::string, std::string> map_id_to_display_name;

    device_order_name_.clear();
    device_order_id_.clear();
    map_name_to_id_.clear();

    device_order_name_.reserve(qt_devices.size());
    device_order_id_.reserve(qt_devices.size());

    for (int i = 0; i < qt_devices.size(); ++i)
    {
        const std::string device_id = toDeviceIdHex(qt_devices[i]);
        const std::string display_name = qt_devices[i].description().toStdString();
        const std::string device_name = toDeviceName(i, qt_devices[i]);

        map_id_to_index[device_id] = i;
        map_id_to_display_name[device_id] = display_name;

        device_order_id_.push_back(device_id);
        device_order_name_.push_back(device_name);
        map_name_to_id_[device_name] = device_id;

        auto it = map_id_to_device_.find(device_id);
        if (it == map_id_to_device_.end())
        {
            auto emplaced = map_id_to_device_.emplace(device_id, CameraDevice(device_id, i, display_name));
            if (emplaced.second)
            {
                // Warm up capability cache so UI can query supported resolutions immediately.
                if (!emplaced.first->second.refreshCameraInfo())
                {
                    LOG(WARNING) << "Failed to refresh camera info after device discovery. id=" << device_id;
                }
            }
        }
        else
        {
            CameraInfo info;
            it->second.getCameraInfo(info);
            if (info.opencv_device_index != i)
            {
                const bool was_running = it->second.isRunning();
                if (was_running)
                {
                    LOG(WARNING) << "Camera index changed, recreating camera device instance. id="
                                 << device_id << " old_index=" << info.opencv_device_index
                                 << " new_index=" << i;
                    it->second.close();
                }

                map_id_to_device_.erase(it);
                map_id_to_device_.emplace(device_id, CameraDevice(device_id, i, display_name));
            }
            else if (info.set_supported_resolution.empty())
            {
                if (!it->second.refreshCameraInfo())
                {
                    LOG(WARNING) << "Failed to refresh camera info for existing device. id=" << device_id;
                }
            }
        }
    }

    std::vector<std::string> removed_ids;
    removed_ids.reserve(map_id_to_device_.size());

    for (const auto& kv : map_id_to_device_)
    {
        if (map_id_to_index.find(kv.first) == map_id_to_index.end())
        {
            removed_ids.push_back(kv.first);
        }
    }

    for (const auto& device_id : removed_ids)
    {
        auto it = map_id_to_device_.find(device_id);
        if (it != map_id_to_device_.end())
        {
            if (it->second.isRunning())
            {
                LOG(WARNING) << "Camera removed during runtime, closing device. id=" << device_id;
                it->second.close();
            }
            map_id_to_device_.erase(it);
        }
    }
}

CameraDevice* CameraManager::getPointerToDevice(const std::string& device_name)
{
    updateAllDevice();

    auto it_name = map_name_to_id_.find(device_name);
    if (it_name != map_name_to_id_.end())
    {
        auto it_device = map_id_to_device_.find(it_name->second);
        if (it_device != map_id_to_device_.end())
        {
            return &it_device->second;
        }
    }

    // Fallback 1: treat input as device id directly.
    auto it_by_id = map_id_to_device_.find(device_name);
    if (it_by_id != map_id_to_device_.end())
    {
        return &it_by_id->second;
    }

    // Fallback 2: match by normalized display name ("N: xxx" -> "xxx").
    const std::string normalized_target = normalizeDeviceDisplayName(device_name);
    for (const auto& kv : map_name_to_id_)
    {
        if (normalizeDeviceDisplayName(kv.first) == normalized_target)
        {
            auto it_device = map_id_to_device_.find(kv.second);
            if (it_device != map_id_to_device_.end())
            {
                return &it_device->second;
            }
        }
    }

    return nullptr;
}

CameraDevice* CameraManager::getPointerToDeviceById(const std::string& device_id)
{
    updateAllDevice();

    auto it = map_id_to_device_.find(device_id);
    if (it == map_id_to_device_.end())
    {
        return nullptr;
    }

    return &it->second;
}

const std::vector<std::string>& CameraManager::getAllDeviceName() const
{
    return device_order_name_;
}

const std::vector<std::string>& CameraManager::getAllDeviceId() const
{
    return device_order_id_;
}

} // namespace Vision
} // namespace VisionCursor
