#include "Particles.h"

#include <cmath>
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

void checkClose(float got, float expected, const char* msg)
{
    if(std::fabs(got - expected) > 0.0001f){
        std::cerr << "FAIL: " << msg << " got=" << got << " expected=" << expected << "\n";
        ++failures;
    }
}

} // namespace

int main()
{
    std::array<Particle, MAX_PARTICLES> parts{};
    particles::resetAllocator();

    Particle* first = particles::alloc(parts);
    check(first == &parts[0], "allocator returns first free slot after reset");
    first->alive = true;

    Particle* second = particles::alloc(parts);
    check(second == &parts[1], "allocator advances to next free slot");

    for(auto& p : parts) p.alive = true;
    Particle* evicted = particles::alloc(parts);
    check(evicted == &parts[2], "full allocator evicts at cursor position");

    std::array<Particle, MAX_PARTICLES> updateParts{};
    updateParts[0].alive = true;
    updateParts[0].x = 10.f;
    updateParts[0].y = 20.f;
    updateParts[0].vx = 100.f;
    updateParts[0].vy = 50.f;
    updateParts[0].ttl = 1.f;
    updateParts[0].maxttl = 2.f;

    particles::update(updateParts, 0.25f);
    check(updateParts[0].alive, "particle remains alive while ttl positive");
    checkClose(updateParts[0].ttl, 0.75f, "particle ttl decreases");
    checkClose(updateParts[0].vx, 98.5f, "particle x velocity damps");
    checkClose(updateParts[0].vy, 53.75f, "particle y velocity damps then receives gravity");
    checkClose(updateParts[0].x, 34.625f, "particle x advances after damping");
    checkClose(updateParts[0].y, 33.4375f, "particle y advances after damping and gravity");
    check(updateParts[0].a == 95, "particle alpha follows ttl fraction");

    updateParts[0].ttl = 0.1f;
    particles::update(updateParts, 0.2f);
    check(!updateParts[0].alive, "particle expires when ttl reaches zero");

    return failures == 0 ? 0 : 1;
}
