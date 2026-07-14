#include "GameTypes.h"
#include "SettingsIO.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

static Config cfg;
static bool g_bot_enabled = false;
static BotDifficulty g_bot_difficulty = BotDifficulty::MEDIUM;
static float g_music_volume = 90.f;
static float g_sfx_volume = 90.f;
static bool g_muted = false;
static GraphicsSettings g_graphics{};
static ControllerSettings g_controllers{};
static BotTuningOverrides g_bot_overrides{};

namespace {

int failures = 0;

void check(bool ok, const char* msg)
{
    if(!ok){
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    }
}

bool containsLine(const std::string& path, const std::string& needle)
{
    std::ifstream ifs(path);
    std::string line;
    while(std::getline(ifs, line))
        if(line == needle) return true;
    return false;
}

} // namespace

int main()
{
    const std::filesystem::path tempPath =
        std::filesystem::temp_directory_path() / "megachlast_settings_integration_test.cfg";
    const std::string path = tempPath.string();
    std::remove(path.c_str());

    cfg.target_score = 13;
    cfg.p_speed = 177.f;
    cfg.fire_cd_p1_frames = 9;
    cfg.fire_cd_p2_frames = 11;
    g_bot_enabled = true;
    g_bot_difficulty = BotDifficulty::HARD;
    g_music_volume = 47.f;
    g_sfx_volume = 63.f;
    g_muted = true;
    g_graphics.window_scale = 3;
    g_graphics.postfx_enabled = false;
    g_graphics.scanlines_enabled = false;
    g_graphics.vignette_enabled = true;
    g_graphics.chromatic_enabled = false;
    g_graphics.copper_bars_enabled = true;
    g_controllers.p1_joystick = 2;
    g_controllers.p2_joystick = -1;

    settings_io::saveSettingsFile(path, cfg, g_bot_enabled, g_bot_difficulty,
                                  g_music_volume, g_sfx_volume, g_muted,
                                  g_graphics,
                                  g_controllers,
                                  g_bot_overrides);
    check(containsLine(path, "BOT_ENABLED=1"), "settings save writes BOT_ENABLED");
    check(containsLine(path, "WINDOW_SCALE=3"), "settings save writes window scale");
    check(containsLine(path, "P1_CONTROLLER=2"), "settings save writes P1 controller");

    cfg.target_score = 1;
    cfg.p_speed = 20.f;
    cfg.fire_cd_p1_frames = 1;
    cfg.fire_cd_p2_frames = 1;
    g_bot_enabled = false;
    g_bot_difficulty = BotDifficulty::EASY;
    g_music_volume = 10.f;
    g_sfx_volume = 10.f;
    g_muted = false;
    g_graphics = {};
    g_controllers = {};

    check(settings_io::loadSettingsFile(path, cfg, g_bot_enabled, g_bot_difficulty,
                                        g_music_volume, g_sfx_volume, g_muted,
                                        g_graphics,
                                        g_controllers,
                                        g_bot_overrides),
          "settings load succeeds for saved file");
    check(cfg.target_score == 13, "target score round-trips");
    check(std::fabs(cfg.p_speed - 177.f) < 0.0001f, "speed round-trips");
    check(cfg.fire_cd_p1_frames == 9, "p1 fire cooldown round-trips");
    check(cfg.fire_cd_p2_frames == 11, "p2 fire cooldown round-trips");
    check(g_bot_enabled, "BOT_ENABLED round-trips");
    check(g_bot_difficulty == BotDifficulty::HARD, "bot difficulty round-trips");
    check(std::fabs(g_music_volume - 47.f) < 0.0001f, "music volume round-trips");
    check(std::fabs(g_sfx_volume - 63.f) < 0.0001f, "sfx volume round-trips");
    check(g_muted, "mute flag round-trips");
    check(g_graphics.window_scale == 3, "window scale round-trips");
    check(!g_graphics.postfx_enabled, "postfx toggle round-trips");
    check(!g_graphics.scanlines_enabled, "scanlines toggle round-trips");
    check(g_graphics.vignette_enabled, "vignette toggle round-trips");
    check(!g_graphics.chromatic_enabled, "chromatic toggle round-trips");
    check(g_graphics.copper_bars_enabled, "copper bars toggle round-trips");
    check(g_controllers.p1_joystick == 2, "P1 controller round-trips");
    check(g_controllers.p2_joystick == -1, "P2 controller round-trips");

    {
        std::ofstream ofs(path, std::ios::trunc);
        ofs << "BOT_ENABLED=maybe\n";
    }
    g_bot_enabled = true;
    check(settings_io::loadSettingsFile(path, cfg, g_bot_enabled, g_bot_difficulty,
                                        g_music_volume, g_sfx_volume, g_muted,
                                        g_graphics,
                                        g_controllers,
                                        g_bot_overrides),
          "settings load succeeds with invalid BOT_ENABLED value");
    check(g_bot_enabled, "invalid BOT_ENABLED does not overwrite previous value");

    {
        std::ofstream ofs(path, std::ios::trunc);
        ofs << "BOT_DIFFICULTY=impossible\n";
    }
    g_bot_difficulty = BotDifficulty::HARD;
    check(settings_io::loadSettingsFile(path, cfg, g_bot_enabled, g_bot_difficulty,
                                        g_music_volume, g_sfx_volume, g_muted,
                                        g_graphics,
                                        g_controllers,
                                        g_bot_overrides),
          "settings load succeeds with invalid BOT_DIFFICULTY value");
    check(g_bot_difficulty == BotDifficulty::HARD, "invalid BOT_DIFFICULTY does not overwrite previous value");

    std::remove(path.c_str());
    return failures == 0 ? 0 : 1;
}
