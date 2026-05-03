#pragma once

#include <string_view>

namespace graphics_profile {

enum class GlProfile {
    Default,
    Clean,
    Nvidia,
    Dri3Off,
    Software,
};

inline const char* glProfileName(GlProfile profile)
{
    switch(profile){
    case GlProfile::Default:  return "default";
    case GlProfile::Clean:    return "clean";
    case GlProfile::Nvidia:   return "nvidia";
    case GlProfile::Dri3Off:  return "dri3-off";
    case GlProfile::Software: return "software";
    }
    return "default";
}

inline bool parseGlProfile(std::string_view value, GlProfile& out)
{
    if(value == "default"){
        out = GlProfile::Default;
    } else if(value == "clean"){
        out = GlProfile::Clean;
    } else if(value == "nvidia"){
        out = GlProfile::Nvidia;
    } else if(value == "dri3-off"){
        out = GlProfile::Dri3Off;
    } else if(value == "software"){
        out = GlProfile::Software;
    } else {
        return false;
    }
    return true;
}

} // namespace graphics_profile
