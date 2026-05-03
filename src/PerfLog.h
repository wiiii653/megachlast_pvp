#pragma once

#include "GameTypes.h"

#include <ctime>
#include <fstream>
#include <string>

namespace perf_log {

struct Options {
    std::string file;
    int interval = 1;
    int duration = 0;
};

bool openCsv(std::ofstream& ofs, const Options& options);
long readVmRssKb();
void updateFps(float dt,
               bool verbose,
               PerfLevel perfLevel,
               float& fpsAccum,
               int& fpsFrames,
               float& fpsDisplay);
bool maybeWriteCsv(std::ofstream& ofs,
                   const Options& options,
                   float dt,
                   float& perfLogAccum,
                   PerfLevel perfLevel,
                   int fxLevel,
                   int fps,
                   int aliveParticles,
                   int aliveBullets,
                   int spawnBudget,
                   int spawnUsed);
bool durationExpired(float totalRunTime, const Options& options);
void writeCsvRow(std::ofstream& ofs,
                 std::time_t now,
                 PerfLevel perfLevel,
                 int fxLevel,
                 int fps,
                 int aliveParticles,
                 int aliveBullets,
                 int spawnBudget,
                 int spawnUsed,
                 long vmRssKb);

} // namespace perf_log
