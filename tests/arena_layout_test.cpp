#include "ArenaLayout.h"
#include "GameLogic.h"
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

int widestFiringLane(const std::array<Mirror, MIRROR_PAIRS * 2>& mirrors, int from, int to)
{
    int width = 0, widest = 0;
    for(int x = from; x <= to; ++x){
        bool clear = true;
        for(const auto& m : mirrors)
            if(m.alive && std::abs(m.x - x) <= MIRROR_R) clear = false;
        width = clear ? width + 1 : 0;
        widest = std::max(widest, width);
    }
    return widest;
}

int traceBankShot(std::array<Mirror, MIRROR_PAIRS * 2> mirrors, int half, int entry,
                  bool barriersIntact, float stepSize = 1.f, float aimOffset = 0.f)
{
    Bullet bullet{};
    bullet.alive = true;
    bullet.x = mirrors[half * MIRROR_PAIRS + entry].x + aimOffset;
    bullet.y = half == 0 ? PLAYER_SPAWN_TOP_Y : playerSpawnBottomY();
    bullet.vy = half == 0 ? 1.f : -1.f;
    std::array<BarrierBrick, BARRIER_BRICKS * 2> bricks{};
    arena_layout::genBarriers(bricks);
    int bounces = 0;
    for(float distance = 0.f; distance < 650.f; distance += stepSize){
        bullet.x += bullet.vx * stepSize;
        bullet.y += bullet.vy * stepSize;
        if(barriersIntact){
            for(const auto& brick : bricks)
                if(std::abs(bullet.x - brick.x) <= BRICK_W * 0.5f &&
                   std::abs(bullet.y - brick.y) <= BRICK_H * 0.5f) return -2;
        }
        for(auto& m : mirrors){
            if(!game_logic::bulletHitsMirror(bullet, m)) continue;
            const auto velocity = game_logic::reflectMirrorVelocity(bullet.vx, bullet.vy, m.slash);
            bullet.vx = velocity.x;
            bullet.vy = velocity.y;
            m.slash = !m.slash;
            bullet.x += bullet.vx * (MIRROR_R + 1.5f);
            bullet.y += bullet.vy * (MIRROR_R + 1.5f);
            ++bounces;
            break;
        }
        if((half == 0 && bullet.y >= playerSpawnBottomY()) ||
           (half == 1 && bullet.y <= PLAYER_SPAWN_TOP_Y)) return bounces;
        if(bullet.x < 0.f || bullet.x > W || bullet.y < 0.f || bullet.y > H) return -1;
    }
    return -1;
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

    for(const auto aspect : {ScreenAspect::Ratio16x10, ScreenAspect::Ratio16x9}){
        setCanvasAspect(aspect);
        for(int preset = 3; preset <= 5; ++preset){
            for(uint32_t sample = 0; sample < 256; ++sample){
                const uint32_t seed = ((sample * 12u + preset) << 5) | (sample % 32u);
                std::array<Mirror, MIRROR_PAIRS * 2> mirrors{}, repeat{};
                arena_layout::LayoutKind kind{};
                char name[24]{};
                arena_layout::genMirrorsSeeded(mirrors, seed, kind, name);
                arena_layout::genMirrorsSeeded(repeat, seed, kind, name);
                int count = 0;
                for(int i = 0; i < MIRROR_PAIRS; ++i){
                    const auto& m = mirrors[i];
                    const auto& opposite = mirrors[i + MIRROR_PAIRS];
                    check(m.alive == repeat[i].alive && m.x == repeat[i].x &&
                          m.y == repeat[i].y && m.slash == repeat[i].slash,
                          "curated mirrors reproduce from the seed");
                    check(m.alive == opposite.alive, "curated density is equal for both players");
                    if(!m.alive) continue;
                    ++count;
                    checkClose(m.x, opposite.x, "curated mirror x symmetry");
                    checkClose(H - m.y, opposite.y, "curated mirror y symmetry");
                    check(m.slash != opposite.slash, "curated reflection orientation symmetry");
                    check(std::abs(m.y - PLAYER_SPAWN_TOP_Y) >= MIRROR_R + 8.f,
                          "curated mirrors keep spawn clearance");
                    for(int j = i + 1; j < MIRROR_PAIRS; ++j){
                        if(!mirrors[j].alive) continue;
                        const float dx = m.x - mirrors[j].x, dy = m.y - mirrors[j].y;
                        check(dx * dx + dy * dy >= MIRROR_PAD * MIRROR_PAD - 0.001f,
                              "curated mirrors keep spacing at both aspect ratios");
                    }
                }
                check(count == (preset == 5 ? 8 : 20), "curated layouts retain intended density");
                if(preset == 5){
                    check(widestFiringLane(mirrors, 210, 430) >= 200,
                          "Open Reactor retains a broad open center");
                } else {
                    const int minWidth = preset == 4 ? 40 : 24;
                    check(widestFiringLane(mirrors, 180, 310) >= minWidth &&
                          widestFiringLane(mirrors, 330, 460) >= minWidth,
                          "dense arenas retain two clear firing gates");
                }
                for(int half = 0; half < 2; ++half){
                    for(int entry : {0, 3}){
                        for(float fps : {60.f, 120.f, 240.f}){
                            int successfulAims = 0;
                            for(int offset = -3; offset <= 3; ++offset)
                                if(traceBankShot(mirrors, half, entry, false, 190.f / fps,
                                                 static_cast<float>(offset)) == 4) ++successfulAims;
                            check(successfulAims >= 4,
                                  "both bank routes offer a usable aiming window at common frame rates");
                        }
                        check(traceBankShot(mirrors, half, entry, true) == -2,
                              "bank routes still require breaking the barriers");
                    }
                }
            }
        }
    }
    setCanvasAspect(ScreenAspect::Ratio16x10);

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
