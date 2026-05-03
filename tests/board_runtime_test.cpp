#include "BoardRuntime.h"

#include <cstring>
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
    board_runtime::State state{};
    state.world_seed = 0x12345678u;

    std::array<Bullet, MAX_BULLETS> bullets{};
    std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
    std::array<PowerUp, MAX_POWERUPS> powerups{};
    std::array<Bomb, MAX_BOMBS> bombs{};
    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers{};
    std::array<SpecialStar, MAX_SPECIAL_STARS> specialStars{};
    Player p1{}, p2{};
    bot_controller::RuntimeState botRuntime{};
    RNG rng(0xBEEFu);

    board_runtime::resetRound(state,
                              p1,
                              p2,
                              bullets,
                              mirrors,
                              powerups,
                              bombs,
                              barriers,
                              specialStars,
                              botRuntime,
                              rng);

    check(state.round_number == 1u, "resetRound increments round number");
    check(state.board_seed != 0u, "resetRound derives non-zero board seed");
    check(p1.energy == 100.f && p2.energy == 100.f, "resetRound restores player energy");
    check(state.layout_name[0] != '\0', "resetRound writes layout name");

    uint32_t firstSeed = state.board_seed;
    board_runtime::resetRound(state,
                              p1,
                              p2,
                              bullets,
                              mirrors,
                              powerups,
                              bombs,
                              barriers,
                              specialStars,
                              botRuntime,
                              rng);
    check(state.round_number == 2u, "second reset increments round number again");
    check(state.board_seed != firstSeed, "board seed changes between rounds");

    std::strcpy(state.layout_name, "VORTEX");
    auto vortex = board_runtime::layoutAccentColor(state);
    check(vortex.r == 255 && vortex.g == 240 && vortex.b == 40, "layout accent maps VORTEX color");

    std::strcpy(state.layout_name, "RANDOM");
    auto random = board_runtime::layoutAccentColor(state);
    check(random.r == 200 && random.g == 200 && random.b == 255, "layout accent default color");

    return failures == 0 ? 0 : 1;
}
