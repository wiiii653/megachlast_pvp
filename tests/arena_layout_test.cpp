#include "ArenaLayout.h"
#include "Random.h"

#include <cmath>
#include <cstdio>
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
    check(arena_layout::deriveBoardSeed(1234u, 1u) == arena_layout::deriveBoardSeed(1234u, 1u),
          "board seed derivation is deterministic");
    check(arena_layout::deriveBoardSeed(1234u, 1u) != arena_layout::deriveBoardSeed(1234u, 2u),
          "board seed derivation changes by round");

    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers{};
    arena_layout::genBarriers(barriers);

    float totalW = BARRIER_BRICKS * BRICK_W + (BARRIER_BRICKS - 1) * BRICK_GAP;
    float startX = (W - totalW) * 0.5f;
    float firstX = startX + BRICK_W * 0.5f;
    float lastX = startX + (BARRIER_BRICKS - 1) * (BRICK_W + BRICK_GAP) + BRICK_W * 0.5f;

    checkClose(barriers[0].x, firstX, "first P1 barrier brick x");
    checkClose(barriers[BARRIER_BRICKS - 1].x, lastX, "last P1 barrier brick x");
    checkClose(barriers[0].y, (H - 58.f) - BARRIER_Y_OFFSET, "P1 barrier y");
    checkClose(barriers[BARRIER_BRICKS].y, 58.f + BARRIER_Y_OFFSET, "P2 barrier y");

    for(const auto& brick : barriers){
        check(brick.alive, "generated barrier brick is alive");
        check(brick.hp == BRICK_MAX_HP, "generated barrier brick has max hp");
        checkClose(brick.hitFlash, 0.f, "generated barrier brick has no hit flash");
    }

    std::array<SpecialStar, MAX_SPECIAL_STARS> stars{};
    RNG rng(0xC0FFEEu);
    arena_layout::spawnSpecialStars(stars, rng);
    for(const auto& star : stars){
        check(star.alive, "spawned special star is alive");
        check(star.x >= W * 0.2f && star.x <= W * 0.8f, "spawned special star x is in middle band");
        check(star.y >= H * 0.25f && star.y <= H * 0.75f, "spawned special star y is in middle band");
    }

    stars[0].x = 1.f;
    stars[0].y = 1.f;
    stars[0].vx = -10.f;
    stars[0].vy = -12.f;
    stars[0].alive = true;
    arena_layout::updateSpecialStars(stars, 1.f);
    checkClose(stars[0].x, SPECIAL_STAR_R, "special star bounces from left wall");
    checkClose(stars[0].y, SPECIAL_STAR_R, "special star bounces from top wall");
    check(stars[0].vx > 0.f, "special star x velocity reverses at left wall");
    check(stars[0].vy > 0.f, "special star y velocity reverses at top wall");

    for(uint32_t seed = 0; seed < 512; ++seed){
        std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
        arena_layout::LayoutKind layoutKind = arena_layout::LayoutKind::RANDOM;
        char layoutName[24] = {};
        arena_layout::genMirrorsSeeded(mirrors, seed, layoutKind, layoutName);
        check(layoutName[0] != '\0', "mirror generation writes a layout name");

        for(int i = 0; i < MIRROR_PAIRS * 2; ++i){
            const auto& m = mirrors[i];
            if(!m.alive) continue;
            check(m.x >= 40.f && m.x <= W - 40.f, "alive mirror x is in arena band");
            if(i < MIRROR_PAIRS)
                check(m.y >= 12.f && m.y <= H / 2.f - 5.f, "alive top mirror y is in top half");
            else
                check(m.y >= H / 2.f + 5.f && m.y <= H - 12.f, "alive bottom mirror y is in bottom half");
        }

        for(int half = 0; half < 2; ++half){
            int base = half * MIRROR_PAIRS;
            for(int i = 0; i < MIRROR_PAIRS; ++i){
                const auto& a = mirrors[base + i];
                if(!a.alive) continue;
                for(int j = i + 1; j < MIRROR_PAIRS; ++j){
                    const auto& b = mirrors[base + j];
                    if(!b.alive) continue;
                    float dx = a.x - b.x;
                    float dy = a.y - b.y;
                    check(dx * dx + dy * dy >= (MIRROR_PAD - 0.001f) * (MIRROR_PAD - 0.001f),
                          "alive mirrors keep minimum spacing within a half");
                }
            }
        }
    }

    struct BombRangeCase {
        arena_layout::LayoutKind kind;
        int minBombs;
        int maxBombs;
    };
    const BombRangeCase cases[] = {
        {arena_layout::LayoutKind::RANDOM, 0, 3},
        {arena_layout::LayoutKind::SPARSE, 0, 1},
        {arena_layout::LayoutKind::VANGUARD, 0, 1},
        {arena_layout::LayoutKind::CROSS, 1, 2},
        {arena_layout::LayoutKind::LABYRINTH, 1, 2},
        {arena_layout::LayoutKind::FORTRESS, 2, 3},
        {arena_layout::LayoutKind::WARCROSS, 2, 3},
    };
    for(const auto& tc : cases){
        std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
        arena_layout::LayoutKind generatedKind = arena_layout::LayoutKind::RANDOM;
        char generatedLayout[24] = {};
        arena_layout::genMirrorsSeeded(mirrors, 0xBEEFu, generatedKind, generatedLayout);
        check(generatedLayout[0] != '\0', "generated layout name is non-empty for bomb tests");

        std::array<Bomb, MAX_BOMBS> bombs{};
        RNG bombRng(0x12340000u);
        arena_layout::placeBombsForLayout(bombs, mirrors, tc.kind, bombRng);

        int aliveBombs = 0;
        for(int i = 0; i < MAX_BOMBS; ++i){
            const auto& bomb = bombs[i];
            if(!bomb.alive) continue;
            ++aliveBombs;
            check(bomb.x >= 50.f && bomb.x <= W - 50.f, "bomb x is inside placement band");
            check(bomb.y >= H * 0.25f && bomb.y <= H * 0.75f, "bomb y is inside placement band");
            check(std::abs(bomb.y - 58.f) >= 35.f, "bomb avoids top spawn row");
            check(std::abs(bomb.y - (H - 58.f)) >= 35.f, "bomb avoids bottom spawn row");
            check(bomb.owner == 0, "placed bomb has neutral owner");

            for(const auto& mirror : mirrors){
                if(!mirror.alive) continue;
                float dx = bomb.x - mirror.x;
                float dy = bomb.y - mirror.y;
                check(dx * dx + dy * dy >= BOMB_PAD * BOMB_PAD, "bomb keeps clear of mirrors");
            }
            for(int j = i + 1; j < MAX_BOMBS; ++j){
                if(!bombs[j].alive) continue;
                float dx = bomb.x - bombs[j].x;
                float dy = bomb.y - bombs[j].y;
                check(dx * dx + dy * dy >= BOMB_PAD * BOMB_PAD, "bombs keep clear of each other");
            }
        }

        check(aliveBombs >= tc.minBombs && aliveBombs <= tc.maxBombs,
              "layout bomb count is inside configured range");
    }

    std::array<PowerUp, MAX_POWERUPS> powerups{};
    RNG powerupRng(0xFACEu);
    arena_layout::spawnPowerUp(powerups, powerupRng);
    check(powerups[0].alive, "spawnPowerUp activates the first free slot");
    check(powerups[0].ttl == POWERUP_TTL, "spawnPowerUp sets initial ttl");
    check(powerups[0].x >= 20.f && powerups[0].x <= W - 20.f, "spawnPowerUp x is inside arena");
    check(powerups[0].y >= H * 0.5f - 20.f && powerups[0].y <= H * 0.5f + 20.f,
          "spawnPowerUp y is in neutral band");

    powerups[0].x = 1.f;
    powerups[0].vx = -5.f;
    powerups[0].ttl = 1.f;
    arena_layout::updatePowerUps(powerups, 0.25f);
    checkClose(powerups[0].x, POWERUP_R, "power-up bounces from left wall");
    check(powerups[0].vx > 0.f, "power-up x velocity reverses at left wall");
    checkClose(powerups[0].ttl, 0.75f, "power-up ttl decreases during update");

    powerups[0].ttl = 0.1f;
    arena_layout::updatePowerUps(powerups, 0.2f);
    check(!powerups[0].alive, "power-up expires when ttl reaches zero");

    return failures == 0 ? 0 : 1;
}
