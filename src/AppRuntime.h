#pragma once

#include "AudioDuck.h"
#include "BotConfig.h"
#include "BotController.h"
#include "CliOptions.h"
#include "FxGovernor.h"
#include "GameTypes.h"
#include "PerfLog.h"

#include <SFML/Audio.hpp>

#include <string>

namespace app_runtime {

struct RuntimeOptions {
    std::string assets_dir = "assets";
    bool no_music = false;
    bool no_postfx = false;
    bool bot_enabled = false;
    bool verbose = false;
    BotDifficulty bot_difficulty = BotDifficulty::MEDIUM;
    PerfLevel perf_level = PerfLevel::MEDIUM;
    perf_log::Options perf_log{};
};

struct AudioSettings {
    float music_volume = 90.0f;
    float sfx_volume = 90.0f;
    bool muted = false;
    audio_duck::State music_duck{};
};

struct MusicRuntime {
    sf::Music menu;
    bool have_menu = false;
    sf::Music ingame;
    bool have_ingame = false;
    sf::Music get_ready;
    bool have_get_ready = false;
};

struct FxRuntimeState {
    int particle_spawn_budget = 0;
    int particle_spawns_this_frame = 0;
    int fx_level = 0;
    fx_governor::State fx_state{};
};

struct BotRuntime {
    BotTuningOverrides overrides{};
    bot_controller::RuntimeState state{};
};

std::string settingsPath(const RuntimeOptions& options);
void applyCliOptions(const cli_options::Parsed& cli, RuntimeOptions& options);

} // namespace app_runtime
