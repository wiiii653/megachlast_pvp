#include "BotController.h"

#include "ProceduralSynth.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bot_controller {
namespace {

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

} // namespace

void resetState(RuntimeState& rt)
{
    rt.strafe_target = W * 0.5f;
    rt.strafe_timer = 0.f;
    rt.prev_p1_x = W * 0.5f;
    rt.state = BotState::IDLE;
    rt.state_timer = 0.f;
    rt.debug_pred_impact_x = -1.f;
    rt.debug_pred_intercept_x = -1.f;
    rt.debug_best_pu_x = -1.f;
    rt.debug_worst_tti = 1e9f;
    rt.log_collect_cd = 0.f;
    rt.log_bomb_cd = 0.f;
}

void update(RuntimeState& rt,
            Player& p2,
            const Player& p1,
            std::array<Bullet, MAX_BULLETS>& bullets,
            const std::array<PowerUp, MAX_POWERUPS>& powerups,
            const std::array<Bomb, MAX_BOMBS>& bombs,
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
        case BotDifficulty::HARD:
            bc = {80.f, cfg.hit_r+2.f,  6.f, 1.00f, 1.00f, 0.95f, 0.3f, 0.8f, 65.f, 1.10f, 0.80f, 44.f, 28.f};
            break;
        default:
            bc = {55.f, cfg.hit_r+4.f, 10.f, 0.95f, 0.97f, 0.75f, 0.6f, 1.5f, 48.f, 0.85f, 0.60f, 34.f, 22.f};
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
