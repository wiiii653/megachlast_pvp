#pragma once

#include "GameTypes.h"

#include <SFML/Graphics.hpp>

#include <array>
#include <functional>

namespace hud_runtime {

struct TextOverlayContext {
    const char* appVersion = "";
    GameState state = GameState::MENU;
    float dt = 0.f;
    float menuAnim = 0.f;
    int menuSel = 0;
    bool showBlink = false;
    float* donateMsgTimer = nullptr;
    PerfLevel perfLevel = PerfLevel::MEDIUM;
    int fxLevel = 0;
    float fpsDisplay = 0.f;
    bool botEnabled = false;
    BotDifficulty botDifficulty = BotDifficulty::MEDIUM;
    int winner = 0;
    int p1Score = 0;
    int p2Score = 0;
    int p1RoundWins = 0;
    int p2RoundWins = 0;
    float knockoutTimer = 0.f;
    int knockoutScorer = 0;
    bool knockoutEndsMatch = false;
    float countdownTimer = 0.f;
    sf::Color layoutColor = sf::Color::White;
    const char* layoutName = "";
    uint32_t boardSeed = 0;
    float fightFlashTimer = 0.f;
    int settingsSel = 0;
    const Config* cfg = nullptr;
    float musicVolume = 0.f;
    float sfxVolume = 0.f;
    const GraphicsSettings* graphicsSettings = nullptr;
    const ControllerSettings* controllers = nullptr;
    const MatchSetup* matchSetup = nullptr;
    std::function<void()> drawMenuTitle;
};

void drawPlayerRows(sf::RenderTarget& rt, sf::Font& font, const Player& p1, const Player& p2);
void drawPersistentInfo(sf::RenderTarget& rt,
                        sf::Font& font,
                        const char* appVersion,
                        PerfLevel perfLevel,
                        int fxLevel,
                        float fpsDisplay,
                        bool botEnabled,
                        BotDifficulty botDifficulty);
void drawMenuInstructions(sf::RenderTarget& rt, sf::Font& font, float menuAnim, bool showBlink, int menuSel);
void drawDonateOverlay(sf::RenderTarget& rt, sf::Font& font, float& donateMsgTimer, float dt);
void drawPausedOverlay(sf::RenderTarget& rt, sf::Font& font);
void drawGameOverOverlay(sf::RenderTarget& rt,
                         sf::Font& font,
                         float menuAnim,
                         int winner,
                         int p1Score,
                         int p2Score,
                         int p1RoundWins,
                         int p2RoundWins,
                         bool showBlink);
void drawMatchSetupOverlay(sf::RenderTarget& rt, sf::Font& font, const MatchSetup& setup);
void drawKnockoutOverlay(sf::RenderTarget& rt,
                         sf::Font& font,
                         float timer,
                         int scorer,
                         bool endsMatch);
void drawCountdownOverlay(sf::RenderTarget& rt,
                          sf::Font& font,
                          float countdownTimer,
                          sf::Color layoutColor,
                          const char* layoutName,
                          uint32_t boardSeed);
void drawFightFlashOverlay(sf::RenderTarget& rt, sf::Font& font, float fightFlashTimer);
void drawSettingsPanel(sf::RenderTarget& rt,
                       sf::Font& font,
                       float menuAnim,
                       int settingsSel,
                       const Config& cfg,
                       float musicVolume,
                       float sfxVolume,
                       bool botEnabled,
                       BotDifficulty botDifficulty,
                       const GraphicsSettings& graphicsSettings,
                       const ControllerSettings& controllers);
void drawFragFloats(sf::RenderTarget& rt,
                    sf::Font& font,
                    const std::array<FragFloat, MAX_FRAG_FLOATS>& fragFloats);
void drawTextOverlays(sf::RenderTarget& rt, sf::Font& font, const TextOverlayContext& context);

} // namespace hud_runtime
