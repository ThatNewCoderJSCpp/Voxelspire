#ifndef VOXELSPIRE_RENDER_WORLD_RENDERER_HPP
#define VOXELSPIRE_RENDER_WORLD_RENDERER_HPP

#include "../world/chunk_mesh.hpp"

namespace voxelspire {

struct WorldRenderStats {
    std::size_t chunks_total   = 0;
    std::size_t chunks_visible = 0;
    std::size_t faces_total    = 0;
    std::size_t faces_drawn    = 0;
    std::size_t quads_total    = 0;
    std::size_t quads_drawn    = 0;
    double      render_distance = 0.0;
};

class WorldRenderer {
public:
    WorldRenderStats render(
        fizmo::windows::Renderer& renderer, const ChunkMeshCache& meshes,
        const fizmo::graphics::Camera3D& camera, double render_distance
    ) const {
        WorldRenderStats stats;
        stats.chunks_total    = meshes.meshes().size();
        stats.faces_total     = meshes.total_faces();
        stats.quads_total     = meshes.total_quads();
        stats.render_distance = render_distance;
        const double max_sq = render_distance * render_distance;
        const vector3d eye = camera.position();

        for (const auto& kv : meshes.meshes()) {
            const ChunkMesh& chunk = kv.second;
            if (chunk.face_count() == 0) continue;
            if (distance_sq(eye, chunk.bounds()) > max_sq) continue;
            if (!camera.is_visible(chunk.bounds().min, chunk.bounds().max)) continue;
            renderer.draw_mesh_at(chunk.mesh(), chunk.origin());
            ++stats.chunks_visible;
            stats.faces_drawn += chunk.face_count();
            stats.quads_drawn += chunk.quad_count();
        }

        return stats;
    }

private:
    static double distance_sq(const vector3d& p, const AABB& b) noexcept {
        const double dx = vmax(vmax(b.min.x - p.x, 0.0), p.x - b.max.x);
        const double dy = vmax(vmax(b.min.y - p.y, 0.0), p.y - b.max.y);
        const double dz = vmax(vmax(b.min.z - p.z, 0.0), p.z - b.max.z);
        return dx * dx + dy * dy + dz * dz;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_WORLD_RENDERER_HPP