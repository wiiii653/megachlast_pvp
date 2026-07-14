#include "HudRuntime.h"

#include "GameConstants.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace hud_runtime {
namespace {

float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

} // namespace

void drawPlayerRows(sf::RenderTarget& rt, sf::Font& font, const Player& p1, const Player& p2)
{
    const float ROW_H  = 11.f;
    const float BAR_W  = 68.f;
    const float BAR_H  = 4.f;
    const float PAD    = 3.f;

    auto drawInlineBar = [&](float bx, float by, float energy, sf::Color col){
        float frac = clampf(energy / 100.f, 0.f, 1.f);
        sf::Color barCol = col;
        if(frac < 0.35f){
            barCol = sf::Color(
                static_cast<uint8_t>(std::min(255.f, 180.f + 75.f * (frac / 0.35f))),
                static_cast<uint8_t>(col.g * frac / 0.35f),
                static_cast<uint8_t>(col.b * frac / 0.35f));
        }
        sf::RectangleShape bg(sf::Vector2f(BAR_W, BAR_H));
        bg.setPosition(sf::Vector2f(bx, by));
        bg.setFillColor(sf::Color(20, 20, 30, 160));
        rt.draw(bg);

        sf::RectangleShape fill(sf::Vector2f(BAR_W * frac, BAR_H));
        fill.setPosition(sf::Vector2f(bx, by));
        fill.setFillColor(barCol);
        rt.draw(fill);

        sf::RectangleShape shine(sf::Vector2f(BAR_W * frac, 1.f));
        shine.setPosition(sf::Vector2f(bx, by));
        shine.setFillColor(sf::Color(255, 255, 255, 50));
        rt.draw(shine);
    };

    auto traitDot = [&](float tx, float ty, float timer, sf::Color col) -> float {
        if(timer <= 0.f) return tx;
        sf::RectangleShape dot(sf::Vector2f(4.f, 4.f));
        dot.setPosition(sf::Vector2f(tx, ty));
        dot.setFillColor(col);
        rt.draw(dot);
        return tx + 5.f;
    };

    {
        char kbuf[16], pbuf[20];
        std::snprintf(kbuf, sizeof(kbuf), "FRAGS:%d", p1.score);
        std::snprintf(pbuf, sizeof(pbuf), " SCORE:%d", p1.points);

        float cx = 4.f;
        float cy = H - ROW_H - 2.f;
        float barY = cy + (ROW_H - BAR_H) * 0.5f;
        float txtY = cy + 1.f;

        float x = cx;
        sf::Text lbl(font, "P1", 8);
        lbl.setFillColor(sf::Color(0, 255, 255, 230));
        lbl.setPosition(sf::Vector2f(x, txtY));
        rt.draw(lbl);
        x += lbl.getLocalBounds().size.x + PAD;

        drawInlineBar(x, barY, p1.energy, sf::Color(0, 255, 255));
        x += BAR_W + PAD;

        sf::Text kt(font, kbuf, 7);
        kt.setFillColor(sf::Color(0, 230, 230, 210));
        kt.setPosition(sf::Vector2f(x, txtY));
        rt.draw(kt);
        x += kt.getLocalBounds().size.x + PAD;

        sf::Text pt(font, pbuf, 7);
        pt.setFillColor(sf::Color(255, 220, 40, 210));
        pt.setPosition(sf::Vector2f(x, txtY));
        rt.draw(pt);
        x += pt.getLocalBounds().size.x + PAD;

        float dotY = cy + (ROW_H - 4.f) * 0.5f;
        x = traitDot(x, dotY, p1.shieldTimer, sf::Color(60, 160, 255, 220));
        x = traitDot(x, dotY, p1.rapidTimer, sf::Color(255, 220, 40, 220));
        x = traitDot(x, dotY, p1.spreadTimer, sf::Color(120, 255, 100, 220));
        traitDot(x, dotY, p1.overdriveTimer, sf::Color(255, 100, 255, 220));
    }

    {
        char kbuf[16], pbuf[20];
        std::snprintf(kbuf, sizeof(kbuf), "FRAGS:%d", p2.score);
        std::snprintf(pbuf, sizeof(pbuf), "SCORE:%d ", p2.points);

        float cy = 2.f;
        float barY = cy + (ROW_H - BAR_H) * 0.5f;
        float txtY = cy + 1.f;

        float rowW = 4.f + 14.f + PAD + BAR_W + PAD;
        sf::Text ktMeasure(font, kbuf, 7);
        rowW += ktMeasure.getLocalBounds().size.x + PAD;
        sf::Text ptMeasure(font, pbuf, 7);
        rowW += ptMeasure.getLocalBounds().size.x + PAD;
        rowW += 4 * 5.f + 4.f;

        float cx = W - rowW - 2.f;

        float x = cx + 4.f;
        float dotY = cy + (ROW_H - 4.f) * 0.5f;
        x = traitDot(x, dotY, p2.shieldTimer, sf::Color(60, 160, 255, 220));
        x = traitDot(x, dotY, p2.rapidTimer, sf::Color(255, 220, 40, 220));
        x = traitDot(x, dotY, p2.spreadTimer, sf::Color(120, 255, 100, 220));
        x = traitDot(x, dotY, p2.overdriveTimer, sf::Color(255, 100, 255, 220));

        sf::Text kt(font, kbuf, 7);
        kt.setFillColor(sf::Color(255, 100, 100, 210));
        kt.setPosition(sf::Vector2f(x, txtY));
        rt.draw(kt);
        x += kt.getLocalBounds().size.x + PAD;

        sf::Text pt(font, pbuf, 7);
        pt.setFillColor(sf::Color(255, 220, 40, 210));
        pt.setPosition(sf::Vector2f(x, txtY));
        rt.draw(pt);
        x += pt.getLocalBounds().size.x + PAD;

        drawInlineBar(x, barY, p2.energy, sf::Color(255, 80, 80));
        x += BAR_W + PAD;

        sf::Text lbl(font, "P2", 8);
        lbl.setFillColor(sf::Color(255, 80, 80, 230));
        lbl.setPosition(sf::Vector2f(x, txtY));
        rt.draw(lbl);
    }
}

