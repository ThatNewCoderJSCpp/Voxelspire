#ifndef VOXELSPIRE_CORE_LIMITS_PERFORMANCE_HPP
#define VOXELSPIRE_CORE_LIMITS_PERFORMANCE_HPP

#include "bounds.hpp"

namespace voxelspire {

struct SimulationLimits {
    static constexpr Bounds tick_rate           { 1.0, 480.0 };
    static constexpr Bounds game_speed          { 0.1, 10.0 };
    static constexpr Bounds max_ticks_per_frame { 1.0, 60.0 };
};

struct StreamingLimits {
    static constexpr Bounds simulation_distance   { 1.0, 32.0 };
    static constexpr Bounds detail_distance       { 1.0, 64.0 };
    static constexpr Bounds unload_margin         { 0.0, 8.0 };
    static constexpr Bounds max_column_jobs       { 1.0, 512.0 };
    static constexpr Bounds max_mesh_jobs         { 1.0, 1024.0 };
    static constexpr Bounds upload_megabytes      { 1.0, 256.0 };
    static constexpr Bounds result_time_budget_ms { 0.5, 33.0 };
};

struct LodLimits {
    static constexpr Bounds tile_chunks        { 1.0, 8.0 };
    static constexpr Bounds max_level          { 1.0, 16.0 };
    static constexpr Bounds heightmap_level    { 0.0, 16.0 };
    static constexpr Bounds coverage_threshold { 0.0, 1.0 };
    static constexpr Bounds max_tile_jobs      { 1.0, 128.0 };
    static constexpr Bounds exact_levels       { 0.0, 8.0 };
    static constexpr Bounds samples_per_cell   { 1.0, 8.0 };
};

struct EntityLimits {
    static constexpr Bounds octree_max_per_node { 1.0, 64.0 };
    static constexpr Bounds octree_max_depth    { 1.0, 16.0 };
    static constexpr Bounds octree_looseness    { 1.0, 4.0 };
    static constexpr Bounds octree_margin       { 0.0, 4.0 };
    static constexpr Bounds push_acceleration   { 0.0, 100.0 };
    static constexpr Bounds max_push_speed      { 0.0, 20.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_PERFORMANCE_HPP