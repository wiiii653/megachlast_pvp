#include "InputRuntime.h"

#include "ControlsInput.h"
#include "ControllerInput.h"
#include "GameConstants.h"

#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Joystick.hpp>

#include <algorithm>
#include <array>
#include <cstdio>

namespace input_runtime {

void handleKeyPressed(const sf::Event::KeyPressed& kp,
                      sf::RenderWindow& win,
                      FrameContext& context);

namespace {

using controls::Action;

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

bool keyTriggers(const controls::Profile& kb, Action a, sf::Keyboard::Scancode sc)
{
    if(sc == sf::Keyboard::Scan::Unknown) return false;
    return kb.actions[static_cast<std::size_t>(a)].containsKey(static_cast<int>(sc));
}

struct ActionList {
    const Action* items = nullptr;
    int count = 0;
};

constexpr std::array<Action, 6> kGlobalActions = {
    Action::UtilMute, Action::UtilVolDown, Action::UtilVolUp,
    Action::UtilSave, Action::UtilLoad, Action::UtilFullscreen,
};

constexpr std::array<Action, 5> kMenuActions = {
    Action::UiLeft, Action::UiRight, Action::UiConfirm,
    Action::UtilSettings, Action::UtilDonate,
};

constexpr std::array<Action, 6> kSettingsActions = {
    Action::UiUp, Action::UiDown, Action::UiLeft, Action::UiRight, Action::UiConfirm,
    Action::UiBack,
};

constexpr std::array<Action, 8> kMatchSetupActions = {
    Action::SetupModLeft, Action::SetupModRight,
    Action::SetupP2ModLeft, Action::SetupP2ModRight,
    Action::SetupArenaL, Action::SetupArenaR,
    Action::SetupStart, Action::SetupBack,
};

constexpr std::array<Action, 2> kPlayingActions = { Action::GamePause, Action::GameReset };
constexpr std::array<Action, 1> kPausedActions = { Action::GamePause };
constexpr std::array<Action, 1> kGameOverActions = { Action::UiConfirm };
constexpr std::array<Action, 4> kDonateActions = { Action::UiUp, Action::UiDown, Action::UiConfirm, Action::UiBack };

ActionList stateEventActions(GameState state)
{
    switch(state){
        case GameState::MENU:       return { kMenuActions.data(),       static_cast<int>(kMenuActions.size()) };
        case GameState::MATCH_SETUP:return { kMatchSetupActions.data(), static_cast<int>(kMatchSetupActions.size()) };
        case GameState::SETTINGS:   return { kSettingsActions.data(),  static_cast<int>(kSettingsActions.size()) };
        case GameState::CONTROLS:   return { kSettingsActions.data(),  static_cast<int>(kSettingsActions.size()) };
        case GameState::DONATE:     return { kDonateActions.data(),    static_cast<int>(kDonateActions.size()) };
        case GameState::PLAYING:    return { kPlayingActions.data(),   static_cast<int>(kPlayingActions.size()) };
        case GameState::PAUSED:     return { kPausedActions.data(),   static_cast<int>(kPausedActions.size()) };
        case GameState::GAME_OVER:  return { kGameOverActions.data(), static_cast<int>(kGameOverActions.size()) };
        case GameState::COUNTDOWN:  return { nullptr, 0 };
    }
    return { nullptr, 0 };
}

// Resolves the state-specific action first, then the global utilities, so a
// key shared with a state action keeps its contextual meaning.
Action resolveAction(const controls::Profile& kb, GameState state, sf::Keyboard::Scancode sc)
{
    const ActionList list = stateEventActions(state);
    for(int i = 0; i < list.count; ++i)
        if(keyTriggers(kb, list.items[i], sc)) return list.items[i];
    for(Action a : kGlobalActions)
        if(keyTriggers(kb, a, sc)) return a;
    return Action::None;
}

void openControlsSubmenu(GameState& state,
                        int& controlsProfile,
                        int& controlsSel,
                        int& controlsScroll,
                        bool& controlsCapturing,
                        int& controlsCaptureAction)
{
    state = GameState::CONTROLS;
    controlsProfile = static_cast<int>(controls::ProfileId::Keyboard);
    controlsSel = 0;
    controlsScroll = 0;
    controlsCapturing = false;
    controlsCaptureAction = static_cast<int>(Action::None);
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
    bool start = false;
    bool select = false;
};

bool rose(bool current, bool previous)
{
    return current && !previous;
}

// Raw per-input snapshot used to detect fresh presses for binding capture.
struct RawPadState {
    std::array<bool, 32> buttons{};
    std::array<int8_t, 8> axes{}; // indexed by sf::Joystick::Axis value
};

RawPadState readRawPad(int joystick)
{
    RawPadState s;
    if(!controls::isConnected(joystick)) return s;
    const unsigned int id = static_cast<unsigned int>(joystick);
    const unsigned int buttonCount = sf::Joystick::getButtonCount(id);
    for(unsigned int b = 0; b < buttonCount && b < s.buttons.size(); ++b)
        s.buttons[b] = sf::Joystick::isButtonPressed(id, b);
    for(int a = 0; a < static_cast<int>(s.axes.size()); ++a){
        const auto axis = static_cast<sf::Joystick::Axis>(a);
        if(!sf::Joystick::hasAxis(id, axis)) continue;
        const float pos = sf::Joystick::getAxisPosition(id, axis);
        if(pos < -controls::kPadDeadZone) s.axes[a] = -1;
        else if(pos > controls::kPadDeadZone) s.axes[a] = 1;
    }
    return s;
}

controls::Input freshPadInput(const RawPadState& current, const RawPadState& previous)
{
    for(std::size_t b = 0; b < current.buttons.size(); ++b)
        if(current.buttons[b] && !previous.buttons[b])
            return controls::buttonInput(static_cast<unsigned int>(b));
    for(std::size_t a = 0; a < current.axes.size(); ++a)
        if(current.axes[a] != 0 && current.axes[a] != previous.axes[a])
            return controls::axisInput(static_cast<sf::Joystick::Axis>(a), current.axes[a] < 0);
    return {};
}

bool padActionActive(int joystick, const controls::Profile& pad, Action a)
{
    if(!controls::isConnected(joystick)) return false;
    return controls::actionPadActive(static_cast<unsigned int>(joystick), pad, a);
}

void emitKeyboardAction(Action a, sf::RenderWindow& win, FrameContext& context)
{
    const controls::Profile& kb = context.controllers->profiles.keyboard();
    const controls::Input& primary = kb.actions[static_cast<std::size_t>(a)].primary();
    if(primary.kind != controls::Input::Key || primary.value < 0) return;
    sf::Event::KeyPressed key{};
    key.scancode = controls::scancodeFromValue(primary.value);
    key.code = sf::Keyboard::Key::Unknown;
    handleKeyPressed(key, win, context);
}

void handleControlsCaptureKey(const sf::Event::KeyPressed& kp, FrameContext& context)
{
    if(kp.scancode == sf::Keyboard::Scan::Escape){
        *context.controlsCapturing = false;
        *context.controlsCaptureAction = static_cast<int>(Action::None);
        return;
    }
    auto& profiles = context.controllers->profiles;
    const auto id = static_cast<controls::ProfileId>(*context.controlsProfile);
    const auto a = static_cast<Action>(*context.controlsCaptureAction);
    if(a != Action::None)
        controls::bindPrimary(profiles.profile(id), a, controls::keyInput(kp.scancode));
    *context.controlsCapturing = false;
    *context.controlsCaptureAction = static_cast<int>(Action::None);
}

void updateControlsController(sf::RenderWindow& win, FrameContext& context)
{
    static RawPadState previousRaw{};
    static UiControllerState previousNav{};
    static GameState previousState = GameState::MENU;
    const int joystick = context.controllers->p1_joystick;
    const controls::Profile& pad = controls::profileForJoystick(context.controllers->profiles, joystick);
    const RawPadState currentRaw = readRawPad(joystick);

    if(*context.state != previousState){
        previousRaw = {};
        previousNav = {};
    }
    previousState = *context.state;

    if(*context.controlsCapturing){
        const controls::Input fresh = freshPadInput(currentRaw, previousRaw);
        if(fresh.valid()){
            const bool isBack =
                pad.actions[static_cast<std::size_t>(Action::UiBack)].contains(fresh);
            if(isBack){
                *context.controlsCapturing = false;
                *context.controlsCaptureAction = static_cast<int>(Action::None);
            } else {
                const auto id = static_cast<controls::ProfileId>(*context.controlsProfile);
                const auto a = static_cast<Action>(*context.controlsCaptureAction);
                if(a != Action::None)
                    controls::bindPrimary(context.controllers->profiles.profile(id), a, fresh);
                *context.controlsCapturing = false;
                *context.controlsCaptureAction = static_cast<int>(Action::None);
            }
        }
        previousRaw = currentRaw;
        return;
    }

    const bool up = padActionActive(joystick, pad, Action::UiUp);
    const bool down = padActionActive(joystick, pad, Action::UiDown);
    const bool left = padActionActive(joystick, pad, Action::UiLeft);
    const bool right = padActionActive(joystick, pad, Action::UiRight);
    const bool south = padActionActive(joystick, pad, Action::UiConfirm);
    const bool east = padActionActive(joystick, pad, Action::UiBack);

    if(rose(up, previousNav.up)) emitKeyboardAction(Action::UiUp, win, context);
    if(rose(down, previousNav.down)) emitKeyboardAction(Action::UiDown, win, context);
    if(rose(left, previousNav.left)) emitKeyboardAction(Action::UiLeft, win, context);
    if(rose(right, previousNav.right)) emitKeyboardAction(Action::UiRight, win, context);
    if(rose(south, previousNav.south)) emitKeyboardAction(Action::UiConfirm, win, context);
    if(rose(east, previousNav.east)) emitKeyboardAction(Action::UiBack, win, context);

    previousNav = { up, down, left, right, south, east, false, false };
    previousRaw = currentRaw;
}

void updateControllerActions(sf::RenderWindow& win, FrameContext& context)
{
    static UiControllerState previous{};
    static GameState previousState = GameState::MENU;
    const GameState state = *context.state;
    const int joystick = context.controllers->p1_joystick;

    if(state == GameState::CONTROLS){
        updateControlsController(win, context);
        previous = {};
        previousState = state;
        return;
    }

    const controls::Profile& pad = controls::profileForJoystick(context.controllers->profiles, joystick);
    UiControllerState current{};
    current.up = padActionActive(joystick, pad, Action::UiUp);
    current.down = padActionActive(joystick, pad, Action::UiDown);
    current.left = padActionActive(joystick, pad, Action::UiLeft);
    current.right = padActionActive(joystick, pad, Action::UiRight);
    current.south = padActionActive(joystick, pad, Action::UiConfirm);
    current.east = padActionActive(joystick, pad, Action::UiBack);
    current.start = padActionActive(joystick, pad, Action::GamePause);
    current.select = padActionActive(joystick, pad, Action::GameReset);

    if(state != previousState) previous = {};

    if(state == GameState::MENU){
        if(rose(current.left, previous.left))
            emitKeyboardAction(Action::UiLeft, win, context);
        if(rose(current.right, previous.right))
            emitKeyboardAction(Action::UiRight, win, context);
        if(rose(current.south, previous.south))
            emitKeyboardAction(Action::UiConfirm, win, context);
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::UiBack, win, context);
    } else if(state == GameState::MATCH_SETUP){
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::SetupBack, win, context);
    } else if(state == GameState::SETTINGS){
        if(rose(current.up, previous.up))
            emitKeyboardAction(Action::UiUp, win, context);
        if(rose(current.down, previous.down))
            emitKeyboardAction(Action::UiDown, win, context);
        if(rose(current.left, previous.left))
            emitKeyboardAction(Action::UiLeft, win, context);
        if(rose(current.right, previous.right))
            emitKeyboardAction(Action::UiRight, win, context);
        if(rose(current.south, previous.south))
            emitKeyboardAction(Action::UiConfirm, win, context);
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::UiBack, win, context);
    } else if(state == GameState::DONATE){
        if(rose(current.up, previous.up))
            emitKeyboardAction(Action::UiUp, win, context);
        if(rose(current.down, previous.down))
            emitKeyboardAction(Action::UiDown, win, context);
        if(rose(current.south, previous.south))
            emitKeyboardAction(Action::UiConfirm, win, context);
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::UiBack, win, context);
    } else if(state == GameState::PLAYING){
        if(rose(current.start, previous.start))
            emitKeyboardAction(Action::GamePause, win, context);
        if(rose(current.select, previous.select))
            emitKeyboardAction(Action::GameReset, win, context);
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::GameMenu, win, context);
    } else if(state == GameState::PAUSED){
        if(rose(current.start, previous.start))
            emitKeyboardAction(Action::GamePause, win, context);
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::GameMenu, win, context);
    } else if(state == GameState::GAME_OVER){
        if(rose(current.south, previous.south))
            emitKeyboardAction(Action::UiConfirm, win, context);
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::UiBack, win, context);
    } else if(state == GameState::COUNTDOWN){
        if(rose(current.east, previous.east))
            emitKeyboardAction(Action::UiBack, win, context);
    }

    previous = current;
    previousState = *context.state;
}

