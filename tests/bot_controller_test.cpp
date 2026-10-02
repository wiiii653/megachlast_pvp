#include "BotController.h"
#include "GameLogic.h"
#include "ProceduralSynth.h"

#include <cmath>
#include <iostream>

namespace {

int failures = 0;
int shots = 0;
ProceduralSynth synth;

void fire(std::array<Bullet, MAX_BULLETS>& bullets, const Player& player, int owner)
{
    if(owner == 2) ++shots;
    for(auto& b : bullets){
        if(b.alive) continue;
        b = {player.x, player.y + (owner == 2 ? 10.f : -10.f), 0.f,
             owner == 2 ? 190.f : -190.f, 3.5f, owner, true};
        break;
    }
}

struct Scenario {
    bot_controller::RuntimeState runtime{};
    Player bot{}, player{};
    std::array<Bullet, MAX_BULLETS> bullets{};
    std::array<PowerUp, MAX_POWERUPS> powerups{};
    std::array<Bomb, MAX_BOMBS> bombs{};
    std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers{};
    BotDifficulty difficulty = BotDifficulty::MEDIUM;
    Config cfg{};
    BotTuningOverrides overrides{};
    RNG rng{42};
    int cooldown = 0;

    Scenario(){
        bot.x = player.x = W * 0.5f;
        bot.y = PLAYER_SPAWN_TOP_Y;
        player.y = playerSpawnBottomY();
        shots = 0;
        for(auto& m : mirrors) m.alive = false;
        for(auto& b : barriers) b.alive = false;
    }

    void update(float dt = 1.f / 120.f){
        bot_controller::update(runtime, bot, player, bullets, powerups, bombs, mirrors, barriers, rng,
                               cooldown, dt, difficulty, cfg, overrides,
                               synth, true, 0.f, fire);
        if(cooldown > 0) --cooldown;
    }
};

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
    synth.init(0x12345678u);
    using bot_controller::computeFireCooldown;

    check(computeFireCooldown(6, 0.f) == 6, "base cooldown unchanged without rapid");
    check(computeFireCooldown(6, 0.1f) == 3, "rapid halves even cooldown");
    check(computeFireCooldown(7, 0.1f) == 3, "rapid halves odd cooldown with floor");
    check(computeFireCooldown(1, 3.f) == 1, "rapid cooldown is clamped to minimum one frame");
    check(computeFireCooldown(0, 0.f) == 1, "base cooldown is clamped to minimum one frame");

