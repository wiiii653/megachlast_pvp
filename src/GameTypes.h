#pragma once

#include "GameConstants.h"

#include <cstdint>

struct Config {
    float p_speed        = 120.0f;
    float bullet_speed   = 190.0f;
    float bullet_ttl     = 3.5f;
    float hit_r          = 5.0f;
    float damage         = 10.0f;
    int   target_score   = 8;
    int   fire_cd_p1_frames = 6;
    int   fire_cd_p2_frames = 6;
};

enum class BotDifficulty : uint8_t { EASY=0, MEDIUM=1, HARD=2 };
enum class PerfLevel : uint8_t { HIGH=0, MEDIUM=1, LOW=2, ULTRA=3 };
enum class GameState : uint8_t { MENU=0, PLAYING=1, PAUSED=2, GAME_OVER=3, COUNTDOWN=4, SETTINGS=5, DONATE=6 };

struct Bullet {
    float x=0, y=0;
    float vx=0, vy=0;
    float ttl=0;
    int   owner=0;
    bool  alive=false;
};

struct Player {
    float x=0, y=0;
    float energy=100.f;
    int   score=0;
    int   points=0;
    float flashTimer=0.f;
    float invulnTimer=0.f;
    float glowPhase=0.f;
    float shieldTimer  =0.f;
    float rapidTimer   =0.f;
    float spreadTimer  =0.f;
    float slowTimer    =0.f;
    float reverseTimer =0.f;
};

struct Mirror {
    float x=0, y=0;
    bool  slash=true;
    float hitFlash=0.f;
    bool  alive=true;
};

enum class PowerUpType : uint8_t {
    SHIELD =0,
    RAPID  =1,
    SPREAD =2,
    HEAL   =3,
    CHAOS  =4,
    REVERSE=5
};

struct PowerUp {
    float x=0, y=0, vx=0, phase=0;
    PowerUpType type=PowerUpType::SHIELD;
    float ttl=0.f;
    bool  alive=false;
};

struct Bomb {
    float x=0, y=0;
    float pulsePhase=0.f;
    bool  alive=false;
    int   owner=0;
};

struct SpecialStar {
    float x=0, y=0;
    float vx=0, vy=0;
    float phase=0;
    bool  alive=false;
};

struct FragFloat {
    float x=0, y=0;
    float ttl=0, maxttl=1.3f;
    int   scorer=0;
    bool  alive=false;
};

struct BarrierBrick {
    float x=0, y=0;
    int   hp=BRICK_MAX_HP;
    float hitFlash=0.f;
    bool  alive=true;
};

struct Particle {
    float   x=0, y=0;
    float   vx=0, vy=0;
    float   ttl=0, maxttl=1.f;
    uint8_t r=255, g=200, b=100, a=255;
    float   size=2.f;
    bool    alive=false;
};

struct Star {
    float x=0, y=0, speed=0, bright=0;
    float blinkTimer=0.f;
};

struct SpectStar {
    float x=0.f, y=0.f, ttl=0.f, maxttl=0.f, size=0.f;
    float hue=0.f;
    float rotSpeed=0.f;
    float rot=0.f;
    int   numRays=4;
    uint8_t r=255,g=200,b=140;
    bool alive=false;
};
