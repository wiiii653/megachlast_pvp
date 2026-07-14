#include "ArenaLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace arena_layout {

static inline float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

uint32_t deriveBoardSeed(uint32_t world, uint32_t round)
{
    uint32_t s = world ^ (round * 2654435761u);
    s ^= s >> 16;
    s *= 0x45d9f3bu;
    s ^= s >> 16;
    return s;
}

void genMirrorsSeeded(std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                      uint32_t boardSeed,
                      LayoutKind& layoutKind,
                      char (&g_layout_name)[24])
{
    for(auto& mirror : mirrors) mirror = Mirror{0.f, 0.f, true, 0.f, false};

    std::mt19937 rng(boardSeed);
    auto irand = [&](int a, int b) -> int {
        return std::uniform_int_distribution<int>(a, b)(rng);
    };
    auto frand = [&](float a, float b) -> float {
        return std::uniform_real_distribution<float>(a, b)(rng);
    };

    // Arena geometry
    const float xMin  = 40.f,  xMax  = W - 40.f;  // 40..600
    // Each half occupies [0, H/2) and [H/2, H).  Spawn clearance is ±15 px from rows 58 (P2) and H-58 (P1).
    const float spawnTopY = PLAYER_SPAWN_TOP_Y;
    const float spawnBotY = playerSpawnBottomY();
    const float clearR    = MIRROR_R + 8.f;       // 15
    const float minSep    = MIRROR_PAD;           // 12

    // Safe zones within each half
    const float tYMin = 12.f,  tYMax = H/2.f - 5.f;   // top half: 12..195
    const float bYMin = H/2.f + 5.f, bYMax = H - 12.f; // bot half: 205..388

    // Per-half placed count
    int topN = 0, botN = 0;
    // We place into the full mirrors array directly for asymmetric layouts,
    // or use a temporary top[] for symmetric ones.
    std::array<Mirror, MIRROR_PAIRS> topArr{};

    // ── Symmetric-half helpers (fill topArr, reflect to mirrors later) ────────
    auto splace = [&](float x, float y, bool slash) -> bool {
        if(x < xMin || x > xMax || y < tYMin || y > tYMax) return false;
        if(std::abs(y - spawnTopY) < clearR) return false;
        for(int k = 0; k < topN; ++k){
            float dx = x - topArr[k].x, dy = y - topArr[k].y;
            if(dx*dx + dy*dy < minSep*minSep) return false;
        }
        topArr[topN++] = Mirror{x, y, slash, 0.f, true};
        return true;
    };
    auto sfillRandom = [&](){
        constexpr int MAX_FILL_ATTEMPTS = MIRROR_PAIRS * 1000;
        for(int attempts = 0; topN < MIRROR_PAIRS && attempts < MAX_FILL_ATTEMPTS; ++attempts){
            float x = frand(xMin, xMax), y = frand(tYMin, tYMax);
            if(std::abs(y - spawnTopY) < clearR) continue;
            bool ok = true;
            for(int j = 0; j < topN && ok; ++j){
                float dx = x - topArr[j].x, dy = y - topArr[j].y;
                if(dx*dx + dy*dy < minSep*minSep) ok = false;
            }
            if(ok) topArr[topN++] = Mirror{x, y, bool(irand(0,1)), 0.f, true};
        }
        for(int i = topN; i < MIRROR_PAIRS; ++i)
            topArr[i] = Mirror{0.f, 0.f, true, 0.f, false};
        topN = MIRROR_PAIRS;
    };
    // reflect topArr → mirrors (symmetric mode)
    auto reflectSymmetric = [&](){
        for(int i = 0; i < MIRROR_PAIRS; ++i){
            mirrors[i]                    = topArr[i];
            mirrors[i + MIRROR_PAIRS]     = topArr[i];
            mirrors[i + MIRROR_PAIRS].y   = H - topArr[i].y;
            mirrors[i + MIRROR_PAIRS].slash = !topArr[i].slash;
        }
    };

    // ── Asymmetric helpers: place directly into mirrors[] ─────────────────────
    // top mirror index 0..MIRROR_PAIRS-1, bot index MIRROR_PAIRS..2*MIRROR_PAIRS-1
    auto aplace = [&](float x, float y, bool slash, bool botHalf) -> bool {
        float yLo = botHalf ? bYMin : tYMin;
        float yHi = botHalf ? bYMax : tYMax;
        float spawnRow = botHalf ? spawnBotY : spawnTopY;
        if(x < xMin || x > xMax || y < yLo || y > yHi) return false;
        if(std::abs(y - spawnRow) < clearR) return false;
        int base = botHalf ? MIRROR_PAIRS : 0;
        int& cnt = botHalf ? botN : topN;
        for(int k = 0; k < cnt; ++k){
            float dx = x - mirrors[base + k].x, dy = y - mirrors[base + k].y;
            if(dx*dx + dy*dy < minSep*minSep) return false;
        }
        mirrors[base + cnt++] = Mirror{x, y, slash, 0.f, true};
        return true;
    };
    auto afillRandom = [&](bool botHalf){
        float yLo = botHalf ? bYMin : tYMin;
        float yHi = botHalf ? bYMax : tYMax;
        float spawnRow = botHalf ? spawnBotY : spawnTopY;
        int base = botHalf ? MIRROR_PAIRS : 0;
        int& cnt = botHalf ? botN : topN;
        constexpr int MAX_FILL_ATTEMPTS = MIRROR_PAIRS * 1000;
        for(int attempts = 0; cnt < MIRROR_PAIRS && attempts < MAX_FILL_ATTEMPTS; ++attempts){
            float x = frand(xMin, xMax), y = frand(yLo, yHi);
            if(std::abs(y - spawnRow) < clearR) continue;
            bool ok = true;
            for(int j = 0; j < cnt && ok; ++j){
                float dx = x - mirrors[base+j].x, dy = y - mirrors[base+j].y;
                if(dx*dx + dy*dy < minSep*minSep) ok = false;
            }
            if(ok) mirrors[base + cnt++] = Mirror{x, y, bool(irand(0,1)), 0.f, true};
        }
        for(int i = cnt; i < MIRROR_PAIRS; ++i)
            mirrors[base + i] = Mirror{0.f, 0.f, true, 0.f, false};
        cnt = MIRROR_PAIRS;
    };
    // ── Pick archetype: bits 5..8 → 12 choices ───────────────────────────────
    const int NUM_LAYOUTS = 12;
    int pick = static_cast<int>((boardSeed >> 5) % NUM_LAYOUTS);

    switch(pick){

    // ──────────────────────────────────────────────────── 0: RANDOM ──────────
    // Pure scatter.  Each run is entirely different.
    case 0:
        layoutKind = LayoutKind::RANDOM;
        std::snprintf(g_layout_name, sizeof g_layout_name, "RANDOM");
        sfillRandom();
        reflectSymmetric();
        break;

    // ──────────────────────────────────────────────────── 1: ZIGZAG ──────────
    // Four horizontal bands; adjacent bands use opposite mirror orientations.
    // Bullets traversing the field get deflected in long diagonal chains.
    case 1: {
        layoutKind = LayoutKind::ZIGZAG;
        std::snprintf(g_layout_name, sizeof g_layout_name, "ZIGZAG");
        struct Band { float y0, y1; bool slash; int n; };
        const Band bands[4] = {
            {tYMin,  43.f,  true,  5},
            {74.f,  113.f, false, 5},
            {117.f, 152.f, true,  5},
            {156.f, tYMax, false, 5},
        };
        const float xSpread = xMax - xMin;
        for(const auto& bd : bands){
            if(topN >= MIRROR_PAIRS) break;
            float spacing = xSpread / (bd.n + 1);
            for(int k = 0; k < bd.n && topN < MIRROR_PAIRS; ++k){
                float x = xMin + (k + 1) * spacing + frand(-18.f, 18.f);
                float y = frand(bd.y0, bd.y1);
                x = clampf(x, xMin, xMax);
                if(!splace(x, y, bd.slash))
                    splace(x + frand(-20.f, 20.f), y + frand(-6.f, 6.f), bd.slash);
            }
        }
        sfillRandom();
        reflectSymmetric();
        break;
    }

    // ──────────────────────────────────────────────────── 2: CLUSTERS ────────
    // Three tight ricochet pockets, each with uniform orientation.
    // Bullets entering a pocket get funnelled sideways.
    case 2: {
        layoutKind = LayoutKind::CLUSTERS;
        std::snprintf(g_layout_name, sizeof g_layout_name, "CLUSTERS");
        struct Nuc { float x, y; bool slash; int count; };
        Nuc nuclei[3] = {
            { frand(55.f,  195.f), frand(tYMin+6.f, 40.f), bool(irand(0,1)), 7 },
            { frand(220.f, 420.f), frand(78.f, tYMax-8.f), bool(irand(0,1)), 7 },
            { frand(430.f, 585.f), frand(tYMin+6.f, 40.f), bool(irand(0,1)), 6 },
        };
        for(auto& n : nuclei){
            if(std::abs(n.y - spawnTopY) < clearR + 16.f)
                n.y = (n.y < spawnTopY) ? (spawnTopY - clearR - 16.f) : (spawnTopY + clearR + 16.f);
            n.y = clampf(n.y, tYMin + 6.f, tYMax - 6.f);
        }
        constexpr float clusterR = 48.f;
        for(auto& n : nuclei){
            for(int k = 0, tries = 0; k < n.count && topN < MIRROR_PAIRS; ++tries){
                if(tries > 400) break;
                float x = n.x + frand(-clusterR, clusterR);
                float y = n.y + frand(-clusterR, clusterR);
                if(splace(x, y, n.slash)) ++k;
            }
        }
        sfillRandom();
        reflectSymmetric();
        break;
    }

    // ──────────────────────────────────────────────────── 3: CHANNELS ────────
    // Three vertical corridors of alternating /\ mirrors.
    // Aim into a channel — get your bullet pinballed sideways.
    case 3: {
        layoutKind = LayoutKind::CHANNELS;
        std::snprintf(g_layout_name, sizeof g_layout_name, "CHANNELS");
        const float cx[3] = { W*0.22f, W*0.50f, W*0.78f };
        const float yaLo = tYMin, yaHi = spawnTopY - clearR - 1.f;
        const float ybLo = spawnTopY + clearR + 1.f, ybHi = tYMax;
        for(int ch = 0; ch < 3 && topN < MIRROR_PAIRS; ++ch){
            int slot = 0;
            for(int s = 0; s < 3 && topN < MIRROR_PAIRS; ++s, ++slot){
                float x = cx[ch] + frand(-20.f, 20.f);
                float y = yaLo + (yaHi - yaLo) * (s + 1) / 4.f + frand(-4.f, 4.f);
                splace(x, y, bool(slot & 1));
            }
            for(int s = 0; s < 4 && topN < MIRROR_PAIRS; ++s, ++slot){
                float x = cx[ch] + frand(-20.f, 20.f);
                float y = ybLo + (ybHi - ybLo) * (s + 1) / 5.f + frand(-4.f, 4.f);
                splace(x, y, bool(slot & 1));
            }
        }
        sfillRandom();
        reflectSymmetric();
        break;
    }

    // ──────────────────────────────────────────────────── 4: FORTRESS ────────
    // Symmetric: a dense mirror wall across the middle of each half = a hard
    // horizontal barrier forcing shots around the sides.
    case 4: {
        layoutKind = LayoutKind::FORTRESS;
        std::snprintf(g_layout_name, sizeof g_layout_name, "FORTRESS");
        const float wallY = tYMax - 18.f;
        const int   wallN = 13;
        const float wallSpc = (xMax - xMin - 20.f) / float(wallN - 1);
        for(int i = 0; i < wallN && topN < MIRROR_PAIRS; ++i){
            float x = xMin + 10.f + i * wallSpc + frand(-5.f, 5.f);
            float y = wallY + frand(-7.f, 7.f);
            splace(x, y, bool(i & 1));
        }
        for(int tries = 0; topN < MIRROR_PAIRS && tries < 500; ++tries){
            float x = frand(xMin, xMax);
            float y = frand(tYMin, spawnTopY - clearR - 3.f);
            splace(x, y, bool(irand(0, 1)));
        }
        sfillRandom();
        reflectSymmetric();
        break;
    }

    // ──────────────────────────────────────────────────── 5: SPARSE ──────────
    // Only 8 mirror pairs — wide open field.  Long-range duelling.
    case 5: {
        layoutKind = LayoutKind::SPARSE;
        std::snprintf(g_layout_name, sizeof g_layout_name, "SPARSE");
        const int   sparseN = 8;
        const float bigSep  = 55.f;
        for(int tries = 0; topN < sparseN && tries < 2000; ++tries){
            float x = frand(xMin + 15.f, xMax - 15.f);
            float y = frand(tYMin, tYMax);
            if(std::abs(y - spawnTopY) < clearR) continue;
            bool ok = true;
            for(int j = 0; j < topN && ok; ++j){
                float dx = x - topArr[j].x, dy = y - topArr[j].y;
                if(dx*dx + dy*dy < bigSep*bigSep) ok = false;
            }
            if(ok) topArr[topN++] = Mirror{x, y, bool(irand(0,1)), 0.f, true};
        }
        for(int i = topN; i < MIRROR_PAIRS; ++i) topArr[i] = Mirror{0.f, 0.f, true, 0.f, false};
        topN = MIRROR_PAIRS;
        reflectSymmetric();
        break;
    }

    // ──────────────────────────────────────────────────── 6: VORTEX ──────────
    // A clockwise spiral of mirrors radiating from the arena centre.
    // Each ring uses opposite orientation to the previous.
    case 6: {
        layoutKind = LayoutKind::VORTEX;
        std::snprintf(g_layout_name, sizeof g_layout_name, "VORTEX");
        const float cx  = W / 2.f;
        const float cy  = (tYMin + tYMax) / 2.f; // ≈ 103
        // 4 rings of 5 mirrors; radius grows with a twist offset per ring
        const int RINGS = 4, PER_RING = 5;
        for(int r = 0; r < RINGS && topN < MIRROR_PAIRS; ++r){
            float radius   = 30.f + r * 38.f;
            float twist    = r * (PI / 8.f);          // spiral offset
            bool  slash    = bool(r & 1);
            for(int k = 0; k < PER_RING && topN < MIRROR_PAIRS; ++k){
                float ang = twist + k * (2.f * PI / PER_RING) + frand(-0.08f, 0.08f);
                float x   = cx + std::cos(ang) * radius + frand(-4.f, 4.f);
                float y   = cy + std::sin(ang) * radius * 0.6f + frand(-4.f, 4.f); // ellipse
                splace(x, y, slash);
            }
        }
        sfillRandom();
        reflectSymmetric();
        break;
    }

    // ──────────────────────────────────────────────────── 7: GAUNTLET ────────
    // Dense side walls create a central kill-lane.
    // Left wall=\, right wall=/ — bullets hitting either wall angle inward.
    case 7: {
        layoutKind = LayoutKind::GAUNTLET;
        std::snprintf(g_layout_name, sizeof g_layout_name, "GAUNTLET");
        const float wallX[2]     = { W * 0.22f, W * 0.78f };
        const bool  wallSlash[2] = { false, true };
        const float yaLo = tYMin, yaHi = spawnTopY - clearR - 1.f;
        const float ybLo = spawnTopY + clearR + 1.f, ybHi = tYMax;
        const float seg[2][2] = { {yaLo, yaHi}, {ybLo, ybHi} };
        for(int w = 0; w < 2 && topN < MIRROR_PAIRS; ++w){
            for(int k = 0; k < 10 && topN < MIRROR_PAIRS; ++k){
                int si = (k < 3) ? 0 : 1;
                float t = (k < 3) ? float(k) / 3.f : float(k-3) / 7.f;
                float y = seg[si][0] + (seg[si][1] - seg[si][0]) * t + frand(-5.f, 5.f);
                float x = wallX[w] + frand(-16.f, 16.f);
                if(!splace(x, y, wallSlash[w]))
                    splace(wallX[w] + frand(-26.f, 26.f), y + frand(-8.f, 8.f), wallSlash[w]);
            }
        }
        sfillRandom();
        reflectSymmetric();
        break;
    }

    // ──────────────────────────────────────────────── 8: CROSS (asymmetric) ──
    // ASYMMETRIC: P2's half has a + shaped mirror cross in the middle; P1's half
    // has two diagonal X-arms.  Completely different zones each side.
    case 8: {
        layoutKind = LayoutKind::CROSS;
        std::snprintf(g_layout_name, sizeof g_layout_name, "CROSS");
        // Top half (P2 side): cross arms — horizontal row + vertical column
        const float tcx = W / 2.f, tcy = (tYMin + tYMax) / 2.f;
        // Horizontal bar: 7 mirrors at y≈tcy
        for(int i = 0; i < 7; ++i){
            float x = xMin + 30.f + i * (xMax - xMin - 60.f) / 6.f + frand(-6.f, 6.f);
            float y = tcy + frand(-8.f, 8.f);
            aplace(x, y, (i & 1) ? false : true, false);
        }
        // Vertical bar: 5 mirrors at x≈tcx
        for(int i = 0; i < 5; ++i){
            float t = float(i) / 4.f;
            float x = tcx + frand(-8.f, 8.f);
            float y = tYMin + 10.f + t * (tYMax - tYMin - 20.f) + frand(-5.f, 5.f);
            if(std::abs(y - spawnTopY) < clearR + 3.f) continue;
            aplace(x, y, (i & 1), false);
        }
        afillRandom(false);
        // Bottom half (P1 side): diagonal X — two crossing arms
        const float bcx = W / 2.f;
        const float bcy = (bYMin + bYMax) / 2.f;
        for(int arm = 0; arm < 2; ++arm){
            bool slash = (arm == 0);
            float ang  = (arm == 0) ? PI * 0.25f : PI * 0.75f;
            for(int i = 0; i < 9 && botN < MIRROR_PAIRS; ++i){
                float t  = (i - 4) * 24.f;
                float x  = bcx + std::cos(ang) * t + frand(-5.f, 5.f);
                float y  = bcy + std::sin(ang) * t * 0.5f + frand(-5.f, 5.f);
                aplace(x, y, slash, true);
            }
        }
        afillRandom(true);
        break;
    }

    // ──────────────────────────────────────── 9: LABYRINTH (fully asymmetric) ─
    // ASYMMETRIC: each half has a distinct S-curve wall of mirrors, creating real
    // navigational geometry that differs top vs. bottom.
    case 9: {
        layoutKind = LayoutKind::LABYRINTH;
        std::snprintf(g_layout_name, sizeof g_layout_name, "LABYRINTH");
        // Top half: S-curve wall — left gate open, right gate open
        //   segment A: right-to-left sweep across upper sub-zone
        //   segment B: left-to-right sweep across lower sub-zone
        const float yaLo = tYMin, yaHi = spawnTopY - clearR - 2.f;
        const float ybLo = spawnTopY + clearR + 2.f, ybHi = tYMax;
        for(int i = 0; i < 9 && topN < MIRROR_PAIRS; ++i){
            float t = float(i) / 8.f;
            float x = xMax - 20.f - t * (xMax - xMin - 40.f) + frand(-10.f, 10.f);
            float y = yaLo + 5.f + t * (yaHi - yaLo - 10.f) + frand(-4.f, 4.f);
            splace(x, y, (i & 1));
        }
        for(int i = 0; i < 9 && topN < MIRROR_PAIRS; ++i){
            float t = float(i) / 8.f;
            float x = xMin + 20.f + t * (xMax - xMin - 40.f) + frand(-10.f, 10.f);
            float y = ybLo + 5.f + t * (ybHi - ybLo - 10.f) + frand(-4.f, 4.f);
            splace(x, y, (i & 1) ? false : true);
        }
        sfillRandom();
        // Bottom half: INDEPENDENTLY place a box frame (4 sides of equal mirrors)
        const float bcx = W / 2.f, bcy = (bYMin + bYMax) / 2.f;
        const float bRx = (xMax - xMin) * 0.35f, bRy = (bYMax - bYMin) * 0.32f;
        for(int i = 0; i < 6 && botN < MIRROR_PAIRS; ++i){ // top edge
            float x = bcx - bRx + (i * 2.f * bRx / 5.f) + frand(-5.f, 5.f);
            aplace(x, bcy - bRy + frand(-5.f, 5.f), (i & 1), true);
        }
        for(int i = 0; i < 6 && botN < MIRROR_PAIRS; ++i){ // bottom edge
            float x = bcx - bRx + (i * 2.f * bRx / 5.f) + frand(-5.f, 5.f);
            aplace(x, bcy + bRy + frand(-5.f, 5.f), (i & 1) ? false : true, true);
        }
        for(int i = 1; i < 4 && botN < MIRROR_PAIRS; ++i){ // left edge
            float y = bcy - bRy + (i * 2.f * bRy / 4.f) + frand(-5.f, 5.f);
            aplace(bcx - bRx + frand(-5.f, 5.f), y, true, true);
        }
        for(int i = 1; i < 4 && botN < MIRROR_PAIRS; ++i){ // right edge
            float y = bcy - bRy + (i * 2.f * bRy / 4.f) + frand(-5.f, 5.f);
            aplace(bcx + bRx + frand(-5.f, 5.f), y, false, true);
        }
        afillRandom(true);
        break;
    }

    // ─────────────────────────────────────── 10: WARCROSS (fully asymmetric) ──
    // ASYMMETRIC: one half sparse (5 mirrors), the other cluttered (full density).
    // Forces different strategies: dodge precision shots vs. exploit ricochet chaos.
    case 10: {
        layoutKind = LayoutKind::WARCROSS;
        std::snprintf(g_layout_name, sizeof g_layout_name, "WARCROSS");
        // Top half: SPARSE — 5 wide-spaced mirrors
        const float bigSep = 65.f;
        for(int tries = 0; topN < 5 && tries < 2000; ++tries){
            float x = frand(xMin + 20.f, xMax - 20.f);
            float y = frand(tYMin, tYMax);
            if(std::abs(y - spawnTopY) < clearR) continue;
            bool ok = true;
            for(int j = 0; j < topN && ok; ++j){
                float dx = x - topArr[j].x, dy = y - topArr[j].y;
                if(dx*dx + dy*dy < bigSep*bigSep) ok = false;
            }
            if(ok) topArr[topN++] = Mirror{x, y, bool(irand(0,1)), 0.f, true};
        }
        for(int i = topN; i < MIRROR_PAIRS; ++i) topArr[i] = Mirror{0.f, 0.f, true, 0.f, false};
        topN = MIRROR_PAIRS;
        reflectSymmetric(); // reflects the sparse top to the corresponding mirror slots
        // But now INDEPENDENTLY overwrite bottom half with a dense zigzag
        botN = 0;
        const float blLo = bYMin, blHi = spawnBotY - clearR - 1.f;
        const float buLo = spawnBotY + clearR + 1.f, buHi = bYMax;
        // two dense bands in bot half alternating slashes
        for(int i = 0; i < 9 && botN < MIRROR_PAIRS; ++i){
            float t = float(i) / 8.f;
            float x = xMin + 25.f + t * (xMax - xMin - 50.f) + frand(-10.f, 10.f);
            float y = blLo + (blHi - blLo) * 0.5f + frand(-12.f, 12.f);
            aplace(x, y, (i & 1), true);
        }
        for(int i = 0; i < 9 && botN < MIRROR_PAIRS; ++i){
            float t = float(i) / 8.f;
            float x = xMin + 25.f + t * (xMax - xMin - 50.f) + frand(-10.f, 10.f);
            float y = buLo + (buHi - buLo) * 0.5f + frand(-12.f, 12.f);
            aplace(x, y, (i & 1) ? false : true, true);
        }
        afillRandom(true);
        break;
    }

    // ──────────────────────────────────────── 11: VANGUARD (fully asymmetric) ──
    // ASYMMETRIC: P2's side has a triangular spearhead of mirrors aimed inward;
    // P1's side has two flanking walls with an open centre lane.
    case 11: {
        layoutKind = LayoutKind::VANGUARD;
        std::snprintf(g_layout_name, sizeof g_layout_name, "VANGUARD");
        // Top half: forward-pointing triangle aimed toward P1
        //   apex at centre-forward (high y = near centre divider), base at spawn row
        const float apex_x = W / 2.f, apex_y = tYMax - 10.f;
        const float base_y = spawnTopY + clearR + 3.f;
        const float flank  = 170.f;
        // Left arm of triangle
        for(int i = 0; i < 7 && topN < MIRROR_PAIRS; ++i){
            float t = float(i) / 6.f;
            float x = apex_x - t * flank + frand(-8.f, 8.f);
            float y = apex_y - t * (apex_y - base_y) + frand(-6.f, 6.f);
            splace(x, y, false);  // \ deflects toward centre
        }
        // Right arm of triangle
        for(int i = 0; i < 7 && topN < MIRROR_PAIRS; ++i){
            float t = float(i) / 6.f;
            float x = apex_x + t * flank + frand(-8.f, 8.f);
            float y = apex_y - t * (apex_y - base_y) + frand(-6.f, 6.f);
            splace(x, y, true);   // / deflects toward centre
        }
        sfillRandom();
        // Bottom half: two flanking walls leave an open centre corridor
        const float lWallX = xMin + (xMax - xMin) * 0.18f;
        const float rWallX = xMin + (xMax - xMin) * 0.82f;
        const float blLo2 = bYMin, blHi2 = spawnBotY - clearR - 2.f;
        const float buLo2 = spawnBotY + clearR + 2.f, buHi2 = bYMax;
        for(int seg = 0; seg < 2; ++seg){
            float y0 = (seg == 0) ? blLo2 : buLo2;
            float y1 = (seg == 0) ? blHi2 : buHi2;
            for(int i = 0; i < 5 && botN < MIRROR_PAIRS; ++i){
                float t = float(i) / 4.f;
                float y = y0 + t * (y1 - y0) + frand(-5.f, 5.f);
                aplace(lWallX + frand(-10.f, 10.f), y, false, true);
            }
            for(int i = 0; i < 5 && botN < MIRROR_PAIRS; ++i){
                float t = float(i) / 4.f;
                float y = y0 + t * (y1 - y0) + frand(-5.f, 5.f);
                aplace(rWallX + frand(-10.f, 10.f), y, true, true);
            }
        }
        afillRandom(true);
        break;
    }

    default:
        layoutKind = LayoutKind::RANDOM;
        std::snprintf(g_layout_name, sizeof g_layout_name, "RANDOM");
        sfillRandom();
        reflectSymmetric();
        break;
    }
}


