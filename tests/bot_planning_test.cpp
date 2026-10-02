#include "ArenaLayout.h"
#include "BotController.h"
#include "FrameRuntime.h"
#include "ProjectileRuntime.h"
#include "ProceduralSynth.h"

#include <cmath>
#include <iostream>

namespace {
int failures = 0, shots = 0;
ProceduralSynth synth;

void check(bool ok, const char* message)
{
    if(!ok){ std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

void fire(std::array<Bullet, MAX_BULLETS>& bullets, const Player& p, int owner)
{
    ++shots;
    auto emit = [&](float vx, float vy){
        for(auto& b : bullets){
            if(b.alive) continue;
            b = {p.x, p.y + 16.f, vx, vy, 3.5f, owner, true};
            break;
        }
    };
    emit(0.f, 190.f);
    if(p.spreadTimer > 0.f){
        emit(-190.f * 0.3090f, 190.f * 0.9511f);
        emit(190.f * 0.3090f, 190.f * 0.9511f);
    }
}

struct Scenario {
    bot_controller::RuntimeState runtime{};
    Player bot{100.f, PLAYER_SPAWN_TOP_Y}, player{200.f, playerSpawnBottomY()};
    std::array<Bullet, MAX_BULLETS> bullets{};
    std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers{};
    std::array<PowerUp, MAX_POWERUPS> powerups{};
    std::array<Bomb, MAX_BOMBS> bombs{};
    std::array<SpecialStar, MAX_SPECIAL_STARS> stars{};
    std::array<Particle, MAX_PARTICLES> particles{};
    std::array<FragFloat, MAX_FRAG_FLOATS> floats{};
    Config cfg{};
    BotTuningOverrides overrides{};
    RNG rng{42};
    int cooldown = 0;

    Scenario(){
        for(auto& m : mirrors) m.alive = false;
        for(auto& b : barriers) b.alive = false;
        shots = 0;
    }

    void think(BotDifficulty difficulty = BotDifficulty::HARD, float dt = 1.f / 120.f){
        bot_controller::update(runtime, bot, player, bullets, powerups, bombs, mirrors, barriers,
                               rng, cooldown, dt, difficulty, cfg, overrides, synth, true, 0.f, fire);
        if(cooldown > 0) --cooldown;
    }

    void simulate(float dt = 1.f / 120.f){
        projectile_runtime::simulateProjectilesAndCollisions(bullets, player, bot, powerups, bombs,
            mirrors, barriers, stars, particles, floats, rng, cfg, synth, true, 0.f, {}, dt);
    }

    void bankPair(){
        mirrors[0] = {100.f, 140.f, false, 0.f, true};
        mirrors[1] = {200.f, 136.f, false, 0.f, true};
    }
};
}

int main()
{
    synth.init(42);
    for(float fps : {60.f, 120.f, 240.f}){
        Scenario s;
        s.bankPair();
        s.think(BotDifficulty::HARD, 1.f / fps);
        check(shots == 1, "hard fires an off-axis bank shot");
        check(s.runtime.plan_bounces == 2, "hard chooses a two-mirror route");
        check(!s.mirrors[0].slash && !s.mirrors[1].slash, "planning does not rotate live mirrors");
        for(int frame = 0; frame < static_cast<int>(fps * 3.f); ++frame) s.simulate(1.f / fps);
        check(s.player.energy < 100.f, "planned bank shot hits in the real projectile simulation");
        check(s.bot.energy == 100.f, "planned bank shot avoids its shooter");
    }
    {
        Scenario s;
        s.bankPair();
        s.think(BotDifficulty::MEDIUM);
        check(shots == 0, "medium retains the previous straight-path aiming behavior");
    }
    {
        Scenario s;
        s.bot.spreadTimer = 5.f;
        s.player.x = 190.f;
        s.think();
        check(shots == 1, "hard recognizes a useful spread side pellet");
        for(int frame = 0; frame < 360; ++frame) s.simulate();
        check(s.player.energy < 100.f, "planned spread shot hits in the real projectile simulation");
    }
    {
        Scenario s;
        s.bot.x = 300.f; s.player.x = 320.f;
        s.runtime.tracking_player = true;
        s.runtime.tracked_vx = -100.f;
        s.runtime.prev_p1_x = s.player.x + 1.f;
        s.think();
        check(shots == 1, "hard covers the current position when led aim could miss after a reversal");
    }
    {
        Scenario s;
        s.bot.x = 56.f; s.player.x = 30.f;
        s.runtime.shot_idle = 3.f;
        s.bullets[0] = {30.f, s.bot.y + 50.f, 0.f, -190.f, 2.f, 1, true};
        for(int frame = 0; frame < 8; ++frame){ s.think(); s.simulate(); }
        check(shots > 0, "hard contests a corner firing position instead of stalling just outside it");
    }
    {
        Scenario s;
        s.runtime.tracking_player = true;
        s.runtime.prev_p1_x = s.player.x + 1.f;
        s.runtime.last_motion_direction = 1.f;
        s.runtime.turn_age = 2.f;
        s.think();
        check(s.runtime.reversal_timer > 2.f, "hard recognizes repeated turns and shortens its aiming lead");
        bot_controller::resetState(s.runtime);
        check(s.runtime.reversal_timer == 0.f && s.runtime.last_motion_direction == 0.f,
              "round reset clears opponent turn history");
    }
    {
        Scenario s;
        s.bot.x = 53.f; s.player.x = 30.f;
        s.runtime.pressure_timer = 1.f / 120.f;
        s.cooldown = 6;
        s.think();
        check(s.runtime.retreat_timer > 0.f && s.bot.x > 53.f,
              "hard moves inward after a corner burst before committing again");
        bot_controller::resetState(s.runtime);
        check(s.runtime.retreat_timer == 0.f, "round reset clears post-burst retreat");
    }
    {
        Scenario s;
        s.bot.x = 52.f; s.player.x = 30.f;
        s.bot.energy = 10.f;
        s.runtime.pressure_timer = 0.15f;
        s.bullets[0] = {30.f, s.bot.y + 50.f, 0.f, -190.f, 2.f, 1, true};
        s.think();
        check(s.bot.x > 52.f, "low-health hard escapes instead of finishing a corner pressure burst");
        bot_controller::resetState(s.runtime);
        check(s.runtime.pressure_timer == 0.f, "round reset clears corner burst commitment");
    }
    {
        Scenario s;
        s.player.x = s.bot.x;
        s.mirrors[0] = {100.f, 140.f, false, 0.f, true};
        s.mirrors[1] = {120.f, 136.f, true, 0.f, true};
        s.think();
        check(shots == 0, "hard rejects a route that returns into its own ship");
    }
    {
        Scenario s;
        s.bankPair();
        s.think();
        const int before = shots;
        for(auto& b : s.bullets) b.alive = false;
        s.mirrors[0].slash = true;
        s.cooldown = 0;
        s.think();
        check(shots == before + 1 && s.mirrors[0].slash,
              "hard rechecks a rotated route and chooses a mirror preparation shot");
        for(int frame = 0; frame < 360; ++frame) s.simulate();
        check(s.player.energy == 100.f && s.bot.energy == 100.f,
              "mirror preparation rotates the board without an unsafe return");
        s.cooldown = 0;
        s.think();
        for(int frame = 0; frame < 360; ++frame) s.simulate();
        check(s.player.energy < 100.f, "hard follows a preparation shot with a working bank shot");
    }
    {
        Scenario s;
        s.player.x = s.bot.x;
        s.barriers[0] = {100.f, 90.f, 3, 0.f, true};
        for(int shot = 0; shot < 4; ++shot){
            s.cooldown = 0;
            s.think();
            if(shot == 0) check(s.barriers[0].hp == 3, "planning does not damage live barriers");
            for(int frame = 0; frame < 360; ++frame) s.simulate();
            if(shot < 3) check(s.player.energy == 100.f, "barrier blocks shots until broken");
        }
        check(!s.barriers[0].alive && s.player.energy < 100.f,
              "hard breaks a useful opening and then hits through it");
    }
    {
        Scenario s;
        s.player.x = s.bot.x;
        s.player.shieldTimer = 12.f;
        s.barriers[0] = {100.f, 90.f, 3, 0.f, true};
        s.think();
        check(shots == 1, "hard prepares a barrier opening while the opponent is shielded");
    }
    {
        Scenario s;
        s.bot.x = s.player.x = 200.f;
        s.overrides.fire_prob = 0.f;
        s.bullets[0] = {100.f, 120.f, 190.f, 0.f, 2.f, 1, true};
        s.mirrors[0] = {200.f, 120.f, true, 0.f, true};
        s.think();
        check(s.bot.x > 200.f, "hard anticipates a horizontal bullet turning toward its ship");
        for(int frame = 0; frame < 120; ++frame){ s.simulate(); s.think(); }
        check(s.bot.energy == 100.f, "hard survives the predicted ricochet");
    }
    {
        Scenario s;
        s.bot.x = s.player.x = 320.f;
        s.overrides.fire_prob = 0.f;
        s.bullets[0] = {320.f, 160.f, 0.f, -190.f, 2.f, 1, true};
        s.barriers[0] = {320.f, 90.f, 3, 0.f, true};
        s.think();
        check(s.runtime.state != bot_controller::BotState::EVADE,
              "hard recognizes cover that stops an incoming shot");
        s.barriers[0].hp = 1;
        s.bullets[1] = {320.f, 175.f, 0.f, -190.f, 2.f, 1, true};
        s.think();
        check(s.runtime.state == bot_controller::BotState::EVADE,
              "hard predicts the follow-up shot after cover breaks");
    }
    for(const auto aspect : {ScreenAspect::Ratio16x10, ScreenAspect::Ratio16x9}){
        setCanvasAspect(aspect);
        for(int preset = 3; preset <= 5; ++preset){
            for(uint32_t sample = 0; sample < 4; ++sample){
                Scenario s;
                s.bot.x = 320.f;
                s.player.x = 240.f + sample * 48.f;
                arena_layout::LayoutKind kind{};
                char name[24]{};
                arena_layout::genMirrorsSeeded(s.mirrors, ((sample * 12u + preset) << 5) | sample, kind, name);
                arena_layout::genBarriers(s.barriers);
                for(int frame = 0; frame < 1200 && s.player.energy == 100.f; ++frame){
                    s.think(); s.simulate();
                }
                if(s.player.energy == 100.f)
                    std::cerr << "Arena failure H=" << H << " preset=" << preset << " sample=" << sample
                              << " bot x=" << s.bot.x << " plan=" << s.runtime.plan_x << " score=" << s.runtime.plan_score << '\n';
                check(s.player.energy < 100.f, "hard finds a working attack through seeded mirrors and barriers");
            }
        }
    }
    setCanvasAspect(ScreenAspect::Ratio16x10);
    // Sustained encounters keep health full to compare decisions without round resets.
    setCanvasAspect(ScreenAspect::Ratio16x9);
    const char* patterns[] = {"Strafing", "Reversing", "Corner"};
    for(int pattern = 0; pattern < 3; ++pattern){
        int dealt = 0, taken = 0;
        for(uint32_t seed = 0; seed < 8; ++seed){
            Scenario s;
            s.cfg.p_speed = 100.f; s.cfg.damage = 5.f;
            s.rng = RNG(seed);
            s.player.x = pattern == 2 ? 30.f : 320.f;
            s.bot.x = 280.f + seed * 10.f;
            for(int frame = 0; frame < 1200; ++frame){
                frame_runtime::updatePlayerTimers(GameState::PLAYING, s.player, s.bot, 1.f / 120.f);
                if(pattern == 0) s.player.x = 320.f + 180.f * std::sin(frame / 216.f);
                if(pattern == 1) s.player.x = 320.f + 65.f * std::sin(frame / 78.f);
                if(frame % 6 == 0){
                    for(auto& b : s.bullets){
                        if(b.alive) continue;
                        b = {s.player.x, s.player.y - 16.f, 0.f, -190.f, 3.5f, 1, true};
                        break;
                    }
                }
                s.think();
                const float botEnergy = s.bot.energy, playerEnergy = s.player.energy;
                s.simulate();
                taken += static_cast<int>(std::lround((botEnergy - s.bot.energy) / s.cfg.damage));
                dealt += static_cast<int>(std::lround((playerEnergy - s.player.energy) / s.cfg.damage));
                s.bot.energy = s.player.energy = 100.f;
            }
        }
        std::cout << patterns[pattern] << ": hard hits dealt=" << dealt << " taken=" << taken << '\n';
        check(dealt >= (pattern == 2 ? 72 : (pattern == 1 ? 125 : 100)),
              "hard sustains useful attacks against movement and corner camping");
        if(pattern == 2)
            check(taken <= 160 && dealt * 2 >= taken, "corner bursts improve damage trades without unlimited exposure");
        else
            check(taken <= 8, "hard preserves evasion against moving opponents");
    }
    setCanvasAspect(ScreenAspect::Ratio16x10);
    return failures ? 1 : 0;
}
