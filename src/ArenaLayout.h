#pragma once

#include "GameTypes.h"
#include "Random.h"

#include <array>
#include <cstdint>

namespace arena_layout {

enum class LayoutKind : uint8_t {
    RANDOM = 0,
    ZIGZAG,
    CLUSTERS,
    CHANNELS,
    FORTRESS,
    SPARSE,
    VORTEX,
    GAUNTLET,
    CROSS,
    LABYRINTH,
    WARCROSS,
    VANGUARD,
};

uint32_t deriveBoardSeed(uint32_t world, uint32_t round);
void genMirrorsSeeded(std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                      uint32_t boardSeed,
                      LayoutKind& layoutKind,
                      char (&layoutName)[24]);
void genBarriers(std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers);
void placeBombsForLayout(std::array<Bomb, MAX_BOMBS>& bombs,
                         const std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                         LayoutKind layoutKind,
                         RNG& rng);
void spawnPowerUp(std::array<PowerUp, MAX_POWERUPS>& powerups, RNG& rng);
void updatePowerUps(std::array<PowerUp, MAX_POWERUPS>& powerups, float dt);
void spawnSpecialStars(std::array<SpecialStar, MAX_SPECIAL_STARS>& stars, RNG& rng);
void updateSpecialStars(std::array<SpecialStar, MAX_SPECIAL_STARS>& stars, float dt);

} // namespace arena_layout
