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

void checkClose(float got, float expected, const char* msg)
{
    if(std::fabs(got - expected) > 0.0001f){
        std::cerr << "FAIL: " << msg << " got=" << got << " expected=" << expected << "\n";
        ++failures;
    }
}

} // namespace

int main()
{
    {
        Player p1{};
        Player p2{};
        p1.score = 1;
        p1.points = 25;
        p2.score = 1;
        p2.points = 50;
        Config cfg{};
        cfg.target_score = 3;
        int winner = 0;
        GameState state = GameState::PLAYING;
        int cd1 = 7;
        int cd2 = 8;
        float powerupSpawnTimer = 99.f;
        float countdownTimer = 0.f;
        int resetCount = 0;
        bool getReadyLoop = true;

        auto resetRound = [&]{
            ++resetCount;
            p1.energy = 100.f;
            p2.energy = 100.f;
            p1.score = 0;
            p2.score = 0;
            p1.points = 0;
            p2.points = 0;
        };
        auto playGetReady = [&](bool loop){ getReadyLoop = loop; };

        round_runtime::applyFragTransition(p1,
                                           p2,
                                           p2,
                                           1,
                                           cfg,
                                           winner,
                                           state,
                                           cd1,
                                           cd2,
                                           powerupSpawnTimer,
                                           countdownTimer,
                                           resetRound,
                                           playGetReady);

        check(resetCount == 1, "non-winning frag resets the round");
        check(p1.score == 2 && p2.score == 1, "frag transition preserves updated scores");
        check(p1.points == 125 && p2.points == 50, "frag transition preserves updated points");
        check(p2.invulnTimer == AFTERKILL_INVULN, "victim gets after-kill invulnerability");
        check(cd1 == 0 && cd2 == 0, "frag transition clears fire cooldowns");
        checkClose(powerupSpawnTimer, POWERUP_SPAWN_INTERVAL * POWERUP_FIRST_SPAWN_FACTOR,
                   "frag transition resets powerup spawn timer");
        checkClose(countdownTimer, 2.f, "frag transition starts short countdown");
        check(state == GameState::COUNTDOWN, "frag transition enters countdown");
        check(winner == 0, "non-winning frag leaves winner unset");
        check(!getReadyLoop, "non-winning frag plays non-looping get-ready track");
    }

    {
        Player p1{};
        Player p2{};
        p1.score = 2;
        Config cfg{};
        cfg.target_score = 3;
        int winner = 0;
        GameState state = GameState::PLAYING;
        int cd1 = 3;
        int cd2 = 4;
        float powerupSpawnTimer = 1.f;
        float countdownTimer = 0.f;
        int resetCount = 0;
        bool getReadyLoop = false;

        round_runtime::applyFragTransition(p1,
                                           p2,
                                           p2,
                                           1,
                                           cfg,
                                           winner,
                                           state,
                                           cd1,
                                           cd2,
                                           powerupSpawnTimer,
                                           countdownTimer,
                                           [&]{ ++resetCount; },
                                           [&](bool loop){ getReadyLoop = loop; });

        check(resetCount == 0, "winning frag does not reset the board");
        check(p1.score == 3, "winning frag increments score");
        check(p1.points == 100, "winning frag increments points");
        check(winner == 1, "winning frag sets winner");
        check(state == GameState::GAME_OVER, "winning frag enters game over");
        check(getReadyLoop, "game over plays looping get-ready track");
        check(cd1 == 3 && cd2 == 4, "winning frag leaves cooldowns untouched");
    }

    {
        Player p1{};
        Player p2{};
        p1.score = 4;
        p2.score = 5;
        int cd1 = 2;
        int cd2 = 3;
        float powerupSpawnTimer = 12.f;
        int resetCount = 0;

        round_runtime::resetPlayingRound(p1, p2, cd1, cd2, powerupSpawnTimer, [&]{ ++resetCount; });

        check(resetCount == 1, "manual reset invokes reset callback");
        check(p1.score == 0 && p2.score == 0, "manual reset clears scores");
        check(cd1 == 0 && cd2 == 0, "manual reset clears cooldowns");
        checkClose(powerupSpawnTimer, POWERUP_SPAWN_INTERVAL * POWERUP_FIRST_SPAWN_FACTOR,
                   "manual reset resets powerup spawn timer");
    }

    return failures == 0 ? 0 : 1;
}
