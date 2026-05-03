#pragma once

#include "GameTypes.h"

#include <array>

namespace particles {

void resetAllocator();
Particle* alloc(std::array<Particle, MAX_PARTICLES>& parts);
void update(std::array<Particle, MAX_PARTICLES>& parts, float dt);

} // namespace particles
