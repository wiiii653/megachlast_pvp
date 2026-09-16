#include "InputRuntime.h"

#include <iostream>

int main()
{
    int failures = 0;
    auto check = [&](bool ok, const char* message){
        if(!ok){
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };
    GameState state = GameState::MATCH_SETUP;
    MatchSetup setup{};
    int starts = 0;
    auto press = [&](sf::Keyboard::Scancode scancode){
        sf::Event::KeyPressed event{};
        event.scancode = scancode;
        if(scancode == sf::Keyboard::Scan::Escape) event.code = sf::Keyboard::Key::Escape;
        input_runtime::handleMatchSetupKeyPressed(event, state, setup, [&]{ ++starts; });
    };

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
    return failures == 0 ? 0 : 1;
}
