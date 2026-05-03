#pragma once

#include <algorithm>

namespace audio_duck {

struct State {
    float depth = 0.f;
    float hold = 0.f;
    float release = 0.22f;
    float gain = 1.f;
};

inline float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

inline void trigger(State& s, float depth, float hold, float release)
{
    s.depth = std::max(s.depth, clampf(depth, 0.f, 0.8f));
    s.hold = std::max(s.hold, std::max(0.02f, hold));
    s.release = std::max(s.release, std::max(0.05f, release));
    float targetGain = 1.f - s.depth;
    s.gain = std::min(s.gain, targetGain);
}

inline void update(State& s, float dt)
{
    float clampedDt = clampf(dt, 0.f, 0.05f);
    if(s.hold > 0.f){
        s.hold = std::max(0.f, s.hold - clampedDt);
        float targetGain = 1.f - s.depth;
        s.gain = std::min(s.gain, targetGain);
        return;
    }

    if(s.gain < 1.f){
        float rel = std::max(0.05f, s.release);
        s.gain = std::min(1.f, s.gain + clampedDt / rel);
    }

    if(s.gain >= 0.999f){
        s.gain = 1.f;
        s.depth = 0.f;
        s.release = 0.22f;
    }
}

inline float gain(const State& s)
{
    return clampf(s.gain, 0.f, 1.f);
}

} // namespace audio_duck
