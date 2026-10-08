#ifndef VOXELSPIRE_CORE_SETTINGS_PERFORMANCE_HPP
#define VOXELSPIRE_CORE_SETTINGS_PERFORMANCE_HPP

#include <cstddef>

namespace voxelspire {

struct SimulationSettings {
    double tick_rate           = 60.0;
    double game_speed          = 1.0;
    int    max_ticks_per_frame = 10;
};

struct StreamingSettings {
    int         simulation_distance    = 8;
    int         detail_distance        = 12;
    int         unload_margin          = 2;
    int         max_column_jobs        = 64;
    int         max_mesh_jobs          = 256;
    std::size_t upload_bytes_per_frame = 16u * 1024u * 1024u;
    double      result_time_budget_ms  = 4.0;
    int         worker_threads         = 0;
};

struct LodSettings {
    bool        enabled            = true;
    int         tile_chunks        = 2;
    int         max_level          = 12;
    int         heightmap_level    = 4;
    double      coverage_threshold = 0.5;
    std::size_t max_tile_jobs      = 32;
    int         exact_levels       = 2;
    int         samples_per_cell   = 4;
};

struct EntitySettings {
    std::size_t octree_max_per_node = 8;
    std::size_t octree_max_depth    = 8;
    double      octree_looseness    = 2.0;
    double      octree_margin       = 1.0;
    double      push_acceleration   = 24.0;
    double      max_push_speed      = 4.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_PERFORMANCE_HPP