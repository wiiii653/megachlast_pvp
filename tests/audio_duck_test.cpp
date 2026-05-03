#include "AudioDuck.h"

#include <cmath>
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

} // namespace

int main()
{
    using namespace audio_duck;

    State s{};
    check(std::fabs(gain(s) - 1.f) < 0.0001f, "duck gain starts at 1.0");

    trigger(s, 0.5f, 0.20f, 0.30f);
    check(gain(s) <= 0.5001f, "trigger lowers gain immediately by depth");

    for(int i = 0; i < 10; ++i) update(s, 0.01f);
    check(gain(s) <= 0.5001f, "gain stays ducked during hold");

    for(int i = 0; i < 40; ++i) update(s, 0.01f);
    check(gain(s) > 0.5f, "gain starts recovering after hold");

    for(int i = 0; i < 80; ++i) update(s, 0.01f);
    check(std::fabs(gain(s) - 1.f) < 0.001f, "gain returns to unity after release");

    trigger(s, 0.25f, 0.06f, 0.12f);
    trigger(s, 0.60f, 0.12f, 0.40f);
    check(gain(s) <= 0.4001f, "stacked trigger keeps deeper duck");

    for(int i = 0; i < 120; ++i){
        update(s, 0.01f);
        check(gain(s) >= 0.f && gain(s) <= 1.f, "gain stays clamped in range");
    }

    return failures == 0 ? 0 : 1;
}
