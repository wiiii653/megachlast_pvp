#include "GameUpdateRuntime.h"

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
    game_update_runtime::InputState input{};
    input.kA = true;
    input.kLCtrl = true;
    input.kRight = true;
    input.kRShift = true;

    game_update_runtime::setFocus(input, false);
    check(!input.focused, "focus loss marks input as unfocused");
    check(!input.kA && !input.kLCtrl && !input.kRight && !input.kRShift,
          "focus loss clears movement and fire keys");

    game_update_runtime::setFocus(input, true);
    check(input.focused, "focus gain re-enables keyboard synchronization");
    check(!input.kA && !input.kLCtrl && !input.kRight && !input.kRShift,
          "focus gain does not restore stale keys");

    return failures == 0 ? 0 : 1;
}
