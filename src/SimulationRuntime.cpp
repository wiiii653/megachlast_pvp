#include "SimulationRuntime.h"

#include "ArenaLayout.h"
#include "GameConstants.h"
#include "Particles.h"
#include "ProceduralSynth.h"

namespace simulation_runtime {

projectile_runtime::Hooks buildProjectileHooks(
    void (*spawnExplosion)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int),
    void (*spawnShipDisintegration)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int),
    void (*spawnSpark)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float),
    void (*spawnHitSpark)(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int),
    void (*spawnFragFloat)(std::array<FragFloat, MAX_FRAG_FLOATS>&, float, float, int),
    void (*triggerMusicDuck)(float, float),
    void (*triggerScreenShake)(float, float),
    const std::function<void(Player&, int)>& applyFragTransition)
{
    projectile_runtime::Hooks hooks{};
    hooks.spawnExplosion = spawnExplosion;
    hooks.spawnShipDisintegration = spawnShipDisintegration;
    hooks.spawnSpark = spawnSpark;
    hooks.spawnHitSpark = spawnHitSpark;
    hooks.spawnFragFloat = spawnFragFloat;
    hooks.triggerMusicDuck = triggerMusicDuck;
    hooks.triggerScreenShake = triggerScreenShake;
    hooks.applyFragTransition = applyFragTransition;
    return hooks;
}

void simulatePlayingFrame(FrameContext& context)
{
    if(context.state != GameState::PLAYING) return;

    game_update_runtime::updateMovementAndFiring(*context.input,
                                                 *context.p1,
                                                 *context.p2,
                                                 *context.bullets,
                                                 *context.powerups,
                                                 *context.bombs,
                                                 *context.particles,
                                                 *context.rng,
                                                 *context.cfg,
                                                 context.botEnabled,
                                                 context.botDifficulty,
                                                 *context.botOverrides,
                                                 *context.botRuntime,
                                                 *context.synth,
                                                 context.muted,
                                                 context.sfxVolume,
                                                 *context.cd1,
                                                 *context.cd2,
                                                 context.dt,
                                                 context.fireFn,
                                                 context.spawnThrusterFn);

    *context.powerupSpawnTimer -= context.dt;
    if(*context.powerupSpawnTimer <= 0.f){
        arena_layout::spawnPowerUp(*context.powerups, *context.rng);
        *context.powerupSpawnTimer = POWERUP_SPAWN_INTERVAL;
    }
    arena_layout::updatePowerUps(*context.powerups, context.dt);
    arena_layout::updateSpecialStars(*context.specialStars, context.dt);

    projectile_runtime::simulateProjectilesAndCollisions(*context.bullets,
                                                         *context.p1,
                                                         *context.p2,
                                                         *context.powerups,
                                                         *context.bombs,
                                                         *context.mirrors,
                                                         *context.barriers,
                                                         *context.specialStars,
                                                         *context.particles,
                                                         *context.fragFloats,
                                                         *context.rng,
                                                         *context.cfg,
                                                         *context.synth,
                                                         context.muted,
                                                         context.sfxVolume,
                                                         *context.projectileHooks,
                                                         context.dt);
}

void updateParticlesAndTrails(ParticleFrameContext& context)
{
    for(const auto& b : *context.bullets)
        if(b.alive) context.spawnTrailFn(*context.particles, *context.rng, b.x, b.y, b.owner);

    for(auto& f : *context.fragFloats)
        if(f.alive){
            f.y -= 25.f * context.dt;
            f.ttl -= context.dt;
            if(f.ttl <= 0.f) f.alive = false;
        }

    particles::update(*context.particles, context.dt);
}

} // namespace simulation_runtime
