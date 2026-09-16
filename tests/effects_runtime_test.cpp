#include "EffectsRuntime.h"

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
    std::array<Particle, MAX_PARTICLES> particles{};
    std::array<FragFloat, MAX_FRAG_FLOATS> fragFloats{};
    RNG rng(0xCAFEu);

    int budget = 256;
    int used = 0;
    effects_runtime::Context context{PerfLevel::HIGH, 0, &budget, &used};

    effects_runtime::spawnTrail(context, particles, rng, 100.f, 100.f, 1);
    check(used == 1, "spawnTrail consumes one particle slot");

    bool anyAlive = false;
    for(const auto& p : particles) if(p.alive){ anyAlive = true; break; }
    check(anyAlive, "spawnTrail emits an alive particle");

    effects_runtime::spawnFragFloat(fragFloats, 50.f, 60.f, 2);
    check(fragFloats[0].alive, "spawnFragFloat activates first slot");
    check(fragFloats[0].scorer == 2, "spawnFragFloat stores scorer");

    std::array<Particle, MAX_PARTICLES> blockedParticles{};
    int zeroBudget = 0;
    int zeroUsed = 0;
    effects_runtime::Context blockedContext{PerfLevel::HIGH, 0, &zeroBudget, &zeroUsed};
    effects_runtime::spawnSpark(blockedContext, blockedParticles, rng, 10.f, 20.f);
    bool blockedAlive = false;
    for(const auto& p : blockedParticles) if(p.alive){ blockedAlive = true; break; }
    check(!blockedAlive, "zero budget blocks spark spawn");

    return failures == 0 ? 0 : 1;
}