void drawPersistentInfo(sf::RenderTarget& rt,
                        sf::Font& font,
                        const char* appVersion,
                        PerfLevel perfLevel,
                        int fxLevel,
                        float fpsDisplay,
                        bool botEnabled,
                        BotDifficulty botDifficulty)
{
    const char* dn[] = {"EASY", "MED", "HARD"};
    const char* pn[] = {"HIGH", "MED", "LOW", "ULTRA"};
    char fb[64];
    std::snprintf(fb, sizeof(fb), "PERF:%s FX:%d FPS:%.0f",
                  pn[static_cast<int>(perfLevel)], fxLevel, fpsDisplay);
    sf::Text pt(font, fb, 7);
    pt.setFillColor(sf::Color(120, 130, 155, 175));
    auto pb = pt.getLocalBounds();

    std::string botStr = std::string("BOT:") + (botEnabled ? "ON" : "OFF");
    if(botEnabled){
        botStr += "[";
        botStr += dn[static_cast<int>(botDifficulty)];
        botStr += "]";
    }
    sf::Text bt(font, botStr, 7);
    bt.setFillColor(botEnabled ? sf::Color(220, 210, 95, 190) : sf::Color(115, 125, 150, 155));
    auto bb = bt.getLocalBounds();

    pt.setPosition(sf::Vector2f(W - pb.size.x - 4.f, H - pb.size.y - 10.f));
    rt.draw(pt);
    bt.setPosition(sf::Vector2f(W - bb.size.x - 4.f, H - pb.size.y - bb.size.y - 12.f));
    rt.draw(bt);

    if(appVersion && appVersion[0] != '\0'){
        sf::Text vt(font, appVersion, 7);
        vt.setFillColor(sf::Color(120, 130, 155, 175));
        vt.setPosition(sf::Vector2f(4.f, 4.f));
        rt.draw(vt);
    }
}

