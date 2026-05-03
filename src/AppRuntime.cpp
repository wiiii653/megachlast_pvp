#include "AppRuntime.h"

namespace app_runtime {

std::string settingsPath(const RuntimeOptions& options)
{
    return options.assets_dir + "/settings.cfg";
}

void applyCliOptions(const cli_options::Parsed& cli, RuntimeOptions& options)
{
    options.assets_dir = cli.assetsDir;
    if(cli.noMusic) options.no_music = true;
    if(cli.noPostfx) options.no_postfx = true;
    if(cli.botEnabled) options.bot_enabled = true;
    if(cli.verbose) options.verbose = true;
    if(cli.hasBotDifficulty) options.bot_difficulty = cli.botDifficulty;
    if(cli.hasPerfLevel) options.perf_level = cli.perfLevel;
    if(cli.hasPerfLogFile) options.perf_log.file = cli.perfLog.file;
    options.perf_log.interval = cli.perfLog.interval;
    options.perf_log.duration = cli.perfLog.duration;
}

} // namespace app_runtime
