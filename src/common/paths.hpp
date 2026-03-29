#pragma once

#include <filesystem>
#include <string>

namespace VisionCursor
{

class Paths
{
public:
    using path = std::filesystem::path;

    static void init(const std::string& base_path = "")
    {
        if (!base_path.empty())
        {
            base_path_ = path(base_path);
        }
        else
        {
            base_path_ = std::filesystem::current_path();
        }
        initialized_ = true;
    }

    static std::string basePath()
    {
        ensureInit();
        return base_path_.string();
    }

    static std::string configDir()
    {
        ensureInit();
        return (base_path_ / "config").string();
    }

    static std::string mediapipeDir()
    {
        ensureInit();
        return (base_path_ / "training_data").string();
    }

    static std::string fsmConfigDir()
    {
        ensureInit();
        return (base_path_ / "config" / "ControlPreset").string();
    }

    static std::string coreConfigFile()
    {
        ensureInit();
        return (base_path_ / "config" / "core_config.json").string();
    }

    static bool ensureDirExists(const std::string& dir)
    {
        path p(dir);
        if (std::filesystem::exists(p))
        {
            return true;
        }
        return std::filesystem::create_directories(p);
    }

    static bool ensureParentDirExists(const std::string& file)
    {
        path p(file);
        return ensureDirExists(p.parent_path().string());
    }

private:
    static void ensureInit()
    {
        if (!initialized_)
        {
            init();
        }
    }

private:
    inline static path base_path_{};
    inline static bool initialized_ = false;
};

} // namespace VisionCursor
