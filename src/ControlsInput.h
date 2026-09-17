#pragma once

// SFML-aware helpers layered on top of ControlsConfig.h: default bindings that
// reproduce the game's historic hard-coded controls, per-device profile
// selection, live pad polling and human-readable labels.

#include "ControlsConfig.h"
#include "ControllerInput.h"

#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <cmath>
#include <string>

namespace controls {

// ---------------------------------------------------------------------------
// Scancode conversions
// ---------------------------------------------------------------------------

inline int scancodeValue(sf::Keyboard::Scancode sc)
{
    return static_cast<int>(sc);
}

inline sf::Keyboard::Scancode scancodeFromValue(int value)
{
    return static_cast<sf::Keyboard::Scancode>(value);
}

inline Input keyInput(sf::Keyboard::Scancode sc)
{
    Input in;
    in.kind = Input::Key;
    in.value = scancodeValue(sc);
    return in;
}

inline Input buttonInput(unsigned int button)
{
    Input in;
    in.kind = Input::Button;
    in.value = static_cast<int>(button);
    return in;
}

inline Input axisInput(sf::Joystick::Axis axis, bool negative)
{
    Input in;
    in.kind = Input::Axis;
    in.value = static_cast<int>(axis);
    in.negative = negative;
    return in;
}

inline bool actionKeyDown(const Profile& profile, Action a)
{
    const ActionBinding& b = profile.actions[static_cast<std::size_t>(a)];
    for(const auto& s : b.slots){
        if(s.kind != Input::Key || s.value < 0) continue;
        if(sf::Keyboard::isKeyPressed(scancodeFromValue(s.value))) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Default bindings — reproduce the original hard-coded controls exactly.
// ---------------------------------------------------------------------------

namespace detail {

inline void setKeys(ActionBinding& b, sf::Keyboard::Scancode primary,
                    sf::Keyboard::Scancode alt = sf::Keyboard::Scan::Unknown,
                    sf::Keyboard::Scancode alt2 = sf::Keyboard::Scan::Unknown)
{
    b.clear();
    b.slots[0] = keyInput(primary);
    if(alt != sf::Keyboard::Scan::Unknown) b.slots[1] = keyInput(alt);
    if(alt2 != sf::Keyboard::Scan::Unknown) b.slots[2] = keyInput(alt2);
}

inline void setInputs(ActionBinding& b, Input primary, Input alt = {}, Input alt2 = {})
{
    b.clear();
    b.slots[0] = primary;
    if(alt.valid()) b.slots[1] = alt;
    if(alt2.valid()) b.slots[2] = alt2;
}

inline Profile makeKeyboardProfile()
{
    Profile p;
    setKeys(p.actions[static_cast<std::size_t>(Action::KbP1Left)],  sf::Keyboard::Scan::A);
    setKeys(p.actions[static_cast<std::size_t>(Action::KbP1Right)], sf::Keyboard::Scan::D);
    setKeys(p.actions[static_cast<std::size_t>(Action::KbP1Fire)],
            sf::Keyboard::Scan::LControl, sf::Keyboard::Scan::Z, sf::Keyboard::Scan::LShift);
    setKeys(p.actions[static_cast<std::size_t>(Action::KbP2Left)],  sf::Keyboard::Scan::Left);
    setKeys(p.actions[static_cast<std::size_t>(Action::KbP2Right)], sf::Keyboard::Scan::Right);
    setKeys(p.actions[static_cast<std::size_t>(Action::KbP2Fire)],
            sf::Keyboard::Scan::RControl, sf::Keyboard::Scan::Slash, sf::Keyboard::Scan::RShift);

    setKeys(p.actions[static_cast<std::size_t>(Action::UiUp)],     sf::Keyboard::Scan::Up);
    setKeys(p.actions[static_cast<std::size_t>(Action::UiDown)],   sf::Keyboard::Scan::Down);
    setKeys(p.actions[static_cast<std::size_t>(Action::UiLeft)],   sf::Keyboard::Scan::Left);
    setKeys(p.actions[static_cast<std::size_t>(Action::UiRight)],  sf::Keyboard::Scan::Right);
    setKeys(p.actions[static_cast<std::size_t>(Action::UiConfirm)], sf::Keyboard::Scan::Enter);
    setKeys(p.actions[static_cast<std::size_t>(Action::UiBack)],   sf::Keyboard::Scan::Escape);

    setKeys(p.actions[static_cast<std::size_t>(Action::SetupModLeft)],   sf::Keyboard::Scan::A);
    setKeys(p.actions[static_cast<std::size_t>(Action::SetupModRight)],  sf::Keyboard::Scan::D);
    setKeys(p.actions[static_cast<std::size_t>(Action::SetupP2ModLeft)],  sf::Keyboard::Scan::Left);
    setKeys(p.actions[static_cast<std::size_t>(Action::SetupP2ModRight)], sf::Keyboard::Scan::Right);
    setKeys(p.actions[static_cast<std::size_t>(Action::SetupArenaL)],    sf::Keyboard::Scan::Q);
    setKeys(p.actions[static_cast<std::size_t>(Action::SetupArenaR)],    sf::Keyboard::Scan::E);
    setKeys(p.actions[static_cast<std::size_t>(Action::SetupStart)],     sf::Keyboard::Scan::Enter);
    setKeys(p.actions[static_cast<std::size_t>(Action::SetupBack)],     sf::Keyboard::Scan::Escape);

    setKeys(p.actions[static_cast<std::size_t>(Action::GamePause)], sf::Keyboard::Scan::Space);
    setKeys(p.actions[static_cast<std::size_t>(Action::GameReset)], sf::Keyboard::Scan::R);
    setKeys(p.actions[static_cast<std::size_t>(Action::GameMenu)],  sf::Keyboard::Scan::Escape);

    setKeys(p.actions[static_cast<std::size_t>(Action::UtilSettings)],   sf::Keyboard::Scan::O);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilDonate)],    sf::Keyboard::Scan::D);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilFullscreen)], sf::Keyboard::Scan::F11);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilSave)],      sf::Keyboard::Scan::K);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilLoad)],      sf::Keyboard::Scan::L);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilBot)],       sf::Keyboard::Scan::B);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilBotDiff)],   sf::Keyboard::Scan::V);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilBotDebug)],  sf::Keyboard::Scan::G);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilPerf)],      sf::Keyboard::Scan::P);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilMute)],      sf::Keyboard::Scan::M);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilVolDown)],   sf::Keyboard::Scan::Comma);
    setKeys(p.actions[static_cast<std::size_t>(Action::UtilVolUp)],     sf::Keyboard::Scan::Period);
    return p;
}

