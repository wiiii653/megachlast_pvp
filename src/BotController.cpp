#include "BotController.h"

#include "ProceduralSynth.h"
#include "GameLogic.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bot_controller {
namespace {

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

struct TraceBoard {
    std::array<Mirror, MIRROR_PAIRS * 2> mirrors;
    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers;
    std::array<PowerUp, MAX_POWERUPS> powerups;
    std::array<Bomb, MAX_BOMBS> bombs;
};

enum class Contact { NONE, EXPIRED, MIRROR, BARRIER, POWERUP, BOMB };
struct TraceContact { Contact kind = Contact::NONE; int index = -1; };

TraceContact advanceTrace(Bullet& b, TraceBoard& board, float dt, float elapsed)
{
    b.ttl -= dt;
    if(b.ttl <= 0.f) return {Contact::EXPIRED};
    b.x += b.vx * dt;
    b.y += b.vy * dt;
    if(b.x < -10.f || b.x > W + 10.f || b.y < -10.f || b.y > H + 10.f)
        return {Contact::EXPIRED};
    for(int i = 0; i < MAX_POWERUPS; ++i){
        const auto& u = board.powerups[i];
        if(!u.alive || u.ttl <= elapsed) continue;
        const float x = clampf(u.x + u.vx * elapsed, POWERUP_R, W - POWERUP_R);
        const float dx = b.x - x, dy = b.y - u.y;
        if(dx * dx + dy * dy <= (POWERUP_R + 2.f) * (POWERUP_R + 2.f))
            return {Contact::POWERUP, i};
    }
    for(int i = 0; i < BARRIER_BRICKS * 2; ++i){
        const auto& brick = board.barriers[i];
        if(brick.alive && std::abs(b.x - brick.x) <= BRICK_W * 0.5f + 2.f &&
           std::abs(b.y - brick.y) <= BRICK_H * 0.5f + 2.f) return {Contact::BARRIER, i};
    }
    for(int i = 0; i < MAX_BOMBS; ++i){
        const auto& bomb = board.bombs[i];
        const float dx = b.x - bomb.x, dy = b.y - bomb.y;
        if(bomb.alive && dx * dx + dy * dy <= (BOMB_R + 3.f) * (BOMB_R + 3.f))
            return {Contact::BOMB, i};
    }
    for(auto& m : board.mirrors){
        if(!game_logic::bulletHitsMirror(b, m)) continue;
        const auto v = game_logic::reflectMirrorVelocity(b.vx, b.vy, m.slash);
        b.vx = v.x; b.vy = v.y;
        m.slash = !m.slash;
        const float speed = std::sqrt(b.vx * b.vx + b.vy * b.vy);
        if(speed > 0.0001f){
            b.x += b.vx / speed * (MIRROR_R + 1.5f);
            b.y += b.vy / speed * (MIRROR_R + 1.5f);
        }
        return {Contact::MIRROR};
    }
    return {};
}

struct ShotValue { float score = 0.f; int bounces = 0; };

ShotValue evaluateRay(float x, const Player& bot, const Player& player, float playerVx,
                      float aimLead, const Config& cfg, const TraceBoard& original, float dt,
                      float vx, float vy, bool allowSetup = true)
{
    TraceBoard board = original;
    Bullet b{x, bot.y + 16.f, vx, vy, cfg.bullet_ttl, 2, true};
    ShotValue result;
    float barrierCost = 0.f;
    for(float time = dt; time <= std::min(cfg.bullet_ttl, 4.f); time += dt){
        const auto contact = advanceTrace(b, board, dt, time);
        if(contact.kind == Contact::EXPIRED) break;
        if(contact.kind == Contact::BARRIER){
            auto& brick = board.barriers[contact.index];
            // Value the route after opening it, discounting the shots needed first.
            barrierCost += std::max(1, brick.hp) * 7.f;
            brick.alive = false;
            continue;
        }
        if(contact.kind == Contact::POWERUP){
            const auto type = board.powerups[contact.index].type;
            float value = 45.f;
            if(type == PowerUpType::HEAL) value = bot.energy < 60.f ? 110.f : 10.f;
            if(type == PowerUpType::SHIELD) value = bot.shieldTimer > 1.f ? 15.f : 70.f;
            return {value - barrierCost - time * 4.f, result.bounces};
        }
        if(contact.kind == Contact::BOMB){
            const auto& bomb = board.bombs[contact.index];
            if(std::hypot(bomb.x - x, bomb.y - bot.y) < BOMB_BLAST_R) return {-150.f, result.bounces};
            const float distance = std::hypot(bomb.x - player.x, bomb.y - player.y);
            return {distance < BOMB_BLAST_R ? 75.f - barrierCost : 0.f, result.bounces};
        }
        if(contact.kind == Contact::MIRROR){
            if(++result.bounces >= 16) break;
            continue;
        }
        if(time > bot.shieldTimer && time > bot.invulnTimer &&
           game_logic::bulletHitsPlayer(b.x, b.y, x, bot.y, cfg.hit_r))
            return {-150.f, result.bounces};
        const float target = clampf(player.x + clampf(playerVx * std::min(time, 0.45f) * aimLead,
                                                      -64.f, 64.f), 8.f, W - 8.f);
        // Movement can reverse before impact; cover current and led positions.
        const float lo = std::min(player.x, target), hi = std::max(player.x, target);
        const float plausibleX = clampf(b.x, lo, hi);
        if(game_logic::bulletHitsPlayer(b.x, b.y, plausibleX, player.y, cfg.hit_r)){
            if(time <= player.shieldTimer || time <= player.invulnTimer){
                if(barrierCost > 0.f) return {35.f - barrierCost * 0.35f - time * 2.f, result.bounces};
                continue;
            }
            const bool direct = game_logic::bulletHitsPlayer(b.x, b.y, player.x, player.y, cfg.hit_r);
            const bool led = game_logic::bulletHitsPlayer(b.x, b.y, target, player.y, cfg.hit_r);
            return {(led ? 125.f : (direct ? 120.f : 100.f)) - barrierCost - time * 8.f, result.bounces};
        }
    }
    if(allowSetup && result.bounces > 0){
        // A harmless first shot can rotate a blocked route into a useful follow-up.
        const auto followUp = evaluateRay(x, bot, player, playerVx, aimLead, cfg,
                                          board, dt, vx, vy, false);
        if(followUp.score > 0.f) return {followUp.score * 0.6f - barrierCost, result.bounces};
    }
    return {0.f, result.bounces};
}

ShotValue evaluateShot(float x, const Player& bot, const Player& player, float playerVx,
                       float aimLead, const Config& cfg, const TraceBoard& board, float dt)
{
    auto best = evaluateRay(x, bot, player, playerVx, aimLead, cfg, board, dt, 0.f, cfg.bullet_speed);
    if(bot.spreadTimer <= 0.f || best.score < -100.f) return best;
    for(float sign : {-1.f, 1.f}){
        const auto side = evaluateRay(x, bot, player, playerVx, aimLead, cfg, board, dt,
                                      sign * cfg.bullet_speed * 0.3090f, cfg.bullet_speed * 0.9511f);
        if(side.score < -100.f) return side;
        if(side.score > best.score) best = side;
    }
    return best;
}

struct ThreatPath {
    std::array<game_logic::Vec2, 49> positions{};
    std::array<float, 49> times{};
    int count = 0;
};

std::array<ThreatPath, MAX_BULLETS> predictThreats(const std::array<Bullet, MAX_BULLETS>& bullets,
                                                TraceBoard board, float dt)
{
    std::array<ThreatPath, MAX_BULLETS> paths{};
    auto future = bullets;
    float nextSample = dt;
    for(float time = dt; time <= 0.8f; time += dt){
        const bool sample = time >= nextSample;
        for(int i = 0; i < MAX_BULLETS; ++i){
            auto& b = future[i];
            if(!b.alive) continue;
            const auto contact = advanceTrace(b, board, dt, time);
            if(contact.kind == Contact::BARRIER){
                auto& brick = board.barriers[contact.index];
                if(--brick.hp <= 0) brick.alive = false;
            }
            if(contact.kind == Contact::POWERUP) board.powerups[contact.index].alive = false;
            if(contact.kind != Contact::NONE && contact.kind != Contact::MIRROR){
                b.alive = false;
                continue;
            }
            auto& path = paths[i];
            if(sample && contact.kind != Contact::MIRROR && path.count < 49){
                path.positions[path.count] = {b.x, b.y};
                path.times[path.count++] = time;
            }
        }
        if(sample) nextSample = time + 1.f / 60.f;
    }
    return paths;
}

void updateAdvanced(RuntimeState& rt, Player& bot, const Player& player,
                std::array<Bullet, MAX_BULLETS>& bullets,
                const std::array<PowerUp, MAX_POWERUPS>& powerups,
                const std::array<Bomb, MAX_BOMBS>& bombs,
                const std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                const std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers,
                bool mirrorAware,
                RNG& rng, int& cooldown, float dt, const Config& cfg,
                float aimLead, float alignTolerance, float fireProbability,
                float powerupInterest, ProceduralSynth& synth,
                bool muted, float volume, FireFn fireFn)
{
    if(dt <= 0.f) return;
    rt.shot_idle += dt;
    const bool finishingBurst = rt.pressure_timer > 0.f && rt.pressure_timer <= dt;
    rt.pressure_timer = std::max(0.f, rt.pressure_timer - dt);
    rt.reversal_timer = std::max(0.f, rt.reversal_timer - dt);
    rt.retreat_timer = std::max(0.f, rt.retreat_timer - dt);
    if(mirrorAware && finishingBurst) rt.retreat_timer = 0.4f;
    float observedVx = rt.tracking_player
        ? clampf((player.x - rt.prev_p1_x) / dt, -cfg.p_speed, cfg.p_speed) : 0.f;
    if(mirrorAware){
        rt.turn_age = std::min(100.f, rt.turn_age + dt);
        if(std::abs(observedVx) > cfg.p_speed * 0.1f){
            const float direction = observedVx > 0.f ? 1.f : -1.f;
            if(rt.last_motion_direction * direction < 0.f){
                if(rt.turn_age < 3.5f) rt.reversal_timer = 3.f;
                rt.turn_age = 0.f;
            }
            rt.last_motion_direction = direction;
        }
    }
    if(observedVx * rt.tracked_vx < 0.f) rt.tracked_vx = 0.f;
    rt.tracked_vx += (observedVx - rt.tracked_vx) * (1.f - std::exp(-dt / 0.08f));
    rt.tracking_player = true;
    rt.prev_p1_x = player.x;
    if(mirrorAware && rt.reversal_timer > 0.f) aimLead *= 0.35f;

    const float travel = std::abs(player.y - bot.y) / std::max(1.f, cfg.bullet_speed);
    const float lead = clampf(rt.tracked_vx * std::min(travel, 0.45f) * aimLead, -64.f, 64.f);
    const float predictedX = clampf(player.x + lead, 8.f, W - 8.f);
    float targetX = rt.attack_phase < 3 ? predictedX : player.x;
    float pickupX = -1.f;
    float pickupScore = 0.9f;
    for(const auto& u : powerups){
        if(!u.alive || u.y <= bot.y) continue;
        const float time = (u.y - bot.y) / std::max(1.f, cfg.bullet_speed);
        if(time >= u.ttl || time >= cfg.bullet_ttl) continue;
        const float x = clampf(u.x + u.vx * time, POWERUP_R, W - POWERUP_R);
        float value = 1.f;
        if(u.type == PowerUpType::HEAL) value = bot.energy < 60.f ? 2.f : 0.2f;
        if(u.type == PowerUpType::SHIELD) value = bot.shieldTimer > 1.f ? 0.3f : 1.5f;
        const float score = powerupInterest * value - std::abs(x - bot.x) / (W * 0.5f);
        if(score > pickupScore){ pickupScore = score; pickupX = x; }
    }
    if(pickupX >= 0.f) targetX = pickupX;

    const float traceDt = clampf(dt, 1.f / 240.f, 1.f / 30.f);
    const TraceBoard board{mirrors, barriers, powerups, bombs};
    std::array<ThreatPath, MAX_BULLETS> paths{};
    if(mirrorAware){
        paths = predictThreats(bullets, board, traceDt);
        rt.plan_timer -= dt;
        if(!rt.mirror_planning || rt.plan_timer <= 0.f){
            float bestUtility = -1e9f;
            const float previousPlanX = rt.plan_x;
            auto consider = [&](float candidate){
                candidate = clampf(candidate, 8.f, W - 8.f);
                const auto value = evaluateShot(candidate, bot, player, rt.tracked_vx, aimLead, cfg, board, traceDt);
                if(value.score <= 0.f) return;
                const float utility = value.score - std::abs(candidate - bot.x) * 0.12f
                    + (std::abs(candidate - previousPlanX) < 2.f ? (value.bounces > 0 ? 3.f : 0.5f) : 0.f)
                    + (rt.reversal_timer > 0.f && std::abs(rt.tracked_vx) > cfg.p_speed * 0.15f &&
                       std::abs(candidate - targetX) < 1.f ? 10.f : 0.f);
                if(utility > bestUtility){
                    bestUtility = utility;
                    rt.plan_x = candidate; rt.plan_score = value.score; rt.plan_bounces = value.bounces;
                }
            };
            rt.plan_score = 0.f;
            rt.plan_bounces = 0;
            consider(bot.x); consider(targetX); consider(player.x);
            const float shipEdge = std::max(1.f, 19.f + cfg.hit_r - 0.5f);
            consider(player.x - shipEdge); consider(player.x + shipEdge);
            for(float offset = -shipEdge; offset <= shipEdge; offset += 4.f)
                consider(player.x + offset);
            for(float x = 16.f; x < W; x += 24.f) consider(x);
            for(const auto& m : mirrors){
                if(!m.alive || m.y < bot.y || m.y >= H * 0.5f) continue;
                for(float offset : {-8.f, -4.f, -3.f, 0.f, 3.f, 4.f, 8.f}) consider(m.x + offset);
            }
            rt.plan_timer = 0.15f;
        }
        if(rt.plan_score > 0.f) targetX = rt.plan_x;
    }
    rt.mirror_planning = mirrorAware;

    const float speed = cfg.p_speed * (bot.slowTimer > 0.f ? 0.5f : 1.f);
    const bool cornerPressure = mirrorAware && (player.x < 60.f || player.x > W - 60.f);
    const bool pressAttack = (rt.shot_idle > 2.f || (cornerPressure && rt.pressure_timer > 0.f)) &&
                             bot.energy >= cfg.damage * 3.f &&
                             std::abs(rt.tracked_vx) < cfg.p_speed * 0.15f &&
                             player.shieldTimer <= 0.f && pickupX < 0.f && rt.retreat_timer <= 0.f;
    if(cornerPressure && rt.retreat_timer > 0.f)
        targetX = player.x < W * 0.5f ? player.x + 55.f : player.x - 55.f;
    const float directions[3] = {0.f, -1.f, 1.f};
    float costs[3]{}, risks[3]{};
    float earliestThreat = 1e9f;
    float impactX = bot.x;
    for(int choice = 0; choice < 3; ++choice){
        const float direction = directions[choice];
        const float moveDistance = mirrorAware ? std::min(speed * 0.25f, std::abs(targetX - bot.x)) : speed * 0.25f;
        const float destination = clampf(bot.x + direction * moveDistance, 8.f, W - 8.f);
        costs[choice] = std::abs(destination - targetX) * (pressAttack ? 3.f : 0.12f);
        costs[choice] += std::max(0.f, 28.f - std::min(destination - 8.f, W - 8.f - destination)) * 0.15f;
        for(int bulletIndex = 0; bulletIndex < MAX_BULLETS; ++bulletIndex){
            const auto& b = bullets[bulletIndex];
            if(!b.alive) continue;
            float danger = 0.f;
            const auto& path = paths[bulletIndex];
            // Compare complete moves; opposing threats must not cancel each other.
            for(int step = 1; step <= (mirrorAware ? path.count : 48); ++step){
                const float time = mirrorAware ? path.times[step - 1] : step / 60.f;
                if(time >= b.ttl) break;
                if(bot.shieldTimer > time || bot.invulnTimer > time) continue;
                const float x = clampf(bot.x + direction * speed * std::min(time, 0.25f), 8.f, W - 8.f);
                const float bx = mirrorAware ? path.positions[step - 1].x : b.x + b.vx * time;
                const float by = mirrorAware ? path.positions[step - 1].y : b.y + b.vy * time;
                const float nx = std::abs(bx - x) / (21.f + cfg.hit_r);
                const float ny = std::abs(by - bot.y) / (9.f + cfg.hit_r);
                const float distance = std::max(nx, ny);
                if(distance < 1.5f){
                    const float severity = distance <= 1.f ? 1000.f : (1.5f - distance) * 100.f;
                    danger = std::max(danger, severity / (0.2f + time));
                    if(choice == 0 && distance <= 1.f && time < earliestThreat){
                        earliestThreat = time;
                        impactX = bx;
                    }
                }
            }
            risks[choice] += danger;
        }
        for(const auto& bomb : bombs){
            if(!bomb.alive) continue;
            const float dx = bomb.x - destination, dy = bomb.y - bot.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if(distance < BOMB_BLAST_R && bot.shieldTimer <= 0.f)
                risks[choice] += (1.f - distance / BOMB_BLAST_R) * 80.f;
        }
        // A healthy bot may briefly contest a firing lane instead of waiting forever.
        costs[choice] += pressAttack ? std::min(60.f, risks[choice]) : risks[choice];
        if(mirrorAware && pressAttack){
            const auto opening = evaluateShot(destination, bot, player, rt.tracked_vx,
                                               aimLead, cfg, board, traceDt);
            costs[choice] -= std::max(0.f, opening.score) * 0.75f;
        }
    }
    int best = 0;
    for(int i = 1; i < 3; ++i) if(costs[i] < costs[best]) best = i;
    rt.move_timer = std::max(0.f, rt.move_timer - dt);
    for(int i = 0; i < 3; ++i){
        if(directions[i] == rt.move_direction && rt.move_timer > 0.f &&
           costs[i] <= costs[best] + 3.f && risks[i] <= risks[best] + 0.01f){
            best = i;
            break;
        }
    }
    if(rt.move_timer <= 0.f || rt.move_direction != directions[best]) rt.move_timer = 0.12f;
    rt.move_direction = directions[best];
    float movement = rt.move_direction;
    if(mirrorAware && movement * (targetX - bot.x) > 0.f && speed * dt > 0.f)
        movement *= std::min(1.f, std::abs(targetX - bot.x) / (speed * dt));
    if(bot.reverseTimer > 0.f) movement = -movement;
    bot.x = clampf(bot.x + movement * speed * dt, 8.f, W - 8.f);
    rt.state = earliestThreat < 0.8f ? BotState::EVADE
        : (pickupX >= 0.f ? BotState::COLLECT : BotState::ATTACK);

    const float shotWidth = std::max(19.f + cfg.hit_r, alignTolerance)
        + (bot.spreadTimer > 0.f ? 20.f : 0.f);
    const bool pressureShot = bot.x >= std::min(player.x, predictedX) - shotWidth &&
                              bot.x <= std::max(player.x, predictedX) + shotWidth;
    const bool pickupShot = pickupX >= 0.f && std::abs(bot.x - pickupX) <= POWERUP_R;
    bool usefulShot = pressureShot || pickupShot;
    if(mirrorAware && cooldown == 0)
        usefulShot = evaluateShot(bot.x, bot, player, rt.tracked_vx, aimLead, cfg, board, traceDt).score > 0.f;
    if(cooldown == 0 && usefulShot && rng.frand(0.f, 1.f) <= fireProbability){
        if(cornerPressure && pressAttack && rt.shot_idle > 2.f) rt.pressure_timer = 0.15f;
        fireFn(bullets, bot, 2);
        synth.play(ProceduralSynth::SFX::FIRE, muted ? 0.f : volume);
        cooldown = computeFireCooldown(cfg.fire_cd_p2_frames, bot.rapidTimer);
        rt.attack_phase = (rt.attack_phase + 1) % 6;
        rt.shot_idle = 0.f;
        if(mirrorAware) rt.plan_timer = 0.f;
    }
    if(rt.debug){
        rt.debug_pred_impact_x = impactX;
        rt.debug_pred_intercept_x = predictedX;
        rt.debug_best_pu_x = pickupX;
        rt.debug_worst_tti = earliestThreat;
    }
}

} // namespace

