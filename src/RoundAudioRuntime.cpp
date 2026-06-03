#include "RoundRuntime.h"

#include <SFML/Audio/Music.hpp>

namespace round_runtime {

void pauseGameplay(GameState& state, bool haveIngameMusic, sf::Music& ingameMusic)
{
    state = GameState::PAUSED;
    if(haveIngameMusic) ingameMusic.pause();
}

void resumeGameplay(GameState& state,
                    bool haveIngameMusic,
                    sf::Music& ingameMusic,
                    const std::function<void(sf::Music&)>& applyAudioSettings)
{
    state = GameState::PLAYING;
    if(haveIngameMusic){
        applyAudioSettings(ingameMusic);
        ingameMusic.play();
    }
}

} // namespace round_runtime