void updateMatchSetupControllers(FrameContext& context)
{
    if(*context.state != GameState::MATCH_SETUP) return;

    static SetupControllerState previous{};
    SetupControllerState current{};
    bool p1ArenaLeft = false;
    bool p1ArenaRight = false;

    auto poll = [&](int joystick, bool& left, bool& right, bool& start){
        left = right = start = false;
        if(!controls::isConnected(joystick)) return;
        const controls::Profile& pad = controls::profileForJoystick(context.controllers->profiles, joystick);
        const unsigned int id = static_cast<unsigned int>(joystick);
        left = controls::actionPadActive(id, pad, Action::SetupModLeft);
        right = controls::actionPadActive(id, pad, Action::SetupModRight);
        start = controls::actionPadActive(id, pad, Action::SetupStart);
    };
    auto pollArena = [&](int joystick){
        p1ArenaLeft = p1ArenaRight = false;
        if(!controls::isConnected(joystick)) return;
        const controls::Profile& pad = controls::profileForJoystick(context.controllers->profiles, joystick);
        const unsigned int id = static_cast<unsigned int>(joystick);
        p1ArenaLeft = controls::actionPadActive(id, pad, Action::SetupArenaL);
        p1ArenaRight = controls::actionPadActive(id, pad, Action::SetupArenaR);
    };

    poll(context.controllers->p1_joystick, current.p1Left, current.p1Right, current.p1Start);
    poll(context.controllers->p2_joystick, current.p2Left, current.p2Right, current.p2Start);
    pollArena(context.controllers->p1_joystick);

    MatchSetup& setup = *context.matchSetup;
    if(current.p1Left && !previous.p1Left) setup.p1_modifier = cycleModifier(setup.p1_modifier, -1);
    if(current.p1Right && !previous.p1Right) setup.p1_modifier = cycleModifier(setup.p1_modifier, 1);
    if(current.p2Left && !previous.p2Left) setup.p2_modifier = cycleModifier(setup.p2_modifier, -1);
    if(current.p2Right && !previous.p2Right) setup.p2_modifier = cycleModifier(setup.p2_modifier, 1);
    if(p1ArenaLeft && !previous.p1ArenaLeft) setup.arena = cycleArena(setup.arena, -1);
    if(p1ArenaRight && !previous.p1ArenaRight) setup.arena = cycleArena(setup.arena, 1);
    if((current.p1Start && !previous.p1Start) || (current.p2Start && !previous.p2Start))
        context.startConfiguredMatch();
    previous = current;
}

} // namespace

