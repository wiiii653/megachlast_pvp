#pragma once

#include "GameTypes.h"
#include "Random.h"

#include <array>
#include <functional>

class ProceduralSynth;

namespace projectile_runtime {

struct Hooks {
    std::function<void(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int)> spawnExplosion;
    std::function<void(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int)> spawnShipDisintegration;
    std::function<void(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float)> spawnSpark;
    std::function<void(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int)> spawnHitSpark;
    std::function<void(std::array<FragFloat, MAX_FRAG_FLOATS>&, float, float, int)> spawnFragFloat;
    std::function<void(float, float)> triggerMusicDuck;
    std::function<void(float, float)> triggerScreenShake;
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
