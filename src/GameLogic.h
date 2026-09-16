#pragma once

#include "GameTypes.h"

#include <algorithm>
#include <cmath>

namespace game_logic {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;
};

inline Vec2 reflectMirrorVelocity(float vx, float vy, bool slash)
{
    if(slash) return {-vy, -vx};
    return {vy, vx};
}

inline bool bulletHitsPlayer(float bulletX, float bulletY, float playerX, float playerY,
                             float hitRadius)
{
    float hr = std::max(1.f, hitRadius);
    float halfW = 19.f + hr;
    float halfH =  7.f + hr;
    float core  =  5.f + hr;

    float dx = bulletX - playerX;
    float dy = bulletY - playerY;
    float adx = std::abs(dx);
    float ady = std::abs(dy);
    if(adx > halfW || ady > halfH) return false;
    if(adx > core && ady > core) return false;
    return true;
}

inline bool bulletHitsPlayer(float bulletX, float bulletY, float playerX, float playerY)
{
    return bulletHitsPlayer(bulletX, bulletY, playerX, playerY, 5.f);
}

inline bool bulletHitsPlayer(const Bullet& bullet, const Player& player)
{
    return bulletHitsPlayer(bullet.x, bullet.y, player.x, player.y);
}

inline bool bulletHitsMirror(const Bullet& bullet, const Mirror& mirror)
{
    if(!mirror.alive) return false;
    float dx = bullet.x - mirror.x;
    float dy = bullet.y - mirror.y;
    return dx * dx + dy * dy <= MIRROR_R * MIRROR_R;
}

} // namespace game_logic
