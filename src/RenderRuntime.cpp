#include "RenderRuntime.h"

#include "GameConstants.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace render_runtime {
namespace {

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

sf::Color hsvToRgb(float h, float s, float v)
{
    h = std::fmod(h, 1.f); if(h < 0.f) h += 1.f;
    float hh = h * 6.f;
    int   i  = static_cast<int>(hh);
    float f  = hh - i;
    float p  = v * (1.f - s);
    float q  = v * (1.f - s * f);
    float tv = v * (1.f - s * (1.f - f));
    auto u = [](float x) -> uint8_t { return static_cast<uint8_t>(clampf(x * 255.f, 0.f, 255.f)); };
    switch(i % 6){
        case 0: return { u(v),  u(tv), u(p)  };
        case 1: return { u(q),  u(v),  u(p)  };
        case 2: return { u(p),  u(v),  u(tv) };
        case 3: return { u(p),  u(q),  u(v)  };
        case 4: return { u(tv), u(p),  u(v)  };
        default: return { u(v),  u(p),  u(q)  };
    }
}

// Hash-based value noise for the title menu's drifting cloud masses.
float hashVal(float x, float y)
{
    float s = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f;
    return s - std::floor(s);
}

float vnoise(float x, float y)
{
    int xi = static_cast<int>(std::floor(x));
    int yi = static_cast<int>(std::floor(y));
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float u = xf * xf * (3.f - 2.f * xf);
    float v = yf * yf * (3.f - 2.f * yf);
    float a = hashVal(xi,     yi);
    float b = hashVal(xi + 1, yi);
    float c = hashVal(xi,     yi + 1);
    float d = hashVal(xi + 1, yi + 1);
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}

// Thresholded fractal noise -> localized darker cloud forms (0 = none, 1 = dense).
float cloudField(float wx, float wy, float t2)
{
    float nx = wx * 0.006f + t2 * 0.018f;
    float ny = wy * 0.006f - t2 * 0.012f;
    // Domain warping for organic, drifting edges.
    float qx = nx + vnoise(nx * 1.4f, ny * 1.4f) * 0.9f;
    float qy = ny + vnoise(ny * 1.4f + 7.f, nx * 1.4f) * 0.9f;
    float n = vnoise(qx, qy) * 0.66f
            + vnoise(qx * 2.7f, qy * 2.7f) * 0.24f
            + vnoise(qx * 5.3f, qy * 5.3f) * 0.10f;
    // High threshold -> only the strongest noise features become clouds, so the
    // masses stay localized and distinct instead of covering the whole field.
    float c = clampf((n - 0.60f) / 0.22f, 0.f, 1.f);
    return c * c * (3.f - 2.f * c); // smoothstep -> distinct masses
}

void hueToRgb(float h, uint8_t& ro, uint8_t& go, uint8_t& bo)
{
    float s = 0.85f;
    float v = 1.0f;
    float c = v * s;
    float x = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f));
    float m = v - c;
    float rf, gf, bf;
    if(h < 60.f){ rf = c; gf = x; bf = 0.f; }
    else if(h < 120.f){ rf = x; gf = c; bf = 0.f; }
    else if(h < 180.f){ rf = 0.f; gf = c; bf = x; }
    else if(h < 240.f){ rf = 0.f; gf = x; bf = c; }
    else if(h < 300.f){ rf = x; gf = 0.f; bf = c; }
    else { rf = c; gf = 0.f; bf = x; }
    ro = static_cast<uint8_t>((rf + m) * 255.f);
    go = static_cast<uint8_t>((gf + m) * 255.f);
    bo = static_cast<uint8_t>((bf + m) * 255.f);
}

sf::Color powerupColor(PowerUpType t)
{
    switch(t){
        case PowerUpType::SHIELD: return sf::Color(60, 160, 255);
        case PowerUpType::RAPID: return sf::Color(255, 220, 40);
        case PowerUpType::SPREAD: return sf::Color(120, 255, 100);
        case PowerUpType::HEAL: return sf::Color(80, 255, 180);
        case PowerUpType::CHAOS: return sf::Color(220, 60, 255);
        case PowerUpType::REVERSE: return sf::Color(255, 100, 30);
        default: return sf::Color::White;
    }
}

void drawSpecialStar(sf::RenderTarget& rt, const SpecialStar& s, float t)
{
    if(!s.alive) return;

    float pulse = 0.6f + 0.4f * std::sin(t * 6.f + s.phase);
    uint8_t ca = static_cast<uint8_t>(220 + 35 * pulse);
    sf::Color col(255, static_cast<uint8_t>(200 * pulse), 0, ca);

    for(int layer = 4; layer >= 1; --layer){
        float gr = SPECIAL_STAR_R + layer * 2.8f;
        sf::CircleShape halo(gr);
        halo.setOrigin(sf::Vector2f(gr, gr));
        halo.setPosition(sf::Vector2f(s.x, s.y));
        uint8_t ga = static_cast<uint8_t>((4 + layer * 6) * pulse);
        halo.setFillColor(sf::Color(255, 160, 0, ga));
        rt.draw(halo, sf::BlendAdd);
    }

    {
        sf::VertexArray va(sf::PrimitiveType::LineStrip, 6);
        for(int i = 0; i <= 5; ++i){
            float a = s.phase + i * (2.f * PI / 5.f);
            va[i].position = {s.x + std::cos(a) * SPECIAL_STAR_R,
                              s.y + std::sin(a) * SPECIAL_STAR_R};
            va[i].color = col;
        }
        rt.draw(va);
    }
    {
        sf::VertexArray va(sf::PrimitiveType::LineStrip, 6);
        for(int i = 0; i <= 5; ++i){
            float a = -s.phase * 0.7f + i * (2.f * PI / 5.f);
            va[i].position = {s.x + std::cos(a) * (SPECIAL_STAR_R * 0.55f),
                              s.y + std::sin(a) * (SPECIAL_STAR_R * 0.55f)};
            va[i].color = sf::Color(255, 255, 80, static_cast<uint8_t>(180 * pulse));
        }
        rt.draw(va, sf::BlendAdd);
    }
    sf::CircleShape core(2.f);
    core.setFillColor(sf::Color(255, 255, 180, ca));
    core.setOrigin(sf::Vector2f(2.f, 2.f));
    core.setPosition(sf::Vector2f(s.x, s.y));
    rt.draw(core, sf::BlendAdd);
}

} // namespace

