#include "RenderRuntime.h"

#include "GameConstants.h"

#include <algorithm>
#include <cstdint>

namespace render_runtime {
namespace {

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

} // namespace

bool isPostfxDisabled(bool noPostfx, GameState state, int fxLevel)
{
    return noPostfx || (state == GameState::PLAYING && fxLevel >= 3);
}

void drawPostfxOverlays(sf::RenderTarget& rt, const PostfxContext& context)
{
    if(!context.postfxDisabled) drawCopperBars(rt, context.menuAnim);
    if(!context.postfxDisabled &&
       context.perfLevel != PerfLevel::ULTRA &&
       !(context.state == GameState::PLAYING && context.fxLevel >= 2))
        context.drawScanlines();
}

void drawSceneOverlays(sf::RenderTarget& rt, const SceneOverlayContext& context)
{
    if(context.state == GameState::MENU){
        sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(W), static_cast<float>(H)));
        overlay.setFillColor(sf::Color(0, 0, 0, 195));
        rt.draw(overlay);

        float cx = W * 0.5f;
        float cy = H * 0.45f;
        float rx = W * 0.38f;
        float ry = H * 0.35f;
        sf::CircleShape vig(1.f, 48);
        vig.setScale(sf::Vector2f(rx, ry));
        vig.setOrigin(sf::Vector2f(1.f, 1.f));
        vig.setPosition(sf::Vector2f(cx, cy));
        vig.setFillColor(sf::Color(8, 6, 18, 50));
        rt.draw(vig, sf::RenderStates(sf::BlendAdd));

        context.drawMenuSpectStars();
    }

    if(context.state != GameState::MENU && !context.postfxDisabled){
        for(int i = 0; i < 12; ++i){
            uint8_t a = static_cast<uint8_t>((12 - i) * 3);
            sf::RectangleShape vig(sf::Vector2f(static_cast<float>(W), 1.f));
            vig.setFillColor(sf::Color(0, 0, 0, a));
            vig.setPosition(sf::Vector2f(0.f, static_cast<float>(i)));
            rt.draw(vig);
            vig.setPosition(sf::Vector2f(0.f, static_cast<float>(H - 1 - i)));
            rt.draw(vig);
        }
        for(int i = 0; i < 8; ++i){
            uint8_t a = static_cast<uint8_t>((8 - i) * 3);
            sf::RectangleShape vig(sf::Vector2f(1.f, static_cast<float>(H)));
            vig.setFillColor(sf::Color(0, 0, 0, a));
            vig.setPosition(sf::Vector2f(static_cast<float>(i), 0.f));
            rt.draw(vig);
            vig.setPosition(sf::Vector2f(static_cast<float>(W - 1 - i), 0.f));
            rt.draw(vig);
        }
    }

    (void)context.menuAnim;
}

void compositeToWindow(sf::RenderWindow& win, const CompositeContext& context)
{
    win.clear(sf::Color::Black);

    float scaleX = static_cast<float>(win.getSize().x) / W;
    float scaleY = static_cast<float>(win.getSize().y) / H;

    float ox = 0.f;
    float oy = 0.f;
    if(context.shakeTimer > 0.f && context.shakeDuration > 0.f){
        float norm = clampf(context.shakeTimer / context.shakeDuration, 0.f, 1.f);
        float decay = norm * norm;
        ox = context.randomRange(-context.shakeIntensity, context.shakeIntensity) * decay * scaleX;
        oy = context.randomRange(-context.shakeIntensity, context.shakeIntensity) * decay * scaleY;
    }

    sf::View winView(sf::FloatRect(
        {0.f, 0.f},
        {static_cast<float>(win.getSize().x), static_cast<float>(win.getSize().y)}));
    win.setView(winView);

    if(context.postfxDisabled){
        sf::Sprite mainS(*context.texture);
        mainS.setScale(sf::Vector2f(scaleX, scaleY));
        mainS.setPosition(sf::Vector2f(ox, oy));
        win.draw(mainS);
        return;
    }

    float caStr = 2.0f + (context.shakeTimer > 0.f ? context.shakeIntensity * 0.4f : 0.f);
    float caDx  = caStr * scaleX;

    sf::Sprite mainS(*context.texture);
    mainS.setScale(sf::Vector2f(scaleX, scaleY));
    mainS.setPosition(sf::Vector2f(ox, oy));
    win.draw(mainS);

    sf::Sprite redS(*context.texture);
    redS.setScale(sf::Vector2f(scaleX, scaleY));
    redS.setPosition(sf::Vector2f(ox - caDx, oy));
    redS.setColor(sf::Color(255, 0, 0, 60));
    win.draw(redS, sf::RenderStates(sf::BlendAdd));

    sf::Sprite blueS(*context.texture);
    blueS.setScale(sf::Vector2f(scaleX, scaleY));
    blueS.setPosition(sf::Vector2f(ox + caDx, oy));
    blueS.setColor(sf::Color(0, 0, 255, 60));
    win.draw(blueS, sf::RenderStates(sf::BlendAdd));

    float WW = static_cast<float>(win.getSize().x);
    float WH = static_cast<float>(win.getSize().y);
    float vdY = WH * 0.26f;
    float vdX = WW * 0.16f;
    sf::Color dark(0, 0, 0, 170), trans(0, 0, 0, 0);

    sf::VertexArray va(sf::PrimitiveType::TriangleStrip, 4);
    va[0].position={0.f, 0.f};    va[0].color=dark;
    va[1].position={WW,  0.f};    va[1].color=dark;
    va[2].position={0.f, vdY};    va[2].color=trans;
    va[3].position={WW,  vdY};    va[3].color=trans;
    win.draw(va);

    va[0].position={0.f, WH};     va[0].color=dark;
    va[1].position={WW,  WH};     va[1].color=dark;
    va[2].position={0.f, WH-vdY}; va[2].color=trans;
    va[3].position={WW,  WH-vdY}; va[3].color=trans;
    win.draw(va);

    va[0].position={0.f,  0.f};   va[0].color=dark;
    va[1].position={0.f,  WH};    va[1].color=dark;
    va[2].position={vdX,  0.f};   va[2].color=trans;
    va[3].position={vdX,  WH};    va[3].color=trans;
    win.draw(va);

    va[0].position={WW,      0.f}; va[0].color=dark;
    va[1].position={WW,      WH};  va[1].color=dark;
    va[2].position={WW-vdX,  0.f}; va[2].color=trans;
    va[3].position={WW-vdX,  WH};  va[3].color=trans;
    win.draw(va);
}

} // namespace render_runtime
