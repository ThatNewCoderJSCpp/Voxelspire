#include "render/greedy_mesher.hpp"

namespace voxelspire {

double FaceShading::factor(Face f) const noexcept {
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

std::uint64_t FaceKey::make(const Color& base, int variation, std::uint8_t shade, bool translucent, std::uint8_t vertex_flags) noexcept {
    return HAS_FACE | (translucent ? TRANSLUCENT : 0) | fizmo::graphics::Vertex3D::pack(base)
         | (static_cast<std::uint64_t>(vclamp(variation, 0, MAX_VARIATION)) << VAR_SHIFT)
         | (static_cast<std::uint64_t>(shade) << SHADE_SHIFT)
         | (static_cast<std::uint64_t>(vertex_flags) << FLAGS_SHIFT);
}

std::uint8_t FaceKey::finish_flags(SurfaceFinish finish) noexcept {
    switch (finish) {
        case SurfaceFinish::Glossy: return fizmo::graphics::CompactGlossy;
        case SurfaceFinish::Liquid: return fizmo::graphics::CompactLiquid;
        case SurfaceFinish::Mirror: return fizmo::graphics::CompactMirror;
        case SurfaceFinish::Matte:  break;
    }

    return 0;
}

Color FaceKey::color(std::uint64_t key) noexcept {
    const auto c = static_cast<std::uint32_t>(key);

    return Color(
        static_cast<std::uint8_t>(c), 
        static_cast<std::uint8_t>(c >> 8),
        static_cast<std::uint8_t>(c >> 16), 
        static_cast<std::uint8_t>(c >> 24)
    );
}

std::uint64_t FaceKey::with_surface(std::uint64_t key, double height, std::uint8_t flow) noexcept {
    const auto steps = static_cast<std::uint64_t>(vclamp(std::lround((1.0 - height) * SURFACE_STEPS), 0L, static_cast<long>(SURFACE_STEPS)));
    key = (key & ~(SURFACE_MASK << SURFACE_SHIFT)) | (steps << SURFACE_SHIFT);
    if (flow == 0) return key;
    key = (key & ~(BYTE_MASK << VAR_SHIFT)) | (static_cast<std::uint64_t>(flow) << VAR_SHIFT);
    return key | (static_cast<std::uint64_t>(fizmo::graphics::CompactFlow) << FLAGS_SHIFT);
}

void face_corners(const vector3d& lo, const vector3d& hi, Face f, std::array<vector3d, 4>& out) noexcept {
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

void emit_scaled_quad(fizmo::graphics::QuadMesh3D& out, const GreedyRect& r, const vector3d& origin, const vector3d& cell) {
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
