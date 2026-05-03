#pragma once

inline constexpr int W     = 640;
inline constexpr int H     = 400;
inline constexpr int SCALE = 4;

inline constexpr int   MAX_BULLETS    = 96;
inline constexpr int   MIRROR_PAIRS   = 20;
inline constexpr float MIRROR_R       = 7.0f;
inline constexpr float MIRROR_PAD     = 12.0f;
inline constexpr float PLAYER_SPAWN_TOP_Y = 58.f;
inline constexpr float PLAYER_SPAWN_BOTTOM_Y = H - PLAYER_SPAWN_TOP_Y;
inline constexpr float SPAWN_ROW_BOMB_EXCLUSION = 35.f;
inline constexpr float SPAWN_INVULN   = 0.35f;
inline constexpr float AFTERKILL_INVULN = 1.5f;
inline constexpr int   MAX_PARTICLES  = 768;
inline constexpr int   NUM_STARS      = 80;

inline constexpr float PI = 3.14159265f;

inline constexpr int   MAX_POWERUPS           = 4;
inline constexpr float POWERUP_R              = 6.f;
inline constexpr float POWERUP_TTL            = 12.f;
inline constexpr float POWERUP_SPAWN_INTERVAL = 7.f;
inline constexpr float POWERUP_FIRST_SPAWN_FACTOR = 0.5f;

inline constexpr int   MAX_BOMBS    = 3;
inline constexpr float BOMB_R       = 8.f;
inline constexpr float BOMB_BLAST_R = 52.f;
inline constexpr float BOMB_PAD     = 34.f;

inline constexpr int   MAX_SPECIAL_STARS = 3;
inline constexpr int   SPECIAL_STAR_PTS  = 50;
inline constexpr float SPECIAL_STAR_R    = 7.f;
inline constexpr float SPECIAL_STAR_SPD  = 22.f;

inline constexpr int MAX_FRAG_FLOATS = 8;

inline constexpr int   BARRIER_BRICKS   = 40;
inline constexpr float BRICK_W          = 12.f;
inline constexpr float BRICK_H          = 8.f;
inline constexpr float BRICK_GAP        = 0.f;
inline constexpr float BARRIER_Y_OFFSET = 32.f;
inline constexpr int   BRICK_MAX_HP     = 3;

inline constexpr int NUM_SPECT = 18;
