#pragma once

#include "BotConfig.h"
#include "BotController.h"
#include "GameUpdateRuntime.h"
#include "GameTypes.h"
#include "ProjectileRuntime.h"
#include "Random.h"

#include <array>
#include <functional>

class ProceduralSynth;

namespace simulation_runtime {

using SpawnTrailFn = std::function<void(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int)>;

struct FrameContext {
    GameState state = GameState::MENU;
    game_update_runtime::InputState* input = nullptr;
    Player* p1 = nullptr;
    Player* p2 = nullptr;
    std::array<Bullet, MAX_BULLETS>* bullets = nullptr;
    std::array<PowerUp, MAX_POWERUPS>* powerups = nullptr;
    std::array<Bomb, MAX_BOMBS>* bombs = nullptr;
    std::array<Mirror, MIRROR_PAIRS*2>* mirrors = nullptr;
    std::array<BarrierBrick, BARRIER_BRICKS*2>* barriers = nullptr;
    std::array<SpecialStar, MAX_SPECIAL_STARS>* specialStars = nullptr;
    std::array<Particle, MAX_PARTICLES>* particles = nullptr;
    std::array<FragFloat, MAX_FRAG_FLOATS>* fragFloats = nullptr;
    RNG* rng = nullptr;
    const Config* cfg = nullptr;
    bool botEnabled = false;
    BotDifficulty botDifficulty = BotDifficulty::MEDIUM;
    const BotTuningOverrides* botOverrides = nullptr;
    bot_controller::RuntimeState* botRuntime = nullptr;
    ProceduralSynth* synth = nullptr;
    bool muted = false;
    float sfxVolume = 0.f;
    int* cd1 = nullptr;
    int* cd2 = nullptr;
    float dt = 0.f;
    float* powerupSpawnTimer = nullptr;
    game_update_runtime::FireFn fireFn = nullptr;
    game_update_runtime::SpawnThrusterFn spawnThrusterFn = nullptr;
    const projectile_runtime::Hooks* projectileHooks = nullptr;
};

struct ParticleFrameContext {
    std::array<Bullet, MAX_BULLETS>* bullets = nullptr;
    std::array<Particle, MAX_PARTICLES>* particles = nullptr;
    std::array<FragFloat, MAX_FRAG_FLOATS>* fragFloats = nullptr;
    RNG* rng = nullptr;
    float dt = 0.f;
    SpawnTrailFn spawnTrailFn = nullptr;
};

projectile_runtime::Hooks buildProjectileHooks(
    decltype(projectile_runtime::Hooks::spawnExplosion) spawnExplosion,
    decltype(projectile_runtime::Hooks::spawnShipDisintegration) spawnShipDisintegration,
    decltype(projectile_runtime::Hooks::spawnSpark) spawnSpark,
    decltype(projectile_runtime::Hooks::spawnHitSpark) spawnHitSpark,
    decltype(projectile_runtime::Hooks::spawnFragFloat) spawnFragFloat,
    decltype(projectile_runtime::Hooks::triggerMusicDuck) triggerMusicDuck,
    decltype(projectile_runtime::Hooks::triggerScreenShake) triggerScreenShake,
    const std::function<void(Player&, int)>& applyFragTransition);

void simulatePlayingFrame(FrameContext& context);

void updateParticlesAndTrails(ParticleFrameContext& context);

} // namespace simulation_runtime
