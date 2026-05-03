#include "AudioRuntime.h"

#include "AssetRuntime.h"

#include <cstdio>

namespace audio_runtime {

void applyAudioSettings(sf::Music& music, bool muted, float musicVolume, float duckGain)
{
    float mv = muted ? 0.f : (musicVolume * duckGain);
    music.setVolume(mv);
}

TrackLoadResult loadMusicTracks(const std::string& assetsDir,
                                bool noMusic,
                                sf::Music& menuMusic,
                                sf::Music& ingameMusic,
                                sf::Music& getReadyMusic)
{
    TrackLoadResult result{};

    std::string menuPath = asset_runtime::findAsset(assetsDir, "menu.mp3");
    if(!noMusic && !menuPath.empty()){
        result.haveMenu = menuMusic.openFromFile(menuPath);
        if(result.haveMenu){
            menuMusic.setLooping(true);
            menuMusic.play();
        } else {
            std::fprintf(stderr, "Failed to load music: %s\n", menuPath.c_str());
        }
    }

    std::string ingamePath = asset_runtime::findAsset(assetsDir, "ingame.mp3", false);
    if(!noMusic && ingamePath.empty())
        std::fprintf(stderr, "Warning: no ingame music asset found\n");
    if(!noMusic && !ingamePath.empty()){
        result.haveIngame = ingameMusic.openFromFile(ingamePath);
        if(result.haveIngame){
            ingameMusic.setLooping(true);
        } else {
            std::fprintf(stderr, "Failed to load ingame music: %s\n", ingamePath.c_str());
        }
    }

    std::string getReadyPath = asset_runtime::findAsset(assetsDir, "get_ready.mp3");
    if(getReadyPath.empty()) getReadyPath = asset_runtime::findAsset(assetsDir, "get ready.mp3");
    if(!getReadyPath.empty()){
        result.haveGetReady = getReadyMusic.openFromFile(getReadyPath);
        if(!result.haveGetReady)
            std::fprintf(stderr, "Failed to load get ready track: %s\n", getReadyPath.c_str());
    }

    return result;
}

void stopAllMusic(bool haveMusic,
                  sf::Music& music,
                  bool haveIngameMusic,
                  sf::Music& ingameMusic,
                  bool haveGetReady,
                  sf::Music& getReady)
{
    if(haveMusic){
        music.stop();
        music.setVolume(0);
    }
    if(haveIngameMusic){
        ingameMusic.stop();
        ingameMusic.setVolume(0);
    }
    if(haveGetReady){
        getReady.stop();
        getReady.setVolume(0);
        getReady.setLooping(false);
    }
}

void playMenuMusic(bool haveMusic,
                   sf::Music& music,
                   bool haveIngameMusic,
                   sf::Music& ingameMusic,
                   bool haveGetReady,
                   sf::Music& getReady,
                   const std::function<void(sf::Music&)>& applyAudioSettingsCb)
{
    if(haveMusic && music.getStatus() == sf::Music::Status::Playing){
        applyAudioSettingsCb(music);
        return;
    }
    stopAllMusic(haveMusic, music, haveIngameMusic, ingameMusic, haveGetReady, getReady);
    if(haveMusic){
        music.setLooping(true);
        applyAudioSettingsCb(music);
        music.play();
    }
}

void playIngameMusic(bool haveMusic,
                     sf::Music& music,
                     bool haveIngameMusic,
                     sf::Music& ingameMusic,
                     bool haveGetReady,
                     sf::Music& getReady,
                     const std::function<void(sf::Music&)>& applyAudioSettingsCb)
{
    stopAllMusic(haveMusic, music, haveIngameMusic, ingameMusic, haveGetReady, getReady);
    if(haveIngameMusic){
        ingameMusic.setLooping(true);
        applyAudioSettingsCb(ingameMusic);
        ingameMusic.play();
    }
}

void playGetReady(bool loop,
                  bool haveMusic,
                  sf::Music& music,
                  bool haveIngameMusic,
                  sf::Music& ingameMusic,
                  bool haveGetReady,
                  sf::Music& getReady,
                  const std::function<void(sf::Music&)>& applyAudioSettingsCb)
{
    stopAllMusic(haveMusic, music, haveIngameMusic, ingameMusic, haveGetReady, getReady);
    if(haveGetReady){
        getReady.setLooping(loop);
        applyAudioSettingsCb(getReady);
        getReady.play();
    }
}

} // namespace audio_runtime
