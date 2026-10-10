#ifndef VOXELSPIRE_CORE_PROCESS_MEMORY_HPP
#define VOXELSPIRE_CORE_PROCESS_MEMORY_HPP

#include <cstdint>
#include <string>
#include "../fizmo.hpp"

namespace voxelspire {

struct ProcessMemory {
    bool          valid            = false;
    std::uint64_t used             = 0;
    std::uint64_t peak             = 0;
    std::uint64_t system_total     = 0;
    std::uint64_t system_available = 0;

    static ProcessMemory read();

private:
    static constexpr std::uint64_t KILOBYTE = 1024;

    static std::uint64_t field(const char* path, const std::string& name);
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_PROCESS_MEMORY_HPP