void genBarriers(std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers)
{
    float totalW = BARRIER_BRICKS * BRICK_W + (BARRIER_BRICKS - 1) * BRICK_GAP;
    float startX = (W - totalW) * 0.5f;
    float yP1 = playerSpawnBottomY() - BARRIER_Y_OFFSET;
    float yP2 = PLAYER_SPAWN_TOP_Y + BARRIER_Y_OFFSET;

    for(int i = 0; i < BARRIER_BRICKS; ++i){
        float cx = startX + i * (BRICK_W + BRICK_GAP) + BRICK_W * 0.5f;
        barriers[i].x = cx;
        barriers[i].y = yP1;
        barriers[i].hp = BRICK_MAX_HP;
        barriers[i].hitFlash = 0.f;
        barriers[i].alive = true;

        barriers[i + BARRIER_BRICKS].x = cx;
        barriers[i + BARRIER_BRICKS].y = yP2;
        barriers[i + BARRIER_BRICKS].hp = BRICK_MAX_HP;
        barriers[i + BARRIER_BRICKS].hitFlash = 0.f;
        barriers[i + BARRIER_BRICKS].alive = true;
    }
}

void placeBombsForLayout(std::array<Bomb, MAX_BOMBS>& bombs,
                         const std::array<Mirror, MIRROR_PAIRS * 2>& mirrors,
                         LayoutKind layoutKind,
                         RNG& rng)
{
    for(auto& bomb : bombs) bomb.alive = false;

    int bombCount = 0;
    switch(layoutKind){
        case LayoutKind::FORTRESS:
        case LayoutKind::WARCROSS:
            bombCount = rng.irand(2, 3);
            break;
        case LayoutKind::SPARSE:
        case LayoutKind::VANGUARD:
            bombCount = rng.irand(0, 1);
            break;
        case LayoutKind::CROSS:
        case LayoutKind::LABYRINTH:
            bombCount = rng.irand(1, 2);
            break;
        default: {
            int roll = rng.irand(0, 99);
            if(roll >= 60) bombCount = 1;
            if(roll >= 85) bombCount = 2;
            if(roll >= 96) bombCount = 3;
            break;
        }
    }

    const float bxMin = 50.f;
    const float bxMax = W - 50.f;
    const float byMin = H * 0.25f;
    const float byMax = H * 0.75f;
    for(int i = 0; i < bombCount; ++i){
        bool placed = false;
        for(int tries = 0; tries < 300 && !placed; ++tries){
            float bx = rng.frand(bxMin, bxMax);
            float by = rng.frand(byMin, byMax);
            if(std::abs(by - PLAYER_SPAWN_TOP_Y) < SPAWN_ROW_BOMB_EXCLUSION ||
               std::abs(by - playerSpawnBottomY()) < SPAWN_ROW_BOMB_EXCLUSION) continue;

            bool ok = true;
            for(const auto& m : mirrors){
                if(!m.alive) continue;
                float dx = bx - m.x;
                float dy = by - m.y;
                if(dx * dx + dy * dy < BOMB_PAD * BOMB_PAD){ ok = false; break; }
            }
            for(int j = 0; j < i && ok; ++j){
                float dx = bx - bombs[j].x;
                float dy = by - bombs[j].y;
                if(dx * dx + dy * dy < BOMB_PAD * BOMB_PAD) ok = false;
            }
            if(ok){
                bombs[i].x = bx;
                bombs[i].y = by;
                bombs[i].pulsePhase = rng.frand(0.f, PI * 2.f);
                bombs[i].alive = true;
                bombs[i].owner = 0;
                placed = true;
            }
        }
    }
}

