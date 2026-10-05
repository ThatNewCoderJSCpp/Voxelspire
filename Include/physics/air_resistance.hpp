#ifndef VOXELSPIRE_PHYSICS_AIR_RESISTANCE_HPP
#define VOXELSPIRE_PHYSICS_AIR_RESISTANCE_HPP

#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include "../core/identifier.hpp"
#include "../core/types.hpp"

namespace voxelspire {

struct AirContext {
    double gravity      = 0.0;
    double dt           = 0.0;
    double world_height = 0.0;
    double drag_factor  = 1.0;

    double fall_sign() const noexcept { return gravity >= 0.0 ? -1.0 : 1.0; }
};

class AirResistance {
public:
    virtual ~AirResistance() = default;

    virtual Identifier id() const = 0;
    virtual double apply(double vertical_velocity, const AirContext& ctx) const = 0;
    virtual double terminal_velocity(const AirContext& ctx) const = 0;

    static constexpr double unlimited() noexcept { return std::numeric_limits<double>::infinity(); }

protected:
    static double scaled_cap(double cap, const AirContext& ctx) noexcept {
        return ctx.drag_factor > 0.0 ? cap / std::sqrt(ctx.drag_factor) : unlimited();
    }

    static double cap_fall(double v, double cap, const AirContext& ctx) noexcept {
        const double along = v * ctx.fall_sign();
        return along > cap ? cap * ctx.fall_sign() : v;
    }
};

class NoAirResistance final : public AirResistance {
public:
    Identifier id() const override { return core_id(Kind::Physics, { "air_resistance", "none" }); }
    double apply(double v, const AirContext&) const override { return v; }
    double terminal_velocity(const AirContext&) const override { return unlimited(); }
};

class TerminalVelocityCap final : public AirResistance {
public:
    explicit TerminalVelocityCap(double max_fall_speed) noexcept : m_max(vmax(max_fall_speed, 0.0)) {}

    Identifier id() const override { return core_id(Kind::Physics, { "air_resistance", "terminal_velocity_cap" }); }
    double apply(double v, const AirContext& ctx) const override { return cap_fall(v, terminal_velocity(ctx), ctx); }
    double terminal_velocity(const AirContext& ctx) const override { return scaled_cap(m_max, ctx); }

    double max_fall_speed() const noexcept { return m_max; }

private:
    double m_max;
};

class WorldHeightLimit final : public AirResistance {
public:
    Identifier id() const override { return core_id(Kind::Physics, { "air_resistance", "world_height_limit" }); }
    double apply(double v, const AirContext& ctx) const override { return cap_fall(v, terminal_velocity(ctx), ctx); }

    double terminal_velocity(const AirContext& ctx) const override {
        return std::sqrt(2.0 * std::fabs(ctx.gravity) * vmax(ctx.world_height, 0.0));
    }
};

class LinearDrag final : public AirResistance {
public:
    explicit LinearDrag(double per_second) noexcept : m_k(vmax(per_second, 0.0)) {}

    static double coefficient_for(double terminal_velocity, double gravity) noexcept {
        return terminal_velocity > 0.0 ? std::fabs(gravity) / terminal_velocity : 0.0;
    }

    Identifier id() const override { return core_id(Kind::Physics, { "air_resistance", "linear_drag" }); }
    double apply(double v, const AirContext& ctx) const override { return v * std::exp(-strength(ctx) * ctx.dt); }

    double terminal_velocity(const AirContext& ctx) const override {
        const double k = strength(ctx);
        return k > 0.0 ? std::fabs(ctx.gravity) / k : unlimited();
    }

    double coefficient() const noexcept { return m_k; }

private:
    double strength(const AirContext& ctx) const noexcept { return m_k * vmax(ctx.drag_factor, 0.0); }

    double m_k;
};

class QuadraticDrag final : public AirResistance {
public:
    explicit QuadraticDrag(double per_block) noexcept : m_c(vmax(per_block, 0.0)) {}

    static double coefficient_for(double terminal_velocity, double gravity) noexcept {
        return terminal_velocity > 0.0 ? std::fabs(gravity) / (terminal_velocity * terminal_velocity) : 0.0;
    }

    Identifier id() const override { return core_id(Kind::Physics, { "air_resistance", "quadratic_drag" }); }
    double apply(double v, const AirContext& ctx) const override { return v / (1.0 + strength(ctx) * std::fabs(v) * ctx.dt); }

    double terminal_velocity(const AirContext& ctx) const override {
        const double c = strength(ctx);
        return c > 0.0 ? std::sqrt(std::fabs(ctx.gravity) / c) : unlimited();
    }

    double coefficient() const noexcept { return m_c; }

private:
    double strength(const AirContext& ctx) const noexcept { return m_c * vmax(ctx.drag_factor, 0.0); }

    double m_c;
};

class CustomAirResistance final : public AirResistance {
public:
    using ApplyFn    = std::function<double(double vertical_velocity, const AirContext& ctx)>;
    using TerminalFn = std::function<double(const AirContext& ctx)>;

    CustomAirResistance(Identifier id, ApplyFn apply, TerminalFn terminal = {})
        : m_id(id), m_apply(std::move(apply)), m_terminal(std::move(terminal)) {}

    Identifier id() const override { return m_id; }
    double apply(double v, const AirContext& ctx) const override { return m_apply ? m_apply(v, ctx) : v; }
    double terminal_velocity(const AirContext& ctx) const override { return m_terminal ? m_terminal(ctx) : unlimited(); }

private:
    Identifier m_id;
    ApplyFn    m_apply;
    TerminalFn m_terminal;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_AIR_RESISTANCE_HPP