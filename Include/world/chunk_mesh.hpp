#ifndef VOXELSPIRE_RENDER_CHUNK_MESH_HPP
#define VOXELSPIRE_RENDER_CHUNK_MESH_HPP

#include <array>
#include <unordered_map>
#include "../world/world.hpp"

namespace voxelspire {

struct FaceShading {
    double up = 1.0, down = 0.5, north_south = 0.8, east_west = 0.62;

    double factor(Face f) const noexcept {
        switch (f) {
            case Face::Up:    return up;
            case Face::Down:  return down;
            case Face::North:
            case Face::South: return north_south;
            default:          return east_west;
        }
    }

    static Color apply(const Color& c, double k) noexcept {
        auto ch = [k](std::uint8_t v) { return static_cast<std::uint8_t>(vclamp(v * k, 0.0, 255.0)); };
        return Color(ch(c.red()), ch(c.green()), ch(c.blue()), c.alpha());
    }
};

inline void face_corners(const BlockPos& p, Face f, std::array<vector3d, 4>& out) noexcept {
    const double x0 = p.x, y0 = p.y, z0 = p.z, x1 = x0 + 1, y1 = y0 + 1, z1 = z0 + 1;
    switch (f) {
        case Face::West:  out = { vector3d{x0,y1,z0}, vector3d{x0,y0,z0}, vector3d{x0,y0,z1}, vector3d{x0,y1,z1} }; break;
        case Face::East:  out = { vector3d{x1,y0,z0}, vector3d{x1,y1,z0}, vector3d{x1,y1,z1}, vector3d{x1,y0,z1} }; break;
        case Face::South: out = { vector3d{x0,y0,z0}, vector3d{x1,y0,z0}, vector3d{x1,y0,z1}, vector3d{x0,y0,z1} }; break;
        case Face::North: out = { vector3d{x1,y1,z0}, vector3d{x0,y1,z0}, vector3d{x0,y1,z1}, vector3d{x1,y1,z1} }; break;
        case Face::Down:  out = { vector3d{x0,y1,z0}, vector3d{x1,y1,z0}, vector3d{x1,y0,z0}, vector3d{x0,y0,z0} }; break;
        case Face::Up:    out = { vector3d{x0,y0,z1}, vector3d{x1,y0,z1}, vector3d{x1,y1,z1}, vector3d{x0,y1,z1} }; break;
    }
}

class ChunkMesh {
public:
    void build(const World& world, const Chunk& chunk, const FaceShading& shading) {
        m_mesh.clear();
        m_faces = 0;
        m_bounds = AABB(chunk.origin().min_corner(),
                        chunk.origin().min_corner() + vector3d{ double(Chunk::SIZE), double(Chunk::SIZE), double(Chunk::SIZE) });
        m_revision = chunk.revision();
        if (chunk.empty()) return;

        const BlockRegistry& reg = world.blocks();
        const BlockPos origin = chunk.origin();
        std::array<vector3d, 4> c;

        for (int ly = 0; ly < Chunk::SIZE; ++ly)
        for (int lz = 0; lz < Chunk::SIZE; ++lz)
        for (int lx = 0; lx < Chunk::SIZE; ++lx) {
            const Block& block = reg.get(chunk.get(lx, ly, lz));
            if (!block.is_visible()) continue;
            const BlockPos pos = origin + BlockPos{ lx, ly, lz };

            for (Face f : ALL_FACES) {
                const BlockPos o = face_offset(f);
                const int nx = lx + o.x, ny = ly + o.y, nz = lz + o.z;
                const BlockId nid = Chunk::in_bounds(nx, ny, nz) ? chunk.get(nx, ny, nz) : world.block_id_at(pos + o);
                if (!block.is_face_visible_against(reg.get(nid))) continue;

                const Color col = FaceShading::apply(block.face_color(f, pos), shading.factor(f));
                face_corners(pos, f, c);
                m_mesh.add_quad(fizmo::graphics::Vertex3D(c[0], col), fizmo::graphics::Vertex3D(c[1], col),
                                fizmo::graphics::Vertex3D(c[2], col), fizmo::graphics::Vertex3D(c[3], col));
                ++m_faces;
            }
        }
    }

    const fizmo::graphics::Mesh3D& mesh() const noexcept { return m_mesh; }
    std::size_t   face_count() const noexcept { return m_faces; }
    const AABB&   bounds()     const noexcept { return m_bounds; }
    std::uint64_t revision()   const noexcept { return m_revision; }

private:
    fizmo::graphics::Mesh3D m_mesh;
    std::size_t             m_faces = 0;
    AABB                    m_bounds;
    std::uint64_t           m_revision = 0;
};

class ChunkMeshCache {
public:
    std::size_t update(World& world) {
        std::size_t rebuilt = 0;

        for (auto& kv : world.chunks()) {
            Chunk& chunk = *kv.second;
            auto it = m_meshes.find(kv.first);
            if (it != m_meshes.end() && !chunk.mesh_dirty()) continue;
            m_meshes[kv.first].build(world, chunk, m_shading);
            chunk.clear_mesh_dirty();
            ++rebuilt;
        }

        for (auto it = m_meshes.begin(); it != m_meshes.end();) {
            if (!world.chunk_at(it->first)) it = m_meshes.erase(it); else ++it;
        }

        m_total_faces = 0;
        for (const auto& kv : m_meshes) m_total_faces += kv.second.face_count();
        return rebuilt;
    }

    const std::unordered_map<ChunkPos, ChunkMesh, ChunkPosHash>& meshes() const noexcept { return m_meshes; }
    std::size_t total_faces() const noexcept { return m_total_faces; }
    FaceShading& shading() noexcept { return m_shading; }

private:
    std::unordered_map<ChunkPos, ChunkMesh, ChunkPosHash> m_meshes;
    std::size_t m_total_faces = 0;
    FaceShading m_shading;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_CHUNK_MESH_HPP