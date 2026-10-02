#ifndef VOXELSPIRE_RENDER_REFLECTION_PLANES_HPP
#define VOXELSPIRE_RENDER_REFLECTION_PLANES_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "../lighting/settings.hpp"
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

    void update(const World& world, const fizmo::graphics::Camera3D& camera, const LightingSettings& s, double render_distance) {
        m_planes.clear();
        m_farthest = 0.0;
        if (!s.planar_reflections || s.max_reflection_planes == 0) { m_chosen.clear(); return; }
        const vector3d eye = camera.position();
        const double reach = s.reflect_any_distance ? render_distance : vmin(s.reflection_plane_distance, render_distance);
        gather(world, eye, reach, s.water_planar_reflections);
        m_scored.clear();

        for (const auto& entry : m_merged) {
            const ReflectionSurface& r = entry.second;
            const int axis = face_axis(r.facing);
            const double front = (component(eye, axis) - r.coordinate()) * (face_positive(r.facing) ? 1.0 : -1.0);
            if (front <= FRONT_MARGIN) continue;
            const double dist = std::sqrt(distance_sq(eye, r.box));
            if (dist > reach) continue;
            const AABB padded = r.box.inflated(FLAT_PAD);
            if (dist > NEAR_ALWAYS && !camera.is_visible(padded.min, padded.max)) continue;
            double score = static_cast<double>(r.cells) / (1.0 + dist * dist);
            if (std::find(m_chosen.begin(), m_chosen.end(), entry.first) != m_chosen.end()) score *= KEEP_BONUS;
            m_scored.emplace_back(score, entry.first);
        }

        std::sort(m_scored.begin(), m_scored.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        m_chosen.clear();
        m_taken.assign(m_scored.size(), false);
        if (s.max_reflection_planes > 1) {
            pick(eye, s, false, true);
            pick(eye, s, true, true);
        }

        pick(eye, s, false, false);
    }

    double farthest() const noexcept { return m_farthest; }

    const std::vector<fizmo::graphics::ReflectionPlane3D>& planes() const noexcept { return m_planes; }

private:
    using Key = std::tuple<int, std::int64_t, bool>;
    static constexpr std::size_t MAX_IDLE_CACHE = 512;

    void pick(const vector3d& eye, const LightingSettings& s, bool water, bool one_of_kind) {
        for (std::size_t i = 0; i < m_scored.size(); ++i) {
            if (m_planes.size() >= s.max_reflection_planes) return;
            if (m_taken[i]) continue;
            const ReflectionSurface& r = m_merged.at(m_scored[i].second);
            if (one_of_kind && r.water != water) continue;
            m_taken[i] = true;
            vector3d point = r.box.center();
            set_component(point, face_axis(r.facing), r.coordinate());
            m_planes.emplace_back(point, r.normal(), r.box.min, r.box.max);
            m_planes.back().resolution = static_cast<float>(r.water ? s.reflection_resolution : s.mirror_resolution);
            m_chosen.push_back(m_scored[i].second);
            m_farthest = vmax(m_farthest, std::sqrt(distance_sq(eye, r.box)));
            if (one_of_kind) return;
        }
    }

    struct GatherKey {
        std::uint64_t  revision = 0;
        std::uintptr_t rules    = 0;
        ColumnPos      column{};
        int            chunk_z  = 0;
        double         reach    = -1.0;
        bool           water    = false;

        bool operator==(const GatherKey& o) const noexcept {
            return revision == o.revision && rules == o.rules && column.x == o.column.x && column.y == o.column.y && chunk_z == o.chunk_z && reach == o.reach && water == o.water;
        }
    };

    void gather(const World& world, const vector3d& eye, double reach, bool water) {
        const BlockPos at = BlockPos::containing(eye);
        const std::uintptr_t rules = reinterpret_cast<std::uintptr_t>(&world.fluid_rules());
        const GatherKey key{ world.revision(), rules, World::column_of(at), World::chunk_pos_of(at).z, reach, water };
        if (key == m_gathered) return;
        m_gathered = key;
        m_merged.clear();
        const double size = Chunk::SIZE;
        const double range = reach + size;

        for (const auto& kv : world.columns()) {
            const ColumnPos& c = kv.first;
            const double dx = vmax(vmax(c.x * size - eye.x, 0.0), eye.x - (c.x + 1) * size);
            const double dy = vmax(vmax(c.y * size - eye.y, 0.0), eye.y - (c.y + 1) * size);
            if (dx * dx + dy * dy > range * range) continue;

            for (int cz : kv.second.chunk_zs) {
                const ChunkPos p{ c.x, c.y, cz };
                const Chunk* chunk = world.chunk_at(p);
                if (!chunk) continue;
                const vector3d origin{ c.x * size, c.y * size, cz * size };
                if (distance_sq(eye, AABB(origin, origin + vector3d{ size, size, size })) > range * range) continue;
                ChunkSurfaces& cached = m_cache[p];
                if (!cached.scanned || cached.revision != chunk->revision() || cached.rules != rules) scan(world, *chunk, p, cached);
                cached.seen = key.revision;

                for (const ReflectionSurface& r : cached.surfaces) {
                    if (r.water && !water) continue;
                    auto k = std::make_tuple(static_cast<int>(r.facing), r.level, r.water);
                    auto it = m_merged.find(k);
                    if (it == m_merged.end()) m_merged.emplace(k, r);
                    else { it->second.box = it->second.box.united(r.box); it->second.cells += r.cells; it->second.heights += r.heights; }
                }
            }
        }

        if (m_cache.size() > MAX_IDLE_CACHE) prune(key.revision);
    }

    struct ChunkSurfaces {
        std::uint64_t                  revision = 0;
        std::uint64_t                  seen     = 0;
        std::uintptr_t                 rules    = 0;
        bool                           scanned  = false;
        std::vector<ReflectionSurface> surfaces;
    };

    static void scan(const World& world, const Chunk& chunk, const ChunkPos& p, ChunkSurfaces& out) {
        out.revision = chunk.revision();
        out.rules = reinterpret_cast<std::uintptr_t>(&world.fluid_rules());
        out.scanned = true;
        out.surfaces.clear();
        const BlockTraits* traits = world.blocks().traits_table();
        const int S = Chunk::SIZE;
        const BlockPos base{ p.x * S, p.y * S, p.z * S };
        std::map<Key, ReflectionSurface> found;

        for (int y = 0; y < S; ++y)
            for (int z = 0; z < S; ++z)
                for (int x = 0; x < S; ++x) {
                    const BlockTraits& t = traits[chunk.get(x, y, z)];
                    if (!t.visible) continue;
                    const BlockPos cell{ base.x + x, base.y + y, base.z + z };

                    if (t.finish == SurfaceFinish::Mirror) {
                        for (Face f : ALL_FACES) {
                            if (t.face(f).plain) continue;
                            const int axis = face_axis(f);
                            const double coord = component(cell.min_corner(), axis) + (face_positive(f) ? component(t.shape.max, axis) : component(t.shape.min, axis));
                            add(found, f, coord, false, t.shape.at(cell));
                        }
                    } else if (t.fluid) {
                        const bool open_above = z + 1 < S ? !traits[chunk.get(x, y, z + 1)].fluid : !world.traits_at({ cell.x, cell.y, cell.z + 1 }).fluid;
                        if (open_above && level_surface(world, cell)) add(found, Face::Up, world.fluid_surface(cell), true, AABB::unit_block(cell));
                    }
                }

        for (auto& entry : found) out.surfaces.push_back(entry.second);
    }

    static bool level_surface(const World& world, const BlockPos& cell) {
        static constexpr std::array<BlockPos, 4> SIDES{ BlockPos{ 1, 0, 0 }, BlockPos{ -1, 0, 0 }, BlockPos{ 0, 1, 0 }, BlockPos{ 0, -1, 0 } };
        static constexpr double TOLERANCE = ReflectionSurface::WATER_BUCKET * 0.5;
        const BlockId id = world.block_id_at(cell);
        const double h = world.fluid_height(cell);
        if (world.fluid_rules().falling(world.fluid_state(cell))) return false;

        for (const BlockPos& d : SIDES) {
            const BlockPos q{ cell.x + d.x, cell.y + d.y, cell.z };
            if (world.block_id_at(q) != id) continue;
            if (std::fabs(world.fluid_height(q) - h) > TOLERANCE) return false;
        }

        return true;
    }

    static void add(std::map<Key, ReflectionSurface>& found, Face f, double coord, bool water, const AABB& box) {
        const std::int64_t level = water ? static_cast<std::int64_t>(std::floor(coord / ReflectionSurface::WATER_BUCKET + 0.5)) : std::llround(coord * ReflectionSurface::LEVEL_SCALE);
        const Key key{ static_cast<int>(f), level, water };
        auto it = found.find(key);

        if (it == found.end()) {
            ReflectionSurface r;
            r.facing  = f;
            r.level   = level;
            r.water   = water;
            r.box     = box;
            r.cells   = 1;
            r.heights = coord;
            found.emplace(key, r);
            return;
        }

        it->second.box = it->second.box.united(box);
        ++it->second.cells;
        it->second.heights += coord;
    }

    void prune(std::uint64_t revision) {
        for (auto it = m_cache.begin(); it != m_cache.end();) it = it->second.seen == revision ? std::next(it) : m_cache.erase(it);
    }

    static double distance_sq(const vector3d& p, const AABB& b) noexcept {
        const double dx = vmax(vmax(b.min.x - p.x, 0.0), p.x - b.max.x);
        const double dy = vmax(vmax(b.min.y - p.y, 0.0), p.y - b.max.y);
        const double dz = vmax(vmax(b.min.z - p.z, 0.0), p.z - b.max.z);
        return dx * dx + dy * dy + dz * dz;
    }

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