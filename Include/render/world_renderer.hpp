#ifndef VOXELSPIRE_RENDER_WORLD_RENDERER_HPP
#define VOXELSPIRE_RENDER_WORLD_RENDERER_HPP

#include <algorithm>
#include <array>
#include <cmath>
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
    std::size_t  shadow_buried       = 0;
    std::size_t  chunks_reflected    = 0;
    double       render_distance     = 0.0;
    TerrainStats terrain;
};

struct CasterSphere {
    vector3d center;
    double   radius = 0.0;
};

struct WorldRenderOptions {
    double                                          render_distance     = 0.0;
    bool                                            cave_culling        = true;
    bool                                            face_culling        = true;
    double                                          shadow_distance     = 0.0;
    bool                                            sun_casters         = true;
    std::vector<CasterSphere>                       point_casters;
    std::vector<fizmo::graphics::ReflectionPlane3D> reflections;
    double                                          reflection_distance = 0.0;
};

struct SortedMesh {
    double                               distance_sq;
    const fizmo::graphics::MeshHandle3D* mesh;
    vector3d                             offset;
    fizmo::graphics::FaceMask            mask = fizmo::graphics::ALL_FACE_GROUPS;
};

class WorldRenderer {
public:
    WorldRenderStats render(
        fizmo::windows::Renderer& renderer, TerrainMeshes& terrain,
        const fizmo::graphics::Camera3D& camera, const WorldRenderOptions& options
    );

private:
    static constexpr std::size_t PORTAL_EDGES = 4;

    struct MirrorView {
        fizmo::graphics::Camera3D            camera;
        vector3d                             eye;
        vector3d                             normal;
        double                               offset = 0.0;
        bool                                 portal = false;
        std::array<vector3d, PORTAL_EDGES>   sides{};
    };

    static void build_portal(MirrorView& v, const fizmo::graphics::ReflectionPlane3D& plane);

    static bool through_portal(const MirrorView& v, const vector3d& lo, const vector3d& hi) noexcept;

    static vector3d reflect_point(const vector3d& p, const vector3d& n, double offset) noexcept { return p - n * (2.0 * (n.dot(p) - offset)); }
    static vector3d reflect_direction(const vector3d& d, const vector3d& n) noexcept { return d - n * (2.0 * n.dot(d)); }

    void build_mirror_views(const fizmo::graphics::Camera3D& camera, const WorldRenderOptions& options);

    bool mirrored(const vector3d& lo, const vector3d& hi) const;

    void add_casters(const TerrainColumn& column, const ColumnPos& c, const vector3d& eye, double reach_sq, const WorldRenderOptions& options, WorldRenderStats& stats);

    static bool near_point_light(const AABB& box, const std::vector<CasterSphere>& lights) noexcept;

    static void count_groups(const fizmo::graphics::MeshHandle3D& mesh, fizmo::graphics::FaceMask mask, std::size_t& drawn, std::size_t& skipped) noexcept;

    static double distance_sq(const vector3d& p, const AABB& b) noexcept;

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