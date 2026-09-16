#pragma once

#include "GameTypes.h"
#include "Random.h"

#include <array>

namespace effects_runtime {

struct Context {
    PerfLevel perfLevel = PerfLevel::MEDIUM;
    int fxLevel = 0;
    int* particleSpawnBudget = nullptr;
    int* particleSpawnsThisFrame = nullptr;
};

void spawnExplosion(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                    float x, float y, int colorHue);
void spawnShipDisintegration(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                             float x, float y, int victimId);
void spawnSpark(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                float x, float y);
void spawnTrail(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                float x, float y, int owner);
void spawnThruster(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                   float x, float y, int id);
void spawnHitSpark(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                   float x, float y, int hitOwner);
void spawnFragFloat(std::array<FragFloat, MAX_FRAG_FLOATS>& ff,
                    float x, float y, int scorer);

} // namespace effects_runtime
