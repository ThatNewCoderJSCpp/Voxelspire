#ifndef VOXELSPIRE_RENDER_WORLD_RENDERER_HPP
#define VOXELSPIRE_RENDER_WORLD_RENDERER_HPP

#include <algorithm>
#include <vector>
#include "terrain_meshes.hpp"

namespace voxelspire {

struct WorldRenderStats {
    std::size_t  chunks_total        = 0;
    std::size_t  chunks_visible      = 0;
    std::size_t  chunks_cave_culled  = 0;
    std::size_t  translucent_visible = 0;
    std::size_t  columns_visited     = 0;
    std::size_t  faces_total         = 0;
    std::size_t  quads_total         = 0;
    std::size_t  quads_drawn         = 0;
    std::size_t  quads_facing_away   = 0;
    std::size_t  lod_tiles_total     = 0;
    std::size_t  lod_tiles_drawn     = 0;
    std::size_t  lod_quads_drawn     = 0;
    std::size_t  batch_items         = 0;
    std::size_t  batches             = 0;
    std::size_t  shadow_casters      = 0;
    std::size_t  chunks_reflected    = 0;
    double       render_distance     = 0.0;
    TerrainStats terrain;
};

struct WorldRenderOptions {
    double render_distance = 0.0;
    bool   cave_culling    = true;
    bool   face_culling    = true;
    double shadow_distance = 0.0;
    std::vector<fizmo::graphics::ReflectionPlane3D> reflections;
    double reflection_distance = 0.0;
};

class WorldRenderer {
public:
    WorldRenderStats render(
        fizmo::windows::Renderer& renderer, TerrainMeshes& terrain,
        const fizmo::graphics::Camera3D& camera, const WorldRenderOptions& options
    ) {
        using fizmo::graphics::FaceMask;
        WorldRenderStats stats;
        stats.render_distance = options.render_distance;
        stats.terrain = terrain.stats();
        stats.chunks_total = stats.terrain.chunk_meshes;
        stats.faces_total  = stats.terrain.chunk_faces;
        stats.quads_total  = stats.terrain.chunk_quads + stats.terrain.translucent_quads;
        const vector3d eye = camera.position();
        const double max_sq = options.render_distance * options.render_distance;
        const CaveCulling& cave = terrain.cave_culling(World::chunk_pos_of(BlockPos::containing(eye)), options.cave_culling);
        m_opaque.clear();
        m_see_through.clear();
        m_casters.clear();
        m_lost_chunks.clear();
        const double shadow_sq = options.shadow_distance * options.shadow_distance;
        m_lost_tiles.clear();
        m_reflected.clear();
        const double size = Chunk::SIZE;
        build_mirror_views(camera, options);
        const double mirror_sq = options.reflection_distance * options.reflection_distance;

        terrain.for_each_drawn_column([&](const ColumnPos& c, const TerrainColumn& column) {
            ++stats.columns_visited;
            const vector3d col_lo{ c.x * size, c.y * size, column.min_z * size };
            const vector3d col_hi{ col_lo.x + size, col_lo.y + size, (column.max_z + 1) * size };
            const double col_sq = distance_sq(eye, AABB(col_lo, col_hi));
            if (options.shadow_distance > 0.0 && col_sq <= shadow_sq) add_casters(column, c, eye, shadow_sq);
            const bool col_main = col_sq <= max_sq && camera.is_visible(col_lo, col_hi);
            const bool col_mirror = col_sq <= mirror_sq && mirrored(col_lo, col_hi);
            if (!col_main && !col_mirror) return;

            for (const auto& slot : column.chunks) {
                const ChunkPos p{ c.x, c.y, slot.first };
                const ChunkRenderEntry& e = *slot.second;
                if (!e.meshed) continue;
                if (e.opaque.lost() || e.translucent.lost()) { m_lost_chunks.push_back(p); continue; }
                if (!e.opaque.valid() && !e.translucent.valid()) continue;
                const vector3d lo{ p.x * size, p.y * size, p.z * size };
                const AABB box(lo, lo + vector3d{ size, size, size });
                const double box_sq = distance_sq(eye, box);
                const bool in_mirror = col_mirror && box_sq <= mirror_sq && mirrored(box.min, box.max);
                bool on_camera = box_sq <= max_sq && camera.is_visible(box.min, box.max);
                if (on_camera && !cave.visible(p)) { ++stats.chunks_cave_culled; on_camera = false; }
                if (!on_camera && !in_mirror) continue;
                if (on_camera) ++stats.chunks_visible;

                if (e.opaque.valid()) {
                    FaceMask mask = options.face_culling ? fizmo::graphics::facing_faces(eye, box.min, box.max) : fizmo::graphics::ALL_FACE_GROUPS;
                    if (in_mirror && options.face_culling) for (const MirrorView& v : m_views) mask |= fizmo::graphics::facing_faces(v.eye, box.min, box.max);

                    if (on_camera) {
                        count_groups(e.opaque, mask, stats.quads_drawn, stats.quads_facing_away);
                        m_front.push_back({ box_sq, &e.opaque, lo, mask });
                    } else {
                        m_reflected.add(e.opaque, lo, mask);
                        ++stats.chunks_reflected;
                    }
                }

                if (on_camera && e.translucent.valid()) {
                    const vector3d mid = box.center() - eye;
                    m_sorted.push_back({ mid.x * mid.x + mid.y * mid.y + mid.z * mid.z, &e.translucent, lo });
                    stats.quads_drawn += e.translucent_quads;
                }
            }
        });

        std::sort(m_front.begin(), m_front.end(), [](const SortedMesh& a, const SortedMesh& b) { return a.distance_sq < b.distance_sq; });
        for (const SortedMesh& s : m_front) m_opaque.add(*s.mesh, s.offset, s.mask);
        m_front.clear();
        const LodLayout& layout = terrain.layout();

        terrain.for_each_drawn_tile([&](const LodTileKey& k, const LodTileEntry& t) {
            ++stats.lod_tiles_total;
            if (t.handle.lost()) { m_lost_tiles.push_back(k); return; }
            if (!t.handle.valid()) return;
            const BlockPos o = layout.origin(k);
            const double tb = layout.tile_blocks(k.level);
            const AABB box({ double(o.x), double(o.y), double(t.min_z) }, { o.x + tb, o.y + tb, double(t.max_z) });
            if (distance_sq(eye, box) > max_sq) return;
            if (!camera.is_visible(box.min, box.max)) return;
            const FaceMask mask = options.face_culling ? fizmo::graphics::facing_faces(eye, box.min, box.max) : fizmo::graphics::ALL_FACE_GROUPS;
            std::size_t away = 0;
            count_groups(t.handle, mask, stats.lod_quads_drawn, away);
            stats.quads_facing_away += away;
            m_opaque.add(t.handle, { double(o.x), double(o.y), 0.0 }, mask);
            ++stats.lod_tiles_drawn;
        });

        std::sort(m_sorted.begin(), m_sorted.end(), [](const SortedMesh& a, const SortedMesh& b) { return a.distance_sq > b.distance_sq; });
        for (const SortedMesh& s : m_sorted) m_see_through.add(*s.mesh, s.offset);
        stats.translucent_visible = m_sorted.size();
        m_sorted.clear();
        using fizmo::graphics::Material3D;
        using fizmo::graphics::Shadow3D;
        const bool casting = !m_casters.empty();
        if (casting)                { renderer.draw_quad_batch(m_casters, Material3D::shadow_caster()); ++stats.batches; }
        if (!m_opaque.empty())      { renderer.draw_quad_batch(m_opaque, Material3D().with_shadow(casting ? Shadow3D::None : Shadow3D::Cast)); ++stats.batches; }
        if (!m_see_through.empty()) { renderer.draw_quad_batch(m_see_through, fizmo::graphics::Material3D::transparent()); ++stats.batches; }
        if (!m_reflected.empty())   { renderer.draw_quad_batch(m_reflected, Material3D().with_shadow(Shadow3D::None).with_view(fizmo::graphics::View3D::ReflectionsOnly)); ++stats.batches; }
        stats.batch_items = m_opaque.size() + m_see_through.size() + m_casters.size() + m_reflected.size();
        stats.shadow_casters = m_casters.size();
        for (const ChunkPos& p : m_lost_chunks) terrain.mark_lost(p);
        for (const LodTileKey& k : m_lost_tiles) terrain.mark_lost(k);
        return stats;
    }

private:
    struct MirrorView {
        fizmo::graphics::Camera3D camera;
        vector3d                  eye;
        vector3d                  normal;
        double                    offset = 0.0;
    };

