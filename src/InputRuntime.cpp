#include "InputRuntime.h"

#include "ControllerInput.h"
#include "GameConstants.h"

#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Joystick.hpp>

#include <algorithm>
#include <cstdio>

namespace input_runtime {

void handleKeyPressed(const sf::Event::KeyPressed& kp,
                      sf::RenderWindow& win,
                      FrameContext& context);

namespace {

RoundModifier cycleModifier(RoundModifier modifier, int delta)
{
    int count = static_cast<int>(RoundModifier::COUNT);
    int next = (static_cast<int>(modifier) + delta + count) % count;
    return static_cast<RoundModifier>(next);
}

ArenaPreset cycleArena(ArenaPreset arena, int delta)
{
    int count = static_cast<int>(ArenaPreset::COUNT);
    int next = (static_cast<int>(arena) + delta + count) % count;
    return static_cast<ArenaPreset>(next);
}

struct SetupControllerState {
    bool p1Left = false;
    bool p1Right = false;
    bool p1ArenaLeft = false;
    bool p1ArenaRight = false;
    bool p1Start = false;
    bool p2Left = false;
    bool p2Right = false;
    bool p2Start = false;
};

struct UiControllerState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool south = false;
    bool east = false;
    bool north = false;
    bool west = false;
    bool start = false;
    bool select = false;
};

UiControllerState pollUiController(int joystick)
{
    UiControllerState current{};
    if(joystick < 0 || joystick >= static_cast<int>(sf::Joystick::Count) ||
       !sf::Joystick::isConnected(static_cast<unsigned int>(joystick))) return current;

    const unsigned int id = static_cast<unsigned int>(joystick);
    const auto axisPressed = [&](sf::Joystick::Axis axis, bool negative){
        if(!sf::Joystick::hasAxis(id, axis)) return false;
        const float position = sf::Joystick::getAxisPosition(id, axis);
        return negative ? position < -35.f : position > 35.f;
    };
    current.left = axisPressed(sf::Joystick::Axis::X, true) || axisPressed(sf::Joystick::Axis::PovX, true);
    current.right = axisPressed(sf::Joystick::Axis::X, false) || axisPressed(sf::Joystick::Axis::PovX, false);
    current.up = axisPressed(sf::Joystick::Axis::Y, true) || axisPressed(sf::Joystick::Axis::PovY, true);
    current.down = axisPressed(sf::Joystick::Axis::Y, false) || axisPressed(sf::Joystick::Axis::PovY, false);
    current.south = controller_input::buttonPressed(id, controller_input::southButton(id));
    current.east = controller_input::buttonPressed(id, controller_input::eastButton(id));
    current.north = controller_input::buttonPressed(id, controller_input::northButton(id));
    current.west = controller_input::buttonPressed(id, controller_input::westButton(id));
    current.start = controller_input::buttonPressed(id, controller_input::startButton(id));
    current.select = controller_input::buttonPressed(id, controller_input::selectButton(id));
    return current;
}

bool rose(bool current, bool previous)
{
    return current && !previous;
}

void emitControllerKey(const sf::Keyboard::Scancode scancode,
                       const sf::Keyboard::Key code,
                       sf::RenderWindow& win,
                       FrameContext& context)
{
    sf::Event::KeyPressed key{};
    key.scancode = scancode;
    key.code = code;
    handleKeyPressed(key, win, context);
}

void updateControllerActions(sf::RenderWindow& win, FrameContext& context)
{
    static UiControllerState previous{};
    static GameState previousState = GameState::MENU;
    const GameState state = *context.state;
    const int joystick = context.controllers->p1_joystick;
    const UiControllerState current = pollUiController(joystick);

    if(state != previousState) previous = {};

    if(state == GameState::MENU){
        if(rose(current.left, previous.left))
            emitControllerKey(sf::Keyboard::Scan::Left, sf::Keyboard::Key::Unknown, win, context);
        if(rose(current.right, previous.right))
            emitControllerKey(sf::Keyboard::Scan::Right, sf::Keyboard::Key::Unknown, win, context);
        if(rose(current.south, previous.south))
            emitControllerKey(sf::Keyboard::Scan::Enter, sf::Keyboard::Key::Enter, win, context);
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    } else if(state == GameState::MATCH_SETUP){
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    } else if(state == GameState::SETTINGS){
        if(rose(current.up, previous.up))
            emitControllerKey(sf::Keyboard::Scan::Up, sf::Keyboard::Key::Unknown, win, context);
        if(rose(current.down, previous.down))
            emitControllerKey(sf::Keyboard::Scan::Down, sf::Keyboard::Key::Unknown, win, context);
        if(rose(current.left, previous.left))
            emitControllerKey(sf::Keyboard::Scan::Left, sf::Keyboard::Key::Unknown, win, context);
        if(rose(current.right, previous.right))
            emitControllerKey(sf::Keyboard::Scan::Right, sf::Keyboard::Key::Unknown, win, context);
        if(rose(current.south, previous.south))
            emitControllerKey(sf::Keyboard::Scan::Enter, sf::Keyboard::Key::Enter, win, context);
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    } else if(state == GameState::DONATE){
        if(rose(current.south, previous.south))
            emitControllerKey(sf::Keyboard::Scan::C, sf::Keyboard::Key::C, win, context);
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    } else if(state == GameState::PLAYING){
        if(rose(current.start, previous.start))
            emitControllerKey(sf::Keyboard::Scan::Space, sf::Keyboard::Key::Space, win, context);
        if(rose(current.select, previous.select))
            emitControllerKey(sf::Keyboard::Scan::R, sf::Keyboard::Key::R, win, context);
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    } else if(state == GameState::PAUSED){
        if(rose(current.start, previous.start))
            emitControllerKey(sf::Keyboard::Scan::Space, sf::Keyboard::Key::Space, win, context);
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    } else if(state == GameState::GAME_OVER){
        if(rose(current.south, previous.south))
            emitControllerKey(sf::Keyboard::Scan::Enter, sf::Keyboard::Key::Enter, win, context);
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    } else if(state == GameState::COUNTDOWN){
        if(rose(current.east, previous.east))
            emitControllerKey(sf::Keyboard::Scan::Escape, sf::Keyboard::Key::Escape, win, context);
    }

    previous = current;
    previousState = *context.state;
}

void updateMatchSetupControllers(FrameContext& context)
{
    if(*context.state != GameState::MATCH_SETUP) return;

    static SetupControllerState previous{};
    auto poll = [](int joystick, bool& left, bool& right, bool& arenaLeft, bool& arenaRight, bool& start){
        left = right = arenaLeft = arenaRight = start = false;
        if(joystick < 0 || joystick >= static_cast<int>(sf::Joystick::Count) ||
           !sf::Joystick::isConnected(static_cast<unsigned int>(joystick))) return;
        unsigned int id = static_cast<unsigned int>(joystick);
        const float x = sf::Joystick::hasAxis(id, sf::Joystick::Axis::X)
                          ? sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::X) : 0.f;
        const float povX = sf::Joystick::hasAxis(id, sf::Joystick::Axis::PovX)
                             ? sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::PovX) : 0.f;
        left = x < -35.f || povX < -35.f;
        right = x > 35.f || povX > 35.f;
        arenaLeft = sf::Joystick::isButtonPressed(id, 4);
        arenaRight = sf::Joystick::isButtonPressed(id, 5);
        start = controller_input::firePressed(id);
    };

    SetupControllerState current{};
    bool p2ArenaLeft = false;
    bool p2ArenaRight = false;
    poll(context.controllers->p1_joystick, current.p1Left, current.p1Right,
         current.p1ArenaLeft, current.p1ArenaRight, current.p1Start);
    poll(context.controllers->p2_joystick, current.p2Left, current.p2Right,
         p2ArenaLeft, p2ArenaRight, current.p2Start);

    MatchSetup& setup = *context.matchSetup;
    if(current.p1Left && !previous.p1Left) setup.p1_modifier = cycleModifier(setup.p1_modifier, -1);
    if(current.p1Right && !previous.p1Right) setup.p1_modifier = cycleModifier(setup.p1_modifier, 1);
    if(current.p2Left && !previous.p2Left) setup.p2_modifier = cycleModifier(setup.p2_modifier, -1);
    if(current.p2Right && !previous.p2Right) setup.p2_modifier = cycleModifier(setup.p2_modifier, 1);
    if(current.p1ArenaLeft && !previous.p1ArenaLeft) setup.arena = cycleArena(setup.arena, -1);
    if(current.p1ArenaRight && !previous.p1ArenaRight) setup.arena = cycleArena(setup.arena, 1);
    if((current.p1Start && !previous.p1Start) || (current.p2Start && !previous.p2Start))
        context.startConfiguredMatch();
    previous = current;
}

} // namespace