void spawnPowerUp(std::array<PowerUp, MAX_POWERUPS>& powerups, RNG& rng)
{
    for(auto& u : powerups){
        if(!u.alive){
            u.alive = true;
            u.type = static_cast<PowerUpType>(rng.irand(0, 5));
            u.ttl = POWERUP_TTL;
            u.x = rng.frand(20.f, W - 20.f);
            u.y = rng.frand(H * 0.5f - 20.f, H * 0.5f + 20.f);
            u.vx = rng.frand(-12.f, 12.f);
            u.phase = rng.frand(0.f, 6.28f);
            return;
        }
    }
}

void updatePowerUps(std::array<PowerUp, MAX_POWERUPS>& powerups, float dt)
{
    for(auto& u : powerups){
        if(!u.alive) continue;
        u.ttl -= dt;
        u.phase += dt * 2.8f;
        u.x += u.vx * dt;
        if(u.x < POWERUP_R)     { u.x = POWERUP_R;     u.vx =  std::abs(u.vx); }
        if(u.x > W - POWERUP_R) { u.x = W - POWERUP_R; u.vx = -std::abs(u.vx); }
        if(u.ttl <= 0.f) u.alive = false;
    }
}

void spawnSpecialStars(std::array<SpecialStar, MAX_SPECIAL_STARS>& stars, RNG& rng)
{
    for(auto& s : stars) s.alive = false;

    for(int i = 0; i < MAX_SPECIAL_STARS; ++i){
        bool placed = false;
        for(int tries = 0; tries < 200 && !placed; ++tries){
            float sx = rng.frand(W * 0.2f, W * 0.8f);
            float sy = rng.frand(H * 0.25f, H * 0.75f);
            bool ok = true;
            for(int j = 0; j < i; ++j){
                float dx = sx - stars[j].x;
                float dy = sy - stars[j].y;
                if(dx * dx + dy * dy < 40.f * 40.f){ ok = false; break; }
            }
            if(ok){
                float angle = rng.frand(0.f, PI * 2.f);
                stars[i].x = sx;
                stars[i].y = sy;
                stars[i].vx = std::cos(angle) * SPECIAL_STAR_SPD;
                stars[i].vy = std::sin(angle) * SPECIAL_STAR_SPD;
                stars[i].phase = rng.frand(0.f, PI * 2.f);
                stars[i].alive = true;
                placed = true;
            }
        }
    }
}

void updateSpecialStars(std::array<SpecialStar, MAX_SPECIAL_STARS>& stars, float dt)
{
    for(auto& s : stars){
        if(!s.alive) continue;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.phase += dt * 3.2f;

        if(s.x < SPECIAL_STAR_R)     { s.x =  SPECIAL_STAR_R;     s.vx =  std::abs(s.vx); }
        if(s.x > W - SPECIAL_STAR_R) { s.x =  W - SPECIAL_STAR_R; s.vx = -std::abs(s.vx); }
        if(s.y < SPECIAL_STAR_R)     { s.y =  SPECIAL_STAR_R;     s.vy =  std::abs(s.vy); }
        if(s.y > H - SPECIAL_STAR_R) { s.y =  H - SPECIAL_STAR_R; s.vy = -std::abs(s.vy); }
    }
}

} // namespace arena_layout
