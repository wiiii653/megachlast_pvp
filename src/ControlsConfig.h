#pragma once

// Core data model for configurable controls. This header is intentionally
// SFML-free so it can be included by serialization code and tests that do not
// link SFML. Physical inputs are represented as plain integers:
//   - Key:    raw sf::Keyboard::Scancode index
//   - Button: raw sf::Joystick button index
//   - Axis:   raw sf::Joystick::Axis index plus a direction flag
// SFML-aware helpers (defaults, polling, labels) live in ControlsInput.h.

#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace controls {

enum class Action : uint8_t {
    None = 0,
    // Keyboard gameplay (player-specific physical keys)
    KbP1Left,
    KbP1Right,
    KbP1Fire,
    KbP2Left,
    KbP2Right,
    KbP2Fire,
    // Pad gameplay (player-agnostic capabilities, applied per assigned player)
    PadMoveLeft,
    PadMoveRight,
    PadFire,
    // Shared UI navigation
    UiUp,
    UiDown,
    UiLeft,
    UiRight,
    UiConfirm,
    UiBack,
    // Match setup
    SetupModLeft,
    SetupModRight,
    SetupP2ModLeft,
    SetupP2ModRight,
    SetupArenaL,
    SetupArenaR,
    SetupStart,
    SetupBack,
    // In-game
    GamePause,
    GameReset,
    GameMenu,
    // Utility shortcuts
    UtilSettings,
    UtilDonate,
    UtilFullscreen,
    UtilSave,
    UtilLoad,
    UtilBot,
    UtilBotDiff,
    UtilBotDebug,
    UtilPerf,
    UtilMute,
    UtilVolDown,
    UtilVolUp,
    Count,
};

constexpr std::size_t kActionCount = static_cast<std::size_t>(Action::Count);

struct Input {
    enum Kind : uint8_t { None = 0, Key = 1, Button = 2, Axis = 3 };
    Kind kind = None;
    int value = 0;         // scancode / button / axis index
    bool negative = false; // axis direction (negative side when true)

    bool valid() const { return kind != None; }
    bool matchesKey(int sc) const { return kind == Key && sc >= 0 && value == sc; }
    bool operator==(const Input& other) const
    {
        return kind == other.kind && value == other.value && negative == other.negative;
    }
    bool operator!=(const Input& other) const { return !(*this == other); }
};

// An action may carry up to three physical inputs. The first is the primary
// binding shown and edited in the Controls menu; the others preserve the
// legacy multi-key/multi-button defaults and are cleared when the primary is
// rebound so a user-chosen binding becomes exclusive.
struct ActionBinding {
    Input slots[3]{};

    bool contains(const Input& in) const
    {
        for(const auto& s : slots)
            if(s == in && s.valid()) return true;
        return false;
    }
    bool containsKey(int sc) const
    {
        for(const auto& s : slots)
            if(s.matchesKey(sc)) return true;
        return false;
    }
    const Input& primary() const { return slots[0]; }
    void setPrimary(const Input& in)
    {
        slots[0] = in;
        slots[1] = {};
        slots[2] = {};
    }
    void clear()
    {
        slots[0] = {};
        slots[1] = {};
        slots[2] = {};
    }
};

using ActionTable = std::array<ActionBinding, kActionCount>;

enum class ProfileId : uint8_t { Keyboard = 0, Xbox = 1, Ps4 = 2, Ps5 = 3, Generic = 4, Count };
constexpr std::size_t kProfileCount = static_cast<std::size_t>(ProfileId::Count);

struct Profile {
    ActionTable actions;
};

struct Profiles {
    std::array<Profile, kProfileCount> byId;
    Profile& profile(ProfileId id) { return byId[static_cast<std::size_t>(id)]; }
    const Profile& profile(ProfileId id) const { return byId[static_cast<std::size_t>(id)]; }
    const Profile& keyboard() const { return profile(ProfileId::Keyboard); }
    Profile& keyboard() { return profile(ProfileId::Keyboard); }
};

inline const char* profileName(ProfileId id)
{
    switch(id){
        case ProfileId::Keyboard: return "Keyboard";
        case ProfileId::Xbox: return "Xbox";
        case ProfileId::Ps4: return "PlayStation 4";
        case ProfileId::Ps5: return "PlayStation 5";
        case ProfileId::Generic: return "Generic Pad";
        case ProfileId::Count: break;
    }
    return "?";
}