    {
        Scenario s;
        s.bullets[0] = {s.bot.x, s.bot.y + 70.f, 0.f, -190.f, 2.f, 1, true};
        s.update();
        check(std::abs(s.bot.x - W * 0.5f) > 0.9f,
              "medium retains the previous hard full-speed escape");
    }
    {
        Scenario s;
        s.bot.x = 10.f;
        s.player.x = 10.f;
        s.bullets[0] = {10.f, s.bot.y + 75.f, 0.f, -190.f, 2.f, 1, true};
        s.update();
        check(s.bot.x > 10.f, "medium escapes inward when threatened at the edge");
    }
    {
        Scenario s;
        s.player.x = 560.f;
        s.bullets[0] = {s.bot.x + 42.f, s.bot.y + 60.f, 0.f, -190.f, 2.f, 1, true};
        s.bullets[1] = {s.bot.x - 42.f, s.bot.y + 60.f, 0.f, -190.f, 2.f, 1, true};
        s.update();
        check(s.bot.x == W * 0.5f, "medium holds a safe gap instead of chasing into crossfire");
    }
    {
        Scenario s;
        s.player.x += 18.f;
        s.update();
        check(shots == 1, "medium fires useful off-center shots within the ship width");
        s.update();
        check(shots == 1, "medium still respects the normal firing cooldown");
    }
    {
        Scenario s;
        s.runtime.debug = true;
        s.update();
        for(int i = 0; i < 30; ++i){ s.player.x += 1.f; s.update(); }
        check(s.runtime.debug_pred_intercept_x >= s.player.x &&
              s.runtime.debug_pred_intercept_x <= s.player.x + 64.f,
              "medium bounds its lead instead of chasing a distant extrapolation");
        for(int i = 0; i < 12; ++i){ s.player.x -= 1.f; s.update(); }
        check(s.runtime.debug_pred_intercept_x < s.player.x,
              "medium updates its aim quickly after a direction reversal");
        bot_controller::resetState(s.runtime);
        check(!s.runtime.tracking_player && s.runtime.attack_phase == 0 && s.runtime.move_timer == 0.f,
              "round reset clears tracking and movement commitments");
    }
    {
        Scenario s;
        s.player.x = 550.f;
        s.bot.energy = 20.f;
        auto& u = s.powerups[0];
        u.alive = true; u.type = PowerUpType::HEAL;
        u.x = s.bot.x; u.y = H * 0.5f; u.ttl = 5.f;
        s.update();
        check(shots == 1, "medium shoots a needed heal even when the player is elsewhere");
    }
    {
        Scenario s;
        s.player.x = 550.f;
        s.bot.slowTimer = 1.f;
        const float oldX = s.bot.x;
        s.update();
        check(std::abs(s.bot.x - oldX) <= s.cfg.p_speed * 0.5f / 120.f + 0.001f,
              "medium obeys the slowed movement speed");
    }
    {
        Scenario s;
        s.bot.energy = 20.f;
        s.runtime.shot_idle = 3.f;
        s.bullets[0] = {s.bot.x, s.bot.y + 70.f, 0.f, -190.f, 2.f, 1, true};
        s.update();
        check(std::abs(s.bot.x - W * 0.5f) > 0.9f,
              "low-health medium keeps evading instead of forcing a damage trade");
    }
    {
        Scenario s;
        s.player.x = 550.f;
        s.bot.reverseTimer = 1.f;
        const float oldX = s.bot.x;
        s.update();
        check(s.bot.x < oldX, "medium remains affected by reversed controls");
    }

    // Repeated open-field encounters isolate movement and firing from arena geometry.
    const char* patternNames[] = {"Stationary", "Strafing", "Reversing", "Corner"};
    for(int pattern = 0; pattern < 4; ++pattern){
        int totalHits = 0, totalShots = 0, hitsDealt = 0;
        for(uint32_t seed = 0; seed < 8; ++seed){
            Scenario s;
            s.rng = RNG(seed);
            s.player.x = pattern == 3 ? 30.f : 320.f;
            s.bot.x = 280.f + seed * 10.f;
            for(int frame = 0; frame < 1200; ++frame){
                if(pattern == 1) s.player.x = 320.f + 180.f * std::sin(frame / 180.f);
                if(pattern == 2) s.player.x = 320.f + 65.f * std::sin(frame / 65.f);
                if(frame % 6 == 0) fire(s.bullets, s.player, 1);
                s.update();
                for(auto& b : s.bullets){
                    if(!b.alive) continue;
                    b.x += b.vx / 120.f; b.y += b.vy / 120.f; b.ttl -= 1.f / 120.f;
                    if(b.ttl <= 0.f || b.y < 0.f || b.y > H){ b.alive = false; continue; }
                    if(b.owner == 1 && game_logic::bulletHitsPlayer(b, s.bot)){
                        ++totalHits; b.alive = false;
                    }
                    if(b.owner == 2 && game_logic::bulletHitsPlayer(b, s.player)){
                        ++hitsDealt; b.alive = false;
                    }
                }
                check(s.bot.x >= 8.f && s.bot.x <= W - 8.f, "medium stays within the board");
            }
            totalShots += shots;
        }
        std::cout << patternNames[pattern] << ": hits taken=" << totalHits
                  << " hits dealt=" << hitsDealt << " shots=" << totalShots << '\n';
        check(totalHits < 80, "medium survives sustained scripted fire across seeds");
        check(totalShots >= 24, "medium keeps firing against stationary, strafing, reversing and corner opponents");
        check(hitsDealt >= 16, "medium pressure produces hits rather than only firing more often");
    }

    return failures == 0 ? 0 : 1;
}
