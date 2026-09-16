#include "GameUpdateRuntime.h"

#include "ControllerInput.h"
#include "GameConstants.h"
#include "ProceduralSynth.h"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>

#include <algorithm>
#include <cmath>

namespace game_update_runtime {
namespace {

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

} // namespace

void syncPlayingKeyboard(InputState& in)
{
    in.kA = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
    in.kD = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);
    in.kLCtrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LControl);
    in.kZ = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Z);
    in.kLShift = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift);
    in.kLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left);
    in.kRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right);
    in.kRCtrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RControl);
    in.kSlash = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Slash);
    in.kRShift = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::RShift);
}

void syncPlayingControllers(InputState& in, const ControllerSettings& controllers)
{
    auto applyController = [](int joystick, bool& left, bool& right, bool& fire){
        if(joystick < 0 || joystick >= static_cast<int>(sf::Joystick::Count) ||
           !sf::Joystick::isConnected(static_cast<unsigned int>(joystick))) return;

        const unsigned int id = static_cast<unsigned int>(joystick);
        constexpr float deadZone = 35.f;
        const float x = sf::Joystick::hasAxis(id, sf::Joystick::Axis::X)
                          ? sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::X) : 0.f;
        const float povX = sf::Joystick::hasAxis(id, sf::Joystick::Axis::PovX)
                             ? sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::PovX) : 0.f;
        left  = left  || x < -deadZone || povX < -deadZone;
        right = right || x > deadZone || povX > deadZone;
        fire = fire || controller_input::firePressed(id);
    };

    applyController(controllers.p1_joystick, in.kA, in.kD, in.kLCtrl);
    applyController(controllers.p2_joystick, in.kLeft, in.kRight, in.kRCtrl);
}

void fireFromPlayer(std::array<Bullet, MAX_BULLETS>& bullets,
                    const Player& player,
                    int owner,
                    float bulletSpeed,
                    float bulletTtl)
{
    float dir = (owner == 1) ? -1.f : 1.f;
    float originY = player.y + dir * 16.f;

    constexpr float SIN18 = 0.3090f;
    constexpr float COS18 = 0.9511f;
    bool spread = (player.spreadTimer > 0.f);

    auto emitBullet = [&](float vx, float vy){
        for(auto& b : bullets){
            if(!b.alive){
                b.alive = true;
                b.owner = owner;
                b.x = player.x;
                b.y = originY;
                b.vx = vx;
                b.vy = vy;
                b.ttl = bulletTtl;
                return;
            }
        }
    };

    emitBullet(0.f, dir * bulletSpeed);
    if(spread){
        emitBullet(-bulletSpeed * SIN18, dir * bulletSpeed * COS18);
        emitBullet(+bulletSpeed * SIN18, dir * bulletSpeed * COS18);
    }
}

void updateMovementAndFiring(InputState& in,
                             Player& p1,
                             Player& p2,
                             std::array<Bullet, MAX_BULLETS>& bullets,
                             const std::array<PowerUp, MAX_POWERUPS>& powerups,
                             const std::array<Bomb, MAX_BOMBS>& bombs,
                             std::array<Particle, MAX_PARTICLES>& particles,
                             RNG& rng,
                             const Config& cfg,
                             bool botEnabled,
                             BotDifficulty botDifficulty,
                             const BotTuningOverrides& botOverrides,
                             bot_controller::RuntimeState& botRuntime,
                             ProceduralSynth& synth,
                             bool muted,
                             float sfxVolume,
                             int& cd1,
                             int& cd2,
                             float dt,
                             FireFn fireFn,
                             SpawnThrusterFn spawnThrusterFn)
{
    sf::Vector2f d1(0.f, 0.f);
    if(in.kA) d1.x -= (p1.reverseTimer > 0.f ? -1.f : 1.f);
    if(in.kD) d1.x += (p1.reverseTimer > 0.f ? -1.f : 1.f);
    float l1 = std::sqrt(d1.x * d1.x + d1.y * d1.y);
    if(l1 > 0.001f) d1 /= l1;
    float p1SpeedMult = (p1.slowTimer > 0.f) ? 0.5f : 1.f;
    if(p1.overdriveTimer > 0.f) p1SpeedMult *= 1.3f;
    p1.x += d1.x * cfg.p_speed * p1SpeedMult * dt;
    spawnThrusterFn(particles, rng, p1.x, p1.y, 1);

    if((in.kLCtrl || in.kZ || in.kLShift) && cd1 == 0){
        fireFn(bullets, p1, 1);
        synth.play(ProceduralSynth::SFX::FIRE, muted ? 0.f : sfxVolume);
        cd1 = bot_controller::computeFireCooldown(cfg.fire_cd_p1_frames, p1.rapidTimer);
    }
    if(cd1 > 0) cd1--;

    if(botEnabled){
        bot_controller::update(botRuntime, p2, p1, bullets, powerups, bombs, rng, cd2, dt,
                               botDifficulty, cfg, botOverrides, synth,
                               muted, sfxVolume, fireFn);
        spawnThrusterFn(particles, rng, p2.x, p2.y, 2);
    } else {
        sf::Vector2f d2(0.f, 0.f);
        if(in.kLeft) d2.x -= (p2.reverseTimer > 0.f ? -1.f : 1.f);
        if(in.kRight) d2.x += (p2.reverseTimer > 0.f ? -1.f : 1.f);
        float l2 = std::sqrt(d2.x * d2.x + d2.y * d2.y);
        if(l2 > 0.001f) d2 /= l2;
        float p2SpeedMult = (p2.slowTimer > 0.f) ? 0.5f : 1.f;
        if(p2.overdriveTimer > 0.f) p2SpeedMult *= 1.3f;
        p2.x += d2.x * cfg.p_speed * p2SpeedMult * dt;
        spawnThrusterFn(particles, rng, p2.x, p2.y, 2);

        if((in.kRCtrl || in.kSlash || in.kRShift) && cd2 == 0){
            fireFn(bullets, p2, 2);
            synth.play(ProceduralSynth::SFX::FIRE, muted ? 0.f : sfxVolume);
            cd2 = bot_controller::computeFireCooldown(cfg.fire_cd_p2_frames, p2.rapidTimer);
        }
    }
    if(cd2 > 0) cd2--;

    p1.x = clampf(p1.x, 8.f, W - 8.f);
    p2.x = clampf(p2.x, 8.f, W - 8.f);
    p1.y = clampf(p1.y, H * 0.50f, H - 10.f);
    p2.y = clampf(p2.y, 10.f, H * 0.50f);
}

} // namespace game_update_runtime
