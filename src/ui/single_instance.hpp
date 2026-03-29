#pragma once

#include <mutex>

#include "config/core_config.h"
#include "core/vision_cursor_orchestrator.h"

namespace VisionCursor
{

class CoreManager final
{
public:
    static CoreManager& instance()
    {
        static CoreManager instance;
        return instance;
    }

    CoreManager(const CoreManager&) = delete;
    CoreManager& operator=(const CoreManager&) = delete;

public:
    Core::VisionCursorOrchestrator& core() noexcept { return orchestrator_; }
    const Core::VisionCursorOrchestrator& core() const noexcept { return orchestrator_; }

    Config::OrchestratorConfig config() const
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        return current_config_;
    }

    void setConfig(const Config::OrchestratorConfig& config)
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        current_config_ = config;
    }

    void resetConfig()
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        current_config_ = Config::CoreConfig::createDefault();
    }

    bool applyConfig()
    {
        Config::OrchestratorConfig snapshot;
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            snapshot = current_config_;
        }
        return orchestrator_.configure(snapshot);
    }

    bool initCore() { return orchestrator_.init(); }
    bool startCore() { return orchestrator_.start(); }
    bool stopCore() { return orchestrator_.stop(); }
    bool clearCore() { return orchestrator_.clear(); }

private:
    CoreManager() : current_config_(Config::CoreConfig::createDefault()) {}
    ~CoreManager() = default;

private:
    Core::VisionCursorOrchestrator orchestrator_;

    mutable std::mutex config_mutex_;
    Config::OrchestratorConfig current_config_;
};

} // namespace VisionCursor