void setMovementKeyPressed(sf::Keyboard::Scancode sc, game_update_runtime::InputState& input,
                          const controls::Profile& keyboard)
{
    using controls::Action;
    if(keyTriggers(keyboard, Action::KbP1Left, sc)) input.kA = true;
    if(keyTriggers(keyboard, Action::KbP1Right, sc)) input.kD = true;
    if(keyTriggers(keyboard, Action::KbP1Fire, sc)) input.kLCtrl = input.kZ = input.kLShift = true;
    if(keyTriggers(keyboard, Action::KbP2Left, sc)) input.kLeft = true;
    if(keyTriggers(keyboard, Action::KbP2Right, sc)) input.kRight = true;
    if(keyTriggers(keyboard, Action::KbP2Fire, sc)) input.kRCtrl = input.kSlash = input.kRShift = true;
}

void setMovementKeyReleased(sf::Keyboard::Scancode sc, game_update_runtime::InputState& input,
                            const controls::Profile& keyboard)
{
    using controls::Action;
    if(keyTriggers(keyboard, Action::KbP1Left, sc)) input.kA = false;
    if(keyTriggers(keyboard, Action::KbP1Right, sc)) input.kD = false;
    if(keyTriggers(keyboard, Action::KbP1Fire, sc)) input.kLCtrl = input.kZ = input.kLShift = false;
    if(keyTriggers(keyboard, Action::KbP2Left, sc)) input.kLeft = false;
    if(keyTriggers(keyboard, Action::KbP2Right, sc)) input.kRight = false;
    if(keyTriggers(keyboard, Action::KbP2Fire, sc)) input.kRCtrl = input.kSlash = input.kRShift = false;
}