inline Profile makePadProfile(ProfileId id)
{
    Profile p;
    const bool playstation = (id == ProfileId::Ps4 || id == ProfileId::Ps5);

    // Movement: left stick or D-pad.
    setInputs(p.actions[static_cast<std::size_t>(Action::PadMoveLeft)],
              axisInput(sf::Joystick::Axis::X, true), axisInput(sf::Joystick::Axis::PovX, true));
    setInputs(p.actions[static_cast<std::size_t>(Action::PadMoveRight)],
              axisInput(sf::Joystick::Axis::X, false), axisInput(sf::Joystick::Axis::PovX, false));
    setInputs(p.actions[static_cast<std::size_t>(Action::PadFire)],
              buttonInput(0), buttonInput(4), buttonInput(5)); // south, L1, R1

    // UI navigation.
    setInputs(p.actions[static_cast<std::size_t>(Action::UiUp)],
              axisInput(sf::Joystick::Axis::Y, true), axisInput(sf::Joystick::Axis::PovY, true));
    setInputs(p.actions[static_cast<std::size_t>(Action::UiDown)],
              axisInput(sf::Joystick::Axis::Y, false), axisInput(sf::Joystick::Axis::PovY, false));
    setInputs(p.actions[static_cast<std::size_t>(Action::UiLeft)],
              axisInput(sf::Joystick::Axis::X, true), axisInput(sf::Joystick::Axis::PovX, true));
    setInputs(p.actions[static_cast<std::size_t>(Action::UiRight)],
              axisInput(sf::Joystick::Axis::X, false), axisInput(sf::Joystick::Axis::PovX, false));
    setInputs(p.actions[static_cast<std::size_t>(Action::UiConfirm)], buttonInput(0)); // south
    setInputs(p.actions[static_cast<std::size_t>(Action::UiBack)], buttonInput(1));    // east

    // Match setup.
    setInputs(p.actions[static_cast<std::size_t>(Action::SetupModLeft)],
              axisInput(sf::Joystick::Axis::X, true), axisInput(sf::Joystick::Axis::PovX, true));
    setInputs(p.actions[static_cast<std::size_t>(Action::SetupModRight)],
              axisInput(sf::Joystick::Axis::X, false), axisInput(sf::Joystick::Axis::PovX, false));
    setInputs(p.actions[static_cast<std::size_t>(Action::SetupArenaL)], buttonInput(4)); // L1
    setInputs(p.actions[static_cast<std::size_t>(Action::SetupArenaR)], buttonInput(5)); // R1
    setInputs(p.actions[static_cast<std::size_t>(Action::SetupStart)], buttonInput(0), buttonInput(4), buttonInput(5));
    setInputs(p.actions[static_cast<std::size_t>(Action::SetupBack)], buttonInput(1));    // east

    // In-game.
    setInputs(p.actions[static_cast<std::size_t>(Action::GamePause)],
              buttonInput(playstation ? 9u : 7u)); // Start
    setInputs(p.actions[static_cast<std::size_t>(Action::GameReset)],
              buttonInput(playstation ? 8u : 6u)); // Select
    setInputs(p.actions[static_cast<std::size_t>(Action::GameMenu)], buttonInput(1)); // east
    return p;
}

} // namespace detail

inline Profile defaultProfile(ProfileId id)
{
    if(id == ProfileId::Keyboard) return detail::makeKeyboardProfile();
    return detail::makePadProfile(id);
}

