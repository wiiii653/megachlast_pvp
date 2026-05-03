#include "FxRuntime.h"

#include <algorithm>
#include <cmath>

namespace fx_runtime {
namespace {

int spawnCount(PerfLevel perfLevel, int high, int med, int low, int ultra)
{
    switch(perfLevel){
        case PerfLevel::HIGH: return high;
        case PerfLevel::MEDIUM: return med;
        case PerfLevel::LOW: return low;
        case PerfLevel::ULTRA: return ultra;
    }
    return med;
}

} // namespace

float currentMusicDuckGain(const audio_duck::State& state)
{
    return audio_duck::gain(state);
}

void triggerMusicDuck(audio_duck::State& state, float depth, float duration)
{
    audio_duck::trigger(state, depth, duration, 0.22f);
}

void triggerScreenShake(float duration,
                        float intensity,
                        float& shakeTimer,
                        float& shakeDuration,
                        float& shakeIntensity)
{
    shakeTimer = std::max(shakeTimer, duration);
    if(duration > 0.f)
        shakeDuration = std::max(shakeDuration, duration);
    shakeIntensity = std::max(shakeIntensity, intensity);
}

void updateGovernor(float dt,
                    GameState state,
                    fx_governor::State& governorState,
                    int& particleSpawnBudget,
                    int& particleSpawnsThisFrame,
                    int& fxLevel)
{
    fx_governor::update(governorState, dt, state == GameState::PLAYING,
                        particleSpawnBudget, particleSpawnsThisFrame);
    fxLevel = governorState.level;
}

void beginParticleSpawnFrame(PerfLevel perfLevel,
                             int fxLevel,
                             int& particleSpawnBudget,
                             int& particleSpawnsThisFrame)
{
    float base = static_cast<float>(spawnCount(perfLevel, 220, 140, 80, 36));
    float scale = fx_governor::levelScale(fxLevel);
    particleSpawnBudget = std::max(16, static_cast<int>(std::lround(base * scale)));
    particleSpawnsThisFrame = 0;
}

} // namespace fx_runtime
