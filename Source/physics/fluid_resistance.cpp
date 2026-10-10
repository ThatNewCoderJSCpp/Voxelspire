#include "physics/fluid_resistance.hpp"

namespace voxelspire {

vector3d FluidResistance::apply(const vector3d& v, const FluidContext& ctx) const {
    if (ctx.submerged <= 0.0 || ctx.dt <= 0.0) return v;
    const double side = side_area(ctx), top = top_area(ctx);
    return { slow_axis(v.x, ctx.free_speed_horizontal, side, ctx),
             slow_axis(v.y, ctx.free_speed_horizontal, side, ctx),
             slow_axis(v.z, v.z > 0.0 ? ctx.free_speed_up : ctx.free_speed_down, top, ctx) };
}

double FluidResistance::slow_axis(double v, double free_speed, double area, const FluidContext& ctx) const {
    const double speed = std::fabs(v);
    const double limit = vmax(free_speed, 0.0);
    if (speed <= limit) return v;
    const double excess = vmax(slow(speed - limit, area, ctx), 0.0);
    return std::copysign(limit + excess, v);
}

QuadraticFluidDrag::QuadraticFluidDrag(double density, double drag_coefficient, double body_mass) noexcept : m_density(vmax(density, 0.0)), m_cd(vmax(drag_coefficient, 0.0)), m_mass(vmax(body_mass, 1e-6)) {}

} // namespace voxelspire