void drawMenuInstructions(sf::RenderTarget& rt, sf::Font& font, float menuAnim, bool showBlink)
{
    struct MenuLine { const char* txt; sf::Color col; };
    const MenuLine lines[] = {
        {"--- PLAYER 1 (bottom) ---", sf::Color(0, 200, 200)},
        {"Move:  A  D (left/right)", sf::Color(150, 235, 235)},
        {"Fire:  Left Ctrl / Z / LShift", sf::Color(150, 235, 235)},
        {"", sf::Color::White},
        {"--- PLAYER 2 (top) ---", sf::Color(255, 100, 100)},
        {"Move:  Left/Right Arrows", sf::Color(255, 160, 160)},
        {"Fire:  Right Ctrl / Slash / RShift", sf::Color(255, 160, 160)},
        {"", sf::Color::White},
        {"Space - Pause      B - Bot", sf::Color(160, 160, 180)},
        {"R     - Reset      V - Diff", sf::Color(160, 160, 180)},
        {"F11   - Fullscreen M - Mute", sf::Color(160, 160, 180)},
        {"O     - Settings  Esc - Quit", sf::Color(160, 160, 180)},
        {"D     - Donate", sf::Color(200, 200, 120)},
    };

    const unsigned INFO_FONT_SIZE = 14u;
    const float INFO_LINE_HEIGHT = static_cast<float>(INFO_FONT_SIZE) + 3.f;
    const float INFO_BLANK_GAP = 8.f;

    float ly = 80.f;
    float totalH = 0.f;
    for(const auto& l : lines){
        if(l.txt[0] == '\0'){
            totalH += INFO_BLANK_GAP;
            continue;
        }
        totalH += INFO_LINE_HEIGHT;
    }
    float availTop = 55.f;
    float availBot = H - 28.f;
    ly = availTop + (availBot - availTop - totalH) * 0.5f - INFO_LINE_HEIGHT * 2.f;

    for(const auto& l : lines){
        if(l.txt[0] == '\0'){
            ly += INFO_BLANK_GAP;
            continue;
        }
        sf::Text shadow(font, l.txt, INFO_FONT_SIZE);
        shadow.setFillColor(sf::Color(0, 0, 0, 140));
        auto sb = shadow.getLocalBounds();
        shadow.setPosition(sf::Vector2f(W / 2.f - sb.size.x / 2.f + 1.f, ly + 1.f));
        rt.draw(shadow);

        sf::Text lt(font, l.txt, INFO_FONT_SIZE);
        lt.setFillColor(l.col);
        auto lb = lt.getLocalBounds();
        lt.setPosition(sf::Vector2f(W / 2.f - lb.size.x / 2.f, ly));
        rt.draw(lt);
        ly += INFO_LINE_HEIGHT;
    }

    if(!showBlink) return;

    float sp = 0.6f + 0.4f * std::sin(menuAnim * 4.f);
    sf::Text start(font, ">> PRESS  ENTER  TO  PLAY <<", 12);
    start.setFillColor(sf::Color(
        static_cast<uint8_t>(200 + 55 * sp),
        static_cast<uint8_t>(200 + 55 * sp),
        static_cast<uint8_t>(100 + 80 * sp)));
    auto sb2 = start.getLocalBounds();
    start.setPosition(sf::Vector2f(W / 2.f - sb2.size.x / 2.f, H - 54.f));
    rt.draw(start);
}

void drawDonateOverlay(sf::RenderTarget& rt, sf::Font& font, float& donateMsgTimer, float dt)
{
    sf::RectangleShape panel(sf::Vector2f((float)W, (float)H));
    panel.setFillColor(sf::Color(0, 0, 0, 210));
    rt.draw(panel);

    sf::Text title(font, "DONATE", 20);
    title.setFillColor(sf::Color(255, 230, 140));
    auto tb = title.getLocalBounds();
    title.setPosition(sf::Vector2f(W / 2.f - tb.size.x / 2.f, 50.f));
    rt.draw(title);

    const std::string donateUrl = "https://buymeacoffee.com/ojnen";
    sf::Text url(font, donateUrl, 12);
    url.setFillColor(sf::Color(200, 200, 255));
    auto ub = url.getLocalBounds();
    url.setPosition(sf::Vector2f(W / 2.f - ub.size.x / 2.f, 140.f));
    rt.draw(url);

    sf::Text hint(font, "Press C to copy link to clipboard. Esc to return.", 9);
    hint.setFillColor(sf::Color(160, 160, 180));
    auto hb = hint.getLocalBounds();
    hint.setPosition(sf::Vector2f(W / 2.f - hb.size.x / 2.f, 170.f));
    rt.draw(hint);

    if(donateMsgTimer > 0.f){
        sf::Text cm(font, "Link copied to clipboard!", 10);
        cm.setFillColor(sf::Color(120, 255, 120));
        auto cb = cm.getLocalBounds();
        cm.setPosition(sf::Vector2f(W / 2.f - cb.size.x / 2.f, 200.f));
        rt.draw(cm);
    }
    if(donateMsgTimer > 0.f) donateMsgTimer = std::max(0.f, donateMsgTimer - dt);
}

