#pragma once

#include "GameTypes.h"

#include <functional>

namespace sf {
class Music;
}

namespace round_runtime {

void applyFragTransition(Player& p1,
                         Player& p2,
                         Player& victim,
                         int scorer,
                         const Config& cfg,
                         int& winner,
                         GameState& state,
                         int& cd1,
                         int& cd2,
                         float& powerupSpawnTimer,
                         float& countdownTimer,
                         const std::function<void()>& resetRound,
                         const std::function<void(bool)>& playGetReady);

void startCountdownFromMenu(Player& p1,
                            Player& p2,
                            int& cd1,
                            int& cd2,
                            float& powerupSpawnTimer,
                            float& countdownTimer,
                            GameState& state,
                            const std::function<void()>& resetRound,
                            const std::function<void(bool)>& playGetReady);

void openSettingsMenu(GameState& state, int& settingsSel);
void openDonateScreen(GameState& state, float& donateMsgTimer);

void pauseGameplay(GameState& state, bool haveIngameMusic, sf::Music& ingameMusic);

void resetPlayingRound(Player& p1,
                       Player& p2,
                       int& cd1,
                       int& cd2,
                       float& powerupSpawnTimer,
                       const std::function<void()>& resetRound);

void resumeGameplay(GameState& state,
                    bool haveIngameMusic,
                    sf::Music& ingameMusic,
                    const std::function<void(sf::Music&)>& applyAudioSettings);

void rematchCountdownRound(Player& p1,
                           Player& p2,
                           int& cd1,
                           int& cd2,
                           float& powerupSpawnTimer,
                           float& countdownTimer,
                           GameState& state,
                           const std::function<void()>& resetRound,
                           const std::function<void(bool)>& playGetReady);

} // namespace round_runtime
