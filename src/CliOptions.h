#pragma once

#include "BotConfig.h"
#include "GameTypes.h"
#include "GraphicsProfile.h"
#include "PerfLog.h"

#include <cstdint>
#include <string>

namespace cli_options {

struct Parsed {
    std::string assetsDir = "assets";
    bool noMusic = false;
    bool noPostfx = false;
    bool botEnabled = false;
    bool verbose = false;
    bool glInfo = false;

    graphics_profile::GlProfile glProfile = graphics_profile::GlProfile::Default;

    bool hasBotDifficulty = false;
    BotDifficulty botDifficulty = BotDifficulty::MEDIUM;

    bool hasPerfLevel = false;
    PerfLevel perfLevel = PerfLevel::MEDIUM;

    bool hasPerfLogFile = false;
    perf_log::Options perfLog;
};

enum class ParseResult : uint8_t { OK = 0, HELP = 1, ERROR = 2 };

void printUsage(const char* argv0);
ParseResult parse(int argc, char** argv, Parsed& out);

} // namespace cli_options
