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

    setCanvasAspect(ScreenAspect::Ratio16x9);
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
    check(W == 640 && H == 360, "16:9 aspect selects a 640x360 arena");
    check(p1.y == 302.f && p2.y == 58.f, "16:9 aspect uses matching player spawn rows");
    setCanvasAspect(ScreenAspect::Ratio16x10);

    state.forced_layout_pick = arenaPresetLayoutPick(ArenaPreset::FORTRESS);
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
    check(state.layout_kind == arena_layout::LayoutKind::FORTRESS,
          "forced arena preset selects its intended layout");

    std::strcpy(state.layout_name, "VORTEX");
    auto vortex = board_runtime::layoutAccentColor(state);
    check(vortex.r == 255 && vortex.g == 240 && vortex.b == 40, "layout accent maps VORTEX color");

    std::strcpy(state.layout_name, "RANDOM");
    auto random = board_runtime::layoutAccentColor(state);
    check(random.r == 200 && random.g == 200 && random.b == 255, "layout accent default color");

    return failures == 0 ? 0 : 1;
}
