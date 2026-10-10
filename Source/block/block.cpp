#include "block/block.hpp"

namespace voxelspire {

BlockShape BlockShape::panel(Face wall, double thickness) noexcept {
    BlockShape s;
    const int axis = face_axis(wall);
    const double t = vclamp(thickness, 0.0, 1.0);
    if (face_positive(wall)) set_component(s.min, axis, 1.0 - t);
    else set_component(s.max, axis, t);
    return s;
}

bool BlockShape::full() const noexcept {
    for (int a = 0; a < 3; ++a) if (component(min, a) != 0.0 || component(max, a) != 1.0) return false;
    return true;
}

bool BlockShape::touches(Face f) const noexcept {
    const int axis = face_axis(f);
    return face_positive(f) ? component(max, axis) >= 1.0 : component(min, axis) <= 0.0;
}

LightEmission LightEmission::tinted(const Color& tint, int level) noexcept {
    const int peak = vmax(vmax(static_cast<int>(tint.red()), static_cast<int>(tint.green())), vmax(static_cast<int>(tint.blue()), 1));
    auto scale = [&](int c) { return static_cast<int>(std::lround(static_cast<double>(level) * c / peak)); };
    return { scale(tint.red()), scale(tint.green()), scale(tint.blue()) };
}

BlockProperties BlockProperties::see_through(bool solid, bool fluid, int light_opacity) noexcept {
    BlockProperties p;
    p.solid         = solid;
    p.opaque        = false;
    p.fluid         = fluid;
    p.light_opacity = light_opacity;
    return p;
}

BlockProperties& BlockProperties::shaped(const BlockShape& s) noexcept {
    shape     = s;
    full_cube = s.full();
    if (!full_cube) { opaque = false; blends = false; light_opacity = LightLimits::CLEAR; }
    return *this;
}

RenderLayer BlockProperties::layer() const noexcept {
    if (!visible) return RenderLayer::None;
    return opaque || !blends ? RenderLayer::Opaque : RenderLayer::Translucent;
}

bool BlockTraits::face_visible(BlockId self, const BlockTraits& a, BlockId other, const BlockTraits& b) noexcept {
    if (!a.visible || b.opaque) return false;
    if (a.fluid && b.solid && !b.opaque && b.full_cube) return false;
    return !(self == other && a.cull_same);
}

Block::Block(Identifier identifier, BlockProperties props) : m_identifier(identifier), m_props(props) {
    if (m_identifier.kind() != Kind::Block) throw std::invalid_argument("block identifiers must be of kind block: " + m_identifier.str());
}

Color Block::face_color(Face face, const BlockPos& pos) const {
    const FaceAppearance a = face_appearance(face);
    return fizmo::graphics::cell_tinted(a.base, fizmo::graphics::cell_variation(pos.x, pos.y, pos.z, static_cast<unsigned int>(vmax(a.variation, 0))), 255);
}

BlockTraits Block::traits() const {
    BlockTraits t;
    t.solid     = m_props.solid;
    t.opaque    = m_props.opaque && m_props.visible;
    t.visible   = m_props.visible;
    t.fluid     = m_props.fluid;
    t.cull_same = m_props.cull_same;
    t.full_cube = m_props.full_cube;
    t.light_opacity = static_cast<std::uint8_t>(t.opaque ? LightLimits::BLOCKING : vclamp(m_props.light_opacity, LightLimits::CLEAR, LightLimits::BLOCKING));
    t.emission  = m_props.emission;
    t.finish    = m_props.finish;
    t.layer     = m_props.layer();
    t.shape     = m_props.shape;
    t.thermal   = m_props.thermal;
    if (t.thermal.insulation < 0.0) t.thermal.insulation = default_insulation(t);
    for (Face f : ALL_FACES) t.faces[static_cast<std::size_t>(f)] = face_appearance(f);
    return t;
}

double Block::default_insulation(const BlockTraits& t) noexcept {
    if (t.fluid) return FLUID_INSULATION;
    if (!t.solid) return 0.0;
    return t.opaque && t.full_cube ? SOLID_INSULATION : SEE_THROUGH_INSULATION;
}

FaceAppearance Block::appearance(const Color& base, int max_variation, bool plain) const noexcept {
    return { base, static_cast<int>(std::lround(max_variation * vmax(m_props.color_variation, 0.0))), plain };
}

BlockProperties AirBlock::air_properties() noexcept {
    BlockProperties p;
    p.solid = p.opaque = p.visible = false;
    p.light_opacity = LightLimits::CLEAR;
    return p;
}

} // namespace voxelspire