inline const char* actionName(Action a)
{
    switch(a){
        case Action::KbP1Left:     return "P1 Move Left";
        case Action::KbP1Right:    return "P1 Move Right";
        case Action::KbP1Fire:     return "P1 Fire";
        case Action::KbP2Left:     return "P2 Move Left";
        case Action::KbP2Right:    return "P2 Move Right";
        case Action::KbP2Fire:     return "P2 Fire";
        case Action::PadMoveLeft:  return "Move Left";
        case Action::PadMoveRight: return "Move Right";
        case Action::PadFire:      return "Fire";
        case Action::UiUp:         return "Navigate Up";
        case Action::UiDown:       return "Navigate Down";
        case Action::UiLeft:       return "Navigate Left";
        case Action::UiRight:      return "Navigate Right";
        case Action::UiConfirm:    return "Confirm";
        case Action::UiBack:       return "Back";
        case Action::SetupModLeft:  return "Modifier Left";
        case Action::SetupModRight: return "Modifier Right";
        case Action::SetupP2ModLeft:  return "P2 Modifier Left";
        case Action::SetupP2ModRight: return "P2 Modifier Right";
        case Action::SetupArenaL:   return "Arena Left";
        case Action::SetupArenaR:   return "Arena Right";
        case Action::SetupStart:    return "Start Match";
        case Action::SetupBack:     return "Setup Back";
        case Action::GamePause:     return "Pause / Resume";
        case Action::GameReset:     return "Reset Round";
        case Action::GameMenu:      return "Open Menu";
        case Action::UtilSettings:  return "Open Settings";
        case Action::UtilDonate:    return "Open Donate";
        case Action::UtilFullscreen:return "Toggle Fullscreen";
        case Action::UtilSave:      return "Save Settings";
        case Action::UtilLoad:      return "Load Settings";
        case Action::UtilBot:       return "Toggle Bot";
        case Action::UtilBotDiff:   return "Bot Difficulty";
        case Action::UtilBotDebug:  return "Bot Debug";
        case Action::UtilPerf:      return "Perf Profile";
        case Action::UtilMute:      return "Mute Audio";
        case Action::UtilVolDown:   return "Volume Down";
        case Action::UtilVolUp:     return "Volume Up";
        case Action::None:          return "(none)";
        case Action::Count:         break;
    }
    return "?";
}

inline const char* actionTag(Action a)
{
    switch(a){
        case Action::KbP1Left:     return "P1L";
        case Action::KbP1Right:    return "P1R";
        case Action::KbP1Fire:     return "P1F";
        case Action::KbP2Left:     return "P2L";
        case Action::KbP2Right:    return "P2R";
        case Action::KbP2Fire:     return "P2F";
        case Action::PadMoveLeft:  return "MVL";
        case Action::PadMoveRight: return "MVR";
        case Action::PadFire:      return "FIR";
        case Action::UiUp:         return "UP";
        case Action::UiDown:       return "DOWN";
        case Action::UiLeft:       return "LEFT";
        case Action::UiRight:      return "RIGHT";
        case Action::UiConfirm:    return "CONF";
        case Action::UiBack:       return "BACK";
        case Action::SetupModLeft:   return "ML";
        case Action::SetupModRight:  return "MR";
        case Action::SetupP2ModLeft: return "P2ML";
        case Action::SetupP2ModRight:return "P2MR";
        case Action::SetupArenaL:    return "AL";
        case Action::SetupArenaR:    return "AR";
        case Action::SetupStart:     return "START";
        case Action::SetupBack:      return "SBACK";
        case Action::GamePause:      return "PAUSE";
        case Action::GameReset:      return "RESET";
        case Action::GameMenu:       return "MENU";
        case Action::UtilSettings:   return "SETT";
        case Action::UtilDonate:     return "DONATE";
        case Action::UtilFullscreen: return "FS";
        case Action::UtilSave:       return "SAVE";
        case Action::UtilLoad:       return "LOAD";
        case Action::UtilBot:        return "BOT";
        case Action::UtilBotDiff:    return "BDIFF";
        case Action::UtilBotDebug:   return "BDBG";
        case Action::UtilPerf:       return "PERF";
        case Action::UtilMute:       return "MUTE";
        case Action::UtilVolDown:    return "VD";
        case Action::UtilVolUp:      return "VU";
        case Action::None:           return "NONE";
        case Action::Count:          break;
    }
    return "?";
}

inline bool isKeyboardAction(Action a)
{
    switch(a){
        case Action::PadMoveLeft:
        case Action::PadMoveRight:
        case Action::PadFire:
            return false;
        default:
            return true;
    }
}