    static vector3d reflect_point(const vector3d& p, const vector3d& n, double offset) noexcept { return p - n * (2.0 * (n.dot(p) - offset)); }
    static vector3d reflect_direction(const vector3d& d, const vector3d& n) noexcept { return d - n * (2.0 * n.dot(d)); }

    void build_mirror_views(const fizmo::graphics::Camera3D& camera, const WorldRenderOptions& options) {
        m_views.clear();
        if (options.reflection_distance <= 0.0) return;

        for (const auto& plane : options.reflections) {
            const double len = plane.normal.magnitude();
            if (len <= 0.0) continue;
            MirrorView v;
            v.normal = plane.normal / len;
            v.offset = v.normal.dot(plane.point);
            v.camera = camera;
            v.eye = reflect_point(camera.position(), v.normal, v.offset);
            v.camera.set_position(v.eye);
            v.camera.look_in(reflect_direction(camera.forward(), v.normal), reflect_direction(camera.up(), v.normal));
            m_views.push_back(v);
        }
    }

    bool mirrored(const vector3d& lo, const vector3d& hi) const {
        const vector3d center = (lo + hi) * 0.5, half = (hi - lo) * 0.5;

        for (const MirrorView& v : m_views) {
            const double reach = std::fabs(v.normal.x) * half.x + std::fabs(v.normal.y) * half.y + std::fabs(v.normal.z) * half.z;
            if (v.normal.dot(center) + reach <= v.offset) continue;
            if (v.camera.is_visible(lo, hi)) return true;
        }

        return false;
    }

