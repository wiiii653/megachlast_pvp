#pragma once

#include <algorithm>

namespace fx_governor {

struct State {
    int level = 0;
    float overload = 0.f;
    float dtSmooth = 1.f / 60.f;
    float levelCooldown = 0.f;
};

inline float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

inline float levelScale(int level)
{
    switch(level){
        case 0:  return 1.f;
        case 1:  return 0.78f;
        case 2:  return 0.58f;
        default: return 0.40f;
    }
}

inline void update(State& s, float dt, bool playing, int spawnBudget, int spawnUsed)
{
    float clampedDt = clampf(dt, 0.f, 0.05f);
    s.dtSmooth = s.dtSmooth + (clampedDt - s.dtSmooth) * 0.12f;
    if(s.levelCooldown > 0.f) s.levelCooldown = std::max(0.f, s.levelCooldown - clampedDt);

    if(!playing){
        s.overload = std::max(0.f, s.overload - clampedDt * 2.0f);
        if(s.level > 0 && s.overload < 0.15f && s.levelCooldown <= 0.f){
            s.level--;
            s.levelCooldown = 0.18f;
        }
        return;
    }

    constexpr float TARGET_DT = 1.f / 60.f;
    float over = (s.dtSmooth - TARGET_DT) / TARGET_DT;
    if(over > 0.03f) s.overload += clampedDt * (0.6f + over * 9.f);
    else             s.overload -= clampedDt * 1.4f;

    if(spawnBudget > 0){
        float sat = static_cast<float>(spawnUsed) / static_cast<float>(spawnBudget);
        if(sat > 0.92f) s.overload += clampedDt * (sat - 0.92f) * 16.f;
    }

    s.overload = clampf(s.overload, 0.f, 4.f);

    if(s.level < 3 && s.levelCooldown <= 0.f){
        static const float upThr[3] = {0.65f, 1.35f, 2.20f};
        if(s.overload > upThr[s.level]){
            s.level++;
            s.levelCooldown = 0.10f;
            return;
        }
    }
    if(s.level > 0 && s.levelCooldown <= 0.f){
        static const float downThr[4] = {0.f, 0.28f, 0.92f, 1.65f};
        if(s.overload < downThr[s.level]){
            s.level--;
            s.levelCooldown = 0.22f;
        }
    }
}

} // namespace fx_governor
