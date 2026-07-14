#pragma once

#include "ArenaLayout.h"
#include "BotController.h"
#include "GameTypes.h"
#include "Random.h"

#include <SFML/Graphics/Color.hpp>

#include <array>

namespace board_runtime {

struct State {
    uint32_t world_seed = 0;
    uint32_t board_seed = 0;
    uint32_t round_number = 0;
    int forced_layout_pick = -1;
    arena_layout::LayoutKind layout_kind = arena_layout::LayoutKind::RANDOM;
    char layout_name[24] = "RANDOM";
};

sf::Color layoutAccentColor(const State& state);

void resetRound(State& state,
                Player& p1,
                Player& p2,
                std::array<Bullet, MAX_BULLETS>& bullets,
                std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                std::array<PowerUp, MAX_POWERUPS>& powerups,
                std::array<Bomb, MAX_BOMBS>& bombs,
                std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers,
                std::array<SpecialStar, MAX_SPECIAL_STARS>& specialStars,
                bot_controller::RuntimeState& botRuntime,
                RNG& rng);

} // namespace board_runtime