void drawPausedOverlay(sf::RenderTarget& rt, sf::Font& font)
{
    sf::RectangleShape dim(sf::Vector2f((float)W, (float)H));
    dim.setFillColor(sf::Color(0, 0, 0, 100));
    rt.draw(dim);

    sf::Text pt(font, "~~ PAUSED ~~", 16);
    pt.setFillColor(sf::Color(200, 200, 255));
    auto pb = pt.getLocalBounds();
    pt.setPosition(sf::Vector2f(W / 2.f - pb.size.x / 2.f, H / 2.f - 10.f));
    rt.draw(pt);

    sf::Text hint(font, "SPACE to resume", 9);
    hint.setFillColor(sf::Color(140, 140, 160));
    auto hb = hint.getLocalBounds();
    hint.setPosition(sf::Vector2f(W / 2.f - hb.size.x / 2.f, H / 2.f + 10.f));
    rt.draw(hint);
}

void drawGameOverOverlay(sf::RenderTarget& rt,
                         sf::Font& font,
                         float menuAnim,
                         int winner,
                         int p1Score,
                         int p2Score,
                         int p1RoundWins,
                         int p2RoundWins,
                         bool showBlink)
{
    sf::RectangleShape dim(sf::Vector2f((float)W, (float)H));
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    rt.draw(dim);

    float wp = 0.7f + 0.3f * std::sin(menuAnim * 3.f);
    std::string winmsg = (winner == 1) ? "PLAYER  1  WINS!" : "PLAYER  2  WINS!";
    sf::Color wc = (winner == 1)
        ? sf::Color(0, static_cast<uint8_t>(200 + 55 * wp), static_cast<uint8_t>(200 + 55 * wp))
        : sf::Color(static_cast<uint8_t>(200 + 55 * wp), 60, 60);
    sf::Text wt(font, winmsg, 16);
    wt.setFillColor(wc);
    auto wb = wt.getLocalBounds();
    wt.setPosition(sf::Vector2f(W / 2.f - wb.size.x / 2.f, H / 2.f - 20.f));
    rt.draw(wt);

    std::string scoreLine = "MATCH " + std::to_string(p1RoundWins) + " : " + std::to_string(p2RoundWins)
                          + "   ROUND " + std::to_string(p1Score) + " : " + std::to_string(p2Score);
    sf::Text st(font, scoreLine, 14);
    st.setFillColor(sf::Color(220, 220, 220));
    auto stb = st.getLocalBounds();
    st.setPosition(sf::Vector2f(W / 2.f - stb.size.x / 2.f, H / 2.f + 2.f));
    rt.draw(st);

    if(!showBlink) return;
    sf::Text rm(font, "ENTER - Rematch", 10);
    rm.setFillColor(sf::Color(200, 200, 160));
    auto rb = rm.getLocalBounds();
    rm.setPosition(sf::Vector2f(W / 2.f - rb.size.x / 2.f, H / 2.f + 20.f));
    rt.draw(rm);
}