inline bool isPadAction(Action a)
{
    switch(a){
        case Action::KbP1Left:
        case Action::KbP1Right:
        case Action::KbP1Fire:
        case Action::KbP2Left:
        case Action::KbP2Right:
        case Action::KbP2Fire:
        case Action::UtilSettings:
        case Action::UtilDonate:
        case Action::UtilFullscreen:
        case Action::UtilSave:
        case Action::UtilLoad:
        case Action::UtilBot:
        case Action::UtilBotDiff:
        case Action::UtilBotDebug:
        case Action::UtilPerf:
        case Action::UtilMute:
        case Action::UtilVolDown:
        case Action::UtilVolUp:
            return false;
        default:
            return true;
    }
}

inline Action actionFromTag(const std::string& tag)
{
    for(int i = static_cast<int>(Action::None) + 1; i < static_cast<int>(Action::Count); ++i){
        Action a = static_cast<Action>(i);
        if(actionTag(a) == tag) return a;
    }
    return Action::None;
}

// Ordered action lists shown in the Controls menu for each profile kind.
inline const std::array<Action, 35>& keyboardActionList()
{
    static const std::array<Action, 35> list = {
        Action::KbP1Left,  Action::KbP1Right, Action::KbP1Fire,
        Action::KbP2Left,  Action::KbP2Right, Action::KbP2Fire,
        Action::UiUp,      Action::UiDown,    Action::UiLeft,
        Action::UiRight,   Action::UiConfirm, Action::UiBack,
        Action::SetupModLeft,  Action::SetupModRight,
        Action::SetupP2ModLeft, Action::SetupP2ModRight,
        Action::SetupArenaL,    Action::SetupArenaR,
        Action::SetupStart,     Action::SetupBack,
        Action::GamePause,  Action::GameReset, Action::GameMenu,
        Action::UtilSettings,   Action::UtilDonate,
        Action::UtilFullscreen, Action::UtilSave,   Action::UtilLoad,
        Action::UtilBot,        Action::UtilBotDiff, Action::UtilBotDebug,
        Action::UtilPerf,       Action::UtilMute,
        Action::UtilVolDown,    Action::UtilVolUp,
    };
    return list;
}

inline const std::array<Action, 18>& padActionList()
{
    static const std::array<Action, 18> list = {
        Action::PadMoveLeft, Action::PadMoveRight, Action::PadFire,
        Action::UiUp,   Action::UiDown,   Action::UiLeft,
        Action::UiRight, Action::UiConfirm, Action::UiBack,
        Action::SetupModLeft,  Action::SetupModRight,
        Action::SetupArenaL,   Action::SetupArenaR,
        Action::SetupStart,    Action::SetupBack,
        Action::GamePause, Action::GameReset, Action::GameMenu,
    };
    return list;
}

inline std::size_t padActionListSize()
{
    return padActionList().size();
}

// Controls submenu layout: row 0 is the profile selector, rows 1..N are the
// profile's actions, then "Restore Defaults" and "Back".
inline std::size_t profileActionCount(ProfileId id)
{
    return id == ProfileId::Keyboard ? keyboardActionList().size() : padActionListSize();
}

inline std::size_t controlsRowCount(ProfileId id)
{
    return 1 + profileActionCount(id) + 2;
}

inline bool controlsRowIsProfile(int row)
{
    return row == 0;
}

inline bool controlsRowIsRestore(int row, ProfileId id)
{
    return row == static_cast<int>(profileActionCount(id)) + 1;
}

inline bool controlsRowIsBack(int row, ProfileId id)
{
    return row == static_cast<int>(profileActionCount(id)) + 2;
}

inline Action controlsRowAction(int row, ProfileId id)
{
    if(row <= 0) return Action::None;
    const std::size_t idx = static_cast<std::size_t>(row - 1);
    if(id == ProfileId::Keyboard){
        if(idx < keyboardActionList().size()) return keyboardActionList()[idx];
        return Action::None;
    }
    if(idx < padActionListSize()) return padActionList()[idx];
    return Action::None;
}

// ---------------------------------------------------------------------------
// Serialization helpers (SFML-free). A profile is encoded as
//   TAG:in|in|in;TAG:in|in|in;...
// where each input is one of:
//   NONE       — unbound
//   <int>      — keyboard scancode index (raw sf::Keyboard::Scancode)
//   B<int>     — joystick button index
//   AX<int>+/- — joystick axis index with direction
// ---------------------------------------------------------------------------

inline std::string encodeInput(const Input& in)
{
    switch(in.kind){
        case Input::Key:    return std::to_string(in.value);
        case Input::Button: return "B" + std::to_string(in.value);
        case Input::Axis:   return "AX" + std::to_string(in.value) + (in.negative ? "-" : "+");
        case Input::None:
        default:            return "NONE";
    }
}

