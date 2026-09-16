#include "CliOptions.h"

#include "SettingsParse.h"

#include <cctype>
#include <cstdio>

namespace cli_options {

void printUsage(const char* argv0)
{
    std::printf("Usage: %s [--no-music] [--no-postfx] [--assets-dir PATH] [--bot]"
                " [--bot-difficulty easy|medium|hard]"
                " [--perf high|medium|low|ultra] [--log-perf PATH]"
                " [--log-interval N] [--log-duration N]"
                " [--gl-profile default|clean|nvidia|dri3-off|software]"
                " [--gl-info] [--smoke-test] [--verbose]\n", argv0);
}

ParseResult parse(int argc, char** argv, Parsed& out)
{
    for(int i = 1; i < argc; ++i){
        std::string a = argv[i];
        if(a == "--help" || a == "-h") return ParseResult::HELP;

        if(a == "--no-music" || a == "-n"){
            out.noMusic = true;
        } else if(a == "--no-postfx"){
            out.noPostfx = true;
        } else if(a == "--bot" || a == "-b"){
            out.botEnabled = true;
        } else if(a == "--verbose" || a == "-v"){
            out.verbose = true;
        } else if(a == "--gl-info"){
            out.glInfo = true;
        } else if(a == "--smoke-test"){
            out.smokeTest = true;
        } else if(a == "--bot-difficulty"){
            if(i + 1 >= argc){
                std::fprintf(stderr, "Missing value for %s\n", a.c_str());
                return ParseResult::ERROR;
            }
            std::string v = argv[++i];
            for(char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if(v == "easy"){
                out.botDifficulty = BotDifficulty::EASY;
                out.hasBotDifficulty = true;
            } else if(v == "medium"){
                out.botDifficulty = BotDifficulty::MEDIUM;
                out.hasBotDifficulty = true;
            } else if(v == "hard"){
                out.botDifficulty = BotDifficulty::HARD;
                out.hasBotDifficulty = true;
            } else {
                std::fprintf(stderr, "Unknown bot difficulty '%s'\n", argv[i]);
                return ParseResult::ERROR;
            }
        } else if(a == "--assets-dir"){
            if(i + 1 >= argc){
                std::fprintf(stderr, "Missing value for %s\n", a.c_str());
                return ParseResult::ERROR;
            }
            out.assetsDir = argv[++i];
        } else if(a == "--perf"){
            if(i + 1 >= argc){
                std::fprintf(stderr, "Missing value for %s\n", a.c_str());
                return ParseResult::ERROR;
            }
            std::string v = argv[++i];
            for(char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if(v == "ultra"){
                out.perfLevel = PerfLevel::ULTRA;
                out.hasPerfLevel = true;
            } else if(v == "low"){
                out.perfLevel = PerfLevel::LOW;
                out.hasPerfLevel = true;
            } else if(v == "medium"){
                out.perfLevel = PerfLevel::MEDIUM;
                out.hasPerfLevel = true;
            } else if(v == "high"){
                out.perfLevel = PerfLevel::HIGH;
                out.hasPerfLevel = true;
            } else {
                std::fprintf(stderr, "Unknown perf level '%s'\n", argv[i]);
                return ParseResult::ERROR;
            }
        } else if(a == "--log-perf"){
            if(i + 1 >= argc){
                std::fprintf(stderr, "Missing value for %s\n", a.c_str());
                return ParseResult::ERROR;
            }
            out.perfLog.file = argv[++i];
            out.hasPerfLogFile = true;
        } else if(a == "--log-interval"){
            if(i + 1 >= argc){
                std::fprintf(stderr, "Missing value for %s\n", a.c_str());
                return ParseResult::ERROR;
            }
            auto parsed = settings_parse::parseIntClamped(argv[++i], 1, 3600);
            if(!parsed){
                std::fprintf(stderr, "Invalid --log-interval value '%s'\n", argv[i]);
                return ParseResult::ERROR;
            }
            out.perfLog.interval = *parsed;
        } else if(a == "--log-duration"){
            if(i + 1 >= argc){
                std::fprintf(stderr, "Missing value for %s\n", a.c_str());
                return ParseResult::ERROR;
            }
            auto parsed = settings_parse::parseIntClamped(argv[++i], 0, 86400);
            if(!parsed){
                std::fprintf(stderr, "Invalid --log-duration value '%s'\n", argv[i]);
                return ParseResult::ERROR;
            }
            out.perfLog.duration = *parsed;
        } else if(a == "--gl-profile"){
            if(i + 1 >= argc){
                std::fprintf(stderr, "Missing value for %s\n", a.c_str());
                return ParseResult::ERROR;
            }
            std::string v = argv[++i];
            for(char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if(!graphics_profile::parseGlProfile(v, out.glProfile)){
                std::fprintf(stderr, "Unknown GL profile '%s'\n", argv[i]);
                return ParseResult::ERROR;
            }
        } else {
            std::fprintf(stderr, "Unknown option '%s'\n", a.c_str());
            return ParseResult::ERROR;
        }
    }

    return ParseResult::OK;
}

} // namespace cli_options
