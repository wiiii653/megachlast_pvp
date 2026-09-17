#include "ControlsConfig.h"

#include <iostream>
#include <string>

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
    using namespace controls;

    // ── Serialization round-trip ─────────────────────────────────────────────
    {
        Profile p;
        ActionBinding b;
        b.slots[0] = { Input::Key, 4, false };     // scancode A
        b.slots[1] = { Input::Key, 7, false };     // scancode D (alt slot)
        p.actions[static_cast<std::size_t>(Action::KbP1Left)] = b;

        ActionBinding fire;
        fire.slots[0] = { Input::Key, 38, false }; // LControl
        p.actions[static_cast<std::size_t>(Action::KbP1Fire)] = fire;

        ActionBinding move;
        move.slots[0] = { Input::Axis, 0, true };  // X axis negative
        move.slots[1] = { Input::Axis, 6, true };  // PovX axis negative
        p.actions[static_cast<std::size_t>(Action::PadMoveLeft)] = move;

        ActionBinding back;
        back.slots[0] = { Input::Button, 1, false };
        p.actions[static_cast<std::size_t>(Action::UiBack)] = back;

        const std::string encoded = encodeProfile(p, true); // keyboard profile
        check(!encoded.empty(), "keyboard profile encodes to a non-empty string");

        Profile decoded;
        check(decodeProfile(encoded, decoded, true), "keyboard profile decodes");
        check(decoded.actions[static_cast<std::size_t>(Action::KbP1Left)].containsKey(4),
              "P1 left primary scancode round-trips");
        check(decoded.actions[static_cast<std::size_t>(Action::KbP1Left)].containsKey(7),
              "P1 left alternate scancode round-trips");
        check(decoded.actions[static_cast<std::size_t>(Action::KbP1Fire)].containsKey(38),
              "P1 fire scancode round-trips");
        check(!decoded.actions[static_cast<std::size_t>(Action::PadMoveLeft)].contains(
                  Input{ Input::Axis, 0, true }),
              "pad-only action is not stored in a keyboard profile");
        check(decoded.actions[static_cast<std::size_t>(Action::UiBack)].contains(
                  Input{ Input::Button, 1, false }),
              "shared back action round-trips through the keyboard profile");
    }

    {
        // Pad profile: pad actions round-trip; keyboard-only actions ignored.
        Profile p;
        ActionBinding move;
        move.slots[0] = { Input::Axis, 0, false }; // X axis positive
        p.actions[static_cast<std::size_t>(Action::PadMoveRight)] = move;
        ActionBinding util;
        util.slots[0] = { Input::Key, 5, false };
        p.actions[static_cast<std::size_t>(Action::UtilMute)] = util;

        const std::string encoded = encodeProfile(p, false);
        Profile decoded;
        check(decodeProfile(encoded, decoded, false), "pad profile decodes");
        check(decoded.actions[static_cast<std::size_t>(Action::PadMoveRight)].contains(
                  Input{ Input::Axis, 0, false }),
              "pad move right round-trips");
        check(!decoded.actions[static_cast<std::size_t>(Action::UtilMute)].containsKey(5),
              "keyboard-only action is not stored in a pad profile");
    }

    // ── Binding capture and conflict handling ────────────────────────────────
    {
        Profile p;
        const Input keyA{ Input::Key, 4, false };
        bindPrimary(p, Action::KbP1Left, keyA);
        check(p.actions[static_cast<std::size_t>(Action::KbP1Left)].contains(keyA),
              "bindPrimary sets the primary binding");

        // Rebinding the same key elsewhere clears the previous action.
        bindPrimary(p, Action::UiLeft, keyA);
        check(p.actions[static_cast<std::size_t>(Action::UiLeft)].contains(keyA),
              "conflicting bind lands on the new action");
        check(!p.actions[static_cast<std::size_t>(Action::KbP1Left)].contains(keyA),
              "conflicting bind is removed from the previous action");

        // Rebinding an action clears its legacy alternate slots.
        ActionBinding fire;
        fire.slots[0] = { Input::Key, 38, false };
        fire.slots[1] = { Input::Key, 44, false }; // Z
        p.actions[static_cast<std::size_t>(Action::KbP1Fire)] = fire;
        bindPrimary(p, Action::KbP1Fire, Input{ Input::Key, 25, false }); // K
        check(p.actions[static_cast<std::size_t>(Action::KbP1Fire)].containsKey(25),
              "new fire binding applied");
        check(!p.actions[static_cast<std::size_t>(Action::KbP1Fire)].containsKey(38) &&
              !p.actions[static_cast<std::size_t>(Action::KbP1Fire)].containsKey(44),
              "legacy alternates cleared after rebind");
    }

    // ── Tag and row helpers ────────────────────────────────────────────────
    check(actionFromTag(actionTag(Action::GamePause)) == Action::GamePause,
          "action tags round-trip");
    check(actionFromTag("NOPE") == Action::None, "unknown tag resolves to None");
    check(isKeyboardAction(Action::UtilPerf) && !isKeyboardAction(Action::PadFire),
          "keyboard action set is exclusive of pad gameplay");
    check(isPadAction(Action::GameMenu) && !isPadAction(Action::KbP1Fire) &&
          !isPadAction(Action::UtilSave),
          "pad action set is exclusive of keyboard gameplay and utilities");

    check(controlsRowIsProfile(0), "row 0 is the profile selector");
    check(controlsRowIsRestore(static_cast<int>(profileActionCount(ProfileId::Keyboard)) + 1,
                               ProfileId::Keyboard),
          "restore row sits after the action list");
    check(controlsRowIsBack(static_cast<int>(controlsRowCount(ProfileId::Keyboard)) - 1,
                            ProfileId::Keyboard),
          "back row is the last row");
    check(controlsRowAction(1, ProfileId::Keyboard) == Action::KbP1Left,
          "row 1 is the first keyboard action");
    check(controlsRowAction(0, ProfileId::Keyboard) == Action::None,
          "profile row has no action");

    return failures == 0 ? 0 : 1;
}
