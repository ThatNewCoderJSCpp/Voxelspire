#include "core/process_memory.hpp"

#include <fstream>
#include <sstream>

#if defined(OS_WINDOWS)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <psapi.h>
    #undef near
    #undef far
#endif

namespace voxelspire {

ProcessMemory ProcessMemory::read() {
    ProcessMemory m;
#if defined(OS_LINUX)
    m.used             = field("/proc/self/status", "VmRSS:");
    m.peak             = field("/proc/self/status", "VmHWM:");
    m.system_total     = field("/proc/meminfo", "MemTotal:");
    m.system_available = field("/proc/meminfo", "MemAvailable:");
    m.valid            = m.used > 0;
#elif defined(OS_WINDOWS)
    PROCESS_MEMORY_COUNTERS counters{};

    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        m.used  = counters.WorkingSetSize;
        m.peak  = counters.PeakWorkingSetSize;
        m.valid = true;
    }

    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);

    if (GlobalMemoryStatusEx(&status)) {
        m.system_total     = status.ullTotalPhys;
        m.system_available = status.ullAvailPhys;
    }
#endif
    return m;
}

std::uint64_t ProcessMemory::field(const char* path, const std::string& name) {
    std::ifstream in(path);
    std::string line;

    while (std::getline(in, line)) {
        if (line.compare(0, name.size(), name) != 0) continue;
        std::istringstream values(line.substr(name.size()));
        std::uint64_t kilobytes = 0;
        values >> kilobytes;
        return kilobytes * KILOBYTE;
    }

    return 0;
}

} // namespace voxelspire
