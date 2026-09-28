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

inline void face_corners(const vector3d& lo, const vector3d& hi, Face f, std::array<vector3d, 4>& out) noexcept {
    const double x0 = lo.x, y0 = lo.y, z0 = lo.z, x1 = hi.x, y1 = hi.y, z1 = hi.z;
    switch (f) {
        case Face::West:  out = { vector3d{x0,y1,z0}, vector3d{x0,y0,z0}, vector3d{x0,y0,z1}, vector3d{x0,y1,z1} }; break;
        case Face::East:  out = { vector3d{x1,y0,z0}, vector3d{x1,y1,z0}, vector3d{x1,y1,z1}, vector3d{x1,y0,z1} }; break;
        case Face::South: out = { vector3d{x0,y0,z0}, vector3d{x1,y0,z0}, vector3d{x1,y0,z1}, vector3d{x0,y0,z1} }; break;
        case Face::North: out = { vector3d{x1,y1,z0}, vector3d{x0,y1,z0}, vector3d{x0,y1,z1}, vector3d{x1,y1,z1} }; break;
        case Face::Down:  out = { vector3d{x0,y1,z0}, vector3d{x1,y1,z0}, vector3d{x1,y0,z0}, vector3d{x0,y0,z0} }; break;
        case Face::Up:    out = { vector3d{x0,y0,z1}, vector3d{x1,y0,z1}, vector3d{x1,y1,z1}, vector3d{x0,y1,z1} }; break;
    }
}

inline void face_corners(const BlockPos& p, Face f, std::array<vector3d, 4>& out) noexcept {
    face_corners(p.min_corner(), p.min_corner() + vector3d{ 1.0, 1.0, 1.0 }, f, out);
}

struct ChunkMeshOptions {
    bool merge_faces = true;
};

class ChunkMesh {
public:
    void build(const World& world, const Chunk& chunk, const FaceShading& shading, const ChunkMeshOptions& options = ChunkMeshOptions{}) {
        m_mesh.clear();
        m_faces = 0;
        m_quads = 0;
        m_bounds = AABB(chunk.origin().min_corner(),
                        chunk.origin().min_corner() + vector3d{ double(Chunk::SIZE), double(Chunk::SIZE), double(Chunk::SIZE) });
        m_origin = chunk.origin().min_corner();
        m_revision = chunk.revision();

        if (!chunk.empty()) {
            for (Face f : ALL_FACES) build_direction(world, chunk, shading, options, f);
        }

        m_mesh.edit_vertices().shrink_to_fit();
        m_mesh.edit_indices().shrink_to_fit();
    }

    const fizmo::graphics::Mesh3D& mesh() const noexcept { return m_mesh; }
    std::size_t   face_count() const noexcept { return m_faces; }
    std::size_t   quad_count() const noexcept { return m_quads; }
    const AABB&   bounds()     const noexcept { return m_bounds; }
    const vector3d& origin()   const noexcept { return m_origin; }
    std::uint64_t revision()   const noexcept { return m_revision; }

private:
    static constexpr int           AREA    = Chunk::SIZE * Chunk::SIZE;
    static constexpr std::uint64_t NO_FACE = 0;
    static constexpr std::uint64_t HAS_FACE = std::uint64_t(1) << 32;

    static BlockPos local(int a, int d, int u, int i, int v, int j) noexcept {
        int p[3];
        p[a] = d; p[u] = i; p[v] = j;
        return { p[0], p[1], p[2] };
    }

    static std::uint64_t face_key(const World& world, const Chunk& chunk, const FaceShading& shading, Face f, const BlockPos& l) {
        const BlockRegistry& reg = world.blocks();
        const Block& block = reg.get(chunk.get(l.x, l.y, l.z));
        if (!block.is_visible()) return NO_FACE;
        const BlockPos o = face_offset(f);
        const BlockPos n = l + o;
        const BlockPos pos = chunk.origin() + l;
        const BlockId nid = Chunk::in_bounds(n.x, n.y, n.z) ? chunk.get(n.x, n.y, n.z) : world.block_id_at(pos + o);
        if (!block.is_face_visible_against(reg.get(nid))) return NO_FACE;
        const Color col = FaceShading::apply(block.face_color(f, pos), shading.factor(f));
        return HAS_FACE | fizmo::graphics::Vertex3D::pack(col);
    }