void setMovementKeyPressed(sf::Keyboard::Scancode sc, game_update_runtime::InputState& input)
{
    switch(sc){
        case sf::Keyboard::Scan::A:        input.kA = true; break;
        case sf::Keyboard::Scan::D:        input.kD = true; break;
        case sf::Keyboard::Scan::LControl: input.kLCtrl = true; break;
        case sf::Keyboard::Scan::Z:        input.kZ = true; break;
        case sf::Keyboard::Scan::LShift:   input.kLShift = true; break;
        case sf::Keyboard::Scan::Left:     input.kLeft = true; break;
        case sf::Keyboard::Scan::Right:    input.kRight = true; break;
        case sf::Keyboard::Scan::RControl: input.kRCtrl = true; break;
        case sf::Keyboard::Scan::Slash:    input.kSlash = true; break;
        case sf::Keyboard::Scan::RShift:   input.kRShift = true; break;
        default: break;
    }
}

void setMovementKeyReleased(sf::Keyboard::Scancode sc, game_update_runtime::InputState& input)
{
    switch(sc){
        case sf::Keyboard::Scan::A:        input.kA = false; break;
        case sf::Keyboard::Scan::D:        input.kD = false; break;
        case sf::Keyboard::Scan::LControl: input.kLCtrl = false; break;
        case sf::Keyboard::Scan::Z:        input.kZ = false; break;
        case sf::Keyboard::Scan::LShift:   input.kLShift = false; break;
        case sf::Keyboard::Scan::Left:     input.kLeft = false; break;
        case sf::Keyboard::Scan::Right:    input.kRight = false; break;
        case sf::Keyboard::Scan::RControl: input.kRCtrl = false; break;
        case sf::Keyboard::Scan::Slash:    input.kSlash = false; break;
        case sf::Keyboard::Scan::RShift:   input.kRShift = false; break;
        default: break;
    }
}

