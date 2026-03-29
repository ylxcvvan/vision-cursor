#pragma once

#include <array>
#include <cstddef>
#include <set>
#include <string>

#include "common/defines.h"
#include "common/types.h"

namespace VisionCursor
{

struct Landmark
{
    float x = 0.0f;
    float y = 0.0f;
    union
    {
        float z = 0.0f;
        float depth;
    };
};

struct HandLandmark
{
    union
    {
        Landmark normalized;
        struct
        {
            float x;
            float y;
            union
            {
                float z;
                float depth;
            };
        };
    };

    Landmark world{};
    bool has_normalized = false;
    bool has_world = false;

    HandLandmark() : normalized{} {}

    void reset()
    {
        normalized = {};
        world = {};
        has_normalized = false;
        has_world = false;
    }

    bool isValid() const { return has_normalized || has_world; }
};

struct HandLandmarkArray
{
    std::array<HandLandmark, HAND_LANDMARK_COUNT> data{};

    HandLandmark& operator[](HandJoint joint) { return data[static_cast<std::size_t>(joint)]; }
    const HandLandmark& operator[](HandJoint joint) const { return data[static_cast<std::size_t>(joint)]; }

    HandLandmark& operator[](std::size_t index) { return data[index]; }
    const HandLandmark& operator[](std::size_t index) const { return data[index]; }

    void reset()
    {
        for (auto& lm : data)
        {
            lm.reset();
        }
    }

    bool isValid() const
    {
        for (const auto& lm : data)
        {
            if (lm.isValid())
            {
                return true;
            }
        }
        return false;
    }
};

struct CameraResolution
{
    int width = 0;
    int height = 0;

    bool operator<(const CameraResolution& other) const
    {
        return width == other.width ? height < other.height : width < other.width;
    }
};

struct CameraInfo
{
    std::string device_id{};
    std::string display_name{};
    std::string name{};
    int opencv_device_index = -1;
    std::set<CameraResolution> set_supported_resolution{};
};

struct MousePosition
{
    int x = 0;
    int y = 0;
};

struct MappingRegion
{
    float left = 0.3f;
    float top = 0.3f;
    float right = 0.7f;
    float bottom = 0.7f;
};

} // namespace VisionCursor
