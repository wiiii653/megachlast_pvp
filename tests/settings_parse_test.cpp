#include "SettingsParse.h"

#include <cmath>
#include <iostream>
#include <optional>

namespace {

int failures = 0;

void check(bool ok, const char* msg)
{
    if(!ok){
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    }
}

void checkInt(std::optional<int> got, int expected, const char* msg)
{
    check(got.has_value() && *got == expected, msg);
}

void checkFloat(std::optional<float> got, float expected, const char* msg)
{
    check(got.has_value() && std::fabs(*got - expected) < 0.0001f, msg);
}

} // namespace

int main()
{
    using namespace settings_parse;

    check(trim("  TARGET_SCORE \t") == "TARGET_SCORE", "trim removes surrounding whitespace");

    checkInt(parseIntClamped("8", 1, 99), 8, "int parser keeps in-range values");
    checkInt(parseIntClamped("-10", 1, 99), 1, "int parser clamps low values");
    checkInt(parseIntClamped("500", 1, 99), 99, "int parser clamps high values");
    check(!parseIntClamped("", 1, 99).has_value(), "int parser rejects empty strings");
    check(!parseIntClamped("12junk", 1, 99).has_value(), "int parser rejects trailing garbage");
    check(!parseIntClamped("999999999999999999999", 1, 99).has_value(), "int parser rejects overflow");

    checkFloat(parseFloatClamped("120.5", 20.f, 400.f), 120.5f, "float parser keeps in-range values");
    checkFloat(parseFloatClamped("-1", 20.f, 400.f), 20.f, "float parser clamps low values");
    checkFloat(parseFloatClamped("999", 20.f, 400.f), 400.f, "float parser clamps high values");
    check(!parseFloatClamped("", 0.f, 2.f).has_value(), "float parser rejects empty strings");
    check(!parseFloatClamped("1.2x", 0.f, 2.f).has_value(), "float parser rejects trailing garbage");
    check(!parseFloatClamped("nan", 0.f, 2.f).has_value(), "float parser rejects nan");
    check(!parseFloatClamped("inf", 0.f, 2.f).has_value(), "float parser rejects positive infinity");
    check(!parseFloatClamped("-inf", 0.f, 2.f).has_value(), "float parser rejects negative infinity");
    check(!parseFloatClamped("1e999", 0.f, 2.f).has_value(), "float parser rejects overflow");

    check(parseBool(" yes ") == std::optional<bool>{true}, "bool parser accepts yes");
    check(parseBool("OFF") == std::optional<bool>{false}, "bool parser accepts off");
    check(!parseBool("maybe").has_value(), "bool parser rejects unknown words");

    if(failures != 0){
        std::cerr << failures << " setting parser checks failed\n";
        return 1;
    }
    return 0;
}