bool handleMatchSetupKeyPressed(const sf::Event::KeyPressed& kp,
                                GameState& state,
                                MatchSetup& setup,
                                const std::function<void()>& startMatch)
{
    if(kp.scancode == sf::Keyboard::Scan::A) setup.p1_modifier = cycleModifier(setup.p1_modifier, -1);
    if(kp.scancode == sf::Keyboard::Scan::D) setup.p1_modifier = cycleModifier(setup.p1_modifier, 1);
    if(kp.scancode == sf::Keyboard::Scan::Left) setup.p2_modifier = cycleModifier(setup.p2_modifier, -1);
    if(kp.scancode == sf::Keyboard::Scan::Right) setup.p2_modifier = cycleModifier(setup.p2_modifier, 1);
    if(kp.scancode == sf::Keyboard::Scan::Q) setup.arena = cycleArena(setup.arena, -1);
    if(kp.scancode == sf::Keyboard::Scan::E) setup.arena = cycleArena(setup.arena, 1);
    if(kp.scancode == sf::Keyboard::Scan::Enter) startMatch();
    if(kp.code == sf::Keyboard::Key::Escape) state = GameState::MENU;
    return true;
}

bool handleSettingsKeyPressed(const sf::Event::KeyPressed& kp,
                              GameState& state,
                              int& settingsSel,
                              Config& cfg,
                              bool& botEnabled,
                              BotDifficulty& botDifficulty,
                              float& musicVolume,
                              float& sfxVolume,
                              GraphicsSettings& graphicsSettings,
                              ControllerSettings& controllers,
                              const std::string& cfgPath,
                              const std::function<void(const std::string&)>& saveSettings,
                              const std::function<bool(const std::string&)>& loadSettings,
                              const std::function<void()>& applyGraphicsSettings)
{
    switch(kp.scancode){
        case sf::Keyboard::Scan::Up:
            settingsSel = (settingsSel + OPT_COUNT - 1) % OPT_COUNT;
            break;
        case sf::Keyboard::Scan::Down:
            settingsSel = (settingsSel + 1) % OPT_COUNT;
            break;
        case sf::Keyboard::Scan::Left:
            if(settingsSel == OPT_MUSIC) musicVolume = std::max(0.f, musicVolume - 5.f);
            else if(settingsSel == OPT_SFX) sfxVolume = std::max(0.f, sfxVolume - 5.f);
            else if(settingsSel == OPT_BOT_DIFF)
                botDifficulty = static_cast<BotDifficulty>((static_cast<int>(botDifficulty) + 2) % 3);
            else if(settingsSel == OPT_TARGET_SCORE) cfg.target_score = std::max(1, cfg.target_score - 1);
            else if(settingsSel == OPT_DAMAGE) cfg.damage = std::max(1.f, cfg.damage - 1.f);
            else if(settingsSel == OPT_FIRE_CD_P1) cfg.fire_cd_p1_frames = std::max(1, cfg.fire_cd_p1_frames - 1);
            else if(settingsSel == OPT_FIRE_CD_P2) cfg.fire_cd_p2_frames = std::max(1, cfg.fire_cd_p2_frames - 1);
            else if(settingsSel == OPT_P_SPEED) cfg.p_speed = std::max(20.f, cfg.p_speed - 5.f);
            else if(settingsSel == OPT_WINDOW_SCALE){
                graphicsSettings.window_scale = std::max(2, graphicsSettings.window_scale - 1);
                applyGraphicsSettings();
            }
            else if(settingsSel == OPT_SCREEN_ASPECT)
                graphicsSettings.screen_aspect = ScreenAspect::Ratio16x10;
            else if(settingsSel == OPT_P1_CONTROLLER)
                controllers.p1_joystick = std::max(-1, controllers.p1_joystick - 1);
            else if(settingsSel == OPT_P2_CONTROLLER)
                controllers.p2_joystick = std::max(-1, controllers.p2_joystick - 1);
            break;
        case sf::Keyboard::Scan::Right:
            if(settingsSel == OPT_MUSIC) musicVolume = std::min(100.f, musicVolume + 5.f);
            else if(settingsSel == OPT_SFX) sfxVolume = std::min(100.f, sfxVolume + 5.f);
            else if(settingsSel == OPT_BOT_DIFF)
                botDifficulty = static_cast<BotDifficulty>((static_cast<int>(botDifficulty) + 1) % 3);
            else if(settingsSel == OPT_TARGET_SCORE) cfg.target_score = std::min(99, cfg.target_score + 1);
            else if(settingsSel == OPT_DAMAGE) cfg.damage = std::min(200.f, cfg.damage + 1.f);
            else if(settingsSel == OPT_FIRE_CD_P1) cfg.fire_cd_p1_frames = std::min(60, cfg.fire_cd_p1_frames + 1);
            else if(settingsSel == OPT_FIRE_CD_P2) cfg.fire_cd_p2_frames = std::min(60, cfg.fire_cd_p2_frames + 1);
            else if(settingsSel == OPT_P_SPEED) cfg.p_speed = std::min(400.f, cfg.p_speed + 5.f);
            else if(settingsSel == OPT_WINDOW_SCALE){
                graphicsSettings.window_scale = std::min(6, graphicsSettings.window_scale + 1);
                applyGraphicsSettings();
            }
            else if(settingsSel == OPT_SCREEN_ASPECT)
                graphicsSettings.screen_aspect = ScreenAspect::Ratio16x9;
            else if(settingsSel == OPT_P1_CONTROLLER)
                controllers.p1_joystick = std::min(7, controllers.p1_joystick + 1);
            else if(settingsSel == OPT_P2_CONTROLLER)
                controllers.p2_joystick = std::min(7, controllers.p2_joystick + 1);
            break;
        case sf::Keyboard::Scan::Enter:
            if(settingsSel == OPT_BOT_ENABLED) botEnabled = !botEnabled;
            else if(settingsSel == OPT_POSTFX) graphicsSettings.postfx_enabled = !graphicsSettings.postfx_enabled;
            else if(settingsSel == OPT_SCANLINES) graphicsSettings.scanlines_enabled = !graphicsSettings.scanlines_enabled;
            else if(settingsSel == OPT_VIGNETTE) graphicsSettings.vignette_enabled = !graphicsSettings.vignette_enabled;
            else if(settingsSel == OPT_CHROMATIC) graphicsSettings.chromatic_enabled = !graphicsSettings.chromatic_enabled;
            else if(settingsSel == OPT_COPPER_BARS) graphicsSettings.copper_bars_enabled = !graphicsSettings.copper_bars_enabled;
            else if(settingsSel == OPT_SAVE) saveSettings(cfgPath);
            else if(settingsSel == OPT_LOAD){
                int oldScale = graphicsSettings.window_scale;
                bool loaded = loadSettings(cfgPath);
                if(loaded && graphicsSettings.window_scale != oldScale) applyGraphicsSettings();
            }
            break;
        default:
            break;
    }

    if(kp.code == sf::Keyboard::Key::K) saveSettings(cfgPath);
    if(kp.code == sf::Keyboard::Key::L){
        int oldScale = graphicsSettings.window_scale;
        bool loaded = loadSettings(cfgPath);
        if(loaded && graphicsSettings.window_scale != oldScale) applyGraphicsSettings();
    }
    if(kp.code == sf::Keyboard::Key::Escape) state = GameState::MENU;
    return true;
}

