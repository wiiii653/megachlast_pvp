#include "RoundRuntime.h"

#include "GameConstants.h"

namespace round_runtime {
namespace {

void resetRoundAndClearScores(Player& p1,
                              Player& p2,
                              int& cd1,
                              int& cd2,
                              float& powerupSpawnTimer,
                              const std::function<void()>& resetRound)
{
    p1.score = 0;
    p2.score = 0;
    resetRound();
    cd1 = 0;
    cd2 = 0;
    powerupSpawnTimer = POWERUP_SPAWN_INTERVAL * POWERUP_FIRST_SPAWN_FACTOR;
}

} // namespace

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
                         const std::function<void(bool)>& playGetReady)
{
    if(scorer == 1){
        p1.score++;
        p1.points += 100;
    } else {
        p2.score++;
        p2.points += 100;
    }

    if(p1.score >= cfg.target_score || p2.score >= cfg.target_score){
        winner = (p1.score >= cfg.target_score) ? 1 : 2;
        state = GameState::GAME_OVER;
        playGetReady(true);
        return;
    }

    int saved1 = p1.score;
    int saved2 = p2.score;
    int points1 = p1.points;
    int points2 = p2.points;
    resetRound();
    p1.score = saved1;
    p2.score = saved2;
    p1.points = points1;
    p2.points = points2;
    victim.invulnTimer = AFTERKILL_INVULN;
    cd1 = 0;
    cd2 = 0;
    powerupSpawnTimer = POWERUP_SPAWN_INTERVAL * POWERUP_FIRST_SPAWN_FACTOR;
    countdownTimer = 2.0f;
    state = GameState::COUNTDOWN;
    playGetReady(false);
}

void startCountdownFromMenu(Player& p1,
                            Player& p2,
                            int& cd1,
                            int& cd2,
                            float& powerupSpawnTimer,
                            float& countdownTimer,
                            GameState& state,
                            const std::function<void()>& resetRound,
                            const std::function<void(bool)>& playGetReady)
{
    resetRoundAndClearScores(p1, p2, cd1, cd2, powerupSpawnTimer, resetRound);
    countdownTimer = 3.f;
    state = GameState::COUNTDOWN;
    playGetReady(false);
}

void openSettingsMenu(GameState& state, int& settingsSel)
{
    state = GameState::SETTINGS;
    settingsSel = 0;
}

void openDonateScreen(GameState& state, float& donateMsgTimer)
{
    state = GameState::DONATE;
    donateMsgTimer = 0.f;
}

void resetPlayingRound(Player& p1,
                       Player& p2,
                       int& cd1,
                       int& cd2,
                       float& powerupSpawnTimer,
                       const std::function<void()>& resetRound)
{
    resetRoundAndClearScores(p1, p2, cd1, cd2, powerupSpawnTimer, resetRound);
}

void rematchCountdownRound(Player& p1,
                           Player& p2,
                           int& cd1,
                           int& cd2,
                           float& powerupSpawnTimer,
                           float& countdownTimer,
                           GameState& state,
                           const std::function<void()>& resetRound,
                           const std::function<void(bool)>& playGetReady)
{
    startCountdownFromMenu(p1,
                           p2,
                           cd1,
                           cd2,
                           powerupSpawnTimer,
                           countdownTimer,
                           state,
                           resetRound,
                           playGetReady);
}

} // namespace round_runtime