void update(RuntimeState& rt,
            Player& p2,
            const Player& p1,
            std::array<Bullet, MAX_BULLETS>& bullets,
            const std::array<PowerUp, MAX_POWERUPS>& powerups,
            const std::array<Bomb, MAX_BOMBS>& bombs,
            const std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
            const std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers,
            RNG& rng,
            int& cd2,
            float dt,
            BotDifficulty difficulty,
            const Config& cfg,
            const BotTuningOverrides& overrides,
            ProceduralSynth& synth,
            bool muted,
            float sfxVolume,
            FireFn fireFn)
{
    rt.log_collect_cd = std::max(0.f, rt.log_collect_cd - dt);
    rt.log_bomb_cd = std::max(0.f, rt.log_bomb_cd - dt);

    struct BotCfg {
        float dodge_zone_y;
        float dodge_x_thr;
        float align_tol;
        float fire_prob;
        float reaction;
        float powerup_interest;
        float strafe_period_lo;
        float strafe_period_hi;
        float bomb_fear;
        float aim_lead;
        float attack_bias;
        float edge_avoid;
        float threat_tunnel;
    };

    BotCfg bc;
    switch(difficulty){
        case BotDifficulty::EASY:
            bc = {35.f, cfg.hit_r+10.f, 22.f, 0.75f, 0.85f, 0.55f, 1.2f, 3.0f, 25.f, 0.40f, 0.40f, 26.f, 16.f};
            break;
        case BotDifficulty::MEDIUM:
        case BotDifficulty::HARD:
        default:
            bc = {80.f, cfg.hit_r+2.f,  6.f, 1.00f, 1.00f, 0.95f, 0.3f, 0.8f, 65.f, 1.10f, 0.80f, 44.f, 28.f};
            break;
    }

    if(overrides.dodge_zone >= 0.f)       bc.dodge_zone_y     = std::max(1.f, overrides.dodge_zone);
    if(overrides.dodge_x_thr >= 0.f)      bc.dodge_x_thr      = std::max(1.f, overrides.dodge_x_thr);
    if(overrides.align_tol >= 0.f)        bc.align_tol        = std::max(1.f, overrides.align_tol);
    if(overrides.fire_prob >= 0.f)        bc.fire_prob        = clampf(overrides.fire_prob, 0.f, 1.f);
    if(overrides.reaction >= 0.f)         bc.reaction         = clampf(overrides.reaction, 0.f, 1.f);
    if(overrides.powerup_interest >= 0.f) bc.powerup_interest = clampf(overrides.powerup_interest, 0.f, 1.f);
    if(overrides.strafe_lo >= 0.f)        bc.strafe_period_lo = std::max(0.05f, overrides.strafe_lo);
    if(overrides.strafe_hi >= 0.f)        bc.strafe_period_hi = std::max(0.05f, overrides.strafe_hi);
    if(bc.strafe_period_hi < bc.strafe_period_lo) std::swap(bc.strafe_period_hi, bc.strafe_period_lo);
    if(overrides.bomb_fear >= 0.f)        bc.bomb_fear        = std::max(0.f, overrides.bomb_fear);

    float aim_lead_override = -1.f;
    switch(difficulty){
        case BotDifficulty::EASY:   aim_lead_override = overrides.aim_lead_easy; break;
        case BotDifficulty::MEDIUM: aim_lead_override = overrides.aim_lead_med;  break;
        case BotDifficulty::HARD:   aim_lead_override = overrides.aim_lead_hard; break;
    }
    if(aim_lead_override >= 0.f) bc.aim_lead = std::max(0.f, aim_lead_override);

    if(rng.frand(0.f, 1.f) > bc.reaction){
        float sp = cfg.p_speed * ((p2.slowTimer > 0.f) ? 0.5f : 1.f);
        float dx = rt.strafe_target - p2.x;
        if(std::abs(dx) > 4.f){
            float dir = (dx > 0.f) ? 1.f : -1.f;
            if(p2.reverseTimer > 0.f) dir = -dir;
            p2.x += dir * sp * 0.4f * dt;
        }
        return;
    }

    if(difficulty != BotDifficulty::EASY){
        updateAdvanced(rt, p2, p1, bullets, powerups, bombs, mirrors, barriers,
                   difficulty == BotDifficulty::HARD, rng, cd2, dt, cfg,
                   bc.aim_lead, bc.align_tol, bc.fire_prob, bc.powerup_interest,
                   synth, muted, sfxVolume, fireFn);
        return;
    }
    rt.tracking_player = false;

    float prune_evade_score = 0.f;
    float want_x = 0.f;
    float want_w = 0.f;

    auto addWant = [&](float dir, float weight){
        want_x += dir * weight;
        want_w += weight;
    };

    float worst_threat_time = 1e9f;
    float worst_threat_x = p2.x;
    for(const auto& b : bullets){
        if(!b.alive) continue;
        float relx = b.x - p2.x;
        float rely = b.y - p2.y;
        float v2 = b.vx*b.vx + b.vy*b.vy;
        if(v2 < 1e-3f) continue;

        float t_horizon = std::min(b.ttl, 1.35f + bc.dodge_zone_y / std::max(40.f, cfg.bullet_speed));
        float t_closest = - (relx*b.vx + rely*b.vy) / v2;
        t_closest = clampf(t_closest, 0.f, t_horizon);

        float cx = relx + b.vx * t_closest;
        float cy = rely + b.vy * t_closest;
        float dist2 = cx*cx + cy*cy;

        float danger_r = bc.dodge_x_thr + bc.threat_tunnel;
        float danger_r2 = danger_r * danger_r;
        if(dist2 < danger_r2){
            float dist = std::sqrt(std::max(0.f, dist2));
            float proximity = 1.f - clampf(dist / std::max(1.f, danger_r), 0.f, 1.f);
            float imminence = 1.f - clampf(t_closest / std::max(0.05f, t_horizon), 0.f, 1.f);
            float urgency = (0.60f * proximity + 0.40f * imminence) * 12.f;
            float away = (cx > 0.f) ? -1.f : 1.f;
            if(std::abs(cx) < 2.f) away = (rng.frand(0.f, 1.f) < 0.5f) ? -1.f : 1.f;
            addWant(away, urgency);
            if(t_closest < worst_threat_time){
                worst_threat_time = t_closest;
                worst_threat_x = p2.x + cx;
            }
        }
    }

    if(worst_threat_time < 1e8f){
        prune_evade_score = std::max(0.f, 1.f - (worst_threat_time / std::max(0.05f, bc.dodge_zone_y / std::max(1.f, cfg.bullet_speed))));
        prune_evade_score *= 2.5f;
    }

    {
        float edgeL = p2.x - 8.f;
        float edgeR = (W - 8.f) - p2.x;
        if(edgeL < bc.edge_avoid){
            float w = (1.f - clampf(edgeL / std::max(1.f, bc.edge_avoid), 0.f, 1.f)) * 4.0f;
            addWant(1.f, w);
        }
        if(edgeR < bc.edge_avoid){
            float w = (1.f - clampf(edgeR / std::max(1.f, bc.edge_avoid), 0.f, 1.f)) * 4.0f;
            addWant(-1.f, w);
        }
    }

    float best_pu_score = 0.f;
    float best_pu_x = 0.f;
    float best_pu_dx = 0.f;
    bool safe_to_collect = (worst_threat_time > 0.0001f && worst_threat_time > (bc.dodge_zone_y * 0.5f) / std::max(1.f, cfg.bullet_speed));
    if(safe_to_collect){
        for(const auto& u : powerups){
            if(!u.alive) continue;
            float dx = u.x - p2.x;
            float adx = std::abs(dx);
            float dy = std::abs(u.y - p2.y);
            if(dy > H * 0.35f) continue;

            float type_w = 0.8f;
            switch(u.type){
                case PowerUpType::SHIELD: type_w = 1.4f; break;
                case PowerUpType::RAPID:  type_w = 1.2f; break;
                case PowerUpType::SPREAD: type_w = 1.1f; break;
                case PowerUpType::HEAL:   type_w = (p2.energy < 60.f) ? 1.6f : 0.6f; break;
                case PowerUpType::CHAOS:  type_w = 0.9f; break;
                case PowerUpType::REVERSE: type_w = 1.0f; break;
                default: type_w = 0.9f; break;
            }

            float dist_pen = clampf(1.f - (adx / (W * 0.5f)), 0.f, 1.f);
            float opp_adx = std::abs(u.x - p1.x);
            float deny_bonus = 0.f;
            if(opp_adx + 20.f < adx && bc.powerup_interest > 0.5f){
                deny_bonus = 0.35f;
            }

            float score = type_w * dist_pen + deny_bonus;
            if(u.type == PowerUpType::HEAL && p2.energy < 40.f) score *= 1.4f;
            score *= bc.powerup_interest;

            if(score > best_pu_score){ best_pu_score = score; best_pu_x = u.x; best_pu_dx = dx; }
        }

        if(best_pu_score > 0.15f){
            addWant((best_pu_dx > 0) ? 1.f : -1.f, best_pu_score * 4.0f);
            if(rt.debug && best_pu_score > 0.5f && rt.log_collect_cd <= 0.f){
                std::fprintf(stderr, "[BOT] Collect target at x=%.1f score=%.2f\n", best_pu_x, best_pu_score);
                rt.log_collect_cd = 0.5f;
            }
        } else {
            float center_dx = (W * 0.5f) - p2.x;
            addWant((center_dx > 0) ? 1.f : -1.f, bc.powerup_interest * 1.5f);
        }
    }

    for(const auto& bm : bombs){
        if(!bm.alive) continue;
        float bdx = bm.x - p2.x;
        float abd = std::abs(bdx);
        if(abd < bc.bomb_fear){
            float away_w = (1.f - (abd / bc.bomb_fear)) * 6.f;
            addWant((bdx > 0) ? -1.f : 1.f, away_w);
            if(rt.debug && abd < bc.bomb_fear * 0.4f && rt.log_bomb_cd <= 0.f){
                std::fprintf(stderr, "[BOT] Avoiding bomb at x=%.1f (dist=%.1f)\n", bm.x, abd);
                rt.log_bomb_cd = 0.5f;
            }
        }
    }

    float p1_vx = 0.f;
    if(dt > 0.f){ p1_vx = (p1.x - rt.prev_p1_x) / dt; }
    float travel_time = std::abs(p1.y - p2.y) / std::max(1.f, cfg.bullet_speed);

    float evade_score   = prune_evade_score;
    float collect_score = best_pu_score;
    float attack_score  = bc.attack_bias;
    if(p2.energy > p1.energy) attack_score += 0.10f;
    if(worst_threat_time < 0.20f) attack_score -= 0.35f;
    if(best_pu_score > 0.55f) attack_score -= 0.15f;

    if(rt.state_timer > 0.f){ rt.state_timer -= dt; }

    if(rt.state_timer <= 0.f){
        if(evade_score > 0.45f){        rt.state = BotState::EVADE;   rt.state_timer = 0.20f; }
        else if(collect_score > attack_score && collect_score > 0.50f){
                                        rt.state = BotState::COLLECT; rt.state_timer = 0.50f;
        } else {                        rt.state = BotState::ATTACK;  rt.state_timer = 0.35f; }
    }

    switch(rt.state){
        case BotState::EVADE:
            if(want_w > 0.001f){ addWant((want_x / want_w > 0.f) ? 1.f : -1.f, 8.0f); }
            break;
        case BotState::COLLECT:
            if(best_pu_score > 0.15f){ addWant((best_pu_dx > 0) ? 1.f : -1.f, best_pu_score * 7.0f); }
            break;
        case BotState::ATTACK:
        case BotState::DENY:
        {
            float predict_x = p1.x + p1_vx * travel_time * bc.aim_lead;
            float dxToPred  = predict_x - p2.x;
            float aim_w = (std::abs(dxToPred) < bc.align_tol * 0.5f) ? 2.0f : 8.0f;
            addWant((dxToPred > 0) ? 1.f : -1.f, aim_w);
            break;
        }
        default:
            break;
    }

    rt.strafe_timer -= dt;
    if(rt.strafe_timer <= 0.f){
        rt.strafe_target = rng.frand(30.f, W - 30.f);
        rt.strafe_timer  = rng.frand(bc.strafe_period_lo, bc.strafe_period_hi);
    }
    {
        float sd = rt.strafe_target - p2.x;
        if(std::abs(sd) > 10.f) addWant((sd > 0) ? 1.f : -1.f, 1.2f);
    }

    float final_dir = (want_w > 0.001f) ? clampf(want_x / want_w, -1.f, 1.f) : 0.f;
    if(p2.reverseTimer > 0.f) final_dir = -final_dir;
    float sp = cfg.p_speed * ((p2.slowTimer > 0.f) ? 0.5f : 1.f);
    p2.x += final_dir * sp * dt;

    if(cd2 == 0){
        float travel_time_local = std::abs(p1.y - p2.y) / std::max(1.f, cfg.bullet_speed);
        float predict_x_local   = p1.x + p1_vx * travel_time_local * bc.aim_lead;
        predict_x_local = clampf(predict_x_local, 8.f, W - 8.f);
        float dxToPredLocal     = predict_x_local - p2.x;
        bool aligned  = (std::abs(dxToPredLocal) <= bc.align_tol);
        if(p2.spreadTimer > 0.f) aligned = aligned || (std::abs(dxToPredLocal) < bc.align_tol * 3.f);
        bool shielded = (p2.shieldTimer > 0.f);

        bool fired = false;
        if((aligned || shielded) && rng.frand(0.f, 1.f) <= bc.fire_prob){
            fireFn(bullets, p2, 2);
            synth.play(ProceduralSynth::SFX::FIRE, muted ? 0.f : sfxVolume);
            cd2 = computeFireCooldown(cfg.fire_cd_p2_frames, p2.rapidTimer);
            fired = true;
        }
        if(!fired &&
           worst_threat_time > (bc.dodge_zone_y * 0.6f) / std::max(1.f, cfg.bullet_speed) &&
           std::abs(dxToPredLocal) < bc.align_tol * 3.0f &&
           rng.frand(0.f, 1.f) < bc.fire_prob * 0.55f){
            fireFn(bullets, p2, 2);
            synth.play(ProceduralSynth::SFX::FIRE, muted ? 0.f : sfxVolume);
            cd2 = computeFireCooldown(cfg.fire_cd_p2_frames, p2.rapidTimer);
        }
    }

    rt.prev_p1_x = p1.x;

    if(rt.debug){
        rt.debug_pred_impact_x = worst_threat_x;
        rt.debug_pred_intercept_x = p1.x + p1_vx * travel_time * bc.aim_lead;
        rt.debug_best_pu_x = best_pu_score > 0.f ? best_pu_x : -1.f;
        rt.debug_worst_tti = worst_threat_time;
    }
}

} // namespace bot_controller