bool handleDonateKeyPressed(const sf::Event::KeyPressed& kp,
                            GameState& state,
                            float& donateMsgTimer)
{
    if(kp.scancode == sf::Keyboard::Scan::C){
        sf::Clipboard::setString("https://buymeacoffee.com/ojnen");
        donateMsgTimer = 2.0f;
    }
    if(kp.code == sf::Keyboard::Key::Escape) state = GameState::MENU;
    return true;
}

void handleStateTransitionKeyPressed(const sf::Event::KeyPressed& kp,
                                     GameState state,
                                     int& menuSel,
                                     const std::function<void()>& startCountdownRound,
                                     const std::function<void()>& openSettings,
                                     const std::function<void()>& openDonate,
                                     const std::function<void()>& pauseGameplay,
                                     const std::function<void()>& resetPlayingRound,
                                     const std::function<void()>& resumeGameplay,
                                     const std::function<void()>& rematchCountdownRound)
{
    if(state == GameState::MENU){
        if(kp.scancode == sf::Keyboard::Scan::Left)
            menuSel = (menuSel + 2) % 3;
        if(kp.scancode == sf::Keyboard::Scan::Right)
            menuSel = (menuSel + 1) % 3;
        if(kp.scancode == sf::Keyboard::Scan::Enter){
            if(menuSel == 0) startCountdownRound();
            else if(menuSel == 1) openSettings();
            else if(menuSel == 2) openDonate();
        }
        if(kp.scancode == sf::Keyboard::Scan::O) openSettings();
        if(kp.scancode == sf::Keyboard::Scan::D) openDonate();
        return;
    }

    if(state == GameState::PLAYING){
        if(kp.scancode == sf::Keyboard::Scan::Space) pauseGameplay();
        if(kp.scancode == sf::Keyboard::Scan::R) resetPlayingRound();
        return;
    }

    if(state == GameState::PAUSED){
        if(kp.scancode == sf::Keyboard::Scan::Space) resumeGameplay();
        return;
    }

    if(state == GameState::GAME_OVER){
        if(kp.scancode == sf::Keyboard::Scan::Enter) rematchCountdownRound();
    }
}

