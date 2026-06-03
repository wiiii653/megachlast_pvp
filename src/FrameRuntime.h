#pragma once

#include "GameConstants.h"
#include "GameTypes.h"

#include <SFML/Audio/Music.hpp>

#include <array>
#include <functional>

namespace frame_runtime {

struct UpdateContext {
    GameState* state = nullptr;
    float* countdownTimer = nullptr;
    float* fightFlashTimer = nullptr;
    float dt = 0.f;
    bool haveMusic = false;
    sf::Music* music = nullptr;
    bool haveIngameMusic = false;
    sf::Music* ingameMusic = nullptr;
    bool haveGetReady = false;
    sf::Music* getReady = nullptr;
    float* shakeTimer = nullptr;
    float* shakeDuration = nullptr;
    float* shakeIntensity = nullptr;
    Player* p1 = nullptr;
    Player* p2 = nullptr;
    std::function<void()> playMenuMusic;
    std::function<void()> playIngameMusic;
};

void enforceSingleTrack(GameState state,
                        bool haveMusic,
                        sf::Music& music,
                        bool haveIngameMusic,
                        sf::Music& ingameMusic,
                        bool haveGetReady,
                        sf::Music& getReady,
                        const std::function<void()>& playMenuMusic);

void updateStateAndTimers(UpdateContext& context);

void updateCountdownAndFightFlash(GameState& state,
                                  float& countdownTimer,
                                  float& fightFlashTimer,
                                  float dt,
                                  const std::function<void()>& playIngameMusic);

void updateScreenShake(float dt,
                       float& shakeTimer,
                       float& shakeDuration,
                       float& shakeIntensity);

void updateMusicDuck(float dt,
                     float& duckApplyAcc,
                     float& lastDuckGain,
                     const std::function<float()>& currentDuckGain,
                     const std::function<void()>& applyActiveMusicSettings);

void updateMenuBlinkAndPhase(float dt,
                             float& menuAnim,
                             float& blinkTimer,
                             bool& showBlink,
                             Player& p1,
                             Player& p2,
                             std::array<Mirror, MIRROR_PAIRS*2>& mirrors,
                             std::array<BarrierBrick, BARRIER_BRICKS*2>& barriers);

void updatePlayerTimers(GameState state, Player& p1, Player& p2, float dt);

} // namespace frame_runtime