bool handleMatchSetupKeyPressed(const sf::Event::KeyPressed& kp,
                                GameState& state,
                                MatchSetup& setup,
                                const controls::Profile& keyboard,
                                const std::function<void()>& startMatch)
{
    const Action a = resolveAction(keyboard, GameState::MATCH_SETUP, kp.scancode);
    switch(a){
        case Action::SetupModLeft:  setup.p1_modifier = cycleModifier(setup.p1_modifier, -1); break;
        case Action::SetupModRight: setup.p1_modifier = cycleModifier(setup.p1_modifier, 1); break;
        case Action::SetupP2ModLeft: setup.p2_modifier = cycleModifier(setup.p2_modifier, -1); break;
        case Action::SetupP2ModRight: setup.p2_modifier = cycleModifier(setup.p2_modifier, 1); break;
        case Action::SetupArenaL: setup.arena = cycleArena(setup.arena, -1); break;
        case Action::SetupArenaR: setup.arena = cycleArena(setup.arena, 1); break;
        case Action::SetupStart: startMatch(); break;
        case Action::SetupBack: state = GameState::MENU; break;
        default: break;
    }
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
                              int& controlsProfile,
                              int& controlsSel,
                              int& controlsScroll,
                              bool& controlsCapturing,
                              int& controlsCaptureAction,
                              const std::string& cfgPath,
                              const std::function<void(const std::string&)>& saveSettings,
                              const std::function<bool(const std::string&)>& loadSettings,
                              const std::function<void()>& applyGraphicsSettings)
{
    const Action a = resolveAction(controllers.profiles.keyboard(), GameState::SETTINGS, kp.scancode);
    switch(a){
        case Action::UiUp:
            settingsSel = (settingsSel + OPT_COUNT - 1) % OPT_COUNT;
            break;
        case Action::UiDown:
            settingsSel = (settingsSel + 1) % OPT_COUNT;
            break;
        case Action::UiLeft:
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
        case Action::UiRight:
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
        case Action::UiConfirm:
            if(settingsSel == OPT_BOT_ENABLED) botEnabled = !botEnabled;
            else if(settingsSel == OPT_POSTFX) graphicsSettings.postfx_enabled = !graphicsSettings.postfx_enabled;
            else if(settingsSel == OPT_SCANLINES) graphicsSettings.scanlines_enabled = !graphicsSettings.scanlines_enabled;
            else if(settingsSel == OPT_VIGNETTE) graphicsSettings.vignette_enabled = !graphicsSettings.vignette_enabled;
            else if(settingsSel == OPT_CHROMATIC) graphicsSettings.chromatic_enabled = !graphicsSettings.chromatic_enabled;
            else if(settingsSel == OPT_COPPER_BARS) graphicsSettings.copper_bars_enabled = !graphicsSettings.copper_bars_enabled;
            else if(settingsSel == OPT_CONTROLS)
                openControlsSubmenu(state, controlsProfile, controlsSel, controlsScroll,
                                    controlsCapturing, controlsCaptureAction);
            else if(settingsSel == OPT_SAVE) saveSettings(cfgPath);
            else if(settingsSel == OPT_LOAD){
                int oldScale = graphicsSettings.window_scale;
                bool loaded = loadSettings(cfgPath);
                if(loaded && graphicsSettings.window_scale != oldScale) applyGraphicsSettings();
            }
            break;
        case Action::UiBack:
            state = GameState::MENU;
            break;
        case Action::UtilSave:
            saveSettings(cfgPath);
            break;
        case Action::UtilLoad:{
            int oldScale = graphicsSettings.window_scale;
            bool loaded = loadSettings(cfgPath);
            if(loaded && graphicsSettings.window_scale != oldScale) applyGraphicsSettings();
            break;
        }
        default:
            break;
    }
    return true;
}