void handleRuntimeToggleKeyPressed(const sf::Event::KeyPressed& kp,
                                   bool& botDebug,
                                   bool& botEnabled,
                                   BotDifficulty& botDifficulty,
                                   PerfLevel& perfLevel)
{
    switch(kp.scancode){
        case sf::Keyboard::Scan::G:
            botDebug = !botDebug;
            std::fprintf(stderr, "BOT_DEBUG %s\n", botDebug ? "ON" : "OFF");
            break;
        case sf::Keyboard::Scan::B:
            botEnabled = !botEnabled;
            std::fprintf(stderr, "Bot %s\n", botEnabled ? "enabled" : "disabled");
            break;
        case sf::Keyboard::Scan::V: {
            int next = (static_cast<int>(botDifficulty) + 1) % 3;
            botDifficulty = static_cast<BotDifficulty>(next);
            const char* n[] = {"EASY", "MEDIUM", "HARD"};
            std::fprintf(stderr, "Bot difficulty: %s\n", n[next]);
            break;
        }
        case sf::Keyboard::Scan::P: {
            int next = (static_cast<int>(perfLevel) + 1) % 4;
            perfLevel = static_cast<PerfLevel>(next);
            const char* n[] = {"HIGH", "MED", "LOW", "ULTRA"};
            std::fprintf(stderr, "Perf level: %s\n", n[next]);
            break;
        }
        default:
            break;
    }
}

