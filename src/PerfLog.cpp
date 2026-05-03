#include "PerfLog.h"

#include <chrono>
#include <cmath>
#include <cstdio>

#if defined(__linux__)
#include <fstream>
#include <sstream>
#include <string>
#elif defined(__APPLE__)
#include <mach/mach.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

namespace perf_log {

namespace {

const char* perfName(PerfLevel p)
{
    switch(p){
        case PerfLevel::HIGH:   return "HIGH";
        case PerfLevel::MEDIUM: return "MED";
        case PerfLevel::LOW:    return "LOW";
        case PerfLevel::ULTRA:  return "ULTRA";
    }
    return "MED";
}

} // namespace

bool openCsv(std::ofstream& ofs, const Options& options)
{
    if(options.file.empty()) return false;
    ofs.open(options.file, std::ios::out);
    if(!ofs){
        std::fprintf(stderr, "Failed to open perf log: %s\n", options.file.c_str());
        return false;
    }
    ofs << "time,perf,fx_level,fps,alive_particles,alive_bullets,spawn_budget,spawn_used,vm_rss_kb\n";
    return true;
}

long readVmRssKb()
{
#if defined(__linux__)
    long vmrss = 0;
    std::ifstream status("/proc/self/status");
    std::string line;
    while(std::getline(status, line)){
        if(line.rfind("VmRSS:", 0) != 0) continue;
        std::istringstream iss(line);
        std::string key;
        long value = 0;
        std::string unit;
        if(iss >> key >> value >> unit) vmrss = value;
        break;
    }
    return vmrss;
#elif defined(__APPLE__)
    mach_task_basic_info_data_t info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if(task_info(mach_task_self(),
                 MACH_TASK_BASIC_INFO,
                 reinterpret_cast<task_info_t>(&info),
                 &count) != KERN_SUCCESS){
        return 0;
    }
    return static_cast<long>(info.resident_size / 1024);
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if(!GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))){
        return 0;
    }
    return static_cast<long>(pmc.WorkingSetSize / 1024);
#else
    return 0;
#endif
}

void updateFps(float dt,
               bool verbose,
               PerfLevel perfLevel,
               float& fpsAccum,
               int& fpsFrames,
               float& fpsDisplay)
{
    fpsAccum += dt;
    fpsFrames++;
    if(fpsAccum < 1.0f) return;

    fpsDisplay = static_cast<float>(fpsFrames) / fpsAccum;
    if(verbose){
        const char* pn[] = {"HIGH", "MED", "LOW", "ULTRA"};
        std::fprintf(stderr, "FPS: %.1f  perf=%s\n", fpsDisplay, pn[static_cast<int>(perfLevel)]);
    }
    fpsAccum = 0.f;
    fpsFrames = 0;
}

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
                   int spawnUsed)
{
    if(!ofs || options.interval <= 0) return false;

    perfLogAccum += dt;
    if(perfLogAccum < static_cast<float>(options.interval)) return false;

    long vmrss = readVmRssKb();
    auto tnow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    writeCsvRow(ofs,
                tnow,
                perfLevel,
                fxLevel,
                fps,
                aliveParticles,
                aliveBullets,
                spawnBudget,
                spawnUsed,
                vmrss);
    perfLogAccum = 0.f;
    return true;
}

bool durationExpired(float totalRunTime, const Options& options)
{
    return options.duration > 0 && totalRunTime >= static_cast<float>(options.duration);
}

void writeCsvRow(std::ofstream& ofs,
                 std::time_t now,
                 PerfLevel perfLevel,
                 int fxLevel,
                 int fps,
                 int aliveParticles,
                 int aliveBullets,
                 int spawnBudget,
                 int spawnUsed,
                 long vmRssKb)
{
    ofs << now << ","
        << perfName(perfLevel) << ","
        << fxLevel << ","
        << fps << ","
        << aliveParticles << ","
        << aliveBullets << ","
        << spawnBudget << ","
        << spawnUsed << ","
        << vmRssKb << "\n";
    ofs.flush();
}

} // namespace perf_log