    static Color unpack(std::uint64_t key) noexcept {
        const auto c = static_cast<std::uint32_t>(key);
        return Color(static_cast<std::uint8_t>(c), static_cast<std::uint8_t>(c >> 8),
                     static_cast<std::uint8_t>(c >> 16), static_cast<std::uint8_t>(c >> 24));
    }

    void build_direction(const World& world, const Chunk& chunk, const FaceShading& shading, const ChunkMeshOptions& options, Face f) {
        constexpr int S = Chunk::SIZE;
        const int a = face_axis(f), u = (a + 1) % 3, v = (a + 2) % 3;
        std::array<std::uint64_t, AREA> mask;

        for (int d = 0; d < S; ++d) {
            for (int j = 0; j < S; ++j)
                for (int i = 0; i < S; ++i)
                    mask[j * S + i] = face_key(world, chunk, shading, f, local(a, d, u, i, v, j));

            for (int j = 0; j < S; ++j) {
                for (int i = 0; i < S;) {
                    const std::uint64_t key = mask[j * S + i];
                    if (key == NO_FACE) { ++i; continue; }

                    int w = 1, h = 1;

                    if (options.merge_faces) {
                        while (i + w < S && mask[j * S + i + w] == key) ++w;

                        for (bool grow = true; grow && j + h < S;) {
                            for (int k = 0; k < w; ++k) if (mask[(j + h) * S + i + k] != key) { grow = false; break; }
                            if (grow) ++h;
                        }
                    }

                    for (int y = 0; y < h; ++y)
                        for (int x = 0; x < w; ++x)
                            mask[(j + y) * S + i + x] = NO_FACE;

                    emit(f, local(a, d, u, i, v, j), a, u, w, v, h, unpack(key));
                    m_faces += static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
                    i += w;
                }
            }
        }
    }

    void emit(Face f, const BlockPos& start, int a, int u, int w, int v, int h, const Color& col) {
        double size[3];
        size[a] = 1.0; size[u] = w; size[v] = h;
        const vector3d lo = start.min_corner();
        std::array<vector3d, 4> c;
        face_corners(lo, lo + vector3d{ size[0], size[1], size[2] }, f, c);
        m_mesh.add_quad(fizmo::graphics::Vertex3D(c[0], col), fizmo::graphics::Vertex3D(c[1], col),
                        fizmo::graphics::Vertex3D(c[2], col), fizmo::graphics::Vertex3D(c[3], col));
        ++m_quads;
    }

    fizmo::graphics::Mesh3D m_mesh;
    std::size_t             m_faces = 0;
    std::size_t             m_quads = 0;
    AABB                    m_bounds;
    vector3d                m_origin{};
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
            m_meshes[kv.first].build(world, chunk, m_shading, m_options);
            chunk.clear_mesh_dirty();
            ++rebuilt;
        }

        for (auto it = m_meshes.begin(); it != m_meshes.end();) {
            if (!world.chunk_at(it->first)) it = m_meshes.erase(it); else ++it;
        }

        m_total_faces = 0;
        m_total_quads = 0;
        for (const auto& kv : m_meshes) { m_total_faces += kv.second.face_count(); m_total_quads += kv.second.quad_count(); }
        return rebuilt;
    }

    const std::unordered_map<ChunkPos, ChunkMesh, ChunkPosHash>& meshes() const noexcept { return m_meshes; }
    std::size_t total_faces() const noexcept { return m_total_faces; }
    std::size_t total_quads() const noexcept { return m_total_quads; }
    FaceShading& shading() noexcept { return m_shading; }
    ChunkMeshOptions& options() noexcept { return m_options; }

private:
    std::unordered_map<ChunkPos, ChunkMesh, ChunkPosHash> m_meshes;
    std::size_t m_total_faces = 0;
    std::size_t m_total_quads = 0;
    FaceShading      m_shading;
    ChunkMeshOptions m_options;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_CHUNK_MESH_HPP