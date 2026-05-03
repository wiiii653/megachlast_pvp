#include "CliOptions.h"

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
    {
        cli_options::Parsed parsed{};
        const char* argv[] = {
            "megablast_pvp_sfml",
            "--assets-dir", "custom_assets",
            "--bot",
            "--bot-difficulty", "hard",
            "--perf", "ultra",
            "--log-perf", "out.csv",
            "--log-interval", "5",
            "--log-duration", "30",
            "--verbose",
            "--gl-info",
            "--gl-profile", "dri3-off",
            "--no-postfx",
            "--no-music",
        };
        auto res = cli_options::parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])),
                                      const_cast<char**>(argv),
                                      parsed);
        check(res == cli_options::ParseResult::OK, "parse accepts valid argument set");
        check(parsed.assetsDir == "custom_assets", "assets-dir override is parsed");
        check(parsed.botEnabled, "--bot enables bot flag");
        check(parsed.hasBotDifficulty && parsed.botDifficulty == BotDifficulty::HARD,
              "--bot-difficulty hard is parsed");
        check(parsed.hasPerfLevel && parsed.perfLevel == PerfLevel::ULTRA,
              "--perf ultra is parsed");
        check(parsed.hasPerfLogFile && parsed.perfLog.file == "out.csv",
              "--log-perf path is parsed");
        check(parsed.perfLog.interval == 5, "--log-interval is parsed");
        check(parsed.perfLog.duration == 30, "--log-duration is parsed");
        check(parsed.verbose, "--verbose enables verbose flag");
        check(parsed.glInfo, "--gl-info enables graphics diagnostics");
        check(parsed.glProfile == graphics_profile::GlProfile::Dri3Off,
              "--gl-profile dri3-off is parsed");
        check(parsed.noPostfx, "--no-postfx enables postfx flag");
        check(parsed.noMusic, "--no-music enables no-music flag");
    }

    {
        cli_options::Parsed parsed{};
        const char* argv[] = {"megablast_pvp_sfml", "--help"};
        auto res = cli_options::parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])),
                                      const_cast<char**>(argv),
                                      parsed);
        check(res == cli_options::ParseResult::HELP, "--help reports help result");
    }

    {
        cli_options::Parsed parsed{};
        const char* argv[] = {"megablast_pvp_sfml", "--perf"};
        auto res = cli_options::parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])),
                                      const_cast<char**>(argv),
                                      parsed);
        check(res == cli_options::ParseResult::ERROR,
              "missing perf value returns error");
    }

    {
        cli_options::Parsed parsed{};
        const char* argv[] = {"megablast_pvp_sfml", "--bad-flag"};
        auto res = cli_options::parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])),
                                      const_cast<char**>(argv),
                                      parsed);
        check(res == cli_options::ParseResult::ERROR,
               "unknown flag returns error");
    }

    {
        cli_options::Parsed parsed{};
        const char* argv[] = {"megablast_pvp_sfml", "--gl-profile", "bogus"};
        auto res = cli_options::parse(static_cast<int>(sizeof(argv) / sizeof(argv[0])),
                                      const_cast<char**>(argv),
                                      parsed);
        check(res == cli_options::ParseResult::ERROR,
              "unknown GL profile returns error");
    }

    return failures == 0 ? 0 : 1;
}
