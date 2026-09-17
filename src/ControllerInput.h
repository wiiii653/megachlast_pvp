#pragma once

#include <SFML/Window/Joystick.hpp>

#include <cctype>
#include <string>

namespace controller_input {

enum class Family { Generic, Xbox, Ps4, Ps5 };

inline Family familyForName(std::string name)
{
    for(char& character : name)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    if(name.find("xbox") != std::string::npos || name.find("x-input") != std::string::npos)
        return Family::Xbox;
    if(name.find("dualsense") != std::string::npos || name.find("ps5") != std::string::npos ||
       name.find("playstation 5") != std::string::npos)
        return Family::Ps5;
    if(name.find("dualshock") != std::string::npos || name.find("ps4") != std::string::npos ||
       name.find("playstation 4") != std::string::npos || name.find("playstation") != std::string::npos ||
       name.find("sony") != std::string::npos)
        return Family::Ps4;
    return Family::Generic;
}

inline Family familyForJoystick(unsigned int joystick)
{
    if(joystick >= sf::Joystick::Count || !sf::Joystick::isConnected(joystick))
        return Family::Generic;
    const auto identification = sf::Joystick::getIdentification(joystick);
    if(identification.vendorId == 0x045e) return Family::Xbox; // Microsoft
    if(identification.vendorId == 0x054c){                    // Sony
        if(identification.productId == 0x0ce6 || identification.productId == 0x0df2)
            return Family::Ps5; // DualSense
        if(identification.productId == 0x05c4 || identification.productId == 0x09cc ||
           identification.productId == 0x0ba0)
            return Family::Ps4; // DualShock 4
        const Family byName = familyForName(identification.name.toAnsiString());
        if(byName != Family::Generic) return byName;
        return Family::Ps4; // unknown Sony device: assume PS layout
    }
    return familyForName(identification.name.toAnsiString());
}

} // namespace controller_input