bool handleControlsKeyPressed(const sf::Event::KeyPressed& kp,
                              GameState& state,
                              int& controlsProfile,
                              int& controlsSel,
                              int& controlsScroll,
                              bool& controlsCapturing,
                              int& controlsCaptureAction,
                              ControllerSettings& controllers,
                              const std::string& cfgPath,
                              const std::function<void(const std::string&)>& saveSettings,
                              const std::function<bool(const std::string&)>& loadSettings)
{
    constexpr int kVisibleRows = 18;
    const Action a = resolveAction(controllers.profiles.keyboard(), GameState::CONTROLS, kp.scancode);
    const auto pid = static_cast<controls::ProfileId>(controlsProfile);
    const int rowCount = static_cast<int>(controls::controlsRowCount(pid));
    switch(a){
        case Action::UiUp:
            controlsSel = (controlsSel + rowCount - 1) % rowCount;
            if(controlsSel < controlsScroll) controlsScroll = controlsSel;
            break;
        case Action::UiDown:
            controlsSel = (controlsSel + 1) % rowCount;
            if(controlsSel >= controlsScroll + kVisibleRows)
                controlsScroll = controlsSel - kVisibleRows + 1;
            break;
        case Action::UiLeft:
        case Action::UiRight:
            if(controls::controlsRowIsProfile(controlsSel)){
                const int delta = (a == Action::UiLeft) ? -1 : 1;
                controlsProfile = (static_cast<int>(controls::ProfileId::Count) +
                                   controlsProfile + delta) %
                                  static_cast<int>(controls::ProfileId::Count);
                controlsSel = 0;
                controlsScroll = 0;
                controlsCapturing = false;
            }
            break;
        case Action::UiConfirm:
            if(controls::controlsRowIsRestore(controlsSel, pid)){
                controllers.profiles.profile(pid) = controls::defaultProfile(pid);
            } else if(controls::controlsRowIsBack(controlsSel, pid)){
                state = GameState::SETTINGS;
            } else {
                const Action action = controls::controlsRowAction(controlsSel, pid);
                if(action != Action::None){
                    controlsCapturing = true;
                    controlsCaptureAction = static_cast<int>(action);
                }
            }
            break;
        case Action::UiBack:
            state = GameState::SETTINGS;
            break;
        case Action::UtilSave:
            saveSettings(cfgPath);
            break;
        case Action::UtilLoad:
            loadSettings(cfgPath);
            break;
        default:
            break;
    }
    return true;
}

