#include "EffectsRuntime.h"

#include "FxGovernor.h"
#include "GameConstants.h"
#include "Particles.h"

#include <algorithm>
#include <cmath>

namespace effects_runtime {
namespace {

int spawnCount(const Context& context, int high, int med, int low, int ultra)
{
    switch(context.perfLevel){
        case PerfLevel::HIGH: return high;
        case PerfLevel::MEDIUM: return med;
        case PerfLevel::LOW: return low;
        case PerfLevel::ULTRA: return ultra;
    }
    return med;
}

float fxLevelScale(const Context& context)
{
    return fx_governor::levelScale(context.fxLevel);
}

int claimParticleSlots(Context& context, int wanted)
{
    if(!context.particleSpawnBudget || !context.particleSpawnsThisFrame) return std::max(0, wanted);
    int remaining = std::max(0, *context.particleSpawnBudget - *context.particleSpawnsThisFrame);
    int granted = std::max(0, std::min(wanted, remaining));
    *context.particleSpawnsThisFrame += granted;
    return granted;
}

} // namespace

void spawnExplosion(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                    float x, float y, int colorHue)
{
    int n = claimParticleSlots(context, spawnCount(context, 28, 18, 10, 5));
    for(int i = 0; i < n; i++) {
        Particle* p = particles::alloc(parts);
        float ang = rng.frand(0.f, 2.f * PI);
        float spd = rng.frand(25.f, 140.f);
        p->x = x + rng.frand(-3.f, 3.f);
        p->y = y + rng.frand(-3.f, 3.f);
        p->vx = std::cos(ang) * spd;
        p->vy = std::sin(ang) * spd;
        p->maxttl = rng.frand(0.5f, 1.1f);
        p->ttl = p->maxttl;
        p->size = rng.frand(1.5f, 3.5f);
        p->r = (colorHue == 1) ? 255 : 50;
        p->g = (colorHue == 1) ? 60 : 255;
        p->b = (colorHue == 1) ? 50 : 255;
        p->a = 255;
        p->alive = true;
    }
    int ring = claimParticleSlots(context, spawnCount(context, 8, 6, 4, 2));
    for(int i = 0; i < ring; i++){
        Particle* p = particles::alloc(parts);
        float ang = (2.f * PI * i) / ring;
        float spd = rng.frand(180.f, 280.f);
        p->x = x;
        p->y = y;
        p->vx = std::cos(ang) * spd;
        p->vy = std::sin(ang) * spd;
        p->maxttl = rng.frand(0.15f, 0.3f);
        p->ttl = p->maxttl;
        p->size = rng.frand(2.f, 4.f);
        p->r = 255;
        p->g = 240;
        p->b = 160;
        p->a = 255;
        p->alive = true;
    }
}

void spawnShipDisintegration(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                             float x, float y, int victimId)
{
    uint8_t baseR, baseG, baseB;
    if(victimId == 1){ baseR = 0; baseG = 220; baseB = 255; }
    else { baseR = 255; baseG = 60; baseB = 50; }

    int chunks = claimParticleSlots(context, spawnCount(context, 24, 16, 10, 6));
    for(int i = 0; i < chunks; i++){
        Particle* p = particles::alloc(parts);
        float ang = rng.frand(0.f, 2.f * PI);
        float spd = rng.frand(15.f, 100.f);
        p->x = x + rng.frand(-20.f, 20.f);
        p->y = y + rng.frand(-10.f, 10.f);
        p->vx = std::cos(ang) * spd + rng.frand(-20.f, 20.f);
        p->vy = std::sin(ang) * spd + rng.frand(-30.f, 30.f);
        p->maxttl = rng.frand(0.6f, 1.4f);
        p->ttl = p->maxttl;
        p->size = rng.frand(3.f, 7.f);
        float hot = rng.frand(0.f, 0.5f);
        p->r = static_cast<uint8_t>(std::min(255.f, baseR + hot * (255 - baseR)));
        p->g = static_cast<uint8_t>(std::min(255.f, baseG + hot * (255 - baseG)));
        p->b = static_cast<uint8_t>(std::min(255.f, baseB + hot * (255 - baseB)));
        p->a = 255;
        p->alive = true;
    }

    int flash = claimParticleSlots(context, spawnCount(context, 12, 8, 5, 3));
    for(int i = 0; i < flash; i++){
        Particle* p = particles::alloc(parts);
        float ang = rng.frand(0.f, 2.f * PI);
        float spd = rng.frand(5.f, 40.f);
        p->x = x + rng.frand(-4.f, 4.f);
        p->y = y + rng.frand(-4.f, 4.f);
        p->vx = std::cos(ang) * spd;
        p->vy = std::sin(ang) * spd;
        p->maxttl = rng.frand(0.08f, 0.22f);
        p->ttl = p->maxttl;
        p->size = rng.frand(5.f, 10.f);
        p->r = 255;
        p->g = 255;
        p->b = static_cast<uint8_t>(rng.irand(200, 255));
        p->a = 255;
        p->alive = true;
    }

    int shrap = claimParticleSlots(context, spawnCount(context, 16, 12, 8, 4));
    for(int i = 0; i < shrap; i++){
        Particle* p = particles::alloc(parts);
        float ang = (2.f * PI * i) / shrap + rng.frand(-0.15f, 0.15f);
        float spd = rng.frand(160.f, 350.f);
        p->x = x;
        p->y = y;
        p->vx = std::cos(ang) * spd;
        p->vy = std::sin(ang) * spd;
        p->maxttl = rng.frand(0.2f, 0.45f);
        p->ttl = p->maxttl;
        p->size = rng.frand(1.5f, 3.f);
        if(i & 1){ p->r = 255; p->g = 240; p->b = 180; }
        else { p->r = baseR; p->g = baseG; p->b = baseB; }
        p->a = 255;
        p->alive = true;
    }
}

void spawnSpark(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                float x, float y)
{
    int n = claimParticleSlots(context, spawnCount(context, 6, 4, 2, 1));
    for(int i = 0; i < n; i++){
        Particle* p = particles::alloc(parts);
        float ang = rng.frand(0.f, 2.f * PI);
        float spd = rng.frand(40.f, 120.f);
        p->x = x;
        p->y = y;
        p->vx = std::cos(ang) * spd;
        p->vy = std::sin(ang) * spd;
        p->maxttl = rng.frand(0.1f, 0.3f);
        p->ttl = p->maxttl;
        p->size = rng.frand(1.f, 2.5f);
        p->r = 200;
        p->g = 200;
        p->b = 255;
        p->a = 255;
        p->alive = true;
    }
}

void spawnTrail(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                float x, float y, int owner)
{
    float keepProb;
    switch(context.perfLevel){
        case PerfLevel::HIGH: keepProb = 1.00f; break;
        case PerfLevel::MEDIUM: keepProb = 0.70f; break;
        case PerfLevel::LOW: keepProb = 0.30f; break;
        case PerfLevel::ULTRA: keepProb = 0.12f; break;
        default: keepProb = 0.70f; break;
    }
    keepProb *= fxLevelScale(context);
    if(rng.frand(0.f, 1.f) > keepProb) return;
    if(claimParticleSlots(context, 1) == 0) return;
    Particle* p = particles::alloc(parts);
    p->x = x;
    p->y = y;
    p->vx = rng.frand(-8.f, 8.f);
    p->vy = rng.frand(-8.f, 8.f);
    p->maxttl = 0.18f;
    p->ttl = p->maxttl;
    p->size = rng.frand(1.f, 2.f);
    if(owner == 1){ p->r = 0; p->g = 220; p->b = 220; }
    else { p->r = 255; p->g = 180; p->b = 0; }
    p->a = 210;
    p->alive = true;
}

void spawnThruster(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                   float x, float y, int id)
{
    float dirY = (id == 1) ? 1.f : -1.f;
    int thrusters = claimParticleSlots(context, 2);
    if(thrusters == 0) return;

    for(int i = 0; i < thrusters; ++i){
        int wing = (thrusters == 1) ? ((rng.irand(0, 1) == 0) ? -1 : 1)
                                    : ((i == 0) ? -1 : 1);
        Particle* p = particles::alloc(parts);
        p->x = x + wing * 7.5f + rng.frand(-2.f, 2.f);
        p->y = y + dirY * 10.f + rng.frand(-2.f, 2.f);
        p->vx = rng.frand(-15.f, 15.f);
        p->vy = dirY * rng.frand(30.f, 80.f);
        p->maxttl = rng.frand(0.18f, 0.38f);
        p->ttl = p->maxttl;
        p->size = rng.frand(2.5f, 4.5f);
        if(id == 1){
            p->r = static_cast<uint8_t>(rng.irand(0, 40));
            p->g = static_cast<uint8_t>(rng.irand(180, 255));
            p->b = static_cast<uint8_t>(rng.irand(200, 255));
        } else {
            p->r = static_cast<uint8_t>(rng.irand(220, 255));
            p->g = static_cast<uint8_t>(rng.irand(60, 160));
            p->b = static_cast<uint8_t>(rng.irand(0, 40));
        }
        p->a = static_cast<uint8_t>(rng.irand(160, 240));
        p->alive = true;
    }
}

void spawnHitSpark(Context& context, std::array<Particle, MAX_PARTICLES>& parts, RNG& rng,
                   float x, float y, int hitOwner)
{
    int n = claimParticleSlots(context, spawnCount(context, 8, 5, 3, 1));
    for(int i = 0; i < n; i++){
        Particle* p = particles::alloc(parts);
        float ang = rng.frand(0.f, 2.f * PI);
        float spd = rng.frand(30.f, 90.f);
        p->x = x;
        p->y = y;
        p->vx = std::cos(ang) * spd;
        p->vy = std::sin(ang) * spd;
        p->maxttl = rng.frand(0.1f, 0.25f);
        p->ttl = p->maxttl;
        p->size = rng.frand(1.f, 3.f);
        if(i == 0){
            p->r = 255; p->g = 255; p->b = 255;
        } else {
            p->r = (hitOwner == 1) ? 0 : 255;
            p->g = (hitOwner == 1) ? 255 : 80;
            p->b = (hitOwner == 1) ? 255 : 50;
        }
        p->a = 255;
        p->alive = true;
    }
}

void spawnFragFloat(std::array<FragFloat, MAX_FRAG_FLOATS>& ff,
                    float x, float y, int scorer)
{
    for(auto& f : ff){
        if(!f.alive){
            f.x = x;
            f.y = y;
            f.maxttl = f.ttl = 1.3f;
            f.scorer = scorer;
            f.alive = true;
            return;
        }
    }
    ff[0] = {x, y, 1.3f, 1.3f, scorer, true};
}

} // namespace effects_runtime
