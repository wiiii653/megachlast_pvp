#pragma once

#include "GameTypes.h"
#include "Random.h"

#include <array>
#include <functional>

class ProceduralSynth;

namespace projectile_runtime {

struct Hooks {
    void (*spawnExplosion)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int) = nullptr;
    void (*spawnShipDisintegration)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int) = nullptr;
    void (*spawnSpark)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float) = nullptr;
    void (*spawnHitSpark)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int) = nullptr;
    void (*spawnFragFloat)(std::array<FragFloat, MAX_FRAG_FLOATS>&, float, float, int) = nullptr;
    void (*triggerMusicDuck)(float, float) = nullptr;
    void (*triggerScreenShake)(float, float) = nullptr;
    std::function<void(Player&, int)> applyFragTransition;
};

void simulateProjectilesAndCollisions(std::array<Bullet, MAX_BULLETS>& bullets,
                                      Player& p1,
                                      Player& p2,
                                      std::array<PowerUp, MAX_POWERUPS>& powerups,
                                      std::array<Bomb, MAX_BOMBS>& bombs,
                                      std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                                      std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers,
                                      std::array<SpecialStar, MAX_SPECIAL_STARS>& specialStars,
                                      std::array<Particle, MAX_PARTICLES>& particles,
                                      std::array<FragFloat, MAX_FRAG_FLOATS>& fragFloats,
                                      RNG& rng,
                                      const Config& cfg,
                                      ProceduralSynth& synth,
                                      bool muted,
                                      float sfxVolume,
                                      const Hooks& hooks,
                                      float dt);

} // namespace projectile_runtime
