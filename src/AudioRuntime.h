#pragma once

#include <SFML/Audio.hpp>

#include <functional>
#include <string>

namespace audio_runtime {

struct TrackLoadResult {
    bool haveMenu = false;
    bool haveIngame = false;
    bool haveGetReady = false;
};

void applyAudioSettings(sf::Music& music, bool muted, float musicVolume, float duckGain);
TrackLoadResult loadMusicTracks(const std::string& assetsDir,
                                bool noMusic,
                                sf::Music& menuMusic,
                                sf::Music& ingameMusic,
                                sf::Music& getReadyMusic);
void stopAllMusic(bool haveMusic,
                  sf::Music& music,
                  bool haveIngameMusic,
                  sf::Music& ingameMusic,
                  bool haveGetReady,
                  sf::Music& getReady);
void playMenuMusic(bool haveMusic,
                   sf::Music& music,
                   bool haveIngameMusic,
                   sf::Music& ingameMusic,
                   bool haveGetReady,
                   sf::Music& getReady,
                   const std::function<void(sf::Music&)>& applyAudioSettings);
void playIngameMusic(bool haveMusic,
                     sf::Music& music,
                     bool haveIngameMusic,
                     sf::Music& ingameMusic,
                     bool haveGetReady,
                     sf::Music& getReady,
                     const std::function<void(sf::Music&)>& applyAudioSettings);
void playGetReady(bool loop,
                  bool haveMusic,
                  sf::Music& music,
                  bool haveIngameMusic,
                  sf::Music& ingameMusic,
                  bool haveGetReady,
                  sf::Music& getReady,
                  const std::function<void(sf::Music&)>& applyAudioSettings);

} // namespace audio_runtime
