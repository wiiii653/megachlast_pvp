#include "RenderRuntime.h"

#include <iostream>

namespace {

int failures = 0;

void check(bool ok, const char* msg)
{
    if(!ok){
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    }
}

} // namespace

int main()
{
    GraphicsSettings graphics{};

    check(!render_runtime::isPostfxDisabled(false, graphics, GameState::MENU, 0),
          "default settings keep menu postfx enabled");
    check(!render_runtime::isPostfxDisabled(false, graphics, GameState::PLAYING, 0),
          "default settings keep playing postfx enabled under low load");
    check(render_runtime::isPostfxDisabled(true, graphics, GameState::MENU, 0),
          "--no-postfx disables postfx");

    graphics.postfx_enabled = false;
    check(render_runtime::isPostfxDisabled(false, graphics, GameState::MENU, 0),
          "settings postfx toggle disables postfx");

    graphics.postfx_enabled = true;
    check(!render_runtime::isPostfxDisabled(false, graphics, GameState::MENU, 3),
          "fx governor does not disable menu postfx");
    check(render_runtime::isPostfxDisabled(false, graphics, GameState::PLAYING, 3),
          "fx governor disables playing postfx at level 3");

    // Plasma background: the in-game field must keep the exact legacy palette
    // (regression guard) and the title menu must darken parts of it with clouds.
    const sf::Color legacyInGame = render_runtime::evalPlasmaBgColor(false, 0.f, 0.f, 0.f);
    check(legacyInGame == sf::Color(20, 13, 42),
          "in-game plasma baseline color preserved at origin");
    const sf::Color legacyInGame2 = render_runtime::evalPlasmaBgColor(false, 640.f, 400.f, 1.5f);
    check(legacyInGame2 == sf::Color(18, 12, 39),
          "in-game plasma baseline color preserved at corner");

    bool cloudFound = false;
    for(int gy = 0; gy <= 400; gy += 20){
        for(int gx = 0; gx <= 640; gx += 20){
            sf::Color nt = render_runtime::evalPlasmaBgColor(false, static_cast<float>(gx), static_cast<float>(gy), 6.f);
            sf::Color tt = render_runtime::evalPlasmaBgColor(true,  static_cast<float>(gx), static_cast<float>(gy), 6.f);
            check(tt.r <= nt.r && tt.g <= nt.g && tt.b <= nt.b,
                  "title clouds never brighten the in-game field");
            if(tt.b < nt.b) cloudFound = true;
        }
    }
    check(cloudFound, "title plasma has visible darker cloud masses");

    return failures == 0 ? 0 : 1;
}
