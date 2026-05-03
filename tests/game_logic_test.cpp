#include "GameLogic.h"

#include <cmath>
#include <iostream>

namespace {

int failures = 0;

void checkClose(float got, float expected, const char* msg)
{
    if(std::fabs(got - expected) > 0.0001f){
        std::cerr << "FAIL: " << msg << " got=" << got << " expected=" << expected << "\n";
        ++failures;
    }
}

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
    using game_logic::reflectMirrorVelocity;

    auto slashUp = reflectMirrorVelocity(0.f, -10.f, true);
    checkClose(slashUp.x, 10.f, "slash mirror redirects upward shot right");
    checkClose(slashUp.y, 0.f, "slash mirror clears upward vertical velocity");

    auto slashRight = reflectMirrorVelocity(10.f, 0.f, true);
    checkClose(slashRight.x, 0.f, "slash mirror clears rightward horizontal velocity");
    checkClose(slashRight.y, -10.f, "slash mirror redirects rightward shot up");

    auto backslashUp = reflectMirrorVelocity(0.f, -10.f, false);
    checkClose(backslashUp.x, -10.f, "backslash mirror redirects upward shot left");
    checkClose(backslashUp.y, 0.f, "backslash mirror clears upward vertical velocity");

    auto backslashRight = reflectMirrorVelocity(10.f, 0.f, false);
    checkClose(backslashRight.x, 0.f, "backslash mirror clears rightward horizontal velocity");
    checkClose(backslashRight.y, 10.f, "backslash mirror redirects rightward shot down");

    Player player{};
    player.x = 100.f;
    player.y = 50.f;
    Bullet bullet{};

    bullet.x = 100.f;
    bullet.y = 50.f;
    check(game_logic::bulletHitsPlayer(bullet, player), "player hit test accepts center body hit");

    bullet.x = 123.9f;
    bullet.y = 50.f;
    check(game_logic::bulletHitsPlayer(bullet, player), "player hit test accepts wide wing hit");

    bullet.x = 125.f;
    bullet.y = 50.f;
    check(!game_logic::bulletHitsPlayer(bullet, player), "player hit test rejects outside horizontal bounds");

    bullet.x = 111.f;
    bullet.y = 61.f;
    check(!game_logic::bulletHitsPlayer(bullet, player), "player hit test rejects trimmed corner");

    bullet.x = 109.f;
    bullet.y = 61.f;
    check(game_logic::bulletHitsPlayer(bullet, player), "player hit test accepts tall body edge");

    bullet.x = 126.f;
    bullet.y = 50.f;
    check(!game_logic::bulletHitsPlayer(bullet.x, bullet.y, player.x, player.y, 5.f),
          "custom hit radius keeps default body width when set to 5");
    check(game_logic::bulletHitsPlayer(bullet.x, bullet.y, player.x, player.y, 7.f),
          "custom hit radius expands player hitbox when increased");

    Mirror mirror{};
    mirror.x = 200.f;
    mirror.y = 100.f;
    mirror.alive = true;
    bullet.x = 200.f + MIRROR_R;
    bullet.y = 100.f;
    check(game_logic::bulletHitsMirror(bullet, mirror), "mirror hit test accepts radius edge");

    bullet.x = 200.f + MIRROR_R + 0.01f;
    bullet.y = 100.f;
    check(!game_logic::bulletHitsMirror(bullet, mirror), "mirror hit test rejects outside radius");

    mirror.alive = false;
    bullet.x = 200.f;
    bullet.y = 100.f;
    check(!game_logic::bulletHitsMirror(bullet, mirror), "mirror hit test rejects dead mirrors");

    return failures == 0 ? 0 : 1;
}