void drawMirror(sf::RenderTarget& rt, const Mirror& m, float t, int fxLevel)
{
    float flash = clampf(m.hitFlash, 0.f, 1.f);
    float pulse = 0.6f + 0.4f * std::sin(t * 4.f + m.x * 0.07f);

    float cr = std::min(255.f, (80  + flash*175.f) * pulse);
    float cg = std::min(255.f, (120 + flash*135.f) * pulse);
    float cb = std::min(255.f, (220 + flash* 35.f) * pulse);
    sf::Color coreCol(uint8_t(cr), uint8_t(cg), uint8_t(cb), 255);

    float R = MIRROR_R - 1.f;
    sf::Vector2f ep0, ep1;
    if(m.slash){
        ep0 = { m.x - R, m.y + R };
        ep1 = { m.x + R, m.y - R };
    } else {
        ep0 = { m.x - R, m.y - R };
        ep1 = { m.x + R, m.y + R };
    }

    if(fxLevel >= 3){
        sf::Vertex line[2];
        line[0].position = ep0; line[0].color = coreCol;
        line[1].position = ep1; line[1].color = coreCol;
        rt.draw(line, 2, sf::PrimitiveType::Lines);
        return;
    }

    auto thickLine = [&](sf::Vector2f a, sf::Vector2f b, float w, sf::Color col){
        float dx = b.x - a.x, dy = b.y - a.y;
        float len = std::sqrt(dx*dx + dy*dy);
        if(len < 0.0001f) return;
        float nx = -dy/len * w * 0.5f;
        float ny =  dx/len * w * 0.5f;
        sf::VertexArray q(sf::PrimitiveType::TriangleStrip, 4);
        q[0].position = { a.x+nx, a.y+ny }; q[0].color = col;
        q[1].position = { a.x-nx, a.y-ny }; q[1].color = col;
        q[2].position = { b.x+nx, b.y+ny }; q[2].color = col;
        q[3].position = { b.x-nx, b.y-ny }; q[3].color = col;
        rt.draw(q);
    };

    if(fxLevel <= 1)
        thickLine(ep0, ep1, 10.f, sf::Color(uint8_t(cr*0.12f), uint8_t(cg*0.12f), uint8_t(cb*0.12f),
                                             uint8_t(35 + flash*20)));
    thickLine(ep0, ep1,  5.f, sf::Color(uint8_t(cr*0.30f), uint8_t(cg*0.30f), uint8_t(cb*0.30f),
                                         uint8_t(80 + flash*40)));
    thickLine(ep0, ep1,  2.f, sf::Color(uint8_t(cr*0.65f), uint8_t(cg*0.65f), uint8_t(cb*0.65f),
                                         uint8_t(160 + flash*60)));

    {
        sf::Vertex line[2];
        line[0].position = ep0; line[0].color = coreCol;
        line[1].position = ep1; line[1].color = coreCol;
        rt.draw(line, 2, sf::PrimitiveType::Lines);
    }

    if(fxLevel <= 1)
    for(int i = 0; i < 2; ++i){
        sf::Vector2f pt = (i == 0) ? ep0 : ep1;
        float gs = 2.5f + flash * 1.5f;
        sf::VertexArray gem(sf::PrimitiveType::TriangleFan, 6);
        gem[0].position = pt;
        gem[0].color    = sf::Color(255, 255, 255, uint8_t(200 + flash*55));
        gem[1].position = { pt.x,      pt.y - gs }; gem[1].color = coreCol;
        gem[2].position = { pt.x + gs, pt.y      }; gem[2].color = sf::Color(uint8_t(cr*0.5f), uint8_t(cg*0.5f), uint8_t(cb*0.5f), 180);
        gem[3].position = { pt.x,      pt.y + gs }; gem[3].color = coreCol;
        gem[4].position = { pt.x - gs, pt.y      }; gem[4].color = sf::Color(uint8_t(cr*0.5f), uint8_t(cg*0.5f), uint8_t(cb*0.5f), 180);
        gem[5].position = { pt.x,      pt.y - gs }; gem[5].color = coreCol;
        rt.draw(gem);
    }
}

