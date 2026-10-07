#ifndef VOXELSPIRE_BLOCK_BLOCK_HPP
#define VOXELSPIRE_BLOCK_BLOCK_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include "block_ids.hpp"

namespace voxelspire {

class World;

using BlockId = std::uint16_t;
constexpr BlockId AIR_ID = 0;

enum class RenderLayer : std::uint8_t { None = 0, Opaque, Translucent };
enum class SurfaceFinish : std::uint8_t { Matte = 0, Glossy, Liquid, Mirror };

struct FaceAppearance {
    Color base;
    int   variation = 0;
    bool  plain     = false;
};

struct BlockShape {
    vector3d min{ 0.0, 0.0, 0.0 };
    vector3d max{ 1.0, 1.0, 1.0 };

    static BlockShape cube() noexcept { return {}; }

    static BlockShape box(const vector3d& lo, const vector3d& hi) noexcept {
        BlockShape s;
        s.min = lo;
        s.max = hi;
        return s;
    }

    static BlockShape panel(Face wall, double thickness) noexcept {
        BlockShape s;
        const int axis = face_axis(wall);
        const double t = vclamp(thickness, 0.0, 1.0);
        if (face_positive(wall)) set_component(s.min, axis, 1.0 - t);
        else set_component(s.max, axis, t);
        return s;
    }

    bool full() const noexcept {
        for (int a = 0; a < 3; ++a) if (component(min, a) != 0.0 || component(max, a) != 1.0) return false;
        return true;
    }

    bool touches(Face f) const noexcept {
        const int axis = face_axis(f);
        return face_positive(f) ? component(max, axis) >= 1.0 : component(min, axis) <= 0.0;
    }

    AABB at(const BlockPos& p) const noexcept { return { p.min_corner() + min, p.min_corner() + max }; }
};

struct LightLimits {
    static constexpr int MAX         = 15;
    static constexpr int CLEAR       = 0;
    static constexpr int BLOCKING    = MAX;
};

struct LightEmission {
    std::uint8_t red = 0, green = 0, blue = 0;

    constexpr LightEmission() noexcept = default;
    constexpr LightEmission(int r, int g, int b) noexcept
        : red(clamp_level(r)), green(clamp_level(g)), blue(clamp_level(b)) {}

    static constexpr LightEmission none() noexcept { return {}; }
    static constexpr LightEmission white(int level) noexcept { return { level, level, level }; }

    static LightEmission tinted(const Color& tint, int level) noexcept {
        const int peak = vmax(vmax(static_cast<int>(tint.red()), static_cast<int>(tint.green())), vmax(static_cast<int>(tint.blue()), 1));
        auto scale = [&](int c) { return static_cast<int>(std::lround(static_cast<double>(level) * c / peak)); };
        return { scale(tint.red()), scale(tint.green()), scale(tint.blue()) };
    }

    constexpr int  level()  const noexcept { return vmax(vmax(static_cast<int>(red), static_cast<int>(green)), static_cast<int>(blue)); }
    constexpr bool emits()  const noexcept { return level() > 0; }
    constexpr int  channel(int c) const noexcept { return c == 0 ? red : (c == 1 ? green : blue); }

private:
    static constexpr std::uint8_t clamp_level(int v) noexcept { return static_cast<std::uint8_t>(vclamp(v, 0, LightLimits::MAX)); }
};

struct BlockThermal {
    static constexpr double AUTO     = -1.0;
    static constexpr double NO_LIMIT = 1e9;

    double heat            = 0.0;
    double insulation      = AUTO;
    double conductivity    = 1.0;
    double min_temperature = -NO_LIMIT;
    double max_temperature = NO_LIMIT;

    double surface(double air) const noexcept { return vclamp(air, min_temperature, max_temperature); }
};

struct BlockProperties {
    bool          solid           = true;
    bool          opaque          = true;
    bool          visible         = true;
    bool          fluid           = false;
    double        color_variation = 1.0;
    bool          cull_same       = true;
    bool          full_cube       = true;
    int           light_opacity   = LightLimits::BLOCKING;
    LightEmission emission;
    SurfaceFinish finish          = SurfaceFinish::Matte;
    BlockShape    shape;
    bool          blends          = true;
    BlockThermal  thermal;

    static BlockProperties see_through(bool solid, bool fluid = false, int light_opacity = LightLimits::CLEAR) noexcept {
        BlockProperties p;
        p.solid         = solid;
        p.opaque        = false;
        p.fluid         = fluid;
        p.light_opacity = light_opacity;
        return p;
    }

