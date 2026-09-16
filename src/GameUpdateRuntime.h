#pragma once

#include "BotConfig.h"
#include "BotController.h"
#include "GameTypes.h"
#include "Random.h"

#include <array>
#include <functional>

class ProceduralSynth;

namespace game_update_runtime {

struct InputState {
    bool focused = true;
    bool kA = false;
    bool kD = false;
    bool kLCtrl = false;
    bool kZ = false;
    bool kLShift = false;
    bool kLeft = false;
    bool kRight = false;
    bool kRCtrl = false;
    bool kSlash = false;
    bool kRShift = false;
};

using FireFn = void(*)(std::array<Bullet, MAX_BULLETS>&, const Player&, int);
using SpawnThrusterFn = std::function<void(std::array<Particle, MAX_PARTICLES>&, RNG&, float, float, int)>;

void syncPlayingKeyboard(InputState& in);
void syncPlayingControllers(InputState& in, const ControllerSettings& controllers);

void setFocus(InputState& in, bool focused);

void fireFromPlayer(std::array<Bullet, MAX_BULLETS>& bullets,
                    const Player& player,
                    int owner,
                    float bulletSpeed,
                    float bulletTtl);

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
                             SpawnThrusterFn spawnThrusterFn);

} // namespace game_update_runtime