void drawMatchSetupOverlay(sf::RenderTarget& rt, sf::Font& font, const MatchSetup& setup)
{
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(W), static_cast<float>(H)));
    dim.setFillColor(sf::Color(0, 0, 0, 170));
    rt.draw(dim);

    sf::Text title(font, "MATCH SETUP", 18);
    title.setFillColor(sf::Color(220, 220, 255));
    auto tb = title.getLocalBounds();
    title.setPosition(sf::Vector2f(W * 0.5f - tb.size.x * 0.5f, H * 0.5f - 62.f));
    rt.draw(title);

    sf::Text p1(font, std::string("P1  < ") + roundModifierName(setup.p1_modifier) + " >", 12);
    p1.setFillColor(sf::Color(80, 240, 255));
    auto p1b = p1.getLocalBounds();
    p1.setPosition(sf::Vector2f(W * 0.5f - p1b.size.x * 0.5f, H * 0.5f - 30.f));
    rt.draw(p1);

    sf::Text p2(font, std::string("P2  < ") + roundModifierName(setup.p2_modifier) + " >", 12);
    p2.setFillColor(sf::Color(255, 110, 100));
    auto p2b = p2.getLocalBounds();
    p2.setPosition(sf::Vector2f(W * 0.5f - p2b.size.x * 0.5f, H * 0.5f - 10.f));
    rt.draw(p2);

    sf::Text arena(font, std::string("ARENA  < ") + arenaPresetName(setup.arena) + " >", 11);
    arena.setFillColor(sf::Color(255, 220, 100));
    auto ab = arena.getLocalBounds();
    arena.setPosition(sf::Vector2f(W * 0.5f - ab.size.x * 0.5f, H * 0.5f + 10.f));
    rt.draw(arena);

    sf::Text hint(font, "Keys: P1 Left/Right, P2 A/D, Arena Q/E   Pad: stick, L1/R1, A", 7);
    hint.setFillColor(sf::Color(180, 180, 205));
    auto hb = hint.getLocalBounds();
    hint.setPosition(sf::Vector2f(W * 0.5f - hb.size.x * 0.5f, H * 0.5f + 32.f));
    rt.draw(hint);
}

void drawKnockoutOverlay(sf::RenderTarget& rt,
                         sf::Font& font,
                         float timer,
                         int scorer,
                         bool endsMatch)
{
    if(timer <= 0.f || scorer == 0) return;
    float pulse = 0.75f + 0.25f * std::sin(timer * 18.f);
    std::string text = endsMatch ? "MATCH POINT!" : "K.O.!";
    sf::Text banner(font, text, endsMatch ? 24 : 30);
    sf::Color color = scorer == 1
        ? sf::Color(70, static_cast<uint8_t>(210 + 45 * pulse), 255)
        : sf::Color(255, static_cast<uint8_t>(100 + 80 * pulse), 80);
    banner.setFillColor(color);
    auto bounds = banner.getLocalBounds();
    banner.setPosition(sf::Vector2f(W * 0.5f - bounds.size.x * 0.5f, H * 0.5f - 48.f));
    rt.draw(banner);

    sf::Text scorerText(font, scorer == 1 ? "PLAYER 1 SCORES" : "PLAYER 2 SCORES", 10);
    scorerText.setFillColor(sf::Color(240, 240, 250, static_cast<uint8_t>(180 + 75 * pulse)));
    auto sb = scorerText.getLocalBounds();
    scorerText.setPosition(sf::Vector2f(W * 0.5f - sb.size.x * 0.5f, H * 0.5f - 22.f));
    rt.draw(scorerText);
}

void drawCountdownOverlay(sf::RenderTarget& rt,
                          sf::Font& font,
                          float countdownTimer,
                          sf::Color layoutColor,
                          const char* layoutName,
                          uint32_t boardSeed)
{
    int num = std::max(1, static_cast<int>(std::ceil(countdownTimer)));
    float alpha = clampf(std::fmod(countdownTimer, 1.f) * 2.f, 0.f, 1.f);
    sf::Text ct(font, std::to_string(num), 28);
    ct.setFillColor(sf::Color(255, 255, 180, static_cast<uint8_t>(180 + 75 * alpha)));
    auto cb = ct.getLocalBounds();
    ct.setPosition(sf::Vector2f(W / 2.f - cb.size.x / 2.f, H / 2.f - 20.f));
    rt.draw(ct);

    sf::Text go(font, "GET READY!", 11);
    go.setFillColor(sf::Color(180, 180, 200, 160));
    auto gb = go.getLocalBounds();
    go.setPosition(sf::Vector2f(W / 2.f - gb.size.x / 2.f, H / 2.f + 12.f));
    rt.draw(go);

    sf::Text lt(font, layoutName, 9);
    lt.setFillColor(sf::Color(layoutColor.r, layoutColor.g, layoutColor.b, 220));
    auto lb = lt.getLocalBounds();
    lt.setPosition(sf::Vector2f(W / 2.f - lb.size.x / 2.f, H / 2.f + 26.f));
    rt.draw(lt);

    char seedBuf[24];
    std::snprintf(seedBuf, sizeof seedBuf, "SEED %08X", boardSeed);
    sf::Text st(font, seedBuf, 7);
    st.setFillColor(sf::Color(120, 120, 140, 130));
    auto sb = st.getLocalBounds();
    st.setPosition(sf::Vector2f(W / 2.f - sb.size.x / 2.f, H / 2.f + 38.f));
    rt.draw(st);
}

