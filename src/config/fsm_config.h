#pragma once

#include <QString>

#include "common/common.h"
#include "interaction/factory/state_machine_factory.h"

namespace VisionCursor
{
namespace Config
{

class FsmConfig
{
public:
    static QString defaultPath();
    static QString presetFilePath(const QString& preset_name);

    static bool load(Interaction::FSM::RuleSet& rules, const QString& preset_name);
    static bool ensureBuiltinPresets(bool overwrite = false);
    static bool generatePreset(const QString& preset_name, ControlPreset preset = ControlPreset::Easy, bool overwrite = false);
};

} // namespace Config
} // namespace VisionCursor
