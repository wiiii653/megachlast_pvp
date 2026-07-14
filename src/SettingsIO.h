#pragma once

#include "BotConfig.h"
#include "GameTypes.h"

#include <string>

namespace settings_io {

void saveSettingsFile(const std::string& path,
                      const Config& cfg,
                      bool botEnabled,
                      BotDifficulty botDifficulty,
                      float musicVolume,
                      float sfxVolume,
                      bool muted,
                      const GraphicsSettings& graphics,
                      const ControllerSettings& controllers,
                      const BotTuningOverrides& botOverrides);

bool loadSettingsFile(const std::string& path,
                      Config& cfg,
                      bool& botEnabled,
                      BotDifficulty& botDifficulty,
                      float& musicVolume,
                      float& sfxVolume,
                      bool& muted,
                      GraphicsSettings& graphics,
                      ControllerSettings& controllers,
                      BotTuningOverrides& botOverrides);

} // namespace settings_io
