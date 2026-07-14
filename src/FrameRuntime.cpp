#include "FrameRuntime.h"

#include <cmath>

namespace frame_runtime {

void enforceSingleTrack(GameState state,
                        bool haveMusic,
                        sf::Music& music,
                        bool haveIngameMusic,
                        sf::Music& ingameMusic,
                        bool haveGetReady,
                        sf::Music& getReady,
                        const std::function<void()>& playMenuMusic)
{
    if(state == GameState::MENU || state == GameState::SETTINGS || state == GameState::MATCH_SETUP){
        if(haveIngameMusic && ingameMusic.getStatus() != sf::Music::Status::Stopped){
            ingameMusic.stop();
            ingameMusic.setVolume(0);
        }
        if(haveGetReady && getReady.getStatus() != sf::Music::Status::Stopped){
            getReady.stop();
            getReady.setVolume(0);
            getReady.setLooping(false);
        }
        if(haveMusic && music.getStatus() == sf::Music::Status::Stopped && playMenuMusic)
            playMenuMusic();
    }

    if(state == GameState::PLAYING || state == GameState::PAUSED ||
       state == GameState::COUNTDOWN || state == GameState::GAME_OVER){
        if(haveMusic && music.getStatus() != sf::Music::Status::Stopped){
            music.stop();
            music.setVolume(0);
        }
    }
}

void updateStateAndTimers(UpdateContext& context)
{
    enforceSingleTrack(*context.state,
                       context.haveMusic,
                       *context.music,
                       context.haveIngameMusic,
                       *context.ingameMusic,
                       context.haveGetReady,
                       *context.getReady,
                       context.playMenuMusic);
    updateCountdownAndFightFlash(*context.state,
                                 *context.countdownTimer,
                                 *context.fightFlashTimer,
                                 context.dt,
                                 context.playIngameMusic);
    updateScreenShake(context.dt,
                      *context.shakeTimer,
                      *context.shakeDuration,
                      *context.shakeIntensity);
    updatePlayerTimers(*context.state, *context.p1, *context.p2, context.dt);
}

void updateCountdownAndFightFlash(GameState& state,
                                  float& countdownTimer,
                                  float& fightFlashTimer,
                                  float dt,
                                  const std::function<void()>& playIngameMusic)
{
    if(state == GameState::COUNTDOWN){
        countdownTimer -= dt;
        if(countdownTimer <= 0.f){
            state = GameState::PLAYING;
            fightFlashTimer = 0.9f;
            playIngameMusic();
        }
    }
    if(fightFlashTimer > 0.f) fightFlashTimer -= dt;
}

void updateScreenShake(float dt,
                       float& shakeTimer,
                       float& shakeDuration,
                       float& shakeIntensity)
{
    if(shakeTimer > 0.f){
        shakeTimer -= dt;
        if(shakeTimer < 0.f) shakeTimer = 0.f;
    } else {
        shakeDuration = 0.f;
        shakeIntensity = 0.f;
    }
}

void updateMusicDuck(float dt,
                     float& duckApplyAcc,
                     float& lastDuckGain,
                     const std::function<float()>& currentDuckGain,
                     const std::function<void()>& applyActiveMusicSettings)
{
    duckApplyAcc += dt;
    float duckGain = currentDuckGain();
    if(duckApplyAcc >= 0.04f && std::fabs(duckGain - lastDuckGain) > 0.02f){
        applyActiveMusicSettings();
        lastDuckGain = duckGain;
        duckApplyAcc = 0.f;
    }
}

void updateMenuBlinkAndPhase(float dt,
                             float& menuAnim,
                             float& blinkTimer,
                             bool& showBlink,
                             Player& p1,
                             Player& p2,
                             std::array<Mirror, MIRROR_PAIRS*2>& mirrors,
                             std::array<BarrierBrick, BARRIER_BRICKS*2>& barriers)
{
    menuAnim += dt;
    blinkTimer += dt;
    if(blinkTimer >= 0.5f){
        blinkTimer = 0.f;
        showBlink = !showBlink;
    }

    p1.glowPhase = std::fmod(p1.glowPhase + dt * 3.5f, 2.f * PI);
    p2.glowPhase = std::fmod(p2.glowPhase + dt * 3.5f, 2.f * PI);
    for(auto& m : mirrors) if(m.hitFlash > 0.f) m.hitFlash -= dt * 4.f;
    for(auto& bk : barriers) if(bk.hitFlash > 0.f) bk.hitFlash -= dt * 5.f;
}

void updatePlayerTimers(GameState state, Player& p1, Player& p2, float dt)
{
    if(p1.flashTimer > 0.f) p1.flashTimer -= dt;
    if(p2.flashTimer > 0.f) p2.flashTimer -= dt;

    if(state != GameState::PLAYING) return;

    if(p1.invulnTimer > 0.f) p1.invulnTimer -= dt;
    if(p2.invulnTimer > 0.f) p2.invulnTimer -= dt;

    if(p1.shieldTimer > 0.f) p1.shieldTimer -= dt;
    if(p1.rapidTimer > 0.f) p1.rapidTimer -= dt;
    if(p1.spreadTimer > 0.f) p1.spreadTimer -= dt;
    if(p1.slowTimer > 0.f) p1.slowTimer -= dt;
    if(p1.reverseTimer > 0.f) p1.reverseTimer -= dt;
    if(p1.overdriveTimer > 0.f) p1.overdriveTimer -= dt;

    if(p2.shieldTimer > 0.f) p2.shieldTimer -= dt;
    if(p2.rapidTimer > 0.f) p2.rapidTimer -= dt;
    if(p2.spreadTimer > 0.f) p2.spreadTimer -= dt;
    if(p2.slowTimer > 0.f) p2.slowTimer -= dt;
    if(p2.reverseTimer > 0.f) p2.reverseTimer -= dt;
    if(p2.overdriveTimer > 0.f) p2.overdriveTimer -= dt;
}

} // namespace frame_runtime