void drawShip(sf::RenderTarget& rt, const Player& p, int id, float t,
              const sf::Texture* tex1, const sf::Texture* tex2, int fxLevel)
{
    if(p.invulnTimer > 0.f){
        float blink = std::fmod(p.invulnTimer * 10.f, 1.f);
        if(blink < 0.5f) return;
    }

    const sf::Texture* tex = (id == 1) ? tex1 : tex2;
    if(tex){
        sf::Vector2u tsz = tex->getSize();
        constexpr float TARGET_W = 48.f;
        float sc = TARGET_W / static_cast<float>(tsz.x);

        sf::Color tint = sf::Color::White;
        if(p.slowTimer > 0.f){
            float st = clampf(p.slowTimer / 2.f, 0.f, 1.f);
            uint8_t dim = static_cast<uint8_t>(255 - st * 110);
            tint = sf::Color(static_cast<uint8_t>(dim * 0.82f),
                             static_cast<uint8_t>(dim * 0.65f),
                             static_cast<uint8_t>(dim * 0.65f));
        } else if(p.reverseTimer > 0.f){
            tint = sf::Color(200, 80, 255, 255);
        }

        sf::Sprite sprite(*tex);
        sprite.setOrigin(sf::Vector2f(tsz.x * 0.5f, tsz.y * 0.5f));
        sprite.setPosition(sf::Vector2f(p.x, p.y));
        sprite.setColor(tint);
        sprite.setScale(sf::Vector2f(sc, (id == 2) ? -sc : sc));
        rt.draw(sprite);

        if(p.flashTimer > 0.f){
            sf::Sprite flash(*tex);
            flash.setOrigin(sf::Vector2f(tsz.x * 0.5f, tsz.y * 0.5f));
            flash.setPosition(sf::Vector2f(p.x, p.y));
            flash.setScale(sf::Vector2f(sc, (id == 2) ? -sc : sc));
            uint8_t fa = static_cast<uint8_t>(200.f * clampf(p.flashTimer / 0.14f, 0.f, 1.f));
            flash.setColor(sf::Color(255, 255, 200, fa));
            rt.draw(flash, sf::RenderStates(sf::BlendAdd));
        }
    } else {
        sf::RectangleShape fb(sf::Vector2f(24.f, 12.f));
        fb.setOrigin(sf::Vector2f(12.f, 6.f));
        fb.setPosition(sf::Vector2f(p.x, p.y));
        fb.setFillColor((id == 1) ? sf::Color(0, 220, 220) : sf::Color(220, 60, 60));
        rt.draw(fb);
    }

    if(p.shieldTimer > 0.f){
        float pulse  = 0.6f + 0.4f * std::sin(t * 9.f);
        float pulse2 = 0.5f + 0.5f * std::sin(t * 6.f + 1.2f);
        float baseSr = 20.f + pulse * 2.5f;

        float hue  = std::fmod(t * 0.25f + p.glowPhase * 0.03f, 1.f);
        float hue2 = std::fmod(hue + 0.33f, 1.f);
        sf::Color cyc  = hsvToRgb(hue,  0.95f, 1.0f);
        sf::Color cyc2 = hsvToRgb(hue2, 0.90f, 1.0f);

        {
            float r = baseSr;
            sf::CircleShape ring(r);
            ring.setOrigin(sf::Vector2f(r, r));
            ring.setPosition(sf::Vector2f(p.x, p.y));
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.f);
            uint8_t a = static_cast<uint8_t>(70 * pulse);
            ring.setOutlineColor(sf::Color(cyc.r, cyc.g, cyc.b, a));
            rt.draw(ring, sf::RenderStates(sf::BlendAdd));
        }

        sf::CircleShape fill(baseSr * 0.75f);
        fill.setOrigin(sf::Vector2f(baseSr*0.75f, baseSr*0.75f));
        fill.setPosition(sf::Vector2f(p.x, p.y));
        fill.setFillColor(sf::Color(cyc.r, cyc.g, cyc.b,
                                    static_cast<uint8_t>(45 * pulse)));
        rt.draw(fill, sf::RenderStates(sf::BlendAdd));

        int ORBS = (fxLevel >= 2) ? 1 : 3;
        float orbitR = baseSr * 1.12f;
        float orbitSpd = t * 3.2f;
        for(int i = 0; i < ORBS; ++i){
            float ang  = orbitSpd + (2.f * PI * i) / ORBS;
            float ox   = p.x + std::cos(ang) * orbitR;
            float oy   = p.y + std::sin(ang) * orbitR;

            sf::CircleShape orbHalo(5.f);
            orbHalo.setOrigin(sf::Vector2f(5.f, 5.f));
            orbHalo.setPosition(sf::Vector2f(ox, oy));
            orbHalo.setFillColor(sf::Color(cyc.r, cyc.g, cyc.b,
                                           static_cast<uint8_t>(80 * pulse)));
            rt.draw(orbHalo, sf::RenderStates(sf::BlendAdd));

            sf::CircleShape orbCore(2.f);
            orbCore.setOrigin(sf::Vector2f(2.f, 2.f));
            orbCore.setPosition(sf::Vector2f(ox, oy));
            orbCore.setFillColor(sf::Color(255, 255, 255,
                                           static_cast<uint8_t>(200 * pulse)));
            rt.draw(orbCore, sf::RenderStates(sf::BlendAdd));

            int trailDots = (fxLevel >= 2) ? 1 : 3;
            for(int tr = 1; tr <= trailDots; ++tr){
                float tang = ang - tr * 0.22f;
                float tx   = p.x + std::cos(tang) * orbitR;
                float ty   = p.y + std::sin(tang) * orbitR;
                float tf   = 1.f - tr * 0.28f;
                sf::CircleShape tail(1.5f);
                tail.setOrigin(sf::Vector2f(1.5f, 1.5f));
                tail.setPosition(sf::Vector2f(tx, ty));
                tail.setFillColor(sf::Color(cyc.r, cyc.g, cyc.b,
                                            static_cast<uint8_t>(120 * pulse * tf)));
                rt.draw(tail, sf::RenderStates(sf::BlendAdd));
            }
        }

        int SPARKS = (fxLevel >= 2) ? 1 : 2;
        float spark_r   = baseSr * 0.88f;
        float sparkSpd  = -(t * 2.0f);
        for(int i = 0; i < SPARKS; ++i){
            float ang = sparkSpd + (2.f * PI * i) / SPARKS;
            float sx  = p.x + std::cos(ang) * spark_r;
            float sy  = p.y + std::sin(ang) * spark_r;

            sf::CircleShape spark(2.5f);
            spark.setOrigin(sf::Vector2f(2.5f, 2.5f));
            spark.setPosition(sf::Vector2f(sx, sy));
            spark.setFillColor(sf::Color(cyc2.r, cyc2.g, cyc2.b,
                                         static_cast<uint8_t>(160 * pulse2)));
            rt.draw(spark, sf::RenderStates(sf::BlendAdd));
        }
    }
}

void drawBullets(sf::RenderTarget& rt,
                 const std::array<Bullet, MAX_BULLETS>& bullets)
{
    static sf::VertexArray halos(sf::PrimitiveType::Triangles);
    static sf::VertexArray cores(sf::PrimitiveType::Triangles);
    halos.clear();
    cores.clear();
    for(const auto& b : bullets){
        if(!b.alive) continue;
        sf::Color coreCol, haloCol;
        if(b.owner == 1){
            coreCol = sf::Color(100, 255, 255, 255);
            haloCol = sf::Color(0, 180, 255, 70);
        } else {
            coreCol = sf::Color(255, 140, 60, 255);
            haloCol = sf::Color(255, 60, 0, 70);
        }
        {
            float x = b.x - 2.f, y = b.y - 3.5f;
            sf::Vertex v; v.color = haloCol;
            v.position = {x,      y};      halos.append(v);
            v.position = {x+4.f,  y};      halos.append(v);
            v.position = {x,      y+7.f};  halos.append(v);
            v.position = {x+4.f,  y};      halos.append(v);
            v.position = {x+4.f,  y+7.f};  halos.append(v);
            v.position = {x,      y+7.f};  halos.append(v);
        }
        {
            float x = b.x - 1.f, y = b.y - 2.5f;
            sf::Vertex v; v.color = coreCol;
            v.position = {x,      y};      cores.append(v);
            v.position = {x+2.f,  y};      cores.append(v);
            v.position = {x,      y+5.f};  cores.append(v);
            v.position = {x+2.f,  y};      cores.append(v);
            v.position = {x+2.f,  y+5.f};  cores.append(v);
            v.position = {x,      y+5.f};  cores.append(v);
        }
    }
    if(halos.getVertexCount() > 0) rt.draw(halos);
    if(cores.getVertexCount() > 0) rt.draw(cores);
}

