#ifndef VOXELSPIRE_RENDER_GREEDY_MESHER_HPP
#define VOXELSPIRE_RENDER_GREEDY_MESHER_HPP

#include <array>
#include <cstdint>
#include <vector>
#include "../core/settings.hpp"
#include <cmath>

namespace voxelspire {

struct FaceShading {
    FaceShadingSettings factors;
    double              strength = 1.0;

    double factor(Face f) const noexcept {
        double raw = factors.east_west;

        switch (f) {
            case Face::Up:    raw = factors.up; break;
            case Face::Down:  raw = factors.down; break;
            case Face::North:
            case Face::South: raw = factors.north_south; break;
            default:          break;
        }

        return 1.0 + (raw - 1.0) * strength;
    }

    std::uint8_t level(Face f) const noexcept {
        return static_cast<std::uint8_t>(vclamp(std::lround(factor(f) * 255.0), 0L, 255L));
    }
};

struct FaceKey {
    static constexpr std::uint64_t NONE          = 0;
    static constexpr std::uint64_t HAS_FACE      = std::uint64_t(1) << 63;
    static constexpr std::uint64_t TRANSLUCENT   = std::uint64_t(1) << 62;
    static constexpr int           VAR_SHIFT     = 32;
    static constexpr int           SHADE_SHIFT   = 40;
    static constexpr int           FLAGS_SHIFT   = 48;
    static constexpr int           SURFACE_SHIFT = 56;
    static constexpr std::uint64_t SURFACE_MASK  = 0x3F;
    static constexpr int           SURFACE_STEPS = 63;
    static constexpr int           MAX_VARIATION = 255;
    static constexpr std::uint64_t BYTE_MASK     = 0xFF;
    static constexpr std::uint64_t COLOR_MASK    = 0xFFFFFFFF;

    static std::uint64_t make(const Color& base, int variation, std::uint8_t shade, bool translucent = false, std::uint8_t vertex_flags = 0) noexcept {
        return HAS_FACE | (translucent ? TRANSLUCENT : 0) | fizmo::graphics::Vertex3D::pack(base)
             | (static_cast<std::uint64_t>(vclamp(variation, 0, MAX_VARIATION)) << VAR_SHIFT)
             | (static_cast<std::uint64_t>(shade) << SHADE_SHIFT)
             | (static_cast<std::uint64_t>(vertex_flags) << FLAGS_SHIFT);
    }

    static std::uint8_t vertex_flags(std::uint64_t key) noexcept { return static_cast<std::uint8_t>(key >> FLAGS_SHIFT); }

    static std::uint64_t with_color(std::uint64_t key, const Color& c) noexcept {
        return (key & ~COLOR_MASK) | fizmo::graphics::Vertex3D::pack(c);
    }

    static std::uint8_t finish_flags(SurfaceFinish finish) noexcept {
        switch (finish) {
            case SurfaceFinish::Glossy: return fizmo::graphics::CompactGlossy;
            case SurfaceFinish::Liquid: return fizmo::graphics::CompactLiquid;
            case SurfaceFinish::Mirror: return fizmo::graphics::CompactMirror;
            case SurfaceFinish::Matte:  break;
        }

        return 0;
    }

    static Color color(std::uint64_t key) noexcept {
        const auto c = static_cast<std::uint32_t>(key);

        return Color(
            static_cast<std::uint8_t>(c), 
            static_cast<std::uint8_t>(c >> 8),
            static_cast<std::uint8_t>(c >> 16), 
            static_cast<std::uint8_t>(c >> 24)
        );
    }

    static std::uint8_t variation(std::uint64_t key) noexcept { return static_cast<std::uint8_t>(key >> VAR_SHIFT); }
    static std::uint8_t shade(std::uint64_t key)     noexcept { return static_cast<std::uint8_t>(key >> SHADE_SHIFT); }
    static bool translucent(std::uint64_t key)       noexcept { return (key & TRANSLUCENT) != 0; }

    static std::uint64_t with_surface(std::uint64_t key, double height, std::uint8_t flow) noexcept {
        const auto steps = static_cast<std::uint64_t>(vclamp(std::lround((1.0 - height) * SURFACE_STEPS), 0L, static_cast<long>(SURFACE_STEPS)));
        key = (key & ~(SURFACE_MASK << SURFACE_SHIFT)) | (steps << SURFACE_SHIFT);
        if (flow == 0) return key;
        key = (key & ~(BYTE_MASK << VAR_SHIFT)) | (static_cast<std::uint64_t>(flow) << VAR_SHIFT);
        return key | (static_cast<std::uint64_t>(fizmo::graphics::CompactFlow) << FLAGS_SHIFT);
    }

