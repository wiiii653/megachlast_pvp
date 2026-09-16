#pragma once

#include "GameUpdateRuntime.h"
#include "GameTypes.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <functional>
#include <string>

namespace input_runtime {

enum SettingsOption {
    OPT_MUSIC = 0,
    OPT_SFX = 1,
    OPT_BOT_ENABLED = 2,
    OPT_BOT_DIFF = 3,
    OPT_TARGET_SCORE = 4,
    OPT_DAMAGE = 5,
    OPT_FIRE_CD_P1 = 6,
    OPT_FIRE_CD_P2 = 7,
    OPT_P_SPEED = 8,
    OPT_WINDOW_SCALE = 9,
    OPT_SCREEN_ASPECT = 10,
    OPT_POSTFX = 11,
    OPT_SCANLINES = 12,
    OPT_VIGNETTE = 13,
    OPT_CHROMATIC = 14,
    OPT_COPPER_BARS = 15,
    OPT_P1_CONTROLLER = 16,
    OPT_P2_CONTROLLER = 17,
    OPT_SAVE = 18,
    OPT_LOAD = 19,
    OPT_COUNT = 20,
};

void setMovementKeyPressed(sf::Keyboard::Scancode sc, game_update_runtime::InputState& input);
void setMovementKeyReleased(sf::Keyboard::Scancode sc, game_update_runtime::InputState& input);

bool handleMatchSetupKeyPressed(const sf::Event::KeyPressed& kp,
                                GameState& state,
                                MatchSetup& setup,
                                const std::function<void()>& startMatch);

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
                              const std::function<void()>& applyGraphicsSettings);

bool handleDonateKeyPressed(const sf::Event::KeyPressed& kp,
                            GameState& state,
                            float& donateMsgTimer);

void handleStateTransitionKeyPressed(const sf::Event::KeyPressed& kp,
                                     GameState state,
                                     int& menuSel,
                                     const std::function<void()>& startCountdownRound,
                                     const std::function<void()>& openSettings,
                                     const std::function<void()>& openDonate,
                                     const std::function<void()>& pauseGameplay,
                                     const std::function<void()>& resetPlayingRound,
                                     const std::function<void()>& resumeGameplay,
                                     const std::function<void()>& rematchCountdownRound);

void handleRuntimeToggleKeyPressed(const sf::Event::KeyPressed& kp,
                                   bool& botDebug,
                                   bool& botEnabled,
                                   BotDifficulty& botDifficulty,
                                   PerfLevel& perfLevel);

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
                                   const std::function<void(const std::string&)>& saveSettings);

struct FrameContext {
    GameState* state = nullptr;
    int* menuSel = nullptr;
    int* settingsSel = nullptr;
    Config* cfg = nullptr;
    bool* botEnabled = nullptr;
    BotDifficulty* botDifficulty = nullptr;
    float* musicVolume = nullptr;
    float* sfxVolume = nullptr;
    GraphicsSettings* graphicsSettings = nullptr;
    ControllerSettings* controllers = nullptr;
    MatchSetup* matchSetup = nullptr;
    float* donateMsgTimer = nullptr;
    bool* botDebug = nullptr;
    PerfLevel* perfLevel = nullptr;
    bool* muted = nullptr;
    bool* isFullscreen = nullptr;
    game_update_runtime::InputState* input = nullptr;

    const std::string* assetsDir = nullptr;
    const std::string* cfgPath = nullptr;

    std::function<void()> playMenuMusic;
    std::function<void()> applyAllMusicSettings;
    std::function<void()> applyGraphicsSettings;
    std::function<void(const std::string&)> saveSettings;
    std::function<bool(const std::string&)> loadSettings;
    std::function<void()> startCountdownRound;
    std::function<void()> startConfiguredMatch;
    std::function<void()> openSettings;
    std::function<void()> openDonate;
    std::function<void()> pauseGameplay;
    std::function<void()> resetPlayingRound;
    std::function<void()> resumeGameplay;
    std::function<void()> rematchCountdownRound;
};

void processEvents(sf::RenderWindow& win,
                   const std::function<void()>& onClosed,
                   const std::function<void()>& onFocusLost,
                   const std::function<void()>& onFocusGained,
                   const std::function<void(const sf::Event::KeyPressed&)>& onKeyPressed,
                   const std::function<void(const sf::Event::KeyReleased&)>& onKeyReleased);

void updateInputForFrame(sf::RenderWindow& win, FrameContext& context);

} // namespace input_runtime