bool handleDonateKeyPressed(const sf::Event::KeyPressed& kp,
                            GameState& state,
                            float& donateMsgTimer,
                            int& donateSel,
                            const controls::Profile& keyboard)
{
    const Action a = resolveAction(keyboard, GameState::DONATE, kp.scancode);
    if(a == Action::UiUp || a == Action::UiDown)
        donateSel = (donateSel + 1) % 2;
    if(a == Action::UiConfirm && donateSel == 0){
        sf::Clipboard::setString("https://buymeacoffee.com/ojnen");
        donateMsgTimer = 2.0f;
    }
    if(a == Action::UiBack) state = GameState::MENU;
    if(kp.scancode == sf::Keyboard::Scan::C){
        sf::Clipboard::setString("https://buymeacoffee.com/ojnen");
        donateMsgTimer = 2.0f;
    }
    return true;
}

void handleStateTransitionKeyPressed(const sf::Event::KeyPressed& kp,
                                     GameState state,
                                     int& menuSel,
                                     const controls::Profile& keyboard,
                                     const std::function<void()>& startCountdownRound,
                                     const std::function<void()>& openSettings,
                                     const std::function<void()>& openDonate,
                                     const std::function<void()>& pauseGameplay,
                                     const std::function<void()>& resetPlayingRound,
                                     const std::function<void()>& resumeGameplay,
                                     const std::function<void()>& rematchCountdownRound)
{
    if(state == GameState::MENU){
        const Action a = resolveAction(keyboard, GameState::MENU, kp.scancode);
        if(a == Action::UiLeft)
            menuSel = (menuSel + 2) % 3;
        if(a == Action::UiRight)
            menuSel = (menuSel + 1) % 3;
        if(a == Action::UiConfirm){
            if(menuSel == 0) startCountdownRound();
            else if(menuSel == 1) openSettings();
            else if(menuSel == 2) openDonate();
        }
        if(a == Action::UtilSettings) openSettings();
        if(a == Action::UtilDonate) openDonate();
        return;
    }

    if(state == GameState::PLAYING){
        const Action a = resolveAction(keyboard, GameState::PLAYING, kp.scancode);
        if(a == Action::GamePause) pauseGameplay();
        if(a == Action::GameReset) resetPlayingRound();
        return;
    }

    if(state == GameState::PAUSED){
        const Action a = resolveAction(keyboard, GameState::PAUSED, kp.scancode);
        if(a == Action::GamePause) resumeGameplay();
        return;
    }

    if(state == GameState::GAME_OVER){
        const Action a = resolveAction(keyboard, GameState::GAME_OVER, kp.scancode);
        if(a == Action::UiConfirm) rematchCountdownRound();
    }
}

