#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "common/common.h"
#include "vision/camera_device.h"

namespace VisionCursor
{
namespace Vision
{

class CameraManager
{
public:
    CameraManager() = default;
    ~CameraManager() = default;

    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;
    CameraManager(CameraManager&&) = delete;
    CameraManager& operator=(CameraManager&&) = delete;

public:
    void updateAllDevice();

    CameraDevice* getPointerToDevice(const std::string& device_name);
    CameraDevice* getPointerToDeviceById(const std::string& device_id);

    const std::vector<std::string>& getAllDeviceName() const;
    const std::vector<std::string>& getAllDeviceId() const;

private:
    std::unordered_map<std::string, CameraDevice> map_id_to_device_;
    std::unordered_map<std::string, std::string> map_name_to_id_;
    std::vector<std::string> device_order_name_;
    std::vector<std::string> device_order_id_;
};

} // namespace Vision
} // namespace VisionCursor
