#include "SettingsIO.h"

#include "SettingsParse.h"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <stdexcept>

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
                      const BotTuningOverrides& botOverrides)
{
    std::ofstream ofs(path);
    if(!ofs){ std::fprintf(stderr, "Failed to open settings for write: %s\n", path.c_str()); return; }
    const char* botNames[] = {"EASY","MEDIUM","HARD"};
    ofs << "TARGET_SCORE="   << cfg.target_score   << "\n"
        << "ROUNDS_TO_WIN=" << cfg.rounds_to_win  << "\n"
        << "P_SPEED="        << cfg.p_speed         << "\n"
        << "BULLET_SPEED="   << cfg.bullet_speed    << "\n"
        << "BULLET_TTL="     << cfg.bullet_ttl      << "\n"
        << "HIT_R="          << cfg.hit_r            << "\n"
        << "DAMAGE="         << cfg.damage           << "\n"
        << "FIRE_CD_P1_FRAMES=" << cfg.fire_cd_p1_frames << "\n"
        << "FIRE_CD_P2_FRAMES=" << cfg.fire_cd_p2_frames << "\n"
        << "BOT_ENABLED="    << (botEnabled ? 1 : 0) << "\n"
        << "BOT_DIFFICULTY=" << botNames[static_cast<int>(botDifficulty)] << "\n"
        << "MUSIC_VOLUME="   << musicVolume << "\n"
        << "SFX_VOLUME="     << sfxVolume   << "\n"
        << "MUTE="           << (muted ? 1 : 0)   << "\n"
        << "WINDOW_SCALE="   << graphics.window_scale << "\n"
        << "SCREEN_ASPECT="  << (graphics.screen_aspect == ScreenAspect::Ratio16x9 ? "16:9" : "16:10") << "\n"
        << "POSTFX_ENABLED=" << (graphics.postfx_enabled ? 1 : 0) << "\n"
        << "SCANLINES="      << (graphics.scanlines_enabled ? 1 : 0) << "\n"
        << "VIGNETTE="       << (graphics.vignette_enabled ? 1 : 0) << "\n"
        << "CHROMATIC_ABERRATION=" << (graphics.chromatic_enabled ? 1 : 0) << "\n"
        << "COPPER_BARS="    << (graphics.copper_bars_enabled ? 1 : 0) << "\n"
        << "P1_CONTROLLER=" << controllers.p1_joystick << "\n"
        << "P2_CONTROLLER=" << controllers.p2_joystick << "\n";

    auto writeOverride = [&](const char* key, float value){
        if(value >= 0.f) ofs << key << "=" << value << "\n";
    };
    writeOverride("BOT_DODGE_ZONE",       botOverrides.dodge_zone);
    writeOverride("BOT_DODGE_X_THR",      botOverrides.dodge_x_thr);
    writeOverride("BOT_ALIGN_TOL",        botOverrides.align_tol);
    writeOverride("BOT_FIRE_PROB",        botOverrides.fire_prob);
    writeOverride("BOT_REACTION",         botOverrides.reaction);
    writeOverride("BOT_POWERUP_INTEREST", botOverrides.powerup_interest);
    writeOverride("BOT_STRAFE_LO",        botOverrides.strafe_lo);
    writeOverride("BOT_STRAFE_HI",        botOverrides.strafe_hi);
    writeOverride("BOT_BOMB_FEAR",        botOverrides.bomb_fear);
    writeOverride("BOT_AIM_LEAD_EASY",    botOverrides.aim_lead_easy);
    writeOverride("BOT_AIM_LEAD_MED",     botOverrides.aim_lead_med);
    writeOverride("BOT_AIM_LEAD_HARD",    botOverrides.aim_lead_hard);

    std::fprintf(stderr, "Saved settings to %s\n", path.c_str());
}

