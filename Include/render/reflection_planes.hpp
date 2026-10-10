#ifndef VOXELSPIRE_RENDER_REFLECTION_PLANES_HPP
#define VOXELSPIRE_RENDER_REFLECTION_PLANES_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "../core/settings.hpp"
#include "../world/world.hpp"

namespace voxelspire {

struct ReflectionSurface {
    Face         facing  = Face::Up;
    std::int64_t level   = 0;
    bool         water   = false;
    AABB         box;
    std::size_t  cells   = 0;
    double       heights = 0.0;

    double coordinate() const noexcept { return water ? heights / static_cast<double>(cells) : static_cast<double>(level) / LEVEL_SCALE; }
    vector3d normal() const noexcept { return face_normal(facing); }

    static constexpr double LEVEL_SCALE  = 256.0;
    static constexpr double WATER_BUCKET = 0.5;
};

class ReflectionPlanes {
public:
    static constexpr double FRONT_MARGIN = 0.05;
    static constexpr double NEAR_ALWAYS  = 3.0;
    static constexpr double KEEP_BONUS   = 2.0;
    static constexpr double FLAT_PAD     = 0.01;

    void update(const World& world, const fizmo::graphics::Camera3D& camera, const LightingSettings& s, double render_distance);

    double farthest() const noexcept { return m_farthest; }

    const std::vector<fizmo::graphics::ReflectionPlane3D>& planes() const noexcept { return m_planes; }

private:
    using Key = std::tuple<int, std::int64_t, bool>;
    static constexpr std::size_t MAX_IDLE_CACHE = 512;

    void pick(const vector3d& eye, const LightingSettings& s, bool water, bool one_of_kind);

    struct GatherKey {
        std::uint64_t  revision = 0;
        std::uintptr_t rules    = 0;
        ColumnPos      column{};
        int            chunk_z  = 0;
        double         reach    = -1.0;
        bool           water    = false;

        bool operator==(const GatherKey& o) const noexcept;
    };

    void gather(const World& world, const vector3d& eye, double reach, bool water);

    struct ChunkSurfaces {
        std::uint64_t                  revision = 0;
        std::uint64_t                  seen     = 0;
        std::uintptr_t                 rules    = 0;
        bool                           scanned  = false;
        std::vector<ReflectionSurface> surfaces;
    };

    static void scan(const World& world, const Chunk& chunk, const ChunkPos& p, ChunkSurfaces& out);

    static bool level_surface(const World& world, const BlockPos& cell);

    static void add(std::map<Key, ReflectionSurface>& found, Face f, double coord, bool water, const AABB& box);

    void prune(std::uint64_t revision);

    static double distance_sq(const vector3d& p, const AABB& b) noexcept;

    std::unordered_map<ChunkPos, ChunkSurfaces, ChunkPosHash> m_cache;
    std::map<Key, ReflectionSurface>                         m_merged;
    std::vector<std::pair<double, Key>>                      m_scored;
    std::vector<Key>                                         m_chosen;
    std::vector<fizmo::graphics::ReflectionPlane3D>          m_planes;
    std::vector<bool>                                        m_taken;
    GatherKey                                                m_gathered;
    double                                                   m_farthest = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_REFLECTION_PLANES_HPP