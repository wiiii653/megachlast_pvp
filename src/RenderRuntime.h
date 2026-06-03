#pragma once

#include "GameTypes.h"
#include "Random.h"

#include <SFML/Graphics.hpp>

#include <array>
#include <functional>

namespace render_runtime {

struct IngameElementsContext {
    GameState state = GameState::MENU;
    float menuAnim = 0.f;
    int fxLevel = 0;
    const std::array<Mirror, MIRROR_PAIRS*2>* mirrors = nullptr;
    const Player* p1 = nullptr;
    const Player* p2 = nullptr;
    const std::array<Bullet, MAX_BULLETS>* bullets = nullptr;
    const sf::Texture* tex1 = nullptr;
    const sf::Texture* tex2 = nullptr;
    std::function<void()> drawPlasmaDivider;
    std::function<void()> drawPowerups;
    std::function<void()> drawBombs;
    std::function<void()> drawSpecialStars;
    std::function<void()> drawBarriers;
    std::function<void()> drawParticles;
    std::function<void()> drawFragFloats;
    std::function<void()> drawPlayerRows;
};

struct SceneOverlayContext {
    GameState state = GameState::MENU;
    bool postfxDisabled = false;
    bool vignetteEnabled = true;
    std::function<void()> drawMenuSpectStars;
};

struct PostfxContext {
    bool postfxDisabled = false;
    PerfLevel perfLevel = PerfLevel::MEDIUM;
    GameState state = GameState::MENU;
    int fxLevel = 0;
    float menuAnim = 0.f;
    bool scanlinesEnabled = true;
    bool copperBarsEnabled = true;
    std::function<void()> drawScanlines;
};

struct CompositeContext {
    const sf::Texture* texture = nullptr;
    bool postfxDisabled = false;
    bool vignetteEnabled = true;
    bool chromaticEnabled = true;
    float shakeTimer = 0.f;
    float shakeDuration = 0.f;
    float shakeIntensity = 0.f;
    std::function<float(float, float)> randomRange;
};

void drawMirror(sf::RenderTarget& rt, const Mirror& m, float t, int fxLevel);
void drawShip(sf::RenderTarget& rt, const Player& p, int id, float t,
              const sf::Texture* tex1, const sf::Texture* tex2, int fxLevel);
void drawBullets(sf::RenderTarget& rt, const std::array<Bullet, MAX_BULLETS>& bullets);
void drawParticles(sf::RenderTarget& rt, const std::array<Particle, MAX_PARTICLES>& particles);
void initStars(std::array<Star, NUM_STARS>& stars, RNG& rng);
void updateStars(std::array<Star, NUM_STARS>& stars, float dt, RNG& rng);
void drawStars(sf::RenderTarget& rt,
               const std::array<Star, NUM_STARS>& stars,
               float globalBright = 1.f);
void initSpectStars(std::array<SpectStar, NUM_SPECT>& stars);
void updateSpectStars(std::array<SpectStar, NUM_SPECT>& stars, float dt, RNG& rng, int state);
void drawSpectStars(sf::RenderTarget& rt, const std::array<SpectStar, NUM_SPECT>& stars);
void drawPlasmaDivider(sf::RenderTarget& rt, float t);
void drawScanlines(sf::RenderTarget& rt);
void drawSpecialStars(sf::RenderTarget& rt,
                      const std::array<SpecialStar, MAX_SPECIAL_STARS>& stars,
                      float t);
void drawPowerUp(sf::RenderTarget& rt, const PowerUp& powerUp, float t);
void drawBomb(sf::RenderTarget& rt, const Bomb& bomb, float t, int fxLevel);
void drawAllBarriers(sf::RenderTarget& rt,
                     const std::array<BarrierBrick, BARRIER_BRICKS * 2>& barriers,
                     float t);
void drawMenuTitle(sf::RenderTarget& rt, sf::Font& font, float t, int fxLevel);
void drawPlasmaBg(sf::RenderTarget& rt, float t);
void drawCopperBars(sf::RenderTarget& rt, float t);
void drawIngameElements(sf::RenderTarget& rt, const IngameElementsContext& context);
bool isPostfxDisabled(bool noPostfx, const GraphicsSettings& graphicsSettings, GameState state, int fxLevel);
void drawPostfxOverlays(sf::RenderTarget& rt, const PostfxContext& context);
void drawSceneOverlays(sf::RenderTarget& rt, const SceneOverlayContext& context);
void compositeToWindow(sf::RenderWindow& win, const CompositeContext& context);

} // namespace render_runtime
