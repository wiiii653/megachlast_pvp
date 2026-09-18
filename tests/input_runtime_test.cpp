#include "InputRuntime.h"
#include "ControlsInput.h"
#include "ControllerInput.h"

#include <iostream>

namespace {

int failures = 0;

void check(bool ok, const char* message)
{
    if(!ok){
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main()
{
    const controls::Profile keyboard =
        controls::defaultProfile(controls::ProfileId::Keyboard);

    GameState state = GameState::MATCH_SETUP;
    MatchSetup setup{};
    int starts = 0;
    auto pressWith = [&](const controls::Profile& profile, sf::Keyboard::Scancode scancode){
        sf::Event::KeyPressed event{};
        event.scancode = scancode;
        if(scancode == sf::Keyboard::Scan::Escape) event.code = sf::Keyboard::Key::Escape;
        input_runtime::handleMatchSetupKeyPressed(event, state, setup, profile,
                                                 [&]{ ++starts; });
    };
    auto press = [&](sf::Keyboard::Scancode scancode){ pressWith(keyboard, scancode); };

    press(sf::Keyboard::Scan::D);
    check(setup.p1_modifier == RoundModifier::RAPID && setup.p2_modifier == RoundModifier::RAPID,
          "D changes only P1's modifier");
    press(sf::Keyboard::Scan::A);
    press(sf::Keyboard::Scan::A);
    check(setup.p1_modifier == RoundModifier::OVERDRIVE, "P1 cycles backwards with wraparound");
    press(sf::Keyboard::Scan::D);
    check(setup.p1_modifier == RoundModifier::SHIELD, "P1 cycles forwards with wraparound");
    press(sf::Keyboard::Scan::Right);
    check(setup.p2_modifier == RoundModifier::SPREAD && setup.p1_modifier == RoundModifier::SHIELD,
          "Right changes only P2's modifier");
    press(sf::Keyboard::Scan::Left);
    check(setup.p2_modifier == RoundModifier::RAPID, "Left cycles P2's modifier backwards");
    const auto arena = setup.arena;
    press(sf::Keyboard::Scan::Q);
    check(setup.arena != arena, "Q changes the arena");
    press(sf::Keyboard::Scan::E);
    check(setup.arena == arena, "E restores the next arena");
    check(starts == 0, "selection keys do not start the match");
    press(sf::Keyboard::Scan::Enter);
    check(starts == 1, "Enter starts the configured match once");
    press(sf::Keyboard::Scan::Escape);
    check(state == GameState::MENU && starts == 1, "Escape returns to menu without starting");

    // Rebind P1 modifier left to J: the new key works, the old one stops.
    controls::Profile rebound = keyboard;
    controls::bindPrimary(rebound, controls::Action::SetupModLeft,
                          controls::keyInput(sf::Keyboard::Scan::J));
    state = GameState::MATCH_SETUP;
    setup = {};
    sf::Event::KeyPressed jEvent{};
    jEvent.scancode = sf::Keyboard::Scan::J;
    input_runtime::handleMatchSetupKeyPressed(jEvent, state, setup, rebound, []{});
    check(setup.p1_modifier == RoundModifier::OVERDRIVE,
          "rebound modifier key drives P1's modifier");
    pressWith(rebound, sf::Keyboard::Scan::A);
    check(setup.p1_modifier == RoundModifier::OVERDRIVE,
          "old modifier key stops working after rebind");
    // Rebinding in one profile must not affect another profile.
    controls::Profile ps4 = controls::defaultProfile(controls::ProfileId::Ps4);
    controls::Profile xbox = controls::defaultProfile(controls::ProfileId::Xbox);
    check(ps4.actions[static_cast<std::size_t>(controls::Action::GamePause)].contains(
              controls::buttonInput(9)),
          "PS4 default pause uses the PS Start button (9)");
    check(xbox.actions[static_cast<std::size_t>(controls::Action::GamePause)].contains(
              controls::buttonInput(7)),
          "Xbox default pause uses the Xbox Start button (7)");
    controls::bindPrimary(ps4, controls::Action::GamePause, controls::buttonInput(7));
    check(ps4.actions[static_cast<std::size_t>(controls::Action::GamePause)].contains(
              controls::buttonInput(7)),
          "PS4 pause rebind applied");
    check(!ps4.actions[static_cast<std::size_t>(controls::Action::GamePause)].contains(
              controls::buttonInput(9)),
          "PS4 default start cleared after rebind");
    check(xbox.actions[static_cast<std::size_t>(controls::Action::GamePause)].contains(
              controls::buttonInput(7)) &&
          !xbox.actions[static_cast<std::size_t>(controls::Action::GamePause)].contains(
              controls::buttonInput(9)),
          "Xbox profile untouched by the PS4 rebind");

    check(controller_input::familyForName("Xbox Wireless Controller") == controller_input::Family::Xbox,
          "Xbox controllers are identified");
    check(controller_input::familyForName("DualSense Wireless Controller") == controller_input::Family::Ps5,
          "PS5 controllers are identified");
    check(controller_input::familyForName("Wireless Controller (PS4)") == controller_input::Family::Ps4,
          "PS4 controllers are identified");
    check(controller_input::familyForName("Some Generic Pad") == controller_input::Family::Generic,
          "unknown controllers fall back to generic");

    int menuSel = 0;
    int menuAction = -1;
    auto menuPress = [&](sf::Keyboard::Scancode scancode){
        sf::Event::KeyPressed event{};
        event.scancode = scancode;
        input_runtime::handleStateTransitionKeyPressed(
            event, GameState::MENU, menuSel, keyboard,
            [&]{ menuAction = 0; }, [&]{ menuAction = 1; }, [&]{ menuAction = 2; },
            []{}, []{}, []{}, []{});
    };
    menuPress(sf::Keyboard::Scan::Right);
    check(menuSel == 1, "menu Right selects Settings");
    menuPress(sf::Keyboard::Scan::Enter);
    check(menuAction == 1, "menu Enter accepts the selected item");
    menuPress(sf::Keyboard::Scan::Right);
    menuPress(sf::Keyboard::Scan::Enter);
    check(menuSel == 2 && menuAction == 2, "menu selection reaches Donate");

    // Escape is the back/return action from the menu screens. Settings and
    // Donate return to the menu; Controls steps back to Settings.
    {
        GameState settingsState = GameState::SETTINGS;
        int settingsSel = 0;
        Config cfg{};
        bool botEnabled = false;
        BotDifficulty botDifficulty = BotDifficulty::EASY;
        float musicVolume = 50.f;
        float sfxVolume = 50.f;
        GraphicsSettings graphicsSettings{};
        ControllerSettings controllers;
        controllers.profiles = controls::defaultProfiles();
        int controlsProfile = 0;
        int controlsSel = 0;
        int controlsScroll = 0;
        bool controlsCapturing = false;
        int controlsCaptureAction = static_cast<int>(controls::Action::None);
        int saves = 0;
        int loads = 0;
        sf::Event::KeyPressed esc{};
        esc.scancode = sf::Keyboard::Scan::Escape;
        esc.code = sf::Keyboard::Key::Escape;
        input_runtime::handleSettingsKeyPressed(
            esc, settingsState, settingsSel, cfg, botEnabled, botDifficulty,
            musicVolume, sfxVolume, graphicsSettings, controllers,
            controlsProfile, controlsSel, controlsScroll, controlsCapturing,
            controlsCaptureAction, "settings.cfg",
            [&](const std::string&){ ++saves; }, [&](const std::string&){ ++loads; return true; },
            []{});
        check(settingsState == GameState::MENU, "Escape returns to the menu from Settings");
        check(saves == 0 && loads == 0, "Escape from Settings only navigates back");

        GameState controlsState = GameState::CONTROLS;
        input_runtime::handleControlsKeyPressed(
            esc, controlsState, controlsProfile, controlsSel, controlsScroll,
            controlsCapturing, controlsCaptureAction, controllers, "settings.cfg",
            [&](const std::string&){ ++saves; }, [&](const std::string&){ ++loads; return true; });
        check(controlsState == GameState::SETTINGS, "Escape returns to Settings from Controls");

        GameState donateState = GameState::DONATE;
        float donateMsgTimer = 0.f;
        int donateSel = 0;
        input_runtime::handleDonateKeyPressed(esc, donateState, donateMsgTimer, donateSel, keyboard);
        check(donateState == GameState::MENU, "Escape returns to the menu from Donate");
    }

    // A non-back key must not navigate back from Settings.
    {
        GameState settingsState = GameState::SETTINGS;
        int settingsSel = 0;
        Config cfg{};
        bool botEnabled = false;
        BotDifficulty botDifficulty = BotDifficulty::EASY;
        float musicVolume = 50.f;
        float sfxVolume = 50.f;
        GraphicsSettings graphicsSettings{};
        ControllerSettings controllers;
        controllers.profiles = controls::defaultProfiles();
        int controlsProfile = 0;
        int controlsSel = 0;
        int controlsScroll = 0;
        bool controlsCapturing = false;
        int controlsCaptureAction = static_cast<int>(controls::Action::None);
        sf::Event::KeyPressed down{};
        down.scancode = sf::Keyboard::Scan::Down;
        input_runtime::handleSettingsKeyPressed(
            down, settingsState, settingsSel, cfg, botEnabled, botDifficulty,
            musicVolume, sfxVolume, graphicsSettings, controllers,
            controlsProfile, controlsSel, controlsScroll, controlsCapturing,
            controlsCaptureAction, "settings.cfg",
            [](const std::string&){}, [](const std::string&){ return false; },
            []{});
        check(settingsState == GameState::SETTINGS && settingsSel == 1,
              "navigation keys move the cursor without leaving Settings");
    }

    return failures == 0 ? 0 : 1;
}