void handleGlobalControlKeyPressed(const sf::Event::KeyPressed& kp,
                                   GameState& state,
                                   bool& muted,
                                   float& musicVolume,
                                   float& sfxVolume,
                                   bool& isFullscreen,
                                   const GraphicsSettings& graphicsSettings,
                                   game_update_runtime::InputState& input,
                                   sf::RenderWindow& win,
                                   const std::string& assetsDir,
                                   const std::function<void()>& playMenuMusic,
                                   const std::function<void()>& applyAllMusicSettings,
                                   const std::function<void(const std::string&)>& saveSettings)
{
    if(kp.code == sf::Keyboard::Key::Escape){
        if(state == GameState::MENU) win.close();
        else {
            state = GameState::MENU;
            playMenuMusic();
        }
    }

    if(kp.code == sf::Keyboard::Key::M){
        muted = !muted;
        applyAllMusicSettings();
    }

    if(kp.code == sf::Keyboard::Key::Comma){
        musicVolume = std::max(0.f, musicVolume - 5.f);
        sfxVolume = std::max(0.f, sfxVolume - 5.f);
        applyAllMusicSettings();
    }

    if(kp.code == sf::Keyboard::Key::Period){
        musicVolume = std::min(100.f, musicVolume + 5.f);
        sfxVolume = std::min(100.f, sfxVolume + 5.f);
        applyAllMusicSettings();
    }

    if(kp.code == sf::Keyboard::Key::K) saveSettings(assetsDir + "/settings.cfg");

    if(kp.scancode == sf::Keyboard::Scan::F11){
        isFullscreen = !isFullscreen;
        if(isFullscreen)
            win.create(sf::VideoMode::getDesktopMode(), "Megachlast PvP",
                       sf::Style::Default, sf::State::Fullscreen);
        else
            win.create(sf::VideoMode(sf::Vector2u{
                static_cast<unsigned int>(W * graphicsSettings.window_scale),
                static_cast<unsigned int>(H * graphicsSettings.window_scale)}),
                "Megachlast PvP", sf::Style::Default, sf::State::Windowed);
        win.setMouseCursorVisible(!isFullscreen);
        win.setFramerateLimit(60);
        input = {};
    }
}

