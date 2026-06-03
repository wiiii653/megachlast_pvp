#pragma once

#include "BotConfig.h"
#include "GameTypes.h"
#include "Random.h"

#include <array>

class ProceduralSynth;

namespace bot_controller {

enum class BotState : uint8_t { IDLE=0, EVADE=1, COLLECT=2, ATTACK=3, DENY=4 };

using FireFn = void(*)(std::array<Bullet, MAX_BULLETS>&, const Player&, int);

struct RuntimeState {
    float strafe_target = W * 0.5f;
    float strafe_timer = 0.f;
    float prev_p1_x = W * 0.5f;
    bool debug = false;
    float debug_pred_impact_x = -1.f;
    float debug_pred_intercept_x = -1.f;
    float debug_best_pu_x = -1.f;
    float debug_worst_tti = 1e9f;
    float log_collect_cd = 0.f;
    float log_bomb_cd = 0.f;
    BotState state = BotState::IDLE;
    float state_timer = 0.f;
};

inline void resetState(RuntimeState& rt)
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

inline int computeFireCooldown(int baseFrames, float rapidTimer)
{
    int clampedBase = (baseFrames < 1) ? 1 : baseFrames;
    if(rapidTimer > 0.f) return (clampedBase / 2 < 1) ? 1 : (clampedBase / 2);
    return clampedBase;
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
            FireFn fireFn);

} // namespace bot_controller
