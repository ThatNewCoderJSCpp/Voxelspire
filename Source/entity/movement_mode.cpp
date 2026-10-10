#include "entity/movement_mode.hpp"

namespace voxelspire {

MovementMode::MovementMode(Identifier id, int priority, Identifier pose, double speed_multiplier, double alt_speed_multiplier) : m_id(id), m_pose(pose), m_priority(priority), m_speed(speed_multiplier), m_alt_speed(alt_speed_multiplier) {}

void MovementMode::update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const {
    const vector3d target = wish * (stats.speed * speed_multiplier(ctx.intent));
    const double accel = (ctx.on_ground ? stats.ground_accel : stats.air_accel) * acceleration_multiplier() * dt;
    v.x = approach(v.x, target.x, accel);
    v.y = approach(v.y, target.y, accel);
    if (can_jump() && ctx.intent.jump && ctx.on_ground) v.z = stats.jump_velocity;
}

double MovementMode::approach(double v, double target, double max_delta) noexcept {
    const double diff = target - v;
    if (std::fabs(diff) <= max_delta) return target;
    return v + (diff > 0.0 ? max_delta : -max_delta);
}

WalkMode::WalkMode(
        double speed_multiplier,
        double alt_speed_multiplier 
) : MovementMode(MovementModes::Walk, 0, Poses::Standing, speed_multiplier, alt_speed_multiplier) {}

SprintMode::SprintMode(
        double speed_multiplier,
        double alt_speed_multiplier 
) : MovementMode(MovementModes::Sprint, 100, Poses::Standing, speed_multiplier, alt_speed_multiplier) {}

CrouchMode::CrouchMode(
        double speed_multiplier,
        double alt_speed_multiplier 
) : MovementMode(MovementModes::Crouch, 200, Poses::Crouching, speed_multiplier, alt_speed_multiplier) {}

CrawlMode::CrawlMode(
        double speed_multiplier,
        double alt_speed_multiplier 
) : MovementMode(MovementModes::Crawl, 300, Poses::Prone, speed_multiplier, alt_speed_multiplier) {}

SwimMode::SwimMode(
        double speed_multiplier,
        double alt_speed_multiplier,
        double accel_multiplier,
        double rise_speed, 
        double sink_speed,
        double vertical_accel, 
        double surface_leap 
) : MovementMode(MovementModes::Swim, DEFAULT_PRIORITY, Poses::Standing, speed_multiplier, alt_speed_multiplier),
          m_accel(accel_multiplier), m_rise(rise_speed), m_sink(sink_speed), m_vertical_accel(vertical_accel), m_leap(surface_leap) {}

void SwimMode::update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const {
    const vector3d target = wish * (stats.speed * speed_multiplier(ctx.intent));
    const double accel = stats.ground_accel * acceleration_multiplier() * dt;
    v.x = approach(v.x, target.x, accel);
    v.y = approach(v.y, target.y, accel);

    if (ctx.intent.jump && !ctx.head_in_fluid && (wish.x != 0.0 || wish.y != 0.0)) {
        v.z = vmax(v.z, stats.jump_velocity * m_leap);
        return;
    }

    const double vz = ctx.intent.jump ? m_rise : (ctx.intent.crouch ? -m_rise : -m_sink);
    v.z = approach(v.z, vz, m_vertical_accel * dt);
}

StrokeMode::StrokeMode(
        double speed_multiplier,
        double alt_speed_multiplier,
        double accel,
        double buoyancy 
) : MovementMode(MovementModes::Stroke, DEFAULT_PRIORITY, Poses::Swimming, speed_multiplier, alt_speed_multiplier), m_accel(accel), m_buoyancy(buoyancy) {}

void StrokeMode::update_velocity(vector3d& v, const vector3d&, const MovementContext& ctx, const MovementStats& stats, double dt) const {
    vector3d dir = ctx.look * ctx.intent.forward + ctx.right * ctx.intent.strafe;
    const double len = dir.magnitude();
    if (len > 1.0) dir = dir / len;
    const vector3d target = dir * (stats.speed * speed_multiplier(ctx.intent)) + vector3d{ 0.0, 0.0, len > 0.0 ? 0.0 : m_buoyancy };
    const double accel = stats.ground_accel * m_accel * dt;
    v.x = approach(v.x, target.x, accel);
    v.y = approach(v.y, target.y, accel);
    v.z = approach(v.z, target.z, accel);
}

FlutterMode::FlutterMode(
        double drag, double min_drop,
        double probe_depth, double speed_multiplier,
        double alt_drag, int priority 
) : MovementMode(MovementModes::Flutter, priority, Poses::Standing, speed_multiplier, speed_multiplier),
          m_drag(drag), m_alt_drag(alt_drag), m_min_drop(min_drop), m_probe(vmax(probe_depth, min_drop)) {}

bool FlutterMode::is_active(const MovementContext& ctx) const {
    if (ctx.on_ground || ctx.in_fluid || ctx.vertical_velocity >= 0.0) return false;
    if (ctx.fall_distance >= m_min_drop) return true;
    const double remaining = ctx.ground_distance ? ctx.ground_distance(m_probe) : m_probe;
    return ctx.fall_distance + remaining >= m_min_drop;
}

FlyMode::FlyMode(
        double speed_multiplier, 
        double vertical_speed,
        double vertical_accel, 
        double alt_speed_multiplier, 
        int priority 
) : MovementMode(MovementModes::Fly, priority, Poses::Standing, speed_multiplier, alt_speed_multiplier),
          m_vertical_speed(vertical_speed), m_vertical_accel(vertical_accel) {}

void FlyMode::update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const {
    const vector3d target = wish * (stats.speed * speed_multiplier(ctx.intent));
    const double accel = stats.air_accel * dt;
    v.x = approach(v.x, target.x, accel);
    v.y = approach(v.y, target.y, accel);
    const double climb = (ctx.intent.jump ? 1.0 : 0.0) - (ctx.intent.crouch ? 1.0 : 0.0);
    v.z = approach(v.z, climb * m_vertical_speed, m_vertical_accel * dt);
}

bool MovementModeRegistry::remove(Identifier id) {
    auto it = std::find_if(m_modes.begin(), m_modes.end(), [&](const auto& m) { return m->id() == id; });
    if (it == m_modes.end()) return false;
    m_modes.erase(it);
    return true;
}

void MovementModeRegistry::register_defaults(MovementModeRegistry& r, const CharacterSettings& c) {
    r.add<WalkMode>(c.speed_ratio(c.walk_speed), c.speed_ratio(c.alt_walk_speed));
    r.add<SprintMode>(c.speed_ratio(c.sprint_speed), c.speed_ratio(c.alt_sprint_speed));
    r.add<CrouchMode>(c.speed_ratio(c.crouch_speed), c.speed_ratio(c.alt_crouch_speed));
    r.add<CrawlMode>(c.speed_ratio(c.crawl_speed), c.speed_ratio(c.alt_crawl_speed));
        
    r.add<SwimMode>(
        c.speed_ratio(c.swim_speed), c.speed_ratio(c.alt_swim_speed), 
        c.acceleration_ratio(c.swim_acceleration),
        c.swim_rise_speed, c.swim_sink_speed, c.swim_vertical_accel, c.surface_leap
    );

    r.add<StrokeMode>(c.speed_ratio(c.stroke_speed), c.speed_ratio(c.alt_stroke_speed), c.acceleration_ratio(c.stroke_acceleration), c.stroke_buoyancy);
}

} // namespace voxelspire
