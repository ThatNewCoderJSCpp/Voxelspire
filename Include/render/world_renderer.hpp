#ifndef VOXELSPIRE_RENDER_WORLD_RENDERER_HPP
#define VOXELSPIRE_RENDER_WORLD_RENDERER_HPP

#include "../world/chunk_mesh.hpp"

namespace voxelspire {

struct WorldRenderStats {
    std::size_t chunks_total   = 0;
    std::size_t chunks_visible = 0;
    std::size_t faces_total    = 0;
    std::size_t faces_drawn    = 0;
};

class WorldRenderer {
public:
    WorldRenderStats render(fizmo::windows::Renderer& renderer, const ChunkMeshCache& meshes, const fizmo::graphics::Camera3D& camera) const {
        WorldRenderStats stats;
        stats.chunks_total = meshes.meshes().size();
        stats.faces_total  = meshes.total_faces();

        for (const auto& kv : meshes.meshes()) {
            const ChunkMesh& chunk = kv.second;
            if (chunk.face_count() == 0) continue;
            if (!camera.is_visible(chunk.bounds().min, chunk.bounds().max)) continue;
            renderer.draw_mesh(chunk.mesh());
            ++stats.chunks_visible;
            stats.faces_drawn += chunk.face_count();
        }

        return stats;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_WORLD_RENDERER_HPP