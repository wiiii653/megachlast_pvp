#include "BotController.h"

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
    using bot_controller::computeFireCooldown;

    check(computeFireCooldown(6, 0.f) == 6, "base cooldown unchanged without rapid");
    check(computeFireCooldown(6, 0.1f) == 3, "rapid halves even cooldown");
    check(computeFireCooldown(7, 0.1f) == 3, "rapid halves odd cooldown with floor");
    check(computeFireCooldown(1, 3.f) == 1, "rapid cooldown is clamped to minimum one frame");
    check(computeFireCooldown(0, 0.f) == 1, "base cooldown is clamped to minimum one frame");

    return failures == 0 ? 0 : 1;
}