bool loadSettingsFile(const std::string& path,
                      Config& cfg,
                      bool& botEnabled,
                      BotDifficulty& botDifficulty,
                      float& musicVolume,
                      float& sfxVolume,
                      bool& muted,
                      GraphicsSettings& graphics,
                      ControllerSettings& controllers,
                      BotTuningOverrides& botOverrides)
{
    std::ifstream ifs(path);
    if(!ifs) return false;

    std::string line;
    while(std::getline(ifs, line)){
        std::size_t comment = line.find('#');
        if(comment != std::string::npos) line = line.substr(0, comment);
        line = settings_parse::trim(line);
        if(line.empty()) continue;
        std::size_t eq = line.find('=');
        if(eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        key = settings_parse::trim(key);
        val = settings_parse::trim(val);
        try {
            auto asInt = [&val](int lo, int hi) -> int {
                auto parsed = settings_parse::parseIntClamped(val, lo, hi);
                if(!parsed) throw std::invalid_argument("invalid int");
                return *parsed;
            };
            auto asFloat = [&val](float lo, float hi) -> float {
                auto parsed = settings_parse::parseFloatClamped(val, lo, hi);
                if(!parsed) throw std::invalid_argument("invalid float");
                return *parsed;
            };

            if     (key=="TARGET_SCORE"  ) cfg.target_score   = asInt(1, 99);
            else if(key=="ROUNDS_TO_WIN" ) cfg.rounds_to_win  = asInt(1, 9);
            else if(key=="P_SPEED"       ) cfg.p_speed        = asFloat(20.f, 400.f);
            else if(key=="BULLET_SPEED"  ) cfg.bullet_speed   = asFloat(20.f, 600.f);
            else if(key=="BULLET_TTL"    ) cfg.bullet_ttl     = asFloat(0.1f, 10.f);
            else if(key=="HIT_R"         ) cfg.hit_r          = asFloat(1.f, 40.f);
            else if(key=="DAMAGE"        ) cfg.damage         = asFloat(1.f, 200.f);
            else if(key=="FIRE_CD_P1_FRAMES") cfg.fire_cd_p1_frames = asInt(1, 60);
            else if(key=="FIRE_CD_P2_FRAMES") cfg.fire_cd_p2_frames = asInt(1, 60);
            else if(key=="FIRE_CD_FRAMES") {
                int cd = asInt(1, 60);
                cfg.fire_cd_p1_frames = cd;
                cfg.fire_cd_p2_frames = cd;
            }
            else if(key=="BOT_ENABLED"){
                auto parsed = settings_parse::parseBool(val);
                if(!parsed) throw std::invalid_argument("invalid bool");
                botEnabled = *parsed;
            }
            else if(key=="BOT_DIFFICULTY"){
                std::string v = val;
                for(char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if(v=="easy")        botDifficulty = BotDifficulty::EASY;
                else if(v=="medium") botDifficulty = BotDifficulty::MEDIUM;
                else if(v=="hard")   botDifficulty = BotDifficulty::HARD;
                else throw std::invalid_argument("invalid bot difficulty");
            }
            else if(key=="BOT_DODGE_ZONE")        botOverrides.dodge_zone      = asFloat(1.f, 200.f);
            else if(key=="BOT_DODGE_X_THR")       botOverrides.dodge_x_thr     = asFloat(1.f, 80.f);
            else if(key=="BOT_ALIGN_TOL")         botOverrides.align_tol       = asFloat(1.f, 80.f);
            else if(key=="BOT_FIRE_PROB")         botOverrides.fire_prob       = asFloat(0.f, 1.f);
            else if(key=="BOT_REACTION")          botOverrides.reaction        = asFloat(0.f, 1.f);
            else if(key=="BOT_POWERUP_INTEREST")  botOverrides.powerup_interest= asFloat(0.f, 1.f);
            else if(key=="BOT_STRAFE_LO")         botOverrides.strafe_lo       = asFloat(0.05f, 5.f);
            else if(key=="BOT_STRAFE_HI")         botOverrides.strafe_hi       = asFloat(0.05f, 8.f);
            else if(key=="BOT_BOMB_FEAR")         botOverrides.bomb_fear       = asFloat(0.f, 200.f);
            else if(key=="BOT_AIM_LEAD_EASY")     botOverrides.aim_lead_easy   = asFloat(0.f, 3.f);
            else if(key=="BOT_AIM_LEAD_MED")      botOverrides.aim_lead_med    = asFloat(0.f, 3.f);
            else if(key=="BOT_AIM_LEAD_HARD")     botOverrides.aim_lead_hard   = asFloat(0.f, 3.f);
            else if(key=="MUSIC_VOLUME") musicVolume = asFloat(0.f, 100.f);
            else if(key=="SFX_VOLUME"  ) sfxVolume   = asFloat(0.f, 100.f);
            else if(key=="MUTE"){
                auto parsed = settings_parse::parseBool(val);
                if(!parsed) throw std::invalid_argument("invalid bool");
                muted = *parsed;
            }
            else if(key=="WINDOW_SCALE") graphics.window_scale = asInt(2, 6);
            else if(key=="SCREEN_ASPECT"){
                if(val == "16:10") graphics.screen_aspect = ScreenAspect::Ratio16x10;
                else if(val == "16:9") graphics.screen_aspect = ScreenAspect::Ratio16x9;
                else throw std::invalid_argument("invalid screen aspect");
            }
            else if(key=="POSTFX_ENABLED"){
                auto parsed = settings_parse::parseBool(val);
                if(!parsed) throw std::invalid_argument("invalid bool");
                graphics.postfx_enabled = *parsed;
            }
            else if(key=="SCANLINES"){
                auto parsed = settings_parse::parseBool(val);
                if(!parsed) throw std::invalid_argument("invalid bool");
                graphics.scanlines_enabled = *parsed;
            }
            else if(key=="VIGNETTE"){
                auto parsed = settings_parse::parseBool(val);
                if(!parsed) throw std::invalid_argument("invalid bool");
                graphics.vignette_enabled = *parsed;
            }
            else if(key=="CHROMATIC_ABERRATION"){
                auto parsed = settings_parse::parseBool(val);
                if(!parsed) throw std::invalid_argument("invalid bool");
                graphics.chromatic_enabled = *parsed;
            }
            else if(key=="COPPER_BARS"){
                auto parsed = settings_parse::parseBool(val);
                if(!parsed) throw std::invalid_argument("invalid bool");
                graphics.copper_bars_enabled = *parsed;
            }
            else if(key=="P1_CONTROLLER") controllers.p1_joystick = asInt(-1, 7);
            else if(key=="P2_CONTROLLER") controllers.p2_joystick = asInt(-1, 7);
        } catch(...) {
            std::fprintf(stderr, "Ignoring invalid setting %s=%s\n", key.c_str(), val.c_str());
        }
    }
    std::fprintf(stderr, "Loaded settings from %s\n", path.c_str());
    return true;
}

} // namespace settings_io
