#include "BoardRuntime.h"
#include "FrameRuntime.h"
#include "ProceduralSynth.h"
#include "RoundRuntime.h"
#include "SimulationRuntime.h"

#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* message)
{
    if(!ok){ std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

void fire(std::array<Bullet, MAX_BULLETS>& bullets, const Player& player, int owner)
{
    const Config cfg{};
    game_update_runtime::fireFromPlayer(bullets, player, owner, cfg.bullet_speed, cfg.bullet_ttl);
}
} // namespace

int main()
{
    Player p1{}, p2{};
    std::array<Bullet, MAX_BULLETS> bullets{};
    std::array<PowerUp, MAX_POWERUPS> powerups{};
    std::array<Bomb, MAX_BOMBS> bombs{};
    std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers{};
    std::array<SpecialStar, MAX_SPECIAL_STARS> stars{};
    std::array<Particle, MAX_PARTICLES> particles{};
    std::array<FragFloat, MAX_FRAG_FLOATS> floats{};
    RNG rng{0xC0FFEEu};
    Config cfg{};
    cfg.target_score = 3;
    cfg.rounds_to_win = 2;
    MatchState match{};
    board_runtime::State board{};
    bot_controller::RuntimeState bot{};
    BotTuningOverrides overrides{};
    game_update_runtime::InputState input{};
    ProceduralSynth synth;
    synth.init(0x12345678u);
    int cd1 = 0, cd2 = 0, winner = 0, deaths = 0;
    float spawnTimer = 0.f, countdown = 0.f, flash = 0.f;
    GameState state = GameState::MENU;
    auto reset = [&]{
        board_runtime::resetRound(board, p1, p2, bullets, mirrors, powerups, bombs,
                                  barriers, stars, bot, rng);
        // Use a clear firing lane to make this match deterministic.
        for(auto& m : mirrors) m.alive = false;
        for(auto& b : barriers) b.alive = false;
        for(auto& b : bombs) b.alive = false;
        for(auto& s : stars) s.alive = false;
    };
    projectile_runtime::Hooks hooks{};
    hooks.applyFragTransition = [&](Player& victim, int scorer){
        ++deaths;
        round_runtime::applyFragTransition(p1, p2, victim, scorer, cfg, match, winner,
                                           state, cd1, cd2, spawnTimer, countdown, reset, [](bool){});
    };
    simulation_runtime::FrameContext context{};
    context.input = &input;
    context.p1 = &p1; context.p2 = &p2;
    context.bullets = &bullets; context.powerups = &powerups;
    context.bombs = &bombs; context.mirrors = &mirrors; context.barriers = &barriers;
    context.specialStars = &stars; context.particles = &particles; context.fragFloats = &floats;
    context.rng = &rng; context.cfg = &cfg;
    context.botEnabled = false;
    context.botOverrides = &overrides; context.botRuntime = &bot;
    context.synth = &synth; context.muted = true;
    context.cd1 = &cd1; context.cd2 = &cd2;
    context.dt = 1.f / 60.f; context.powerupSpawnTimer = &spawnTimer;
    context.fireFn = fire;
    context.spawnThrusterFn = [](auto&, auto&, float, float, int){};
    context.projectileHooks = &hooks;
    auto tick = [&]{
        frame_runtime::updateCountdownAndFightFlash(state, countdown, flash, context.dt, []{}, match.starting_round);
        frame_runtime::updatePlayerTimers(state, p1, p2, context.dt);
        context.state = state;
        simulation_runtime::simulatePlayingFrame(context);
    };
    round_runtime::startCountdownFromMenu(p1, p2, match, cd1, cd2, spawnTimer,
                                          countdown, state, reset, [](bool){});
    int expected1 = 0, expected2 = 0;
    // Both humans score, both win a round, then P1 wins the deciding round.
    for(int scorer : {2, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1}){
        input = {};
        const bool newRound = match.starting_round;
        for(int i = 0; i < 600 && state == GameState::COUNTDOWN; ++i) tick();
        check(state == GameState::PLAYING, "PvP countdown returns to playing");
        check(newRound ? flash > 1.f : flash == 0.f,
              "full match announces each new round but not ordinary respawns");
        spawnTimer = 1000.f;
        input.kLCtrl = scorer == 1;
        input.kRCtrl = scorer == 2;
        const int previousDeaths = deaths;
        for(int i = 0; i < 1200 && deaths == previousDeaths; ++i) tick();
        check(deaths == previousDeaths + 1, "human firing and travelling bullets produce one kill");
        if(scorer == 1) ++expected1; else ++expected2;
        std::cout << "PvP kill " << deaths << ": frags " << p1.frags << ':' << p2.frags
                  << ", round " << p1.score << ':' << p2.score
                  << ", wins " << match.p1_round_wins << ':' << match.p2_round_wins << '\n';
        check(p1.frags == expected1 && p2.frags == expected2, "match frags never reset between rounds");
        check(p1.points == expected1 * 100 && p2.points == expected2 * 100, "kill points persist across rounds");
    }
    check(state == GameState::GAME_OVER && winner == 1, "full PvP match ends with P1 winning");
    check(match.p1_round_wins == 2 && match.p2_round_wins == 1, "three-round match counts round wins correctly");
    check(p1.frags == 7 && p2.frags == 5, "final HUD frags include kills from all three rounds");
    input.kLCtrl = input.kRCtrl = true;
    for(int i = 0; i < 600; ++i) tick();
    check(state == GameState::GAME_OVER && winner == 1, "holding fire keeps PvP at game over");
    check(p1.frags == 7 && p2.frags == 5 && deaths == 12, "game over freezes combat and match scores");
    round_runtime::rematchCountdownRound(p1, p2, match, cd1, cd2, spawnTimer,
                                         countdown, state, reset, [](bool){});
    check(p1.frags == 0 && p2.frags == 0, "rematch clears match frags");
    p1.frags = 4; p2.frags = 2;
    round_runtime::resetPlayingRound(p1, p2, match, cd1, cd2, spawnTimer, reset);
    check(p1.frags == 0 && p2.frags == 0, "manual score reset clears match frags");
    return failures == 0 ? 0 : 1;
}
