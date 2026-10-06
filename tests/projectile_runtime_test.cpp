#include "ProjectileRuntime.h"

#include "ProceduralSynth.h"
#include "RoundRuntime.h"

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

struct Fixture {
    std::array<Bullet, MAX_BULLETS> bullets{};
    std::array<PowerUp, MAX_POWERUPS> powerups{};
    std::array<Bomb, MAX_BOMBS> bombs{};
    std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers{};
    std::array<SpecialStar, MAX_SPECIAL_STARS> specialStars{};
    std::array<Particle, MAX_PARTICLES> particles{};
    std::array<FragFloat, MAX_FRAG_FLOATS> fragFloats{};
    Player p1{100.f, 350.f};
    Player p2{500.f, 50.f};
    Config cfg{};
    RNG rng{0xC0FFEEu};

    Fixture()
    {
        for(auto& mirror : mirrors) mirror.alive = false;
        for(auto& barrier : barriers) barrier.alive = false;
    }
};

void simulate(Fixture& f, ProceduralSynth& synth, const projectile_runtime::Hooks& hooks = {})
{
    projectile_runtime::simulateProjectilesAndCollisions(
        f.bullets, f.p1, f.p2, f.powerups, f.bombs, f.mirrors, f.barriers,
        f.specialStars, f.particles, f.fragFloats, f.rng, f.cfg, synth,
        true, 100.f, hooks, 0.f);
}

void setBullet(Bullet& bullet, float x, float y, int owner)
{
    bullet = {x, y, 0.f, 0.f, 1.f, owner, true};
}

} // namespace

int main()
{
    ProceduralSynth synth;
    synth.init(0x12345678u);

    {
        Fixture f;
        f.powerups[0] = {200.f, 200.f, 0.f, 0.f, PowerUpType::SHIELD, 4.f, true};
        setBullet(f.bullets[0], 200.f, 200.f, 1);
        simulate(f, synth);
        check(!f.powerups[0].alive && !f.bullets[0].alive, "shield collection consumes orb and bullet");
        check(std::fabs(f.p1.shieldTimer - 5.f) < 0.0001f, "shield collection grants five seconds");
    }

    {
        Fixture f;
        f.p1.energy = 70.f;
        f.powerups[0] = {220.f, 200.f, 0.f, 0.f, PowerUpType::HEAL, 4.f, true};
        setBullet(f.bullets[0], 220.f, 200.f, 1);
        simulate(f, synth);
        check(f.p1.energy == 100.f, "heal power-up clamps energy to maximum");
    }

    {
        Fixture f;
        f.powerups[0] = {240.f, 200.f, 0.f, 0.f, PowerUpType::REVERSE, 4.f, true};
        setBullet(f.bullets[0], 240.f, 200.f, 1);
        simulate(f, synth);
        check(std::fabs(f.p2.reverseTimer - 5.f) < 0.0001f, "reverse power-up affects the opponent");
    }

    {
        Fixture f;
        f.p1.x = 320.f;
        f.p1.y = 200.f;
        f.p1.energy = 10.f;
        f.bombs[0] = {320.f, 200.f, 0.f, true, 0};
        setBullet(f.bullets[0], 320.f, 200.f, 2);
        int fragCount = 0;
        int scorer = 0;
        projectile_runtime::Hooks hooks{};
        hooks.applyFragTransition = [&](Player&, int creditedPlayer){
            ++fragCount;
            scorer = creditedPlayer;
        };
        simulate(f, synth, hooks);
        check(!f.bombs[0].alive && !f.bullets[0].alive, "bomb detonation consumes bomb and projectile");
        check(fragCount == 1 && scorer == 2, "bomb kill credits the player who shot the bomb");
    }

    for(int owner : {1, 2}){
        Fixture f;
        Player& shooter = owner == 1 ? f.p1 : f.p2;
        Player& victim = owner == 1 ? f.p2 : f.p1;
        shooter.score = f.cfg.target_score - 1;
        victim.energy = f.cfg.damage;
        MatchState match{};
        (owner == 1 ? match.p1_round_wins : match.p2_round_wins) = f.cfg.rounds_to_win - 1;
        GameState state = GameState::PLAYING;
        int winner = 0, cd1 = 0, cd2 = 0;
        float spawnTimer = 0.f, countdownTimer = 0.f;
        int fragCount = 0;
        for(int i = 0; i < 3; ++i) setBullet(f.bullets[i], victim.x, victim.y, owner);
        setBullet(f.bullets[3], shooter.x, shooter.y, owner == 1 ? 2 : 1);
        projectile_runtime::Hooks hooks{};
        hooks.applyFragTransition = [&](Player& target, int scorer){
            ++fragCount;
            round_runtime::applyFragTransition(f.p1, f.p2, target, scorer, f.cfg, match,
                                               winner, state, cd1, cd2, spawnTimer, countdownTimer,
                                               []{}, [](bool){});
        };
        simulate(f, synth, hooks);
        check(fragCount == 1, "winning frame counts only one death despite overlapping bullets");
        check(shooter.score == f.cfg.target_score && shooter.points == 100,
              "winning frame awards exactly one frag and 100 points");
        check((owner == 1 ? match.p1_round_wins : match.p2_round_wins) == f.cfg.rounds_to_win,
              "winning frame awards exactly one round win");
        check(winner == owner && state == GameState::GAME_OVER, "winning frame ends the match");
        check(shooter.energy == 100.f, "later bullets cannot damage the winner after match end");
    }

    for(int owner : {1, 2}){
        for(bool bombKill : {false, true}){
            Fixture f;
            Player& victim = owner == 1 ? f.p1 : f.p2;
            victim.energy = f.cfg.damage;
            // A returning reflected bullet retains its original owner.
            setBullet(f.bullets[0], victim.x, victim.y, owner);
            setBullet(f.bullets[1], victim.x, victim.y, owner);
            if(bombKill) f.bombs[0] = {victim.x, victim.y, 0.f, true, 0};
            int fragCount = 0, scorer = 0;
            projectile_runtime::Hooks hooks{};
            hooks.applyFragTransition = [&](Player& target, int creditedPlayer){
                ++fragCount;
                scorer = creditedPlayer;
                check(&target == &victim, "self-kill identifies the correct victim");
            };
            simulate(f, synth, hooks);
            check(fragCount == 1, "self-kill stops collision processing after one frag");
            check(scorer == (owner == 1 ? 2 : 1), "bullet and bomb self-kills credit the opponent");
        }
    }

    return failures == 0 ? 0 : 1;
}
