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
                      const BotTuningOverrides& botOverrides);

bool loadSettingsFile(const std::string& path,
                      Config& cfg,
                      bool& botEnabled,
                      BotDifficulty& botDifficulty,
                      float& musicVolume,
                      float& sfxVolume,
                      bool& muted,
                      BotTuningOverrides& botOverrides);

} // namespace settings_io
