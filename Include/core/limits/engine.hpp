#ifndef VOXELSPIRE_CORE_LIMITS_ENGINE_HPP
#define VOXELSPIRE_CORE_LIMITS_ENGINE_HPP

namespace voxelspire {

struct EngineLimits {
    static constexpr int CHUNK_SIZE           = 16;
    static constexpr int WORLD_MIN_Z          = -512;
    static constexpr int WORLD_MAX_Z          = 1024;
    static constexpr int WORLD_MAX_HORIZONTAL = 950'000'000;

    static constexpr double MAX_MOVE_PER_TICK = 64.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_ENGINE_HPP