inline Profiles defaultProfiles()
{
    Profiles p;
    for(int i = 0; i < static_cast<int>(ProfileId::Count); ++i)
        p.byId[static_cast<std::size_t>(i)] = defaultProfile(static_cast<ProfileId>(i));
    return p;
}

inline ProfileId profileIdForFamily(controller_input::Family family)
{
    switch(family){
        case controller_input::Family::Xbox: return ProfileId::Xbox;
        case controller_input::Family::Ps4:  return ProfileId::Ps4;
        case controller_input::Family::Ps5:  return ProfileId::Ps5;
        case controller_input::Family::Generic:
        default:                              return ProfileId::Generic;
    }
}

inline const Profile& profileForJoystick(const Profiles& profiles, int joystick)
{
    if(joystick < 0 || joystick >= static_cast<int>(sf::Joystick::Count) ||
       !sf::Joystick::isConnected(static_cast<unsigned int>(joystick)))
        return profiles.profile(ProfileId::Generic);
    return profiles.profile(profileIdForFamily(controller_input::familyForJoystick(
        static_cast<unsigned int>(joystick))));
}

// ---------------------------------------------------------------------------
// Live pad polling
// ---------------------------------------------------------------------------

constexpr float kPadDeadZone = 35.f;

inline bool isConnected(int joystick)
{
    return joystick >= 0 && joystick < static_cast<int>(sf::Joystick::Count) &&
           sf::Joystick::isConnected(static_cast<unsigned int>(joystick));
}

inline bool padInputActive(unsigned int joystick, const Input& in)
{
    switch(in.kind){
        case Input::Button:
            return in.value >= 0 &&
                   sf::Joystick::getButtonCount(joystick) > static_cast<unsigned int>(in.value) &&
                   sf::Joystick::isButtonPressed(joystick, static_cast<unsigned int>(in.value));
        case Input::Axis: {
            const auto axis = static_cast<sf::Joystick::Axis>(in.value);
            if(!sf::Joystick::hasAxis(joystick, axis)) return false;
            const float pos = sf::Joystick::getAxisPosition(joystick, axis);
            return in.negative ? pos < -kPadDeadZone : pos > kPadDeadZone;
        }
        case Input::None:
        case Input::Key:
        default:
            return false;
    }
}

inline bool actionPadActive(unsigned int joystick, const Profile& pad, Action a)
{
    const ActionBinding& b = pad.actions[static_cast<std::size_t>(a)];
    for(const auto& s : b.slots)
        if(s.valid() && padInputActive(joystick, s)) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Human-readable labels
// ---------------------------------------------------------------------------

inline std::string keyLabel(int value)
{
    if(value < 0) return std::string();
    const auto sc = scancodeFromValue(value);
    switch(sc){
        case sf::Keyboard::Scan::Unknown: return "?";
        case sf::Keyboard::Scan::LControl: return "L-Ctrl";
        case sf::Keyboard::Scan::RControl: return "R-Ctrl";
        case sf::Keyboard::Scan::LShift: return "L-Shift";
        case sf::Keyboard::Scan::RShift: return "R-Shift";
        case sf::Keyboard::Scan::Escape: return "Esc";
        case sf::Keyboard::Scan::Enter: return "Enter";
        case sf::Keyboard::Scan::Space: return "Space";
        case sf::Keyboard::Scan::Left: return "Left";
        case sf::Keyboard::Scan::Right: return "Right";
        case sf::Keyboard::Scan::Up: return "Up";
        case sf::Keyboard::Scan::Down: return "Down";
        case sf::Keyboard::Scan::Comma: return ",";
        case sf::Keyboard::Scan::Period: return ".";
        case sf::Keyboard::Scan::Slash: return "/";
        default: break;
    }
    return sf::Keyboard::getDescription(sc).toAnsiString();
}

inline std::string axisLabel(int value, bool negative)
{
    const auto axis = static_cast<sf::Joystick::Axis>(value);
    const char* name = "?";
    switch(axis){
        case sf::Joystick::Axis::X: name = "Left X"; break;
        case sf::Joystick::Axis::Y: name = "Left Y"; break;
        case sf::Joystick::Axis::Z: name = "Right X"; break;
        case sf::Joystick::Axis::R: name = "Right Y"; break;
        case sf::Joystick::Axis::U: name = "Z"; break;
        case sf::Joystick::Axis::V: name = "RZ"; break;
        case sf::Joystick::Axis::PovX: name = "D-pad X"; break;
        case sf::Joystick::Axis::PovY: name = "D-pad Y"; break;
    }
    return std::string(name) + (negative ? " -" : " +");
}

inline std::string inputLabel(const Input& in)
{
    switch(in.kind){
        case Input::Key: return keyLabel(in.value);
        case Input::Button: return "Btn " + std::to_string(in.value);
        case Input::Axis: return axisLabel(in.value, in.negative);
        case Input::None:
        default: return "-";
    }
}

inline std::string bindingLabel(const ActionBinding& b)
{
    std::string out;
    for(const auto& s : b.slots){
        if(!s.valid()) continue;
        if(!out.empty()) out += " / ";
        out += inputLabel(s);
    }
    if(out.empty()) out = "UNBOUND";
    return out;
}

} // namespace controls
