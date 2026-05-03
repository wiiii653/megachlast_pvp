#pragma once

#include "GameTypes.h"
#include "Random.h"

#include <array>

namespace effects_runtime {

void setFrameContext(PerfLevel perfLevel,
                     int fxLevel,
                     int& particleSpawnBudget,
                     int& particleSpawnsThisFrame);

void spawnExplosion(std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                    float x, float y, int colorHue);
void spawnShipDisintegration(std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                             float x, float y, int victimId);
void spawnSpark(std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                float x, float y);
void spawnTrail(std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                float x, float y, int owner);
void spawnThruster(std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                   float x, float y, int id);
void spawnHitSpark(std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                   float x, float y, int hitOwner);
void spawnFragFloat(std::array<FragFloat, MAX_FRAG_FLOATS>& ff,
                    float x, float y, int scorer);

} // namespace effects_runtime
