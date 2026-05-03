#pragma once

#include "AudioDuck.h"
#include "FxGovernor.h"
#include "GameTypes.h"

namespace fx_runtime {

float currentMusicDuckGain(const audio_duck::State& state);
void triggerMusicDuck(audio_duck::State& state, float depth, float duration);

void triggerScreenShake(float duration,
                        float intensity,
                        float& shakeTimer,
                        float& shakeDuration,
                        float& shakeIntensity);

void updateGovernor(float dt,
                    GameState state,
                    fx_governor::State& governorState,
                    int& particleSpawnBudget,
                    int& particleSpawnsThisFrame,
                    int& fxLevel);

void beginParticleSpawnFrame(PerfLevel perfLevel,
                             int fxLevel,
                             int& particleSpawnBudget,
                             int& particleSpawnsThisFrame);

} // namespace fx_runtime