inline std::string encodeActionBinding(const ActionBinding& b)
{
    std::string out;
    for(int i = 0; i < 3; ++i){
        if(i) out += "|";
        out += encodeInput(b.slots[i]);
    }
    return out;
}

inline bool parseInput(const std::string& tokenIn, Input& out)
{
    std::string s = tokenIn;
    // local trim
    auto isSpace = [](unsigned char c){ return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    std::size_t first = 0;
    while(first < s.size() && isSpace(static_cast<unsigned char>(s[first]))) ++first;
    std::size_t last = s.size();
    while(last > first && isSpace(static_cast<unsigned char>(s[last - 1]))) --last;
    s = s.substr(first, last - first);

    out = {};
    if(s.empty() || s == "NONE") return true;

    if(s.size() >= 2 && s[0] == 'B'){
        bool digits = s.size() > 1;
        for(std::size_t i = 1; i < s.size(); ++i)
            if(s[i] < '0' || s[i] > '9') digits = false;
        if(!digits) return false;
        out.kind = Input::Button;
        out.value = std::atoi(s.c_str() + 1);
        return out.value >= 0;
    }

    if(s.size() >= 3 && s.rfind("AX", 0) == 0){
        std::size_t i = 2;
        const std::size_t digitStart = i;
        while(i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
        if(i == digitStart) return false; // missing axis index
        const int axis = std::atoi(s.substr(digitStart, i - digitStart).c_str());
        if(i >= s.size()) return false; // missing direction
        if(s[i] == '-') out.negative = true;
        else if(s[i] == '+') out.negative = false;
        else return false;
        ++i;
        if(i != s.size()) return false; // trailing characters
        out.kind = Input::Axis;
        out.value = axis;
        return out.value >= 0;
    }

    // keyboard scancode index
    for(char c : s)
        if(c < '0' || c > '9') return false;
    out.kind = Input::Key;
    out.value = std::atoi(s.c_str());
    return out.value >= 0;
}

inline bool decodeActionBinding(const std::string& text, ActionBinding& out)
{
    out.clear();
    std::size_t start = 0;
    int slot = 0;
    while(start <= text.size() && slot < 3){
        std::size_t sep = text.find('|', start);
        std::string token = (sep == std::string::npos)
            ? text.substr(start) : text.substr(start, sep - start);
        Input parsed;
        if(!parseInput(token, parsed)) return false;
        out.slots[slot] = parsed;
        ++slot;
        if(sep == std::string::npos) break;
        start = sep + 1;
    }
    return true;
}

inline std::string encodeProfile(const Profile& profile, bool keyboardProfile)
{
    std::string out;
    const auto writeAction = [&](Action a){
        if(!out.empty()) out += ";";
        out += std::string(actionTag(a)) + ":";
        out += encodeActionBinding(profile.actions[static_cast<std::size_t>(a)]);
    };
    if(keyboardProfile){
        for(Action a : keyboardActionList()) writeAction(a);
    } else {
        for(Action a : padActionList()) writeAction(a);
    }
    return out;
}

inline bool decodeProfile(const std::string& text, Profile& profile, bool keyboardProfile)
{
    std::size_t start = 0;
    while(start <= text.size()){
        std::size_t sep = text.find(';', start);
        std::string segment = (sep == std::string::npos)
            ? text.substr(start) : text.substr(start, sep - start);
        if(!segment.empty()){
            std::size_t colon = segment.find(':');
            if(colon != std::string::npos){
                std::string tag = segment.substr(0, colon);
                std::string value = segment.substr(colon + 1);
                Action a = actionFromTag(tag);
                if(a != Action::None){
                    bool relevant = keyboardProfile ? isKeyboardAction(a) : isPadAction(a);
                    if(relevant){
                        ActionBinding b;
                        if(decodeActionBinding(value, b))
                            profile.actions[static_cast<std::size_t>(a)] = b;
                    }
                }
            }
        }
        if(sep == std::string::npos) break;
        start = sep + 1;
    }
    return true;
}

// Removes `in` from every action of `profile` so a captured input is never
// bound to two actions at once, then binds it as the primary of `a`.
inline void bindPrimary(Profile& profile, Action a, const Input& in)
{
    if(!in.valid()) return;
    for(std::size_t i = 0; i < kActionCount; ++i){
        ActionBinding& b = profile.actions[i];
        if(b.contains(in) && static_cast<Action>(i) != a)
            b.clear();
    }
    profile.actions[static_cast<std::size_t>(a)].setPrimary(in);
}

} // namespace controls
