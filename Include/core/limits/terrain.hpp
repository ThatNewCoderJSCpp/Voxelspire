#ifndef VOXELSPIRE_CORE_LIMITS_TERRAIN_HPP
#define VOXELSPIRE_CORE_LIMITS_TERRAIN_HPP

#include "bounds.hpp"
#include "engine.hpp"

namespace voxelspire {

struct BiomeOptionLimits {
    static constexpr Bounds weight       { 0.0, 4.0 };
    static constexpr Bounds size         { 0.25, 4.0 };
    static constexpr Bounds shift        { -1.0, 1.0 };
    static constexpr Bounds filler_depth { 1.0, 16.0 };
    static constexpr Bounds temperature  { -50.0, 60.0 };
    static constexpr Bounds swing        { 0.0, 60.0 };
    static constexpr Bounds rainfall     { 0.0, 4.0 };
    static constexpr Bounds waves        { 0.0, 3.0 };
};

struct CaveLimits {
    static constexpr Bounds amount         { 0.0, 4.0 };
    static constexpr Bounds size           { 0.25, 4.0 };
    static constexpr Bounds flatness       { 0.25, 4.0 };
    static constexpr Bounds cavern_roof    { 2.0, 32.0 };
    static constexpr Bounds depth          { 0.0, 512.0 };
    static constexpr Bounds deep_growth    { 0.0, 3.0 };
    static constexpr Bounds entrance_depth { 4.0, 128.0 };
    static constexpr Bounds canyon_depth   { 4.0, 96.0 };
};

struct TerrainLimits {
    static constexpr Bounds sea_level       { EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z };
    static constexpr Bounds bedrock_layers  { 0.0, 8.0 };
    static constexpr Bounds biome_size      { 0.25, 8.0 };
    static constexpr Bounds continent_size  { 0.25, 8.0 };
    static constexpr Bounds size_variation  { 0.0, 0.75 };
    static constexpr Bounds climate_shift   { 0.0, 0.5 };
    static constexpr Bounds ocean_amount    { 0.0, 2.0 };
    static constexpr Bounds land_height     { 4.0, 120.0 };
    static constexpr Bounds ocean_depth     { 4.0, 120.0 };
    static constexpr Bounds mountain_amount { 0.0, 2.0 };
    static constexpr Bounds mountain_height { 0.0, 300.0 };
    static constexpr Bounds flatness        { 0.0, 2.0 };
    static constexpr Bounds hill_height     { 0.0, 60.0 };
    static constexpr Bounds roughness       { 0.0, 4.0 };
    static constexpr Bounds river_width     { 0.25, 4.0 };
    static constexpr Bounds river_depth     { 1.0, 20.0 };
    static constexpr Bounds shelf_width     { 0.25, 4.0 };
    static constexpr Bounds snow_line       { 0.2, 4.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_TERRAIN_HPP