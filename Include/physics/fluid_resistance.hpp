#ifndef VOXELSPIRE_PHYSICS_FLUID_RESISTANCE_HPP
#define VOXELSPIRE_PHYSICS_FLUID_RESISTANCE_HPP

#include <cmath>
#include <string>
#include "../core/identifier.hpp"
#include "../core/types.hpp"
#include "../entity/player_defaults.hpp"

namespace voxelspire {

struct FluidContext {
    double dt          = 0.0;
    double submerged   = 0.0;
    double width       = 0.0;
    double height      = 0.0;
    double free_speed_horizontal = 0.0;
    double free_speed_up         = 0.0;
    double free_speed_down       = 0.0;
};

class FluidResistance {
public:
    virtual ~FluidResistance() = default;

    virtual Identifier id() const = 0;

    vector3d apply(const vector3d& v, const FluidContext& ctx) const {
        if (ctx.submerged <= 0.0 || ctx.dt <= 0.0) return v;
        const double side = side_area(ctx), top = top_area(ctx);
        return { slow_axis(v.x, ctx.free_speed_horizontal, side, ctx),
                 slow_axis(v.y, ctx.free_speed_horizontal, side, ctx),
                 slow_axis(v.z, v.z > 0.0 ? ctx.free_speed_up : ctx.free_speed_down, top, ctx) };
    }

protected:
    virtual double slow(double excess, double area, const FluidContext& ctx) const = 0;

    static double side_area(const FluidContext& ctx) noexcept { return ctx.width * ctx.height * ctx.submerged; }
    static double top_area(const FluidContext& ctx) noexcept { return ctx.width * ctx.width; }

private:
    double slow_axis(double v, double free_speed, double area, const FluidContext& ctx) const {
        const double speed = std::fabs(v);
        const double limit = vmax(free_speed, 0.0);
        if (speed <= limit) return v;
        const double excess = vmax(slow(speed - limit, area, ctx), 0.0);
        return std::copysign(limit + excess, v);
    }
};

class NoFluidResistance final : public FluidResistance {
public:
    Identifier id() const override { return core_id(Kind::Physics, { "fluid_resistance", "none" }); }

protected:
    double slow(double excess, double, const FluidContext&) const override { return excess; }
};

class LinearFluidDrag final : public FluidResistance {
public:
    explicit LinearFluidDrag(double per_second) noexcept : m_k(vmax(per_second, 0.0)) {}

    Identifier id() const override { return core_id(Kind::Physics, { "fluid_resistance", "linear" }); }
    double per_second() const noexcept { return m_k; }

protected:
    double slow(double excess, double, const FluidContext& ctx) const override {
        return excess * std::exp(-m_k * ctx.submerged * ctx.dt);
    }

private:
    double m_k;
};

class TickFluidDamping final : public FluidResistance {
public:
    static constexpr double MINECRAFT_FACTOR = 0.8;
    static constexpr double MINECRAFT_TICKS  = 20.0;

    explicit TickFluidDamping(double factor_per_tick = MINECRAFT_FACTOR, double ticks_per_second = MINECRAFT_TICKS) noexcept
        : m_factor(vclamp(factor_per_tick, 0.0, 1.0)), m_ticks(vmax(ticks_per_second, 0.0)) {}

    Identifier id() const override { return core_id(Kind::Physics, { "fluid_resistance", "tick_damping" }); }

protected:
    double slow(double excess, double, const FluidContext& ctx) const override {
        return excess * std::pow(m_factor, m_ticks * ctx.dt * ctx.submerged);
    }

private:
    double m_factor, m_ticks;
};

class QuadraticFluidDrag final : public FluidResistance {
public:
    static constexpr double WATER_DENSITY    = 1000.0;
    static constexpr double DRAG_COEFFICIENT = 1.0;
    static constexpr double BODY_MASS        = PlayerDefaults::mass;
    static constexpr double HALF             = 0.5;

    explicit QuadraticFluidDrag(double density = WATER_DENSITY, double drag_coefficient = DRAG_COEFFICIENT, double body_mass = BODY_MASS) noexcept
        : m_density(vmax(density, 0.0)), m_cd(vmax(drag_coefficient, 0.0)), m_mass(vmax(body_mass, 1e-6)) {}

    Identifier id() const override { return core_id(Kind::Physics, { "fluid_resistance", "quadratic" }); }

protected:
    double slow(double excess, double area, const FluidContext& ctx) const override {
        const double c = HALF * m_density * m_cd * area / m_mass;
        return excess / (1.0 + c * excess * ctx.dt);
    }

private:
    double m_density;
    double m_cd;
    double m_mass;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_FLUID_RESISTANCE_HPP