void handleRuntimeToggleKeyPressed(const sf::Event::KeyPressed& kp,
                                   const controls::Profile& keyboard,
                                   bool& botDebug,
                                   bool& botEnabled,
                                   BotDifficulty& botDifficulty,
                                   PerfLevel& perfLevel)
{
    const int sc = static_cast<int>(kp.scancode);
    if(keyboard.actions[static_cast<std::size_t>(Action::UtilBotDebug)].containsKey(sc)){
        botDebug = !botDebug;
        std::fprintf(stderr, "BOT_DEBUG %s\n", botDebug ? "ON" : "OFF");
    }
    if(keyboard.actions[static_cast<std::size_t>(Action::UtilBot)].containsKey(sc)){
        botEnabled = !botEnabled;
        std::fprintf(stderr, "Bot %s\n", botEnabled ? "enabled" : "disabled");
    }
    if(keyboard.actions[static_cast<std::size_t>(Action::UtilBotDiff)].containsKey(sc)){
        int next = (static_cast<int>(botDifficulty) + 1) % 3;
        botDifficulty = static_cast<BotDifficulty>(next);
        const char* names[] = {"EASY", "MEDIUM", "HARD"};
        std::fprintf(stderr, "Bot difficulty: %s\n", names[next]);
    }
    if(keyboard.actions[static_cast<std::size_t>(Action::UtilPerf)].containsKey(sc)){
        int next = (static_cast<int>(perfLevel) + 1) % 4;
        perfLevel = static_cast<PerfLevel>(next);
        const char* names[] = {"HIGH", "MED", "LOW", "ULTRA"};
        std::fprintf(stderr, "Perf level: %s\n", names[next]);
    }
}

