// ProceduralSynth.cpp — POKEY-soul procedural SFX engine
// See ProceduralSynth.h for interface documentation.
//
// ── POKEY authenticity notes ────────────────────────────────────────────────
//  • Square waves with zero anti-aliasing  — aliasing IS the crunch.
//  • Poly5  LFSR (period 31): characteristic buzzy noise for hits/fire.
//  • Poly17 LFSR (period 131071): washy rumble for explosions.
//  • Frequency quantized to the POKEY divider grid (64 kHz base ÷ (div+1))
//    so sweeps "staircase" exactly like the real chip.
//  • 4–6 bit amplitude crush: gives the lo-fi snap that defines POKEY.
//  • Hard clipping, zero knee — POKEY never smoothed anything.
// ────────────────────────────────────────────────────────────────────────────

#include "ProceduralSynth.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>

// ═════════════════════════════════════════════════════════════════════════════
// Internal DSP  (anonymous namespace — not visible outside this TU)
// ═════════════════════════════════════════════════════════════════════════════
namespace {

constexpr int   SR = ProceduralSynth::SAMPLE_RATE;
constexpr float PI = 3.14159265358979f;

// ── POKEY polynomial noise LFSRs ─────────────────────────────────────────────

// poly5  (period 31)  — buzzy, crunchy, "missile" noise
inline void poly5_tick(uint32_t& s) noexcept
{
    uint32_t b = ((s >> 4) ^ (s >> 2)) & 1u;
    s = ((s << 1) | b) & 0x1Fu;
}

// poly17 (period 131071) — washy rumble, closer to white noise
inline void poly17_tick(uint32_t& s) noexcept
{
    uint32_t b = ((s >> 16) ^ (s >> 11)) & 1u;
    s = ((s << 1) | b) & 0x1FFFFu;
}

// ── POKEY frequency grid ──────────────────────────────────────────────────────
// Maps a continuous Hz value to the nearest POKEY divider frequency.
// Creates the characteristic pitch "staircase" during sweeps.
inline float pokeyFreq(float hz) noexcept
{
    int div = static_cast<int>(64000.f / std::max(hz, 1.f) + 0.5f) - 1;
    div = std::max(div, 0);
    return 64000.f / static_cast<float>(div + 1);
}

// ── ADSR envelope ─────────────────────────────────────────────────────────────
inline float adsr(float t, float atk, float dec, float sus,
                  float rel, float dur) noexcept
{
    float env;
    if      (t < atk)            env = t / atk;
    else if (t < atk + dec)      env = 1.f - (t - atk) / dec * (1.f - sus);
    else if (t < dur - rel)      env = sus;
    else                         env = sus * std::max(0.f, 1.f - (t - (dur - rel)) / rel);
    return std::max(0.f, env);
}

// ── Bit-crush quantiser ───────────────────────────────────────────────────────
// bits = 4 → heavy 1989 crunch, 8 → mild, 16 → effectively clean
inline float crush(float s, float bits) noexcept
{
    if (bits >= 15.0f) return s;
    float levels = std::exp2f(bits - 1.f);
    return std::round(s * levels) / levels;
}

// ─────────────────────────────────────────────────────────────────────────────
// Core synthesis kernel — one POKEY-style oscillator + LFSR noise
// ─────────────────────────────────────────────────────────────────────────────
struct POKEYParams
{
    float    freq0    = 440.f;   // start frequency Hz
    float    freq1    = 440.f;   // end frequency Hz (linear sweep)
    float    noise    = 0.0f;    // 0=pure square tone, 1=pure LFSR noise
    bool     poly5    = true;    // poly5 (buzzy) vs poly17 (washy)
    float    duty     = 0.5f;    // pulse-width 0.1–0.9
    float    atk      = 0.001f;
    float    dec      = 0.05f;
    float    sus      = 0.70f;
    float    rel      = 0.05f;
    float    dur      = 0.20f;   // total duration seconds
    float    bits     = 16.f;    // bit-crush depth
    float    amp      = 0.85f;   // peak amplitude 0–1
    bool     pokGrid  = true;    // quantise to POKEY frequency grid
    uint32_t seed     = 0;
};

std::vector<int16_t> pokeyGen(const POKEYParams& p)
{
    int N = static_cast<int>(p.dur * SR + 0.5f);
    std::vector<int16_t> out(N, 0);

    // Seed LFSRs (never zero)
    uint32_t lf5  = (p.seed ^ 0x19582Au) & 0x1Fu;    if (!lf5)  lf5  = 1u;
    uint32_t lf17 = (p.seed ^ 0xB74C3Eu) & 0x1FFFFu; if (!lf17) lf17 = 1u;

    float phase = 0.f;

    for (int i = 0; i < N; ++i)
    {
        float t  = static_cast<float>(i) / SR;
        float tN = t / p.dur;

        // ── Envelope ───────────────────────────────────────────────────────
        float env = adsr(t, p.atk, p.dec, p.sus, p.rel, p.dur) * p.amp;

        // ── Frequency (linear sweep → POKEY grid) ──────────────────────────
        float raw = p.freq0 + (p.freq1 - p.freq0) * tN;
        float frq = p.pokGrid ? pokeyFreq(raw) : raw;

        // ── Phase accumulator ───────────────────────────────────────────────
        phase += frq / SR;
        if (phase >= 1.f) phase -= 1.f;

        // ── Square-wave oscillator ──────────────────────────────────────────
        float tone = (phase < p.duty) ? 1.f : -1.f;

        // ── LFSR noise ──────────────────────────────────────────────────────
        poly5_tick(lf5);
        poly17_tick(lf17);
        float noisy = ((p.poly5 ? lf5 : lf17) & 1u) ? 1.f : -1.f;

        // ── Mix ─────────────────────────────────────────────────────────────
        float s = tone * (1.f - p.noise) + noisy * p.noise;
        s *= env;

        // ── Bit-crush → hard-clip ───────────────────────────────────────────
        s = crush(s, p.bits);
        s = std::max(-1.f, std::min(1.f, s));

        out[i] = static_cast<int16_t>(s * 32767.f);
    }
    return out;
}

// ── 3-way mix (avoids nested mixDown calls that double-clip) ───────────────
std::vector<int16_t> mixDown3(const std::vector<int16_t>& a,
                                const std::vector<int16_t>& b,
                                const std::vector<int16_t>& c)
{
    size_t N = std::max({a.size(), b.size(), c.size()});
    std::vector<int16_t> out(N, 0);
    for (size_t i = 0; i < N; ++i)
    {
        float fa = (i < a.size()) ? a[i] / 32767.f : 0.f;
        float fb = (i < b.size()) ? b[i] / 32767.f : 0.f;
        float fc = (i < c.size()) ? c[i] / 32767.f : 0.f;
        float s  = std::max(-1.f, std::min(1.f, fa + fb + fc));
        out[i]   = static_cast<int16_t>(s * 32767.f);
    }
    return out;
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════════════════════
// ProceduralSynth public API
// ═════════════════════════════════════════════════════════════════════════════

void ProceduralSynth::init(uint32_t masterSeed)
{
    if (masterSeed == 0)
    {
        std::random_device rd;
        masterSeed = rd();
    }
    rng_.seed(masterSeed);

    const int nBufs   = SFX_COUNT * VARIATIONS;
    const int nSounds = SFX_COUNT * VARIATIONS * POLYPHONY;

    bufs_.resize(nBufs);
    sounds_.clear();
    sounds_.reserve(nSounds);
    robins_.assign(nBufs, 0);

    using GenFn = std::vector<int16_t>(ProceduralSynth::*)(uint32_t) const;
    static const GenFn gens[SFX_COUNT] = {
        &ProceduralSynth::genFire,
        &ProceduralSynth::genReflect,
        &ProceduralSynth::genHit,
        &ProceduralSynth::genExplosion,
        &ProceduralSynth::genPowerupSpawn,
        &ProceduralSynth::genPowerupCollect,
        &ProceduralSynth::genCountdown,
        &ProceduralSynth::genSlowHit,
    };

    for (int s = 0; s < SFX_COUNT; ++s)
    {
        uint32_t sfxSeed = masterSeed ^ static_cast<uint32_t>(s * 0x9E3779B9u);
        for (int v = 0; v < VARIATIONS; ++v)
        {
            uint32_t varSeed = sfxSeed ^ static_cast<uint32_t>(v * 0x6C62272Eu);
            auto samples = (this->*gens[s])(varSeed);

            int bi = bufIdx(s, v);
            if (!bufs_[bi].loadFromSamples(
                    samples.data(),
                    static_cast<uint64_t>(samples.size()),
                    1u,
                    static_cast<unsigned>(SAMPLE_RATE),
                    {sf::SoundChannel::Mono}))
            {
                std::fprintf(stderr, "ProceduralSynth: failed to create buffer for sfx=%d var=%d\n", s, v);
            }

            for (int poly = 0; poly < POLYPHONY; ++poly)
                sounds_.push_back(std::make_unique<sf::Sound>(bufs_[bi]));
        }
    }

    std::fprintf(stderr, "ProceduralSynth: init done (%d buffers, seed=%u)\n",
                 nBufs, masterSeed);
}

void ProceduralSynth::play(SFX sfx, float volume)
{
    std::uniform_int_distribution<int> d(0, VARIATIONS - 1);
    play(sfx, d(rng_), volume);
}

void ProceduralSynth::play(SFX sfx, int variation, float volume)
{
    int s   = static_cast<int>(sfx);
    variation = std::clamp(variation, 0, VARIATIONS - 1);
    int ri  = s * VARIATIONS + variation;
    int poly = robins_[ri];
    robins_[ri] = (poly + 1) % POLYPHONY;

    auto& snd = *sounds_[soundIdx(s, variation, poly)];
    snd.setVolume(std::clamp(volume, 0.f, 100.f));
    if (snd.getStatus() == sf::Sound::Status::Playing)
        snd.stop();
    snd.play();
}

// ═════════════════════════════════════════════════════════════════════════════
// Per-SFX generators
// ═════════════════════════════════════════════════════════════════════════════

// ── FIRE ──────────────────────────────────────────────────────────────────────
// Punchy descending "PEW" — three fat layers: detuned tone pair + noise snap.
// ~90–150 ms, beefy 3.5-bit crunch, sub-octave doubles the weight.
std::vector<int16_t> ProceduralSynth::genFire(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    float base = 1300.f + U(rng) * 500.f;           // 1300–1800 Hz
    float end  = base   * (0.20f + U(rng) * 0.12f); // drops to 20–32 %
    float dur  = 0.090f + U(rng) * 0.060f;          // 90–150 ms (longer = fatter)
    float duty = 0.40f  + U(rng) * 0.20f;           // wider pulse width
    float bits = 3.5f   + U(rng) * 1.0f;            // 3.5–4.5 bit crunch

    // Layer 1: main descending tone body
    auto body = pokeyGen({
        .freq0 = base, .freq1 = end,
        .noise = 0.08f, .poly5 = true,
        .duty  = duty,
        .atk   = 0.001f,
        .dec   = dur * 0.30f,
        .sus   = 0.20f,
        .rel   = dur * 0.40f,
        .dur   = dur,
        .bits  = bits, .amp = 0.75f,
        .pokGrid = true, .seed = seed,
    });

    // Layer 2: detuned sub-octave for thickness
    auto sub = pokeyGen({
        .freq0 = base * 0.48f, .freq1 = end * 0.50f,
        .noise = 0.12f, .poly5 = true,
        .duty  = 0.55f,
        .atk   = 0.001f,
        .dec   = dur * 0.40f,
        .sus   = 0.25f,
        .rel   = dur * 0.35f,
        .dur   = dur,
        .bits  = bits - 0.5f, .amp = 0.45f,
        .pokGrid = true, .seed = seed ^ 0xFA770001u,
    });

    // Layer 3: short noise burst at attack for "snap"
    float clickDur = 0.014f + U(rng) * 0.008f;
    auto snap = pokeyGen({
        .freq0 = 3000.f + U(rng) * 800.f,
        .freq1 = 600.f,
        .noise = 0.70f, .poly5 = true,
        .duty  = 0.50f,
        .atk   = 0.0004f,
        .dec   = clickDur * 0.5f,
        .sus   = 0.0f,
        .rel   = clickDur * 0.3f,
        .dur   = clickDur,
        .bits  = 3.5f, .amp = 0.65f,
        .pokGrid = false, .seed = seed ^ 0xF12E0001u,
    });

    return mixDown3(body, sub, snap);
}

// ── REFLECT ───────────────────────────────────────────────────────────────────
// Metallic "TCHING" — three-tone harmonic shimmer: fundamental, 5th, low body.
// Detuned fundamental pair + low octave body for fat ring. 60–100 ms.
std::vector<int16_t> ProceduralSynth::genReflect(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    float base  = 2000.f + U(rng) * 500.f;          // 2000–2500 Hz
    float dur   = 0.060f + U(rng) * 0.040f;         // 60–100 ms (fatter tail)
    float bits  = 6.0f   + U(rng) * 2.0f;           // 6–8 bit, grainier

    // Layer 1: fundamental ping rising slightly
    auto fund = pokeyGen({
        .freq0 = base, .freq1 = base * (1.12f + U(rng) * 0.18f),
        .noise = 0.08f + U(rng) * 0.06f, .poly5 = true,
        .duty  = 0.30f + U(rng) * 0.12f,
        .atk   = 0.0004f,
        .dec   = dur * 0.18f,
        .sus   = 0.45f,
        .rel   = dur * 0.55f,
        .dur   = dur,
        .bits  = bits, .amp = 0.58f,
        .pokGrid = true, .seed = seed,
    });

    // Layer 2: harmonic overtone at perfect 5th (×1.5) — metallic ring
    auto overtone = pokeyGen({
        .freq0 = base * 1.5f, .freq1 = base * 1.62f,
        .noise = 0.04f, .poly5 = true,
        .duty  = 0.22f,
        .atk   = 0.0004f,
        .dec   = dur * 0.22f,
        .sus   = 0.30f,
        .rel   = dur * 0.45f,
        .dur   = dur * 0.85f,
        .bits  = bits + 1.f, .amp = 0.38f,
        .pokGrid = true, .seed = seed ^ 0xBE110001u,
    });

    // Layer 3: low-octave body for heft
    auto body = pokeyGen({
        .freq0 = base * 0.50f, .freq1 = base * 0.55f,
        .noise = 0.10f, .poly5 = true,
        .duty  = 0.50f,
        .atk   = 0.0005f,
        .dec   = dur * 0.30f,
        .sus   = 0.20f,
        .rel   = dur * 0.40f,
        .dur   = dur * 0.7f,
        .bits  = bits - 1.f, .amp = 0.30f,
        .pokGrid = true, .seed = seed ^ 0xBE220002u,
    });

    return mixDown3(fund, overtone, body);
}

// ── HIT ───────────────────────────────────────────────────────────────────────
// "BZZZZT-CRACK" — punchy POKEY damage. Three fat layers:
//   1) Heavy poly5 noise sweep (the buzz body) — wider, louder
//   2) Sharp high-freq click transient (impact snap)
//   3) Sub-bass thump with more weight and longer tail
// 220–320 ms, chest-shaking.
std::vector<int16_t> ProceduralSynth::genHit(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    float dur   = 0.220f + U(rng) * 0.100f;         // 220–320 ms (longer tail)
    float bits  = 3.5f   + U(rng) * 1.0f;           // 3.5–4.5 bit HEAVY crunch

    // Layer 1: main buzz body — heavy noise sweep down
    auto buzz = pokeyGen({
        .freq0 = 420.f + U(rng) * 140.f,
        .freq1 = 28.f  + U(rng) * 20.f,
        .noise = 0.78f + U(rng) * 0.18f,
        .poly5 = true,
        .duty  = 0.55f,
        .atk   = 0.0008f,
        .dec   = dur * 0.20f,
        .sus   = 0.60f,
        .rel   = dur * 0.45f,
        .dur   = dur,
        .bits  = bits, .amp = 0.88f,
        .pokGrid = true, .seed = seed,
    });

    // Layer 2: sharp click transient at impact
    float clickDur = 0.012f + U(rng) * 0.008f;
    auto click = pokeyGen({
        .freq0 = 2000.f + U(rng) * 700.f,
        .freq1 = 350.f,
        .noise = 0.45f, .poly5 = true,
        .duty  = 0.50f,
        .atk   = 0.0003f,
        .dec   = clickDur * 0.4f,
        .sus   = 0.0f,
        .rel   = clickDur * 0.4f,
        .dur   = clickDur,
        .bits  = 3.5f, .amp = 0.75f,
        .pokGrid = false, .seed = seed ^ 0xC2AC4001u,
    });

    // Layer 3: sub-bass thump — deeper and heavier
    auto sub = pokeyGen({
        .freq0 = 65.f + U(rng) * 25.f,
        .freq1 = 18.f,
        .noise = 0.25f, .poly5 = false,
        .duty  = 0.55f,
        .atk   = 0.001f,
        .dec   = 0.025f,
        .sus   = 0.40f,
        .rel   = dur * 0.55f,
        .dur   = dur * 0.8f,
        .bits  = 4.0f, .amp = 0.62f,
        .pokGrid = true, .seed = seed ^ 0xBA550001u,
    });

    return mixDown3(buzz, click, sub);
}

// ── EXPLOSION ─────────────────────────────────────────────────────────────────
// "BOOOM-KRRRSHH" — enormous three-layer death explosion.
//   Layer 1: deep poly17 sub-bass rumble — heavier, lower, longer
//   Layer 2: mid-freq poly5 crackle — wider stereo-feel chaos
//   Layer 3: high-freq noise burst — harder BANG with sub-content
// 650–950 ms of absolute devastation. Shakes the screen.
std::vector<int16_t> ProceduralSynth::genExplosion(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    float dur = 0.65f + U(rng) * 0.30f;   // 650–950 ms (longer = bigger)

    // Layer 1: deep poly17 sub-bass rumble — the earthquake
    auto rumble = pokeyGen({
        .freq0 = 120.f + U(rng) * 50.f,
        .freq1 = 10.f  + U(rng) * 6.f,      // drops even lower
        .noise = 0.88f + U(rng) * 0.10f,
        .poly5 = false,  // poly17 = washy deep rumble
        .duty  = 0.55f,
        .atk   = 0.001f,
        .dec   = 0.050f,
        .sus   = 0.75f,
        .rel   = dur * 0.65f,
        .dur   = dur,
        .bits  = 3.0f + U(rng) * 0.8f,  // EXTRA crunchy
        .amp   = 0.90f,                  // louder foundation
        .pokGrid = true, .seed = seed ^ 0xDEADBEEFu,
    });

    // Layer 2: mid-freq poly5 crackle — the shrapnel
    float crackDur = dur * (0.55f + U(rng) * 0.30f);
    auto crackle = pokeyGen({
        .freq0 = 350.f + U(rng) * 180.f,
        .freq1 = 35.f  + U(rng) * 18.f,
        .noise = 0.92f + U(rng) * 0.06f,
        .poly5 = true,
        .duty  = 0.48f + U(rng) * 0.10f,
        .atk   = 0.001f,
        .dec   = 0.018f,
        .sus   = 0.50f,
        .rel   = crackDur * 0.58f,
        .dur   = crackDur,
        .bits  = 3.5f + U(rng) * 0.5f,
        .amp   = 0.68f,                 // louder shrapnel
        .pokGrid = true, .seed = seed ^ 0x0BEEFC0Du,
    });

    // Layer 3: high-freq noise burst — the initial BANG
    float bangDur = 0.030f + U(rng) * 0.018f;  // slightly longer bang
    auto bang = pokeyGen({
        .freq0 = 2800.f + U(rng) * 1000.f,
        .freq1 = 150.f,
        .noise = 0.96f,
        .poly5 = true,
        .duty  = 0.50f,
        .atk   = 0.0002f,
        .dec   = bangDur * 0.45f,
        .sus   = 0.0f,
        .rel   = bangDur * 0.4f,
        .dur   = bangDur,
        .bits  = 3.5f, .amp = 0.92f,    // louder transient
        .pokGrid = false, .seed = seed ^ 0xBA560001u,
    });

    return mixDown3(rumble, crackle, bang);
}

// ── POWERUP SPAWN ─────────────────────────────────────────────────────────────
// Ethereal ascending shimmer — "something magical materialised".
// 4-note ascending run with wider intervals and warm, fat tones.
// Wider duty cycles, more grain, slightly lower base for weight.
std::vector<int16_t> ProceduralSynth::genPowerupSpawn(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    float root    = 380.f + U(rng) * 140.f;   // 380–520 Hz (lower = fatter)
    float ratio   = 1.22f + U(rng) * 0.08f;   // interval per step
    float noteDur = 0.060f + U(rng) * 0.018f;  // 60–78 ms per note (longer)
    float bits    = 5.5f + U(rng) * 1.5f;      // crunchier
    float tailDur = noteDur * 1.4f;             // last note lingers more

    int   totalN = static_cast<int>((noteDur * 3 + tailDur) * SR + 0.5f);
    std::vector<int16_t> out(totalN, 0);

    for (int n = 0; n < 4; ++n)
    {
        float freq = root * std::pow(ratio, static_cast<float>(n));
        float nd = (n == 3) ? tailDur : noteDur;
        auto note = pokeyGen({
            .freq0 = freq, .freq1 = freq * (1.05f + n * 0.012f),
            .noise = 0.04f, .poly5 = true,
            .duty  = 0.35f,                     // wider pulse = fatter tone
            .atk   = 0.001f,
            .dec   = nd * 0.22f,
            .sus   = 0.60f,
            .rel   = nd * 0.42f,
            .dur   = nd,
            .bits  = bits, .amp = 0.58f + n * 0.10f,
            .pokGrid = true, .seed = seed ^ static_cast<uint32_t>(n),
        });

        int offset = static_cast<int>(n * noteDur * SR + 0.5f);
        for (int i = 0; i < static_cast<int>(note.size())
                         && (offset + i) < totalN; ++i)
        {
            float fs = out[offset + i] / 32767.f + note[i] / 32767.f;
            out[offset + i] = static_cast<int16_t>(
                std::max(-1.f, std::min(1.f, fs)) * 32767.f);
        }
    }
    return out;
}

// ── POWERUP COLLECT ───────────────────────────────────────────────────────────
// Triumphant ascending major-chord arpeggio with climactic octave finish.
// 5 notes: root → M3 → P5 → octave → high P5. FAT version: wider pulses,
// lower root for chest-feel, crunchier crunch, higher amps. 340–480 ms.
std::vector<int16_t> ProceduralSynth::genPowerupCollect(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    float root    = 300.f + U(rng) * 130.f;          // 300–430 Hz (lower = weightier)
    float noteDur = 0.052f + U(rng) * 0.020f;        // 52–72 ms each (slightly longer)
    float bits    = 5.0f + U(rng) * 1.5f;            // crunchier

    // Extended major chord: root, M3, P5, octave, high P5
    static constexpr float intervals[5] = { 1.0f, 1.25f, 1.5f, 2.0f, 3.0f };
    float totalDur = noteDur * 5.0f;

    int totalN = static_cast<int>(totalDur * SR + 0.5f);
    std::vector<int16_t> out(totalN, 0);

    for (int n = 0; n < 5; ++n)
    {
        float freq = root * intervals[n];
        float nd   = (n >= 3) ? noteDur * 1.6f : noteDur; // last notes linger more
        float porta = (n == 4) ? 1.18f : 1.0f;            // final note sweeps higher
        auto note = pokeyGen({
            .freq0 = freq, .freq1 = freq * porta,
            .noise = 0.02f, .poly5 = true,
            .duty  = 0.35f,                               // wider = fatter
            .atk   = 0.001f,
            .dec   = nd * 0.16f,
            .sus   = 0.85f,
            .rel   = nd * 0.40f,
            .dur   = nd,
            .bits  = bits, .amp = 0.55f + n * 0.08f,
            .pokGrid = true, .seed = seed ^ static_cast<uint32_t>(n * 7u),
        });

        int offset = static_cast<int>(n * noteDur * SR + 0.5f);
        for (int i = 0; i < static_cast<int>(note.size())
                         && (offset + i) < totalN; ++i)
        {
            float fs = out[offset + i] / 32767.f + note[i] / 32767.f;
            out[offset + i] = static_cast<int16_t>(
                std::max(-1.f, std::min(1.f, fs)) * 32767.f);
        }
    }
    return out;
}

// ── COUNTDOWN ─────────────────────────────────────────────────────────────────
// Clean decisive beep. The four variants cycle through A4→B4→C5→E5 so callers
// can use variation 0/1/2/3 for "3-2-1-GO" with natural rising tension.
// Pure square, no noise, EXACT musical pitch (pokGrid=false).
//
//   Example calls:
//     synth.play(ProceduralSynth::SFX::COUNTDOWN, 0);  // "3" beep  440 Hz
//     synth.play(ProceduralSynth::SFX::COUNTDOWN, 1);  // "2" beep  494 Hz
//     synth.play(ProceduralSynth::SFX::COUNTDOWN, 2);  // "1" beep  523 Hz
//     synth.play(ProceduralSynth::SFX::COUNTDOWN, 3);  // "GO" beep 659 Hz
std::vector<int16_t> ProceduralSynth::genCountdown(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    // Slot by seed bits so the 4 variants naturally correspond to 4 pitches
    static constexpr float PITCHES[4] = { 440.f, 494.f, 523.f, 659.f };
    int   slot = static_cast<int>((seed >> 8) & 3);
    float freq = PITCHES[slot];
    float dur  = 0.100f + U(rng) * 0.040f;   // 100–140 ms
    float bits = 8.0f   + U(rng) * 2.0f;     // fairly clean 8–10-bit

    return pokeyGen({
        .freq0 = freq, .freq1 = freq,
        .noise = 0.00f, .poly5 = true,
        .duty  = 0.50f,
        .atk   = 0.002f,
        .dec   = dur * 0.10f,
        .sus   = 0.85f,
        .rel   = dur * 0.35f,
        .dur   = dur,
        .bits  = bits, .amp = 0.70f,
        .pokGrid = false, // exact musical pitch
        .seed  = seed,
    });
}

// ── SLOW HIT ──────────────────────────────────────────────────────────────────
// "CHUNK-WOOOOM" — freeze-ray impact. Three fat layers:
//   Layer 1: heavy descending bass thud — deeper and wider
//   Layer 2: percussive high click (attack transient)
//   Layer 3: lingering sub-bass drone with more weight
// 220–320 ms total with a thick tail suggesting time dilation.
std::vector<int16_t> ProceduralSynth::genSlowHit(uint32_t seed) const
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(0.f, 1.f);

    float dur  = 0.220f + U(rng) * 0.100f;   // 220–320 ms (fatter)
    float bits = 3.5f   + U(rng) * 1.0f;     // crunchier

    // Layer 1: heavy bass thud dropping fast — wider pulse, louder
    auto thud = pokeyGen({
        .freq0 = 100.f + U(rng) * 35.f,
        .freq1 = 18.f  + U(rng) * 8.f,
        .noise = 0.65f + U(rng) * 0.20f,
        .poly5 = true,
        .duty  = 0.55f,
        .atk   = 0.0008f,
        .dec   = dur * 0.18f,
        .sus   = 0.60f,
        .rel   = dur * 0.55f,
        .dur   = dur,
        .bits  = bits, .amp = 0.90f,
        .pokGrid = true, .seed = seed ^ 0x11112222u,
    });

    // Layer 2: percussive click — harder snap
    float clickDur = 0.014f + U(rng) * 0.006f;
    auto click = pokeyGen({
        .freq0 = 800.f + U(rng) * 400.f,
        .freq1 = 120.f,
        .noise = 0.40f, .poly5 = true,
        .duty  = 0.50f,
        .atk   = 0.0004f,
        .dec   = 0.007f,
        .sus   = 0.00f,
        .rel   = 0.006f,
        .dur   = clickDur,
        .bits  = 3.5f, .amp = 0.72f,
        .pokGrid = true, .seed = seed ^ 0x22223333u,
    });

    // Layer 3: low drone tail — heavier "frozen in time" feel
    float droneDur = dur * 0.9f;
    auto drone = pokeyGen({
        .freq0 = 42.f + U(rng) * 12.f,
        .freq1 = 28.f + U(rng) * 8.f,
        .noise = 0.20f, .poly5 = false, // poly17 washy
        .duty  = 0.55f,
        .atk   = dur * 0.12f,  // slow swell in
        .dec   = droneDur * 0.18f,
        .sus   = 0.45f,
        .rel   = droneDur * 0.55f,
        .dur   = droneDur,
        .bits  = 4.0f, .amp = 0.52f,
        .pokGrid = true, .seed = seed ^ 0x44445555u,
    });

    return mixDown3(thud, click, drone);
}
