// ProceduralSynth.h — POKEY-soul procedural SFX engine for Megachlast PvP
// SFML 3.x only, no extra dependencies.
//
// Captures the Atari POKEY character: hard-edged square waves, polynomial-noise
// LFSR texture, stepped pitch sweeps (POKEY divider grid), and 4-bit amplitude
// crunch — then adds procedural variation so every match sounds fresh.
//
// Usage example:
//   ProceduralSynth sfx;
//   sfx.init();                                         // call once at startup
//   sfx.play(ProceduralSynth::SFX::FIRE);              // random variant
//   sfx.play(ProceduralSynth::SFX::EXPLOSION, 0);      // specific variant 0
//   sfx.play(ProceduralSynth::SFX::COUNTDOWN, 2, 80.f);// vol = 80

#pragma once
#include <SFML/Audio.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

class ProceduralSynth
{
public:
    static constexpr int SAMPLE_RATE = 44100; ///< output Hz
    static constexpr int VARIATIONS  = 4;     ///< random flavors per SFX
    static constexpr int POLYPHONY   = 4;     ///< simultaneous voices per SFX

    /// All in-game SFX types.
    enum class SFX : int
    {
        FIRE            = 0, ///< player fires a bullet     [key 1]
        REFLECT,             ///< bullet hits a mirror      [key 2]
        HIT,                 ///< player takes non-fatal damage [key 3]
        HIT_BARRIER,         ///< bullet hits barrier brick [key 4]
        HIT_BOMB,            ///< bullet hits bomb core     [key 5]
        HIT_POWERUP,         ///< bullet hits power-up orb  [key 6]
        HIT_STAR,            ///< bullet hits special star  [key 7]
        EXPLOSION,           ///< player dies / round over  [key 4]
        POWERUP_SPAWN,       ///< power-up orb appears      [key 5]
        POWERUP_COLLECT,     ///< player collects power-up  [key 6]
        COUNTDOWN,           ///< countdown tick beep       [key 7]
        SLOW_HIT,            ///< player enters slow state  [key 8]
        COUNT
    };
    static constexpr int SFX_COUNT = static_cast<int>(SFX::COUNT);

    // ── Life-cycle ──────────────────────────────────────────────────────────

    /// Generate and store all sound banks. Must be called before play().
    /// @param masterSeed  0 = use std::random_device (different each run)
    void init(uint32_t masterSeed = 0);

    // ── Playback ────────────────────────────────────────────────────────────

    /// Play a random variant. volume is 0–100 (SFML scale).
    void play(SFX sfx, float volume = 100.f);

    /// Play a specific variant index (0 .. VARIATIONS-1).
    void play(SFX sfx, int variation, float volume = 100.f);

private:
    // Flat, stable-address storage after init().
    std::vector<sf::SoundBuffer>            bufs_;    // SFX_COUNT * VARIATIONS
    std::vector<std::unique_ptr<sf::Sound>> sounds_;  // SFX_COUNT * VARIATIONS * POLYPHONY
    std::vector<int>                        robins_;  // round-robin idx (same size as bufs_)
    std::mt19937                            rng_;

    // Index helpers
    int bufIdx  (int s, int v)         const { return s * VARIATIONS + v; }
    int soundIdx(int s, int v, int p)  const
        { return s * (VARIATIONS * POLYPHONY) + v * POLYPHONY + p; }

    // ── Per-SFX generators — each returns 16-bit mono at SAMPLE_RATE ────────

    std::vector<int16_t> genFire          (uint32_t seed) const;
    std::vector<int16_t> genReflect       (uint32_t seed) const;
    std::vector<int16_t> genHit           (uint32_t seed) const;
    std::vector<int16_t> genBarrierHit    (uint32_t seed) const;
    std::vector<int16_t> genBombHit       (uint32_t seed) const;
    std::vector<int16_t> genPowerupHit    (uint32_t seed) const;
    std::vector<int16_t> genStarHit       (uint32_t seed) const;
    std::vector<int16_t> genExplosion     (uint32_t seed) const;
    std::vector<int16_t> genPowerupSpawn  (uint32_t seed) const;
    std::vector<int16_t> genPowerupCollect(uint32_t seed) const;
    std::vector<int16_t> genCountdown     (uint32_t seed) const;
    std::vector<int16_t> genSlowHit       (uint32_t seed) const;
};