void drawParticles(sf::RenderTarget& rt, const std::array<Particle, MAX_PARTICLES>& particles)
{
    static sf::VertexArray va(sf::PrimitiveType::Triangles);
    va.clear();
    for(const auto& p : particles){
        if(!p.alive || p.a == 0) continue;
        float sz = std::max(1.f, p.size);
        float x = p.x - sz * 0.5f;
        float y = p.y - sz * 0.5f;
        sf::Color col(p.r, p.g, p.b, p.a);
        sf::Vertex v;
        v.color = col;
        v.position = {x, y}; va.append(v);
        v.position = {x + sz, y}; va.append(v);
        v.position = {x, y + sz}; va.append(v);
        v.position = {x + sz, y}; va.append(v);
        v.position = {x + sz, y + sz}; va.append(v);
        v.position = {x, y + sz}; va.append(v);
    }
    if(va.getVertexCount() > 0) rt.draw(va);
}

void initStars(std::array<Star, NUM_STARS>& stars, RNG& rng)
{
    for(auto& s : stars){
        s.x = rng.frand(0.f, static_cast<float>(W));
        s.y = rng.frand(0.f, static_cast<float>(H));
        s.speed = rng.frand(1.5f, 12.f);
        s.bright = rng.frand(0.3f, 1.f);
        s.blinkTimer = 0.f;
    }
}

void updateStars(std::array<Star, NUM_STARS>& stars, float dt, RNG& rng)
{
    for(auto& s : stars){
        s.y += s.speed * dt;
        if(s.y > H + 1.f){
            s.y = rng.frand(-2.f, 0.f);
            s.x = rng.frand(0.f, static_cast<float>(W));
            s.speed = rng.frand(1.5f, 12.f);
            s.bright = rng.frand(0.3f, 1.f);
            s.blinkTimer = 0.f;
        }
        if(s.blinkTimer <= 0.f){
            if(rng.frand(0.f, 1.f) < dt * 2.f / NUM_STARS)
                s.blinkTimer = rng.frand(0.08f, 0.22f);
        } else {
            s.blinkTimer -= dt;
        }
    }
}

void drawStars(sf::RenderTarget& rt,
               const std::array<Star, NUM_STARS>& stars,
               float globalBright,
               bool menuOval)
{
    static sf::RectangleShape dot(sf::Vector2f(1.f, 1.f));
    static sf::RectangleShape glow(sf::Vector2f(3.f, 3.f));
    for(const auto& s : stars){
        float b = s.bright * globalBright;
        // In the title menu, darken stars outside the oval mask along with the
        // background so the masked region keeps its full brightness.
        if(menuOval && !isInsideMenuOval(s.x, s.y))
            b *= kMenuMaskDarkenFactor;
        float depth = clampf((s.speed - 1.5f) / 10.5f, 0.f, 1.f);
        if(s.blinkTimer > 0.f){
            float flare = std::sin(s.blinkTimer / 0.22f * PI);
            b = std::min(1.f, b + flare * 2.2f);
            uint8_t gv = static_cast<uint8_t>(std::min(255.f, flare * 180.f));
            glow.setPosition(sf::Vector2f(s.x - 1.f, s.y - 1.f));
            glow.setFillColor(sf::Color(gv, gv, static_cast<uint8_t>(std::min(255, gv + 60)), 120));
            rt.draw(glow);
        }
        uint8_t v = static_cast<uint8_t>(std::min(255.f, b * 200.f));
        uint8_t cr = static_cast<uint8_t>(std::min(255.f, v * (0.90f + 0.10f * depth)));
        uint8_t cg = static_cast<uint8_t>(std::min(255.f, v * (0.92f + 0.08f * depth)));
        uint8_t cb = static_cast<uint8_t>(std::min(255.f, v * (1.05f - 0.05f * depth) + 25.f * (1.f - depth)));
        dot.setPosition(sf::Vector2f(s.x, s.y));
        dot.setFillColor(sf::Color(cr, cg, cb, 255));
        rt.draw(dot);
    }
}

void initSpectStars(std::array<SpectStar, NUM_SPECT>& stars)
{
    for(auto& s : stars){
        s.alive = false;
        s.ttl = 0.f;
        s.maxttl = 0.f;
    }
}

void updateSpectStars(std::array<SpectStar, NUM_SPECT>& stars, float dt, RNG& rng, int state)
{
    float spawnRateTotal = (state == 0) ? 0.6f : 0.15f;
    for(auto& s : stars){
        if(s.alive){
            s.ttl -= dt;
            s.rot += s.rotSpeed * dt;
            s.hue += 40.f * dt;
            if(s.hue >= 360.f) s.hue -= 360.f;
            hueToRgb(s.hue, s.r, s.g, s.b);
            if(s.ttl <= 0.f) s.alive = false;
        } else if(rng.frand(0.f, 1.f) < dt * spawnRateTotal / static_cast<float>(NUM_SPECT)) {
            s.alive = true;
            s.x = rng.frand(0.f, static_cast<float>(W));
            s.y = rng.frand(0.f, static_cast<float>(H));
            s.maxttl = rng.frand(0.3f, 0.8f);
            s.ttl = s.maxttl;
            s.size = rng.frand(5.f, 16.f) * ((state == 0) ? 1.0f : 0.5f);
            s.hue = rng.frand(0.f, 360.f);
            hueToRgb(s.hue, s.r, s.g, s.b);
            s.numRays = rng.irand(0, 1) ? 6 : 4;
            s.rotSpeed = rng.frand(-1.5f, 1.5f);
            s.rot = rng.frand(0.f, 6.28f);
        }
    }
}