    void add_casters(const TerrainColumn& column, const ColumnPos& c, const vector3d& eye, double reach_sq) {
        const double size = Chunk::SIZE;

        for (const auto& slot : column.chunks) {
            const ChunkRenderEntry& e = *slot.second;
            if (!e.meshed || !e.opaque.valid()) continue;
            const vector3d lo{ c.x * size, c.y * size, slot.first * size };
            if (distance_sq(eye, AABB(lo, lo + vector3d{ size, size, size })) > reach_sq) continue;
            m_casters.add(e.opaque, lo);
        }
    }

    struct SortedMesh {
        double                               distance_sq;
        const fizmo::graphics::MeshHandle3D* mesh;
        vector3d                             offset;
        fizmo::graphics::FaceMask            mask = fizmo::graphics::ALL_FACE_GROUPS;
    };

    static void count_groups(const fizmo::graphics::MeshHandle3D& mesh, fizmo::graphics::FaceMask mask, std::size_t& drawn, std::size_t& skipped) noexcept {
        const auto& groups = mesh.groups();

        for (std::size_t g = 0; g < groups.size(); ++g) {
            if (mask & (1u << g)) drawn += groups[g].count;
            else skipped += groups[g].count;
        }
    }

    static double distance_sq(const vector3d& p, const AABB& b) noexcept {
        const double dx = vmax(vmax(b.min.x - p.x, 0.0), p.x - b.max.x);
        const double dy = vmax(vmax(b.min.y - p.y, 0.0), p.y - b.max.y);
        const double dz = vmax(vmax(b.min.z - p.z, 0.0), p.z - b.max.z);
        return dx * dx + dy * dy + dz * dz;
    }

    fizmo::graphics::QuadBatch3D m_opaque;
    fizmo::graphics::QuadBatch3D m_see_through;
    fizmo::graphics::QuadBatch3D m_casters;
    fizmo::graphics::QuadBatch3D m_reflected;
    std::vector<MirrorView>      m_views;
    std::vector<SortedMesh>      m_sorted;
    std::vector<SortedMesh>      m_front;
    std::vector<ChunkPos>        m_lost_chunks;
    std::vector<LodTileKey>      m_lost_tiles;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_WORLD_RENDERER_HPP