    BlockProperties& glowing(LightEmission light) noexcept { emission = light; return *this; }
    BlockProperties& finished(SurfaceFinish surface) noexcept { finish = surface; return *this; }
    BlockProperties& heating(double celsius) noexcept { thermal.heat = celsius; return *this; }
    BlockProperties& insulating(double share) noexcept { thermal.insulation = share; return *this; }
    BlockProperties& conducting(double speed) noexcept { thermal.conductivity = speed; return *this; }
    BlockProperties& no_warmer_than(double celsius) noexcept { thermal.max_temperature = celsius; return *this; }
    BlockProperties& no_colder_than(double celsius) noexcept { thermal.min_temperature = celsius; return *this; }

    BlockProperties& shaped(const BlockShape& s) noexcept {
        shape     = s;
        full_cube = s.full();
        if (!full_cube) { opaque = false; blends = false; light_opacity = LightLimits::CLEAR; }
        return *this;
    }

    RenderLayer layer() const noexcept {
        if (!visible) return RenderLayer::None;
        return opaque || !blends ? RenderLayer::Opaque : RenderLayer::Translucent;
    }
};

struct BlockTraits {
    bool          solid     = false;
    bool          opaque    = false;
    bool          visible   = false;
    bool          fluid     = false;
    bool          cull_same = true;
    bool          full_cube = true;
    std::uint8_t  light_opacity = LightLimits::CLEAR;
    LightEmission emission;
    SurfaceFinish finish    = SurfaceFinish::Matte;
    RenderLayer   layer     = RenderLayer::None;
    BlockShape    shape;
    BlockThermal  thermal;
    std::array<FaceAppearance, FACE_COUNT> faces{};

    const FaceAppearance& face(Face f) const noexcept { return faces[static_cast<std::size_t>(f)]; }

    static bool face_visible(BlockId self, const BlockTraits& a, BlockId other, const BlockTraits& b) noexcept {
        if (!a.visible || b.opaque) return false;
        if (a.fluid && b.solid && !b.opaque && b.full_cube) return false;
        return !(self == other && a.cull_same);
    }
};

class Block {
public:
    Block(Identifier identifier, BlockProperties props) : m_identifier(identifier), m_props(props) {
        if (m_identifier.kind() != Kind::Block) throw std::invalid_argument("block identifiers must be of kind block: " + m_identifier.str());
    }
    virtual ~Block() = default;

    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    BlockId                handle()     const noexcept { return m_handle; }
    Identifier             identifier() const noexcept { return m_identifier; }
    const std::string&     name()       const { return m_identifier.name(); }
    const BlockProperties& properties() const noexcept { return m_props; }

    bool is_solid()   const noexcept { return m_props.solid; }
    bool is_opaque()  const noexcept { return m_props.opaque; }
    bool is_visible() const noexcept { return m_props.visible; }
    bool is_fluid()   const noexcept { return m_props.fluid; }
    RenderLayer layer() const noexcept { return m_props.layer(); }

    virtual FaceAppearance face_appearance(Face face) const = 0;

    Color face_color(Face face, const BlockPos& pos) const {
        const FaceAppearance a = face_appearance(face);
        return fizmo::graphics::cell_tinted(a.base, fizmo::graphics::cell_variation(pos.x, pos.y, pos.z, static_cast<unsigned int>(vmax(a.variation, 0))), 255);
    }

    virtual AABB collision_box(const BlockPos& pos) const noexcept { return m_props.shape.at(pos); }

    virtual void on_placed(World&, const BlockPos&) {}
    virtual void on_removed(World&, const BlockPos&) {}

    BlockTraits traits() const {
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

protected:
    static constexpr double SOLID_INSULATION       = 0.7;
    static constexpr double SEE_THROUGH_INSULATION = 0.35;
    static constexpr double FLUID_INSULATION       = 0.5;

    static double default_insulation(const BlockTraits& t) noexcept {
        if (t.fluid) return FLUID_INSULATION;
        if (!t.solid) return 0.0;
        return t.opaque && t.full_cube ? SOLID_INSULATION : SEE_THROUGH_INSULATION;
    }

    FaceAppearance appearance(const Color& base, int max_variation, bool plain = false) const noexcept {
        return { base, static_cast<int>(std::lround(max_variation * vmax(m_props.color_variation, 0.0))), plain };
    }

private:
    friend class BlockRegistry;
    BlockId         m_handle = AIR_ID;
    Identifier      m_identifier;
    BlockProperties m_props;
};

class AirBlock final : public Block {
public:
    AirBlock() : Block(BlockIds::AIR, air_properties()) {}
    FaceAppearance face_appearance(Face) const override { return { Color(0, 0, 0, 0), 0 }; }

private:
    static BlockProperties air_properties() noexcept {
        BlockProperties p;
        p.solid = p.opaque = p.visible = false;
        p.light_opacity = LightLimits::CLEAR;
        return p;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCK_HPP