    static double surface_drop(std::uint64_t key) noexcept {
        return static_cast<double>((key >> SURFACE_SHIFT) & SURFACE_MASK) / SURFACE_STEPS;
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

struct FaceCell {
    static constexpr int CORNERS = 4;
    static constexpr std::uint32_t OPEN_SKY = fizmo::graphics::LIGHT_FULL_SKY;

    std::uint64_t key = FaceKey::NONE;
    std::array<std::uint32_t, CORNERS> light{ OPEN_SKY, OPEN_SKY, OPEN_SKY, OPEN_SKY };
    std::array<std::uint8_t, CORNERS>  shade{};

    static FaceCell of(std::uint64_t key) noexcept {
        FaceCell c;
        c.key = key;
        c.shade.fill(FaceKey::shade(key));
        return c;
    }

    static const FaceCell& of(const FaceCell& c) noexcept { return c; }

    bool empty() const noexcept { return key == FaceKey::NONE; }
    bool operator==(const FaceCell& o) const noexcept { return key == o.key && light == o.light && shade == o.shade; }
    bool operator!=(const FaceCell& o) const noexcept { return !(*this == o); }
};

struct GreedyRect {
    Face          face;
    int           start[3];
    int           size[3];
    std::uint64_t key;
    FaceCell      cell;
};

class GreedyMesher {
public:
    template <typename KeyFn, typename EmitFn>
    std::size_t mesh(const int dims[3], bool merge, KeyFn&& key_at, EmitFn&& emit) {
        std::size_t faces = 0;
        for (Face f : ALL_FACES) faces += mesh_direction(dims, f, merge, key_at, emit);
        return faces;
    }

    template <typename KeyFn, typename EmitFn>
    std::size_t mesh_direction(const int dims[3], Face f, bool merge, KeyFn&& key_at, EmitFn&& emit) {
        const int a = face_axis(f), u = (a + 1) % 3, v = (a + 2) % 3;
        const int nu = dims[u], nv = dims[v];
        m_mask.resize(static_cast<std::size_t>(nu) * static_cast<std::size_t>(nv));
        std::size_t faces = 0;

        for (int d = 0; d < dims[a]; ++d) {
            bool any = false;

            for (int j = 0; j < nv; ++j)
                for (int i = 0; i < nu; ++i) {
                    int p[3];
                    p[a] = d; p[u] = i; p[v] = j;
                    FaceCell& cell = m_mask[static_cast<std::size_t>(j) * nu + i];
                    cell = FaceCell::of(key_at(f, p[0], p[1], p[2]));
                    any |= !cell.empty();
                }

            if (!any) continue;

            for (int j = 0; j < nv; ++j) {
                for (int i = 0; i < nu;) {
                    const FaceCell cell = m_mask[static_cast<std::size_t>(j) * nu + i];
                    if (cell.empty()) { ++i; continue; }
                    int w = 1, h = 1;

                    if (merge) {
                        while (i + w < nu && m_mask[static_cast<std::size_t>(j) * nu + i + w] == cell) ++w;

                        for (bool grow = true; grow && j + h < nv;) {
                            for (int k = 0; k < w; ++k)
                                if (m_mask[static_cast<std::size_t>(j + h) * nu + i + k] != cell) { grow = false; break; }
                            if (grow) ++h;
                        }
                    }

                    for (int y = 0; y < h; ++y)
                        for (int x = 0; x < w; ++x) m_mask[static_cast<std::size_t>(j + y) * nu + i + x].key = FaceKey::NONE;

                    GreedyRect r;
                    r.face = f;
                    r.start[a] = d; r.start[u] = i; r.start[v] = j;
                    r.size[a] = 1;  r.size[u] = w;  r.size[v] = h;
                    r.key  = cell.key;
                    r.cell = cell;
                    emit(r);
                    faces += static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
                    i += w;
                }
            }
        }

        return faces;
    }

private:
    std::vector<FaceCell> m_mask;
};

inline void emit_scaled_quad(fizmo::graphics::QuadMesh3D& out, const GreedyRect& r, const vector3d& origin, const vector3d& cell) {
    const vector3d lo{ origin.x + r.start[0] * cell.x, origin.y + r.start[1] * cell.y, origin.z + r.start[2] * cell.z };
    const vector3d hi{ lo.x + r.size[0] * cell.x, lo.y + r.size[1] * cell.y, lo.z + r.size[2] * cell.z };
    std::array<vector3d, 4> c;
    face_corners(lo, hi, r.face, c);
    const Color col = FaceKey::color(r.key);
    const auto cf = static_cast<fizmo::graphics::CellFace>(r.face);
    const std::uint8_t var = FaceKey::variation(r.key), flags = FaceKey::vertex_flags(r.key);
    using V = fizmo::graphics::CompactVertex3D;
    using fizmo::graphics::BakedLight;
    const FaceCell& fc = r.cell;
    auto vertex = [&](int k) { return V(c[static_cast<std::size_t>(k)], col, cf, fc.shade[static_cast<std::size_t>(k)], var, flags, BakedLight::unpack(fc.light[static_cast<std::size_t>(k)])); };
    out.add_quad(vertex(0), vertex(1), vertex(2), vertex(3));
}

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_GREEDY_MESHER_HPP