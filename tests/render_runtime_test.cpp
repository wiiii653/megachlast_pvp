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

    return failures == 0 ? 0 : 1;
}