void handleGlobalControlKeyPressed(const sf::Event::KeyPressed& kp,
                                   GameState& state,
                                   const controls::Profile& keyboard,
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
    const int sc = static_cast<int>(kp.scancode);
    const auto binds = [&](Action a){
        return keyboard.actions[static_cast<std::size_t>(a)].containsKey(sc);
    };

    if(binds(Action::UiBack)){
        if(state == GameState::MENU) win.close();
        else {
            state = GameState::MENU;
            playMenuMusic();
        }
    }

    // "Open menu" (usually the east pad button / Escape while playing) returns
    // to the menu from any active screen.
    if(binds(Action::GameMenu)){
        if(state != GameState::MENU){
            state = GameState::MENU;
            playMenuMusic();
        }
    }

    if(binds(Action::UtilMute)){
        muted = !muted;
        applyAllMusicSettings();
    }

    if(binds(Action::UtilVolDown)){
        musicVolume = std::max(0.f, musicVolume - 5.f);
        sfxVolume = std::max(0.f, sfxVolume - 5.f);
        applyAllMusicSettings();
    }

    if(binds(Action::UtilVolUp)){
        musicVolume = std::min(100.f, musicVolume + 5.f);
        sfxVolume = std::min(100.f, sfxVolume + 5.f);
        applyAllMusicSettings();
    }

    if(binds(Action::UtilSave)) saveSettings(assetsDir + "/settings.cfg");

    if(binds(Action::UtilFullscreen)){
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
    const controls::Profile& kb = context.controllers->profiles.keyboard();

    if(state == GameState::CONTROLS){
        if(context.controlsCapturing && *context.controlsCapturing){
            handleControlsCaptureKey(kp, context);
            return;
        }
        handleControlsKeyPressed(kp, state,
                                *context.controlsProfile,
                                *context.controlsSel,
                                *context.controlsScroll,
                                *context.controlsCapturing,
                                *context.controlsCaptureAction,
                                *context.controllers,
                                *context.cfgPath,
                                context.saveSettings,
                                context.loadSettings);
        return;
    }

    setMovementKeyPressed(kp.scancode, *context.input, kb);

    if(state == GameState::MATCH_SETUP){
        handleMatchSetupKeyPressed(kp, state, *context.matchSetup, kb, context.startConfiguredMatch);
        return;
    }

    if(state == GameState::SETTINGS){
        float oldMusicVol = *context.musicVolume;
        float oldSfxVol = *context.sfxVolume;
        bool oldMuted = *context.muted;
        handleSettingsKeyPressed(kp,
                                 state,
                                 *context.settingsSel,
                                 *context.cfg,
                                 *context.botEnabled,
                                 *context.botDifficulty,
                                 *context.musicVolume,
                                 *context.sfxVolume,
                                 *context.graphicsSettings,
                                 *context.controllers,
                                 *context.controlsProfile,
                                 *context.controlsSel,
                                 *context.controlsScroll,
                                 *context.controlsCapturing,
                                 *context.controlsCaptureAction,
                                 *context.cfgPath,
                                 context.saveSettings,
                                 context.loadSettings,
                                 context.applyGraphicsSettings);
        if(*context.musicVolume != oldMusicVol || *context.sfxVolume != oldSfxVol || *context.muted != oldMuted)
            context.applyAllMusicSettings();
        return;
    }

    if(state == GameState::DONATE){
        handleDonateKeyPressed(kp, state, *context.donateMsgTimer, *context.donateSel, kb);
        return;
    }

    handleRuntimeToggleKeyPressed(kp, kb, *context.botDebug, *context.botEnabled,
                                 *context.botDifficulty, *context.perfLevel);
    handleGlobalControlKeyPressed(kp,
                                  state,
                                  kb,
                                  *context.muted,
                                  *context.musicVolume,
                                  *context.sfxVolume,
                                  *context.isFullscreen,
                                  *context.graphicsSettings,
                                  *context.input,
                                  win,
                                  *context.assetsDir,
                                  context.playMenuMusic,
                                  context.applyAllMusicSettings,
                                  context.saveSettings);
    handleStateTransitionKeyPressed(kp,
                                    state,
                                    *context.menuSel,
                                    kb,
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
    const controls::Profile& kb = context.controllers->profiles.keyboard();
    processEvents(
        win,
        [&]{ win.close(); },
        [&]{ game_update_runtime::setFocus(input, false); },
        [&]{ game_update_runtime::setFocus(input, true); },
        [&](const sf::Event::KeyPressed& kp){
            handleKeyPressed(kp, win, context);
        },
        [&](const sf::Event::KeyReleased& kr){
            setMovementKeyReleased(kr.scancode, input, kb);
        });

    updateMatchSetupControllers(context);
    updateControllerActions(win, context);

    if(*context.state == GameState::PLAYING){
        game_update_runtime::syncPlayingKeyboard(input, kb);
        game_update_runtime::syncPlayingControllers(input, *context.controllers);
    }
}

} // namespace input_runtime
