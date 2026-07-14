#include "ProjectileRuntime.h"

#include "ArenaLayout.h"
#include "GameConstants.h"
#include "GameLogic.h"
#include "ProceduralSynth.h"

#include <algorithm>
#include <cmath>

namespace projectile_runtime {

namespace {

float scaledSfxVolume(bool muted, float base, float scale)
{
    if(muted) return 0.f;
    return std::clamp(base * scale, 0.f, 100.f);
}

} // namespace

void simulateProjectilesAndCollisions(std::array<Bullet, MAX_BULLETS>& bullets,
                                      Player& p1,
                                      Player& p2,
                                      std::array<PowerUp, MAX_POWERUPS>& powerups,
                                      std::array<Bomb, MAX_BOMBS>& bombs,
                                      std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                                      std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers,
                                      std::array<SpecialStar, MAX_SPECIAL_STARS>& specialStars,
                                      std::array<Particle, MAX_PARTICLES>& particles,
                                      std::array<FragFloat, MAX_FRAG_FLOATS>& fragFloats,
                                      RNG& rng,
                                      const Config& cfg,
                                      ProceduralSynth& synth,
                                      bool muted,
                                      float sfxVolume,
                                      const Hooks& hooks,
                                      float dt)
{
    for(auto& b : bullets){
        if(!b.alive) continue;
        b.ttl -= dt;
        if(b.ttl <= 0.f){ b.alive = false; continue; }
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        if(b.x < -10 || b.x > W + 10 || b.y < -10 || b.y > H + 10){
            b.alive = false;
            continue;
        }

        bool hitPowerup = false;
        for(auto& u : powerups){
            if(!u.alive) continue;
            float dx = b.x - u.x, dy = b.y - u.y;
            if(dx * dx + dy * dy > (POWERUP_R + 2.f) * (POWERUP_R + 2.f)) continue;

            Player& collector = (b.owner == 1) ? p1 : p2;
            switch(u.type){
                case PowerUpType::SHIELD:
                    collector.shieldTimer = 5.f;
                    break;
                case PowerUpType::RAPID:
                    collector.rapidTimer = 5.f;
                    break;
                case PowerUpType::SPREAD:
                    collector.spreadTimer = 5.f;
                    break;
                case PowerUpType::HEAL:
                    collector.energy = std::min(100.f, collector.energy + 40.f);
                    break;
                case PowerUpType::CHAOS: {
                    for(auto& m2 : mirrors) m2.slash = (rng.irand(0, 1) == 0);

                    int roll = rng.irand(0, 99);
                    int addBombCount = 0;
                    if(roll < 60) addBombCount = 1;
                    else if(roll < 90) addBombCount = 2;
                    else addBombCount = 3;

                    for(int bi = 0; bi < addBombCount; ++bi){
                        bool placedAny = false;
                        for(auto& bomb : bombs){
                            if(bomb.alive) continue;
                            bool placed = false;
                            for(int tries = 0; tries < 200 && !placed; ++tries){
                                float bx = rng.frand(50.f, W - 50.f);
                                float by = rng.frand(H * 0.25f, H * 0.75f);
                                if(std::abs(by - PLAYER_SPAWN_TOP_Y) < SPAWN_ROW_BOMB_EXCLUSION ||
                                   std::abs(by - playerSpawnBottomY()) < SPAWN_ROW_BOMB_EXCLUSION) continue;
                                bool ok = true;
                                for(const auto& m : mirrors){
                                    if(!m.alive) continue;
                                    float mdx = bx - m.x, mdy = by - m.y;
                                    if(mdx * mdx + mdy * mdy < BOMB_PAD * BOMB_PAD){ ok = false; break; }
                                }
                                if(!ok) continue;
                                for(const auto& b2 : bombs){
                                    if(!b2.alive) continue;
                                    float bdx = bx - b2.x, bdy = by - b2.y;
                                    if(bdx * bdx + bdy * bdy < BOMB_PAD * BOMB_PAD){ ok = false; break; }
                                }
                                if(!ok) continue;

                                bomb.x = bx;
                                bomb.y = by;
                                bomb.pulsePhase = rng.frand(0.f, PI * 2.f);
                                bomb.alive = true;
                                bomb.owner = 0;
                                placed = true;
                                placedAny = true;
                            }
                            if(placedAny) break;
                        }
                    }

                    int extraPU = rng.irand(0, 2);
                    for(int k = 0; k < extraPU; ++k){
                        for(auto& pu : powerups){
                            if(pu.alive) continue;
                            pu.alive = true;
                            pu.type = static_cast<PowerUpType>(rng.irand(0, 5));
                            pu.ttl = POWERUP_TTL;
                            pu.x = rng.frand(20.f, W - 20.f);
                            pu.y = rng.frand(H * 0.5f - 20.f, H * 0.5f + 20.f);
                            pu.vx = rng.frand(-12.f, 12.f);
                            pu.phase = rng.frand(0.f, 6.28f);
                            break;
                        }
                    }

                    arena_layout::spawnSpecialStars(specialStars, rng);
                    break;
                }
                case PowerUpType::REVERSE: {
                    Player& opp = (b.owner == 1) ? p2 : p1;
                    opp.reverseTimer += 5.f;
                    break;
                }
            }

            if(hooks.spawnSpark) hooks.spawnSpark(particles, rng, u.x, u.y);
            synth.play(ProceduralSynth::SFX::HIT_POWERUP, scaledSfxVolume(muted, sfxVolume, 0.82f));
            u.alive = false;
            b.alive = false;
            hitPowerup = true;
            break;
        }
        if(hitPowerup) continue;

        bool hitBarrier = false;
        for(auto& bk : barriers){
            if(!bk.alive) continue;
            float dx = std::abs(b.x - bk.x), dy = std::abs(b.y - bk.y);
            if(dx > BRICK_W * 0.5f + 2.f || dy > BRICK_H * 0.5f + 2.f) continue;
            bk.hp--;
            bk.hitFlash = 1.0f;
            b.alive = false;
            if(hooks.spawnSpark) hooks.spawnSpark(particles, rng, bk.x + (b.x - bk.x) * 0.4f, bk.y);
            if(bk.hp <= 0) bk.alive = false;
            synth.play(ProceduralSynth::SFX::HIT_BARRIER, scaledSfxVolume(muted, sfxVolume, 0.60f));
            hitBarrier = true;
            break;
        }
        if(hitBarrier) continue;

        bool hitBomb = false;
        for(auto& bomb : bombs){
            if(!bomb.alive) continue;
            float dx = b.x - bomb.x, dy = b.y - bomb.y;
            if(dx * dx + dy * dy > (BOMB_R + 3.f) * (BOMB_R + 3.f)) continue;

            bomb.owner = b.owner;
            bomb.alive = false;
            b.alive = false;
            hitBomb = true;
            float bombEx = bomb.x, bombEy = bomb.y;
            int bombShooter = b.owner;

            if(hooks.spawnExplosion) hooks.spawnExplosion(particles, rng, bomb.x, bomb.y, 2);
            synth.play(ProceduralSynth::SFX::HIT_BOMB, scaledSfxVolume(muted, sfxVolume, 0.72f));
            synth.play(ProceduralSynth::SFX::EXPLOSION, scaledSfxVolume(muted, sfxVolume, 0.95f));
            if(hooks.triggerMusicDuck) hooks.triggerMusicDuck(0.45f, 0.25f);
            if(hooks.triggerScreenShake) hooks.triggerScreenShake(0.35f, 6.0f);

            for(auto& m : mirrors){
                if(!m.alive) continue;
                float mDx = bomb.x - m.x, mDy = bomb.y - m.y;
                if(mDx * mDx + mDy * mDy < BOMB_BLAST_R * BOMB_BLAST_R){
                    m.alive = false;
                    if(hooks.spawnSpark) hooks.spawnSpark(particles, rng, m.x, m.y);
                }
            }

            bool roundTransitioned = false;
            Player* targets2[] = { &p1, &p2 };
            for(Player* tgt : targets2){
                if(tgt->invulnTimer > 0.f) continue;
                if(tgt->shieldTimer > 0.f) continue;
                float pDx = bomb.x - tgt->x, pDy = bomb.y - tgt->y;
                float dist2 = pDx * pDx + pDy * pDy;
                if(dist2 < BOMB_BLAST_R * BOMB_BLAST_R){
                    float dmg = cfg.damage * 2.5f * (1.f - std::sqrt(dist2) / BOMB_BLAST_R);
                    tgt->energy -= dmg;
                    tgt->flashTimer = 0.18f;
                    tgt->slowTimer = std::min(tgt->slowTimer + 0.4f, 2.0f);
                    if(hooks.spawnHitSpark) hooks.spawnHitSpark(particles, rng, tgt->x, tgt->y, b.owner);
                    if(tgt->energy <= 0.f){
                        tgt->energy = 0.f;
                        bool selfKillB = (tgt == &p1 && b.owner == 1) || (tgt == &p2 && b.owner == 2);
                        int scorerB = selfKillB ? (b.owner == 1 ? 2 : 1) : b.owner;
                        if(hooks.spawnExplosion) hooks.spawnExplosion(particles, rng, tgt->x, tgt->y, (scorerB == 1) ? 0 : 1);
                        if(hooks.spawnShipDisintegration) hooks.spawnShipDisintegration(particles, rng, tgt->x, tgt->y, (tgt == &p1) ? 1 : 2);
                        if(hooks.spawnFragFloat) hooks.spawnFragFloat(fragFloats, tgt->x, tgt->y, scorerB);
                        synth.play(ProceduralSynth::SFX::EXPLOSION, scaledSfxVolume(muted, sfxVolume, 1.00f));
                        if(hooks.triggerMusicDuck) hooks.triggerMusicDuck(0.55f, 0.30f);
                        if(hooks.triggerScreenShake) hooks.triggerScreenShake(0.50f, 9.0f);
                        if(hooks.applyFragTransition) hooks.applyFragTransition(*tgt, scorerB);
                        roundTransitioned = true;
                        if(roundTransitioned) break;
                    }
                }
            }
            if(roundTransitioned) break;

            const float fspd = 210.f;
            const float fttl = 0.9f;
            int emitted = 0;
            for(auto& fb : bullets){
                if(fb.alive) continue;
                float ang = emitted * (PI / 4.f);
                fb.x = bombEx;
                fb.y = bombEy;
                fb.vx = std::cos(ang) * fspd;
                fb.vy = std::sin(ang) * fspd;
                fb.ttl = fttl;
                fb.owner = bombShooter;
                fb.alive = true;
                if(++emitted >= 8) break;
            }
            break;
        }
        if(hitBomb) continue;

        bool hitStar = false;
        for(auto& ss : specialStars){
            if(!ss.alive) continue;
            float sdx = b.x - ss.x, sdy = b.y - ss.y;
            if(sdx * sdx + sdy * sdy > SPECIAL_STAR_R * SPECIAL_STAR_R) continue;
            ss.alive = false;
            b.alive = false;
            hitStar = true;
            if(b.owner == 1) p1.points += SPECIAL_STAR_PTS;
            else p2.points += SPECIAL_STAR_PTS;
            if(hooks.spawnExplosion) hooks.spawnExplosion(particles, rng, ss.x, ss.y, 0);
            if(hooks.spawnSpark) hooks.spawnSpark(particles, rng, ss.x, ss.y);
            synth.play(ProceduralSynth::SFX::HIT_STAR, scaledSfxVolume(muted, sfxVolume, 0.86f));
            break;
        }
        if(hitStar) continue;

        bool reflected = false;
        for(auto& m : mirrors){
            if(game_logic::bulletHitsMirror(b, m)){
                auto v = game_logic::reflectMirrorVelocity(b.vx, b.vy, m.slash);
                b.vx = v.x;
                b.vy = v.y;
                m.slash = !m.slash;
                m.hitFlash = 1.0f;
                if(hooks.spawnSpark) hooks.spawnSpark(particles, rng, m.x, m.y);
                synth.play(ProceduralSynth::SFX::REFLECT, scaledSfxVolume(muted, sfxVolume, 0.78f));
                float len = std::sqrt(b.vx * b.vx + b.vy * b.vy);
                if(len > 0.0001f){
                    b.x += (b.vx / len) * (MIRROR_R + 1.5f);
                    b.y += (b.vy / len) * (MIRROR_R + 1.5f);
                }
                reflected = true;
                break;
            }
        }
        if(reflected) continue;

        Player* targets[] = { &p1, &p2 };
        for(Player* tgt : targets){
            if(!game_logic::bulletHitsPlayer(b.x, b.y, tgt->x, tgt->y, cfg.hit_r)) continue;
            if(tgt->invulnTimer > 0.f) continue;
            if(tgt->shieldTimer > 0.f) continue;

            b.alive = false;
            tgt->energy -= cfg.damage;
            tgt->flashTimer = 0.14f;
            if(hooks.spawnHitSpark) hooks.spawnHitSpark(particles, rng, tgt->x, tgt->y, b.owner);
            synth.play(ProceduralSynth::SFX::HIT, scaledSfxVolume(muted, sfxVolume, 0.96f));
            if(hooks.triggerMusicDuck) hooks.triggerMusicDuck(0.20f, 0.12f);
            tgt->slowTimer = std::min(tgt->slowTimer + 0.5f, 2.0f);
            synth.play(ProceduralSynth::SFX::SLOW_HIT, scaledSfxVolume(muted, sfxVolume, 0.70f));

            if(tgt->energy <= 0.f){
                tgt->energy = 0.f;
                bool selfKill = (tgt == &p1 && b.owner == 1) || (tgt == &p2 && b.owner == 2);
                int scorer = selfKill ? (b.owner == 1 ? 2 : 1) : b.owner;
                if(hooks.spawnExplosion) hooks.spawnExplosion(particles, rng, tgt->x, tgt->y, (scorer == 1) ? 0 : 1);
                if(hooks.spawnShipDisintegration) hooks.spawnShipDisintegration(particles, rng, tgt->x, tgt->y, (tgt == &p1) ? 1 : 2);
                if(hooks.spawnFragFloat) hooks.spawnFragFloat(fragFloats, tgt->x, tgt->y, scorer);
                synth.play(ProceduralSynth::SFX::EXPLOSION, scaledSfxVolume(muted, sfxVolume, 1.00f));
                if(hooks.triggerMusicDuck) hooks.triggerMusicDuck(0.55f, 0.30f);
                if(hooks.triggerScreenShake) hooks.triggerScreenShake(0.50f, 9.0f);
                if(hooks.applyFragTransition) hooks.applyFragTransition(*tgt, scorer);
            }
            break;
        }
    }
}

} // namespace projectile_runtime