void drawSpectStars(sf::RenderTarget& rt, const std::array<SpectStar, NUM_SPECT>& stars)
{
    for(const auto& s : stars){
        if(!s.alive) continue;
        float lifeT = s.ttl / s.maxttl;
        float env;
        if(lifeT > 0.85f) env = (1.f - lifeT) / 0.15f;
        else if(lifeT > 0.2f) env = 1.f;
        else env = lifeT / 0.2f;
        env = env * env;
        float pulse = 0.8f + 0.2f * std::sin(s.ttl * 18.f);

        for(int layer = 2; layer >= 0; --layer){
            float lf = 1.0f - layer * 0.3f;
            sf::CircleShape c;
            float rad = s.size * lf * pulse;
            c.setRadius(rad);
            c.setOrigin(sf::Vector2f(rad, rad));
            c.setPosition(sf::Vector2f(s.x, s.y));
            float a = env * (0.25f - layer * 0.06f);
            uint8_t ca = static_cast<uint8_t>(std::min(255.f, a * 255.f));
            c.setFillColor(sf::Color(s.r, s.g, s.b, ca));
            rt.draw(c, sf::RenderStates(sf::BlendAdd));
        }

        int nRays = s.numRays;
        float rayLen = s.size * 1.8f * pulse * env;
        float rayW = std::max(0.5f, s.size * 0.08f);
        sf::RectangleShape ray(sf::Vector2f(rayLen, rayW));
        ray.setOrigin(sf::Vector2f(rayLen * 0.5f, rayW * 0.5f));
        ray.setPosition(sf::Vector2f(s.x, s.y));
        uint8_t ra = static_cast<uint8_t>(std::min(255.f, env * pulse * 220.f));
        ray.setFillColor(sf::Color(s.r, s.g, s.b, ra));
        float angleStep = 3.14159265f / static_cast<float>(nRays);
        for(int i = 0; i < nRays; ++i){
            float angle = s.rot + i * angleStep;
            ray.setRotation(sf::radians(angle));
            rt.draw(ray, sf::RenderStates(sf::BlendAdd));
        }

        float ray2Len = rayLen * 0.6f;
        float ray2W = std::max(0.4f, rayW * 0.5f);
        sf::RectangleShape ray2(sf::Vector2f(ray2Len, ray2W));
        ray2.setOrigin(sf::Vector2f(ray2Len * 0.5f, ray2W * 0.5f));
        ray2.setPosition(sf::Vector2f(s.x, s.y));
        uint8_t r2a = static_cast<uint8_t>(std::min(255.f, env * pulse * 140.f));
        ray2.setFillColor(sf::Color(255, 255, 255, r2a));
        for(int i = 0; i < nRays; ++i){
            float angle = s.rot + (i + 0.5f) * angleStep;
            ray2.setRotation(sf::radians(angle));
            rt.draw(ray2, sf::RenderStates(sf::BlendAdd));
        }

        sf::CircleShape core;
        float coreR = s.size * 0.18f * pulse;
        core.setRadius(coreR);
        core.setOrigin(sf::Vector2f(coreR, coreR));
        core.setPosition(sf::Vector2f(s.x, s.y));
        uint8_t wa = static_cast<uint8_t>(std::min(255.f, env * 255.f));
        core.setFillColor(sf::Color(255, 255, 255, wa));
        rt.draw(core, sf::RenderStates(sf::BlendAdd));

        float streakLen = s.size * 2.5f * env;
        float streakW = std::max(0.3f, s.size * 0.04f);
        sf::RectangleShape streak(sf::Vector2f(streakLen, streakW));
        streak.setOrigin(sf::Vector2f(streakLen * 0.5f, streakW * 0.5f));
        streak.setPosition(sf::Vector2f(s.x, s.y));
        uint8_t sa = static_cast<uint8_t>(std::min(200.f, env * 160.f));
        streak.setFillColor(sf::Color(200, 230, 255, sa));
        streak.setRotation(sf::radians(0.f));
        rt.draw(streak, sf::RenderStates(sf::BlendAdd));
    }
}

void drawPlasmaDivider(sf::RenderTarget& rt, float t)
{
    static sf::RectangleShape seg(sf::Vector2f(2.f, 2.f));
    for(int x = 0; x < W; x += 2){
        float wave = std::sin(x * 0.12f + t * 3.5f) * 2.0f
                   + std::sin(x * 0.08f - t * 2.2f) * 1.5f;
        float ypos = H / 2.f + wave;
        float phase = std::fmod(x * 0.04f + t * 2.f, 2.f * PI);
        float glow = 0.6f + 0.4f * std::sin(phase);
        uint8_t r8 = static_cast<uint8_t>(15 + glow * 25);
        uint8_t g8 = static_cast<uint8_t>(40 + glow * 40);
        uint8_t b8 = static_cast<uint8_t>(90 + glow * 38);
        seg.setPosition(sf::Vector2f(static_cast<float>(x), ypos - 0.5f));
        seg.setFillColor(sf::Color(r8, g8, b8, 120));
        rt.draw(seg);
        seg.setPosition(sf::Vector2f(static_cast<float>(x), ypos));
        seg.setFillColor(sf::Color(
            static_cast<uint8_t>(std::min(255.f, r8 + 40.f * glow)),
            static_cast<uint8_t>(std::min(255.f, g8 + 40.f * glow)),
            static_cast<uint8_t>(std::min(255.f, b8 + 60.f)), 200));
        rt.draw(seg);
    }
}

void drawScanlines(sf::RenderTarget& rt)
{
    static sf::RectangleShape line(sf::Vector2f(static_cast<float>(W), 1.f));
    line.setFillColor(sf::Color(0, 0, 0, 55));
    for(int y = 1; y < H; y += 2){
        line.setPosition(sf::Vector2f(0.f, static_cast<float>(y)));
        rt.draw(line);
    }
}

void drawSpecialStars(sf::RenderTarget& rt,
                      const std::array<SpecialStar, MAX_SPECIAL_STARS>& stars,
                      float t)
{
    for(const auto& s : stars) drawSpecialStar(rt, s, t);
}