void drawFightFlashOverlay(sf::RenderTarget& rt, sf::Font& font, float fightFlashTimer)
{
    float frac = clampf(fightFlashTimer / 0.9f, 0.f, 1.f);
    float sz = 24.f + (1.f - frac) * 16.f;
    uint8_t fa = static_cast<uint8_t>(std::min(255.f, frac * 2.f * 255.f));
    sf::Text ft(font, "FIGHT!", static_cast<unsigned>(sz));
    float fp = 0.5f + 0.5f * std::sin(fightFlashTimer * 20.f);
    ft.setFillColor(sf::Color(255,
        static_cast<uint8_t>(220 + 35 * fp),
        static_cast<uint8_t>(60 + 80 * fp), fa));
    auto fb = ft.getLocalBounds();
    ft.setPosition(sf::Vector2f(W / 2.f - fb.size.x / 2.f, H / 2.f - fb.size.y / 2.f));
    rt.draw(ft);
}

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
                       const ControllerSettings& controllers)
{
    constexpr int OPT_MUSIC = 0;
    constexpr int OPT_SFX = 1;
    constexpr int OPT_BOT_ENABLED = 2;
    constexpr int OPT_BOT_DIFF = 3;
    constexpr int OPT_TARGET_SCORE = 4;
    constexpr int OPT_DAMAGE = 5;
    constexpr int OPT_FIRE_CD_P1 = 6;
    constexpr int OPT_FIRE_CD_P2 = 7;
    constexpr int OPT_P_SPEED = 8;
    constexpr int OPT_WINDOW_SCALE = 9;
    constexpr int OPT_SCREEN_ASPECT = 10;
    constexpr int OPT_POSTFX = 11;
    constexpr int OPT_SCANLINES = 12;
    constexpr int OPT_VIGNETTE = 13;
    constexpr int OPT_CHROMATIC = 14;
    constexpr int OPT_COPPER_BARS = 15;
    constexpr int OPT_P1_CONTROLLER = 16;
    constexpr int OPT_P2_CONTROLLER = 17;
    constexpr int OPT_SAVE = 18;
    constexpr int OPT_LOAD = 19;

    float panelW = 278.f, panelH = 350.f;
    float px = W / 2.f - panelW / 2.f, py = H / 2.f - panelH / 2.f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(sf::Vector2f(px, py));
    panel.setFillColor(sf::Color(6, 6, 18, 230));
    panel.setOutlineThickness(0.f);
    rt.draw(panel);

    float hp = 0.6f + 0.4f * std::sin(menuAnim * 3.f);
    sf::RectangleShape hbar(sf::Vector2f(panelW, 20.f));
    hbar.setPosition(sf::Vector2f(px, py));
    hbar.setFillColor(sf::Color(
        static_cast<uint8_t>(20 + hp * 30),
        static_cast<uint8_t>(20 + hp * 20),
        static_cast<uint8_t>(60 + hp * 60), 200));
    rt.draw(hbar);

    sf::Text titleT(font, "SETTINGS", 13);
    titleT.setFillColor(sf::Color(200, 200, 255));
    auto ttl = titleT.getLocalBounds();
    titleT.setPosition(sf::Vector2f(px + panelW / 2.f - ttl.size.x / 2.f, py + 3.f));
    rt.draw(titleT);

    float ly = py + 28.f;
    char tmp[72];

    auto drawOpt = [&](int idx, const char* label){
        bool sel = (settingsSel == idx);
        if(sel){
            sf::RectangleShape selBg(sf::Vector2f(panelW - 8.f, 12.f));
            selBg.setPosition(sf::Vector2f(px + 4.f, ly - 1.f));
            selBg.setFillColor(sf::Color(60, 60, 100, 120));
            rt.draw(selBg);
        }
        sf::Text t(font, label, 10);
        t.setFillColor(sel ? sf::Color(255, 220, 120) : sf::Color(190, 190, 210));
        t.setPosition(sf::Vector2f(px + 12.f, ly));
        rt.draw(t);
    };

    auto drawVolumeBar = [&](float value, sf::Color fillColor, float rowY){
        constexpr int segs = 10;
        constexpr float segW = 8.f;
        constexpr float segH = 6.f;
        constexpr float segGap = 1.f;
        constexpr float rightInset = 8.f;
        float barTotalW = segs * segW + (segs - 1) * segGap;
        float barX = px + panelW - rightInset - barTotalW;
        int filled = static_cast<int>(std::round((value / 100.f) * segs));
        for(int i = 0; i < segs; ++i){
            sf::RectangleShape cell(sf::Vector2f(segW, segH));
            cell.setPosition(sf::Vector2f(barX + i * (segW + segGap), rowY));
            cell.setFillColor(i < filled ? fillColor : sf::Color(35, 35, 55));
            rt.draw(cell);
        }
    };

    std::snprintf(tmp, sizeof(tmp), "Music Volume: %3d%%", (int)musicVolume);
    drawOpt(OPT_MUSIC, tmp);
    drawVolumeBar(musicVolume, sf::Color(120, 180, 255), ly);
    ly += 18.f;

    std::snprintf(tmp, sizeof(tmp), "SFX Volume:   %3d%%", (int)sfxVolume);
    drawOpt(OPT_SFX, tmp);
    drawVolumeBar(sfxVolume, sf::Color(255, 180, 120), ly);
    ly += 18.f;

    std::snprintf(tmp, sizeof(tmp), "Bot Enabled: %s", botEnabled ? "ON " : "OFF");
    drawOpt(OPT_BOT_ENABLED, tmp);
    ly += 14.f;

    const char* diffNms[] = {"EASY", "MED", "HARD"};
    std::snprintf(tmp, sizeof(tmp), "Bot Difficulty: %s", diffNms[static_cast<int>(botDifficulty)]);
    drawOpt(OPT_BOT_DIFF, tmp);
    ly += 18.f;

    std::snprintf(tmp, sizeof(tmp), "Target Score: %2d", cfg.target_score);
    drawOpt(OPT_TARGET_SCORE, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Damage:       %4.1f", cfg.damage);
    drawOpt(OPT_DAMAGE, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "P1 Fire CD (f): %2d", cfg.fire_cd_p1_frames);
    drawOpt(OPT_FIRE_CD_P1, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "P2 Fire CD (f): %2d", cfg.fire_cd_p2_frames);
    drawOpt(OPT_FIRE_CD_P2, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Player Speed: %3d", (int)std::lround(cfg.p_speed));
    drawOpt(OPT_P_SPEED, tmp);
    ly += 18.f;

    int canvasH = graphicsSettings.screen_aspect == ScreenAspect::Ratio16x9 ? H_16X9 : H_16X10;
    std::snprintf(tmp, sizeof(tmp), "Resolution: %dx%d",
                  CANVAS_W * graphicsSettings.window_scale,
                  canvasH * graphicsSettings.window_scale);
    drawOpt(OPT_WINDOW_SCALE, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Aspect: %s (restart)",
                  graphicsSettings.screen_aspect == ScreenAspect::Ratio16x9 ? "16:9" : "16:10");
    drawOpt(OPT_SCREEN_ASPECT, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Post FX: %s", graphicsSettings.postfx_enabled ? "ON " : "OFF");
    drawOpt(OPT_POSTFX, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Scanlines: %s", graphicsSettings.scanlines_enabled ? "ON " : "OFF");
    drawOpt(OPT_SCANLINES, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Vignette: %s", graphicsSettings.vignette_enabled ? "ON " : "OFF");
    drawOpt(OPT_VIGNETTE, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Chromatic: %s", graphicsSettings.chromatic_enabled ? "ON " : "OFF");
    drawOpt(OPT_CHROMATIC, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "Copper Bars: %s", graphicsSettings.copper_bars_enabled ? "ON " : "OFF");
    drawOpt(OPT_COPPER_BARS, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "P1 Controller: %s", controllers.p1_joystick < 0 ? "OFF" : ("Joy " + std::to_string(controllers.p1_joystick + 1)).c_str());
    drawOpt(OPT_P1_CONTROLLER, tmp);
    ly += 14.f;

    std::snprintf(tmp, sizeof(tmp), "P2 Controller: %s", controllers.p2_joystick < 0 ? "OFF" : ("Joy " + std::to_string(controllers.p2_joystick + 1)).c_str());
    drawOpt(OPT_P2_CONTROLLER, tmp);
    ly += 18.f;

    drawOpt(OPT_SAVE, "[ Save Settings ]");
    ly += 14.f;
    drawOpt(OPT_LOAD, "[ Load Settings ]");
    ly += 20.f;

    sf::Text hint(font,
        "Up/Down: select   Left/Right: adjust   Enter: toggle/save/load   Esc: back", 7);
    hint.setFillColor(sf::Color(100, 100, 120));
    auto hb = hint.getLocalBounds();
    hint.setPosition(sf::Vector2f(px + panelW / 2.f - hb.size.x / 2.f, py + panelH - 14.f));
    rt.draw(hint);
}

void drawFragFloats(sf::RenderTarget& rt,
                    sf::Font& font,
                    const std::array<FragFloat, MAX_FRAG_FLOATS>& fragFloats)
{
    for(const auto& f : fragFloats){
        if(!f.alive) continue;
        float frac = clampf(f.ttl / f.maxttl, 0.f, 1.f);
        float scale = 1.f + (1.f - frac) * 0.55f;
        uint8_t alpha = static_cast<uint8_t>(frac * 255.f);
        sf::Text ft(font, "FRAG!",
                    static_cast<unsigned>(std::max(1.f, 16.f * scale)));
        sf::Color fc = (f.scorer == 1)
            ? sf::Color(0, 255, 255, alpha)
            : sf::Color(255, 80, 80, alpha);
        ft.setFillColor(fc);
        auto fb = ft.getLocalBounds();
        ft.setPosition(sf::Vector2f(f.x - fb.size.x * 0.5f,
                                    f.y - fb.size.y * 0.5f));
        rt.draw(ft);
    }
}

void drawTextOverlays(sf::RenderTarget& rt, sf::Font& font, const TextOverlayContext& context)
{
    if(context.state == GameState::DONATE && context.donateMsgTimer)
        drawDonateOverlay(rt, font, *context.donateMsgTimer, context.dt);

    drawPersistentInfo(rt, font, context.appVersion, context.perfLevel, context.fxLevel, context.fpsDisplay,
                       context.botEnabled, context.botDifficulty);

    if(context.state == GameState::MENU){
        if(context.drawMenuTitle) context.drawMenuTitle();
        drawMenuInstructions(rt, font, context.menuAnim, context.showBlink);
    }

    if(context.state == GameState::MATCH_SETUP && context.matchSetup)
        drawMatchSetupOverlay(rt, font, *context.matchSetup);

    if(context.state == GameState::PAUSED)
        drawPausedOverlay(rt, font);

    if(context.state == GameState::GAME_OVER)
        drawGameOverOverlay(rt, font, context.menuAnim, context.winner,
                            context.p1Score, context.p2Score,
                            context.p1RoundWins, context.p2RoundWins, context.showBlink);

    if(context.state == GameState::COUNTDOWN)
        drawCountdownOverlay(rt, font, context.countdownTimer, context.layoutColor,
                             context.layoutName, context.boardSeed);

    if(context.fightFlashTimer > 0.f && context.state == GameState::PLAYING)
        drawFightFlashOverlay(rt, font, context.fightFlashTimer);

    if(context.state == GameState::SETTINGS && context.cfg && context.graphicsSettings && context.controllers)
        drawSettingsPanel(rt, font, context.menuAnim, context.settingsSel, *context.cfg,
                          context.musicVolume, context.sfxVolume,
                          context.botEnabled, context.botDifficulty,
                          *context.graphicsSettings, *context.controllers);

    drawKnockoutOverlay(rt, font, context.knockoutTimer,
                        context.knockoutScorer, context.knockoutEndsMatch);
}

} // namespace hud_runtime
