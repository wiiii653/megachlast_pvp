#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <string>

namespace settings_parse {

inline std::string trim(std::string s)
{
    auto first = std::find_if_not(s.begin(), s.end(), [](unsigned char c){ return std::isspace(c); });
    auto last = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c){ return std::isspace(c); }).base();
    if(first >= last) return {};
    return std::string(first, last);
}

inline std::optional<int> parseIntClamped(const std::string& text, int lo, int hi)
{
    try {
        std::string s = trim(text);
        std::size_t pos = 0;
        int value = std::stoi(s, &pos);
        if(pos != s.size()) return std::nullopt;
        return std::clamp(value, lo, hi);
    } catch(...) {
        return std::nullopt;
    }
}

inline std::optional<float> parseFloatClamped(const std::string& text, float lo, float hi)
{
    try {
        std::string s = trim(text);
        std::size_t pos = 0;
        float value = std::stof(s, &pos);
        if(pos != s.size()) return std::nullopt;
        if(!std::isfinite(value)) return std::nullopt;
        return std::clamp(value, lo, hi);
    } catch(...) {
        return std::nullopt;
    }
}

inline std::optional<bool> parseBool(const std::string& text)
{
    std::string s = trim(text);
    for(char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if(s == "1" || s == "true" || s == "yes" || s == "on") return true;
    if(s == "0" || s == "false" || s == "no" || s == "off") return false;
    return std::nullopt;
}

} // namespace settings_parse
