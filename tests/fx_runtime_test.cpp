#include "FxRuntime.h"

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

} // namespace

int main()
{
    audio_duck::State duck{};
    check(fx_runtime::currentMusicDuckGain(duck) == 1.f, "default duck gain is 1");

    fx_runtime::triggerMusicDuck(duck, 0.4f, 0.2f);
    check(fx_runtime::currentMusicDuckGain(duck) < 1.f, "duck trigger lowers gain");

    float shakeTimer = 0.f;
    float shakeDuration = 0.f;
    float shakeIntensity = 0.f;
    fx_runtime::triggerScreenShake(0.3f, 5.f, shakeTimer, shakeDuration, shakeIntensity);
    check(shakeTimer == 0.3f && shakeDuration == 0.3f && shakeIntensity == 5.f,
          "screen shake trigger stores values");

    fx_governor::State governor{};
    int budget = 120;
    int used = 120;
    int fxLevel = 0;
    for(int i = 0; i < 200; ++i)
        fx_runtime::updateGovernor(1.f / 30.f, GameState::PLAYING, governor, budget, used, fxLevel);
    check(fxLevel >= 1 && fxLevel <= 3, "fx governor rises under sustained load");

    int spawnBudget = 0;
    int spawnUsed = 99;
    fx_runtime::beginParticleSpawnFrame(PerfLevel::ULTRA, 3, spawnBudget, spawnUsed);
    check(spawnBudget >= 16, "spawn frame computes minimum budget");
    check(spawnUsed == 0, "spawn frame resets used count");

    return failures == 0 ? 0 : 1;
}