void drawPowerUp(sf::RenderTarget& rt, const PowerUp& powerUp, float t)
{
    if(!powerUp.alive) return;
    sf::Color col = powerupColor(powerUp.type);

    float alpha = (powerUp.ttl < 2.f) ? clampf(powerUp.ttl * 0.5f, 0.f, 1.f) : 1.f;
    float pulse = 0.7f + 0.3f * std::sin(t * 4.f + powerUp.phase);

    auto acol = [&](sf::Color c, float a) -> sf::Color {
        return sf::Color(static_cast<uint8_t>(c.r * pulse),
                         static_cast<uint8_t>(c.g * pulse),
                         static_cast<uint8_t>(c.b * pulse),
                         static_cast<uint8_t>(255 * a * alpha));
    };

    sf::CircleShape glow(POWERUP_R + 3.f);
    glow.setPosition(sf::Vector2f(powerUp.x - POWERUP_R - 3.f, powerUp.y - POWERUP_R - 3.f));
    glow.setFillColor(sf::Color(col.r / 3, col.g / 3, col.b / 3,
                                static_cast<uint8_t>(80 * alpha)));
    rt.draw(glow);

    constexpr int SIDES = 6;
    float radius = POWERUP_R;
    sf::VertexArray hex(sf::PrimitiveType::Triangles, SIDES * 3);
    for(int i = 0; i < SIDES; ++i){
        float a0 = powerUp.phase + i * (2.f * PI / SIDES);
        float a1 = powerUp.phase + (i + 1) * (2.f * PI / SIDES);
        hex[i * 3 + 0].position = sf::Vector2f(powerUp.x, powerUp.y);
        hex[i * 3 + 1].position = sf::Vector2f(powerUp.x + radius * std::cos(a0),
                                               powerUp.y + radius * std::sin(a0));
        hex[i * 3 + 2].position = sf::Vector2f(powerUp.x + radius * std::cos(a1),
                                               powerUp.y + radius * std::sin(a1));
        sf::Color fc = acol(col, (i % 2 == 0) ? 1.f : 0.7f);
        hex[i * 3 + 0].color = fc;
        hex[i * 3 + 1].color = fc;
        hex[i * 3 + 2].color = fc;
    }
    rt.draw(hex);

    sf::CircleShape dot(2.f);
    dot.setPosition(sf::Vector2f(powerUp.x - 2.f, powerUp.y - 2.f));
    dot.setFillColor(acol(sf::Color::White, 0.95f));
    rt.draw(dot);
}

void drawBomb(sf::RenderTarget& rt, const Bomb& bomb, float t, int fxLevel)
{
    if(!bomb.alive) return;

    float pulse = 0.5f + 0.5f * std::sin(t * 4.f + bomb.pulsePhase);
    float spin = t * 1.8f + bomb.pulsePhase;
    float spin2 = -(t * 2.7f + bomb.pulsePhase);
    float threat = 0.4f + 0.6f * std::abs(std::sin(t * 8.f + bomb.pulsePhase));

    if(fxLevel >= 3){
        float r = BOMB_R;
        sf::CircleShape body(r);
        body.setOrigin(sf::Vector2f(r, r));
        body.setPosition(sf::Vector2f(bomb.x, bomb.y));
        body.setFillColor(sf::Color(26, 8, 44, 240));
        rt.draw(body);

        sf::CircleShape core(2.2f);
        core.setOrigin(sf::Vector2f(2.2f, 2.2f));
        core.setPosition(sf::Vector2f(bomb.x, bomb.y));
        core.setFillColor(sf::Color(220, 120, 255, static_cast<uint8_t>(160 + 60 * pulse)));
        rt.draw(core, sf::BlendAdd);
        return;
    }

    auto seg = [&](float ax, float ay, float bx, float by, sf::Color col){
        sf::Vertex v[2];
        v[0].position = {ax, ay}; v[0].color = col;
        v[1].position = {bx, by}; v[1].color = col;
        rt.draw(v, 2, sf::PrimitiveType::Lines);
    };

    auto circleOutline = [&](float cx, float cy, float r, sf::Color col, int segs = 32){
        static sf::VertexArray va(sf::PrimitiveType::LineStrip, 33);
        va.resize(segs + 1);
        for(int i = 0; i <= segs; ++i){
            float a = i * 2.f * PI / segs;
            va[i].position = {cx + std::cos(a) * r, cy + std::sin(a) * r};
            va[i].color = col;
        }
        rt.draw(va);
    };

    int glowLayers = (fxLevel >= 2) ? 2 : 5;
    for(int layer = glowLayers; layer >= 1; --layer){
        float gr = BOMB_R + layer * 3.2f;
        sf::CircleShape halo(gr);
        halo.setOrigin(sf::Vector2f(gr, gr));
        halo.setPosition(sf::Vector2f(bomb.x, bomb.y));
        uint8_t ga = static_cast<uint8_t>((5 + layer * 5) * pulse);
        halo.setFillColor(sf::Color(160, 0, 255, ga));
        rt.draw(halo);
    }

    float ro = BOMB_R + 5.f;
    int outerTicks = (fxLevel >= 2) ? 3 : 6;
    for(int i = 0; i < outerTicks; ++i){
        float a0 = spin + i * (2.f * PI / 6.f);
        float a1 = a0 + 0.25f;
        float px0 = bomb.x + std::cos(a0) * ro;
        float py0 = bomb.y + std::sin(a0) * ro;
        float px1 = bomb.x + std::cos(a1) * ro;
        float py1 = bomb.y + std::sin(a1) * ro;
        uint8_t cv = static_cast<uint8_t>(180 + 75 * pulse);
        seg(px0, py0, px1, py1, sf::Color(cv, 0, 255, 200));
    }

    float ri = BOMB_R + 1.5f;
    int innerTicks = (fxLevel >= 2) ? 2 : 4;
    for(int i = 0; i < innerTicks; ++i){
        float a0 = spin2 + i * (PI * 0.5f);
        float a1 = a0 + 0.35f;
        float px0 = bomb.x + std::cos(a0) * ri;
        float py0 = bomb.y + std::sin(a0) * ri;
        float px1 = bomb.x + std::cos(a1) * ri;
        float py1 = bomb.y + std::sin(a1) * ri;
        uint8_t cv = static_cast<uint8_t>(120 + 135 * threat);
        seg(px0, py0, px1, py1, sf::Color(255, cv, 255, 210));
    }

    {
        float r = BOMB_R;
        sf::CircleShape body(r);
        body.setOrigin(sf::Vector2f(r, r));
        body.setPosition(sf::Vector2f(bomb.x, bomb.y));
        body.setFillColor(sf::Color(8, 0, 18));
        rt.draw(body);
    }

    circleOutline(bomb.x, bomb.y, BOMB_R - 2.f,
                  sf::Color(200, 0, 255, static_cast<uint8_t>(160 + 95 * pulse)));

    {
        float cr = 2.2f * (0.6f + 0.4f * threat);
        sf::CircleShape node(cr);
        node.setOrigin(sf::Vector2f(cr, cr));
        node.setPosition(sf::Vector2f(bomb.x, bomb.y));
        node.setFillColor(sf::Color(220, 140, 255, static_cast<uint8_t>(200 + 55 * threat)));
        rt.draw(node);
    }

    float spokeLen = BOMB_R - 2.5f;
    float spokeA = spin2 * 0.5f;
    int spokeCount = (fxLevel >= 2) ? 2 : 4;
    for(int i = 0; i < spokeCount; ++i){
        float a = spokeA + i * (PI * 0.5f);
        uint8_t sa = static_cast<uint8_t>(80 + 80 * pulse);
        seg(bomb.x, bomb.y,
            bomb.x + std::cos(a) * spokeLen,
            bomb.y + std::sin(a) * spokeLen,
            sf::Color(180, 80, 255, sa));
    }
}

