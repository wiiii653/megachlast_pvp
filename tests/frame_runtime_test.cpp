#include "FrameRuntime.h"

#include <iostream>

namespace {

int failures = 0;

void check(bool ok, const char* msg)
{
    if(!ok){
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    }
}

void runUpdate(GameState state, int& menuPlayCount)
{
    float countdownTimer = 0.f;
    float fightFlashTimer = 0.f;
    float shakeTimer = 0.f;
    float shakeDuration = 0.f;
    float shakeIntensity = 0.f;
    Player p1{};
    Player p2{};
    sf::Music menu;
    sf::Music ingame;
    sf::Music getReady;

    frame_runtime::UpdateContext context{};
    context.state = &state;
    context.countdownTimer = &countdownTimer;
    context.fightFlashTimer = &fightFlashTimer;
    context.dt = 1.f / 60.f;
    context.haveMusic = true;
    context.music = &menu;
    context.haveIngameMusic = false;
    context.ingameMusic = &ingame;
    context.haveGetReady = false;
    context.getReady = &getReady;
    context.shakeTimer = &shakeTimer;
    context.shakeDuration = &shakeDuration;
    context.shakeIntensity = &shakeIntensity;
    context.p1 = &p1;
    context.p2 = &p2;
    context.playMenuMusic = [&]{ ++menuPlayCount; };
    context.playIngameMusic = []{};

    frame_runtime::updateStateAndTimers(context);
}

} // namespace

int main()
{
    int menuPlayCount = 0;
    runUpdate(GameState::MENU, menuPlayCount);
    check(menuPlayCount == 1, "menu state restarts stopped menu music");

    runUpdate(GameState::SETTINGS, menuPlayCount);
    check(menuPlayCount == 2, "settings state restarts stopped menu music");

    runUpdate(GameState::PLAYING, menuPlayCount);
    check(menuPlayCount == 2, "playing state does not restart menu music");

    return failures == 0 ? 0 : 1;
}