void handleKeyPressed(const sf::Event::KeyPressed& kp,
                      sf::RenderWindow& win,
                      FrameContext& context)
{
    GameState& state = *context.state;
    int& settingsSel = *context.settingsSel;
    Config& cfg = *context.cfg;
    bool& botEnabled = *context.botEnabled;
    BotDifficulty& botDifficulty = *context.botDifficulty;
    float& musicVolume = *context.musicVolume;
    float& sfxVolume = *context.sfxVolume;
    GraphicsSettings& graphicsSettings = *context.graphicsSettings;
    ControllerSettings& controllers = *context.controllers;
    float& donateMsgTimer = *context.donateMsgTimer;
    bool& botDebug = *context.botDebug;
    PerfLevel& perfLevel = *context.perfLevel;
    bool& muted = *context.muted;
    bool& isFullscreen = *context.isFullscreen;
    game_update_runtime::InputState& input = *context.input;

    if(state == GameState::MATCH_SETUP){
        handleMatchSetupKeyPressed(kp, state, *context.matchSetup, context.startConfiguredMatch);
        return;
    }

    if(state == GameState::SETTINGS){
        float oldMusicVol = musicVolume;
        float oldSfxVol = sfxVolume;
        bool oldMuted = muted;
        handleSettingsKeyPressed(kp,
                                 state,
                                 settingsSel,
                                 cfg,
                                 botEnabled,
                                 botDifficulty,
                                 musicVolume,
                                 sfxVolume,
                                 graphicsSettings,
                                 controllers,
                                 *context.cfgPath,
                                 context.saveSettings,
                                 context.loadSettings,
                                 context.applyGraphicsSettings);
        if(musicVolume != oldMusicVol || sfxVolume != oldSfxVol || muted != oldMuted)
            context.applyAllMusicSettings();
        return;
    }

    if(state == GameState::DONATE){
        handleDonateKeyPressed(kp, state, donateMsgTimer);
        return;
    }

    handleRuntimeToggleKeyPressed(kp, botDebug, botEnabled, botDifficulty, perfLevel);
    setMovementKeyPressed(kp.scancode, input);
    handleGlobalControlKeyPressed(kp,
                                  state,
                                  muted,
                                  musicVolume,
                                  sfxVolume,
                                  isFullscreen,
                                  graphicsSettings,
                                  input,
                                  win,
                                  *context.assetsDir,
                                  context.playMenuMusic,
                                  context.applyAllMusicSettings,
                                  context.saveSettings);
    handleStateTransitionKeyPressed(kp,
                                    state,
                                    *context.menuSel,
                                    context.startCountdownRound,
                                    context.openSettings,
                                    context.openDonate,
                                    context.pauseGameplay,
                                    context.resetPlayingRound,
                                    context.resumeGameplay,
                                    context.rematchCountdownRound);
}

void processEvents(sf::RenderWindow& win,
                   const std::function<void()>& onClosed,
                   const std::function<void()>& onFocusLost,
                   const std::function<void()>& onFocusGained,
                   const std::function<void(const sf::Event::KeyPressed&)>& onKeyPressed,
                   const std::function<void(const sf::Event::KeyReleased&)>& onKeyReleased)
{
    while(true){
        auto oe = win.pollEvent();
        if(!oe.has_value()) break;
        const auto& e = *oe;

        if(e.is<sf::Event::Closed>()) onClosed();
        if(e.is<sf::Event::FocusLost>()) onFocusLost();
        if(e.is<sf::Event::FocusGained>()) onFocusGained();
        if(const auto* kp = e.getIf<sf::Event::KeyPressed>()) onKeyPressed(*kp);
        if(const auto* kr = e.getIf<sf::Event::KeyReleased>()) onKeyReleased(*kr);
    }
}

void updateInputForFrame(sf::RenderWindow& win, FrameContext& context)
{
    auto& input = *context.input;
    processEvents(
        win,
        [&]{ win.close(); },
        [&]{ game_update_runtime::setFocus(input, false); },
        [&]{ game_update_runtime::setFocus(input, true); },
        [&](const sf::Event::KeyPressed& kp){
            handleKeyPressed(kp, win, context);
        },
        [&](const sf::Event::KeyReleased& kr){
            setMovementKeyReleased(kr.scancode, input);
        });

    updateMatchSetupControllers(context);
    updateControllerActions(win, context);

    if(*context.state == GameState::PLAYING){
        game_update_runtime::syncPlayingKeyboard(input);
        game_update_runtime::syncPlayingControllers(input, *context.controllers);
    }
}

} // namespace input_runtime