void drawAllBarriers(sf::RenderTarget& rt,
                     const std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers,
                     float t)
{
    static sf::VertexArray va(sf::PrimitiveType::Triangles);
    va.clear();

    for(const auto& brick : barriers){
        if(!brick.alive) continue;
        const float flash = clampf(brick.hitFlash, 0.f, 1.f);
        const float left = brick.x - BRICK_W * 0.5f;

        for(int xi = 0; xi < static_cast<int>(BRICK_W); xi += 2){
            float wx = left + xi;
            float wave = std::sin(wx * 0.12f + t * 3.5f) * 2.0f
                       + std::sin(wx * 0.08f - t * 2.2f) * 1.5f;
            float ypos = brick.y + wave;

            float phase = std::fmod(wx * 0.04f + t * 2.f, 2.f * PI);
            float glow = 0.6f + 0.4f * std::sin(phase);
            glow = glow + flash * (1.f - glow);

            uint8_t r8 = static_cast<uint8_t>(std::min(255.f, 30.f + glow * 50.f + flash * 175.f));
            uint8_t g8 = static_cast<uint8_t>(std::min(255.f, 80.f + glow * 80.f + flash * 95.f));
            uint8_t b8 = static_cast<uint8_t>(std::min(255.f, 180.f + glow * 75.f));

            auto addQuad = [&](float qx, float qy, sf::Color col){
                sf::Vertex v;
                v.color = col;
                v.position = {qx, qy}; va.append(v);
                v.position = {qx + 2.f, qy}; va.append(v);
                v.position = {qx, qy + 2.f}; va.append(v);
                v.position = {qx + 2.f, qy}; va.append(v);
                v.position = {qx + 2.f, qy + 2.f}; va.append(v);
                v.position = {qx, qy + 2.f}; va.append(v);
            };

            if(brick.hp >= 3){
                sf::Color outerCol(r8 / 4, g8 / 4, b8 / 3, static_cast<uint8_t>(80 * glow));
                addQuad(wx, ypos - 2.f, outerCol);
                addQuad(wx, ypos + 2.f, outerCol);
            }
            if(brick.hp >= 2){
                sf::Color innerCol(r8 / 2, g8 / 2, b8 / 2, static_cast<uint8_t>(130 * glow));
                addQuad(wx, ypos - 1.f, innerCol);
                addQuad(wx, ypos + 1.f, innerCol);
            }

            uint8_t cr = static_cast<uint8_t>(std::min(255.f, r8 + 80.f * glow));
            uint8_t cg = static_cast<uint8_t>(std::min(255.f, g8 + 80.f * glow));
            addQuad(wx, ypos, sf::Color(cr, cg, 255, 255));
        }
    }

    if(va.getVertexCount() > 0) rt.draw(va);
}

void drawMenuTitle(sf::RenderTarget& rt, sf::Font& font, float t, int fxLevel)
{
    static sf::Shader titleSweepShader;
    static bool titleSweepShaderInit = false;
    if(!titleSweepShaderInit){
        const std::string kTitleSweepFrag = R"GLSL(
            uniform sampler2D texture;
            uniform float sweepX;
            uniform float softHalf;
            uniform float coreHalf;
            uniform vec4 glowColor;

            void main()
            {
                vec4 px = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;
                float dx = abs(gl_FragCoord.x - sweepX);
                float soft = clamp(1.0 - dx / softHalf, 0.0, 1.0);
                float core = clamp(1.0 - dx / coreHalf, 0.0, 1.0);
                float intensity = soft * 0.28 + core * 0.72;
                float outA = px.a * intensity * glowColor.a;
                gl_FragColor = vec4(glowColor.rgb * outA, outA);
            }
        )GLSL";
        titleSweepShaderInit = titleSweepShader.loadFromMemory(kTitleSweepFrag, sf::Shader::Type::Fragment);
    }

    sf::Text title(font, "MEGACHLAST PvP", 29);
    title.setStyle(sf::Text::Bold);
    sf::Text sub(font, "// ONE SCREEN DUEL //", 9);

    auto tb = title.getLocalBounds();
    auto sb = sub.getLocalBounds();
    float spacing = 6.f;
    float blockH = tb.size.y + spacing + sb.size.y;
    float baseY = H * 0.5f - blockH * 0.5f - 27.f;
    float bob = std::sin(t * 1.6f) * 1.4f;
    float cx = W/2.f - tb.size.x * 0.5f;
    float titleY = baseY + bob;

    float pulse = 0.75f + 0.25f * std::sin(t * 2.8f);
    float cpulse = std::abs(std::sin(t * 0.7f));
    title.setFillColor(sf::Color(
        static_cast<uint8_t>(80  + 175*pulse),
        static_cast<uint8_t>(80  + 120*pulse*cpulse),
        255));
    sf::Text titleShadow = title;
    titleShadow.setFillColor(sf::Color(0, 0, 0, 180));
    titleShadow.setPosition(sf::Vector2f(cx + 3.f, titleY + 3.f));
    rt.draw(titleShadow);
    title.setPosition(sf::Vector2f(cx, titleY));
    rt.draw(title);

    if(fxLevel <= 1 && titleSweepShaderInit){
        float sweepSpan = tb.size.x + 52.f;
        float sweepX = cx - 26.f + std::fmod(t * 140.f, sweepSpan);
        titleSweepShader.setUniform("sweepX", sweepX);
        titleSweepShader.setUniform("softHalf", 18.f);
        titleSweepShader.setUniform("coreHalf", 6.f);
        titleSweepShader.setUniform("glowColor", sf::Glsl::Vec4(0.94f, 0.98f, 1.00f, 0.95f));

        sf::Text titleFx = title;
        titleFx.setFillColor(sf::Color::White);
        titleFx.setPosition(sf::Vector2f(cx, titleY + 1.8f));

        sf::RenderStates rs(sf::BlendAdd);
        rs.shader = &titleSweepShader;
        rt.draw(titleFx, rs);
    }

    float spulse = 0.5f + 0.5f * std::sin(t * 1.9f + 1.f);
    sub.setFillColor(sf::Color(
        static_cast<uint8_t>(60  + 80*spulse),
        static_cast<uint8_t>(60  + 80*spulse),
        static_cast<uint8_t>(160 + 95*spulse)));
    constexpr float subtitleRow = 9.f;
    sub.setPosition(sf::Vector2f(W/2.f - sb.size.x/2.f,
                                 titleY + tb.size.y + spacing + subtitleRow + bob * 0.25f));
    rt.draw(sub);

}

