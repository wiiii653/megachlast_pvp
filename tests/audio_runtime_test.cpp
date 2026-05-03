#include "AudioRuntime.h"

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
    sf::Music menu;
    sf::Music ingame;
    sf::Music getReady;

    auto none = audio_runtime::loadMusicTracks("/path/that/does/not/exist", false, menu, ingame, getReady);
    check(!none.haveMenu, "missing menu track returns false");
    check(!none.haveIngame, "missing ingame track returns false");
    check(!none.haveGetReady, "missing get-ready track returns false");

    auto disabled = audio_runtime::loadMusicTracks("/path/that/does/not/exist", true, menu, ingame, getReady);
    check(!disabled.haveMenu, "--no-music path keeps menu track disabled");
    check(!disabled.haveIngame, "--no-music path keeps ingame track disabled");
    check(!disabled.haveGetReady, "missing get-ready still disabled");

    return failures == 0 ? 0 : 1;
}
