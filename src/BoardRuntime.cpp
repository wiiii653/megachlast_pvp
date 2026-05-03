#include "BoardRuntime.h"

#include "GameConstants.h"

namespace board_runtime {
namespace {

void respawn(Player& p, int id)
{
    p.energy = 100.f;
    p.x = W * 0.5f;
    p.y = (id == 1) ? PLAYER_SPAWN_BOTTOM_Y : PLAYER_SPAWN_TOP_Y;
    p.flashTimer = 0.f;
    p.invulnTimer = SPAWN_INVULN;
    p.shieldTimer = 0.f;
    p.rapidTimer = 0.f;
    p.spreadTimer = 0.f;
    p.slowTimer = 0.f;
    p.reverseTimer = 0.f;
}

} // namespace

sf::Color layoutAccentColor(const State& state)
{
    if(state.layout_name[0] == 'Z') return sf::Color(255, 140, 20);
    if(state.layout_name[0] == 'C' && state.layout_name[1] == 'L') return sf::Color(200, 60, 255);
    if(state.layout_name[0] == 'C' && state.layout_name[1] == 'H') return sf::Color(60, 220, 255);
    if(state.layout_name[0] == 'F') return sf::Color(255, 50, 70);
    if(state.layout_name[0] == 'S') return sf::Color(80, 255, 140);
    if(state.layout_name[0] == 'V' && state.layout_name[1] == 'O') return sf::Color(255, 240, 40);
    if(state.layout_name[0] == 'G') return sf::Color(255, 60, 200);
    if(state.layout_name[0] == 'C' && state.layout_name[1] == 'R') return sf::Color(80, 255, 255);
    if(state.layout_name[0] == 'L') return sf::Color(255, 160, 30);
    if(state.layout_name[0] == 'W') return sf::Color(60, 180, 255);
    if(state.layout_name[0] == 'V' && state.layout_name[1] == 'A') return sf::Color(255, 80, 120);
    return sf::Color(200, 200, 255);
}

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
                RNG& rng)
{
    respawn(p1, 1);
    respawn(p2, 2);
    bot_controller::resetState(botRuntime);
    p1.shieldTimer = p1.rapidTimer = p1.spreadTimer = p1.slowTimer = p1.reverseTimer = 0.f;
    p2.shieldTimer = p2.rapidTimer = p2.spreadTimer = p2.slowTimer = p2.reverseTimer = 0.f;
    for(auto& b : bullets) b.alive = false;
    for(auto& u : powerups) u.alive = false;

    ++state.round_number;
    state.board_seed = arena_layout::deriveBoardSeed(state.world_seed, state.round_number);
    arena_layout::genMirrorsSeeded(mirrors, state.board_seed, state.layout_kind, state.layout_name);
    arena_layout::genBarriers(barriers);
    arena_layout::placeBombsForLayout(bombs, mirrors, state.layout_kind, rng);
    arena_layout::spawnSpecialStars(specialStars, rng);
}

} // namespace board_runtime
