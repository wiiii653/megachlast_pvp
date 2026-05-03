#include "InputRuntime.h"

#include "GameConstants.h"

#include <SFML/Window/Clipboard.hpp>

#include <algorithm>
#include <cstdio>

namespace input_runtime {

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

bool handleSettingsKeyPressed(const sf::Event::KeyPressed& kp,
                              GameState& state,
                              int& settingsSel,
                              Config& cfg,
                              bool& botEnabled,
                              BotDifficulty& botDifficulty,
                              float& musicVolume,
                              float& sfxVolume,
                              const std::string& cfgPath,
                              const std::function<void(const std::string&)>& saveSettings,
                              const std::function<bool(const std::string&)>& loadSettings)
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
            break;
        case sf::Keyboard::Scan::Enter:
            if(settingsSel == OPT_BOT_ENABLED) botEnabled = !botEnabled;
            else if(settingsSel == OPT_SAVE) saveSettings(cfgPath);
            else if(settingsSel == OPT_LOAD) loadSettings(cfgPath);
            break;
        default:
            break;
    }

    if(kp.code == sf::Keyboard::Key::K) saveSettings(cfgPath);
    if(kp.code == sf::Keyboard::Key::L) loadSettings(cfgPath);
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
                                     const std::function<void()>& startCountdownRound,
                                     const std::function<void()>& openSettings,
                                     const std::function<void()>& openDonate,
                                     const std::function<void()>& pauseGameplay,
                                     const std::function<void()>& resetPlayingRound,
                                     const std::function<void()>& resumeGameplay,
                                     const std::function<void()>& rematchCountdownRound)
{
    if(state == GameState::MENU){
        if(kp.scancode == sf::Keyboard::Scan::Enter) startCountdownRound();
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
                static_cast<unsigned int>(W * SCALE),
                static_cast<unsigned int>(H * SCALE)}),
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
    float& donateMsgTimer = *context.donateMsgTimer;
    bool& botDebug = *context.botDebug;
    PerfLevel& perfLevel = *context.perfLevel;
    bool& muted = *context.muted;
    bool& isFullscreen = *context.isFullscreen;
    game_update_runtime::InputState& input = *context.input;

    if(state == GameState::SETTINGS){
        float oldMusicVol = musicVolume;
        handleSettingsKeyPressed(kp,
                                 state,
                                 settingsSel,
                                 cfg,
                                 botEnabled,
                                 botDifficulty,
                                 musicVolume,
                                 sfxVolume,
                                 *context.cfgPath,
                                 context.saveSettings,
                                 context.loadSettings);
        if(musicVolume != oldMusicVol) context.applyAllMusicSettings();
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
                                  input,
                                  win,
                                  *context.assetsDir,
                                  context.playMenuMusic,
                                  context.applyAllMusicSettings,
                                  context.saveSettings);
    handleStateTransitionKeyPressed(kp,
                                    state,
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
                   const std::function<void(const sf::Event::KeyPressed&)>& onKeyPressed,
                   const std::function<void(const sf::Event::KeyReleased&)>& onKeyReleased)
{
    while(true){
        auto oe = win.pollEvent();
        if(!oe.has_value()) break;
        const auto& e = *oe;

        if(e.is<sf::Event::Closed>()) onClosed();
        if(e.is<sf::Event::FocusLost>()) onFocusLost();
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
        [&]{ input = {}; },
        [&](const sf::Event::KeyPressed& kp){
            handleKeyPressed(kp, win, context);
        },
        [&](const sf::Event::KeyReleased& kr){
            setMovementKeyReleased(kr.scancode, input);
        });

    if(*context.state == GameState::PLAYING) game_update_runtime::syncPlayingKeyboard(input);
}

} // namespace input_runtime
