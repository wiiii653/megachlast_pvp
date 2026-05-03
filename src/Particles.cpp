#include "Particles.h"

#include <algorithm>
#include <cstdint>

namespace particles {
namespace {

int g_part_cursor = 0;

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

} // namespace

void resetAllocator()
{
    g_part_cursor = 0;
}

Particle* alloc(std::array<Particle, MAX_PARTICLES>& parts)
{
    Particle* out = &parts[g_part_cursor];
    g_part_cursor = (g_part_cursor + 1) % MAX_PARTICLES;
    return out;
}

void update(std::array<Particle, MAX_PARTICLES>& parts, float dt)
{
    for(auto& p : parts){
        if(!p.alive) continue;
        p.ttl -= dt;
        if(p.ttl <= 0.f){ p.alive = false; continue; }
        p.vx *= 0.985f;
        p.vy *= 0.985f;
        p.vy += 18.f * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        float frac = clampf(p.ttl / p.maxttl, 0.f, 1.f);
        p.a = static_cast<uint8_t>(255.f * frac);
    }
}

} // namespace particles