sf::Color evalPlasmaBgColor(bool titleMode, float wx, float wy, float t2)
{
    float v =   std::sin(wx * 0.013f + t2 * 1.1f)
              + std::sin(wy * 0.016f - t2 * 0.7f)
              + std::sin((wx - wy) * 0.0085f + t2 * 0.5f)
              + std::sin(std::sqrt(wx*wx + wy*wy) * 0.005f - t2 * 0.9f);
    v = v * 0.125f + 0.5f;
    if(!titleMode){
        // In-game duel field: unchanged reference palette.
        return sf::Color(
            static_cast<uint8_t>(6  + v * 28.f),
            static_cast<uint8_t>(6  + v * 14.f),
            static_cast<uint8_t>(14 + v * 56.f));
    }
    // Title menu: same blue field as the duel, with distinct darker cloud
    // masses drifting slowly over it.
    float cloud = cloudField(wx, wy, t2);
    float dark = 1.f - 0.50f * cloud;
    // The oval mask region stays at full brightness; the surrounding
    // background is darkened by 20% so the mask stands out.
    dark *= menuMaskFactor(wx, wy);
    return sf::Color(
        static_cast<uint8_t>(clampf((6.f  + v * 28.f) * dark, 0.f, 255.f)),
        static_cast<uint8_t>(clampf((6.f  + v * 14.f) * dark, 0.f, 255.f)),
        static_cast<uint8_t>(clampf((14.f + v * 56.f) * dark, 0.f, 255.f)));
}

void drawPlasmaBg(sf::RenderTarget& rt, float t, bool titleMode)
{
    constexpr int STEP = 40;
    const int GXN = W / STEP + 2;
    const int GYN = H / STEP + 2;

    static sf::VertexArray va(sf::PrimitiveType::Triangles,
                              static_cast<std::size_t>((GXN-1) * (GYN-1) * 6));
    std::size_t vi = 0;
    for(int gy = 0; gy < GYN-1; gy++){
        for(int gx = 0; gx < GXN-1; gx++){
            float x0 = gx * STEP, y0 = gy * STEP;
            float x1 = x0 + STEP, y1 = y0 + STEP;
            sf::Color c00 = evalPlasmaBgColor(titleMode, x0, y0, t);
            sf::Color c10 = evalPlasmaBgColor(titleMode, x1, y0, t);
            sf::Color c01 = evalPlasmaBgColor(titleMode, x0, y1, t);
            sf::Color c11 = evalPlasmaBgColor(titleMode, x1, y1, t);
            va[vi].position = {x0,y0}; va[vi].color = c00; vi++;
            va[vi].position = {x1,y0}; va[vi].color = c10; vi++;
            va[vi].position = {x0,y1}; va[vi].color = c01; vi++;
            va[vi].position = {x1,y0}; va[vi].color = c10; vi++;
            va[vi].position = {x1,y1}; va[vi].color = c11; vi++;
            va[vi].position = {x0,y1}; va[vi].color = c01; vi++;
        }
    }
    rt.draw(va);
}

void drawCopperBars(sf::RenderTarget& rt, float t)
{
    constexpr int BARS = 8;
    static sf::RectangleShape bar(sf::Vector2f((float)W, 1.f));
    for(int y = 0; y < BARS; y++){
        float fade   = 1.f - static_cast<float>(y) / BARS;
        float hue    = std::fmod(t * 0.15f + y * 0.06f, 1.f);
        float bright = 0.18f + 0.08f * std::sin(t * 2.0f + y * 0.5f);
        sf::Color c  = hsvToRgb(hue, 0.55f, bright);
        c.a = static_cast<uint8_t>(40.f * fade * fade);
        bar.setFillColor(c);
        bar.setPosition(sf::Vector2f(0.f, (float)y));
        rt.draw(bar);
        hue    = std::fmod(t * 0.15f + (BARS - 1 - y) * 0.06f + 0.5f, 1.f);
        bright = 0.18f + 0.08f * std::sin(t * 2.0f + (BARS-1-y) * 0.5f + PI);
        c      = hsvToRgb(hue, 0.55f, bright);
        c.a    = static_cast<uint8_t>(40.f * fade * fade);
        bar.setFillColor(c);
        bar.setPosition(sf::Vector2f(0.f, (float)(H - 1 - y)));
        rt.draw(bar);
    }
}

void drawIngameElements(sf::RenderTarget& rt, const IngameElementsContext& context)
{
    if(context.state == GameState::MENU) return;

    context.drawPlasmaDivider();
    for(const auto& m : *context.mirrors) if(m.alive) drawMirror(rt, m, context.menuAnim, context.fxLevel);
    context.drawPowerups();
    context.drawBombs();
    context.drawSpecialStars();
    context.drawBarriers();
    drawShip(rt, *context.p1, 1, context.menuAnim, context.tex1, context.tex2, context.fxLevel);
    drawShip(rt, *context.p2, 2, context.menuAnim, context.tex1, context.tex2, context.fxLevel);
    drawBullets(rt, *context.bullets);
    context.drawParticles();
    context.drawFragFloats();
    context.drawPlayerRows();
}

} // namespace render_runtime
