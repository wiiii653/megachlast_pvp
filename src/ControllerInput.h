#pragma once

#include <SFML/Window/Joystick.hpp>

#include <cctype>
#include <string>

namespace controller_input {

enum class Family { Generic, Xbox, PlayStation };

inline Family familyForName(std::string name)
{
    for(char& character : name)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    if(name.find("xbox") != std::string::npos || name.find("x-input") != std::string::npos)
        return Family::Xbox;
    if(name.find("sony") != std::string::npos || name.find("dualshock") != std::string::npos ||
       name.find("dualsense") != std::string::npos ||
       name.find("playstation") != std::string::npos || name.find("ps4") != std::string::npos ||
       name.find("ps5") != std::string::npos)
        return Family::PlayStation;
    return Family::Generic;
}

inline Family familyForJoystick(unsigned int joystick)
{
    const auto identification = sf::Joystick::getIdentification(joystick);
    if(identification.vendorId == 0x054c) return Family::PlayStation; // Sony
    if(identification.vendorId == 0x045e) return Family::Xbox;        // Microsoft
    return familyForName(identification.name.toAnsiString());
}

inline bool isButtonPressed(unsigned int joystick, unsigned int button)
{
    return sf::Joystick::getButtonCount(joystick) > button &&
           sf::Joystick::isButtonPressed(joystick, button);
}

inline bool firePressed(unsigned int joystick)
{
    // SFML reports raw joystick buttons. Xbox/XInput layouts put A at 0, and
    // the PlayStation layouts used by SFML put Cross at 0.
    const unsigned int south = 0u;
    return isButtonPressed(joystick, south) ||
           isButtonPressed(joystick, 4) || // L1/LB
           isButtonPressed(joystick, 5);   // R1/RB
}

inline unsigned int southButton(unsigned int /*joystick*/)
{
    return 0u;
}

inline unsigned int eastButton(unsigned int joystick)
{
    return familyForJoystick(joystick) == Family::PlayStation ? 1u : 1u;
}

inline unsigned int northButton(unsigned int joystick)
{
    return familyForJoystick(joystick) == Family::PlayStation ? 2u : 3u;
}

inline unsigned int westButton(unsigned int joystick)
{
    return familyForJoystick(joystick) == Family::PlayStation ? 3u : 2u;
}

inline unsigned int startButton(unsigned int joystick)
{
    return familyForJoystick(joystick) == Family::PlayStation ? 9u : 7u;
}

inline unsigned int selectButton(unsigned int joystick)
{
    return familyForJoystick(joystick) == Family::PlayStation ? 8u : 6u;
}

inline bool buttonPressed(unsigned int joystick, unsigned int button)
{
    return isButtonPressed(joystick, button);
}

} // namespace controller_input
