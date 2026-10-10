#include "entity/player.hpp"

namespace voxelspire {

Player::Player(const AttributeRegistry& attributes, const MovementModeRegistry& modes, EntityBody body) : Entity(body.standing().width, body.standing().height),
          m_attributes(attributes), m_modes(&modes), m_body(std::move(body)),
          m_eye_height(m_body.standing().eye_height), m_prev_eye_height(m_eye_height) {}

bool Player::casts_capsule_shadow(fizmo::graphics::CapsuleOccluder3D& out, double alpha) const {
    out = fizmo::graphics::CapsuleOccluder3D::standing(interpolated_position(alpha), height(), capsule_radius());
    return true;
}

void Player::respawn(const vector3d& feet) noexcept {
    set_position(feet);
    set_velocity({});
    set_on_ground(false);
    apply_pose(Poses::Standing);
    m_eye_height = m_prev_eye_height = m_body.standing().eye_height;
}

void Player::tick(TickContext& ctx) {
    begin_tick();
    m_prev_eye_height = m_eye_height;
    auto fits_pose = [&](Identifier pose) { return fits(ctx, pose); };
    auto ground    = [&](double max_depth) { return ctx.physics.ground_distance(*this, ctx.world, max_depth); };
    const bool fluid = in_fluid(ctx.world);
    const bool started_on_ground = m_on_ground;
    const MovementIntent intent = limited(m_intent, fluid);
    m_report = {};
    m_report.in_fluid   = fluid;
    m_report.head_under = head_in_fluid(ctx.world);
    m_report.straining  = (m_intent.sprint && m_intent.forward > 0.0) || (fluid && (m_intent.swim || m_intent.jump));

    const MovementContext mctx{
        intent,
        ctx.world,
        m_on_ground,
        fluid,
        fits_pose,
        m_velocity.z,
        m_fall_distance,
        ground,
        m_report.head_under,
        look_direction(),
        flat_right()
    };

    m_mode = m_modes->select(mctx);
    if (!m_mode) return;
    m_report.activity = m_mode->id();
    if (m_mode->pose() != m_pose && fits(ctx, m_mode->pose())) apply_pose(m_mode->pose());
    vector3d wish = flat_forward() * intent.forward + flat_right() * intent.strafe;
    const double len = wish.magnitude();
    if (len > 1.0) wish = wish / len;
    m_report.moving = len > 0.0;
    MovementStats stats = movement_stats();
    stats.speed *= m_limits.speed * (m_mode->id() == MovementModes::Sprint ? m_limits.sprint_speed : 1.0);
    const double submerged = ctx.physics.submerged_fraction(*this, ctx.world);
    if (!mctx.in_fluid) stats.speed *= ctx.physics.wade_factor(submerged);

    ctx.physics.apply_fluid(
        *this, submerged, stats.speed * m_mode->speed_multiplier(intent),
        m_mode->rising_speed_limit(stats, intent),
        m_mode->sinking_speed_limit(stats, intent),
        ctx.dt
    );

    m_drift = ctx.physics.approach_drift(m_drift, ctx.physics.current_drift(*this, ctx.world, submerged), ctx.dt);
    vector3d v = m_velocity - m_drift;
    const double rise_from = v.z;
    m_mode->update_velocity(v, wish, mctx, stats, ctx.dt);
    m_report.jumped = started_on_ground && intent.jump && m_mode->can_jump() && v.z > 0.0 && rise_from <= 0.0;
    if (submerged > 0.0) weigh_down(v, rise_from, submerged, ctx.dt);
    v = v + m_drift;
    if (v.z > 0.0 && m_on_ground) m_on_ground = false;
    m_velocity = v;
    ctx.physics.apply_falling_water(*this, ctx.world, submerged, ctx.dt);
    ctx.physics.apply_gravity(*this, ctx.dt, effective_gravity_scale() * ctx.physics.buoyancy_scale(submerged), effective_drag_scale());
    const double fall_cap = ctx.physics.absorb_fall(*this, ctx.world, submerged, m_was_submerged);
    const CollisionResult moved = ctx.physics.move(*this, ctx.world, m_velocity * ctx.dt);
    m_report.landed_fall = moved.landed_after_falling;
    if (submerged > 0.0) m_fall_distance = vmin(m_fall_distance, fall_cap);
    m_was_submerged = submerged > 0.0;
    const double target_eye = m_body.get(m_pose).eye_height;
    m_eye_height += (target_eye - m_eye_height) * vmin(1.0, EYE_SMOOTHING * ctx.dt);
    if (std::fabs(target_eye - m_eye_height) < 1e-4) m_eye_height = target_eye;
}

MovementStats Player::movement_stats() const {
    MovementStats s;
    s.speed         = m_attributes.value(Attributes::MovementSpeed);
    s.ground_accel  = m_attributes.value(Attributes::GroundAcceleration);
    s.air_accel     = m_attributes.value(Attributes::AirAcceleration);
    s.jump_velocity = m_attributes.value(Attributes::JumpVelocity);
    s.gravity_scale = m_attributes.value(Attributes::GravityScale);
    s.drag_scale    = m_attributes.value(Attributes::DragScale);
    return s;
}

double Player::effective_gravity_scale() const {
    return m_attributes.value(Attributes::GravityScale) * (m_mode ? m_mode->gravity_scale() : 1.0);
}

double Player::effective_drag_scale() const {
    return m_attributes.value(Attributes::DragScale) * (m_mode ? m_mode->drag_scale(m_intent) : 1.0);
}

MovementIntent Player::limited(MovementIntent intent, bool fluid) const noexcept {
    if (!m_limits.can_sprint) { intent.sprint = false; intent.swim = false; }
    if (!m_limits.can_jump && !fluid) intent.jump = false;
    return intent;
}

void Player::weigh_down(vector3d& v, double rise_from, double submerged, double dt) const noexcept {
    if (v.z > rise_from && v.z > 0.0) v.z = rise_from + (v.z - rise_from) * m_limits.rise;
    v.z -= m_limits.sink * submerged * dt;
}

bool Player::fits(TickContext& ctx, Identifier pose) const {
    const PoseDimensions& d = m_body.get(pose);
    const double h = d.width * 0.5;
    const AABB box{ { m_position.x - h, m_position.y - h, m_position.z }, { m_position.x + h, m_position.y + h, m_position.z + d.height } };
    return !ctx.physics.collides(ctx.world, box);
}

bool Player::under_surface(const World& world, const vector3d& p) {
    const BlockPos cell = BlockPos::containing(p);
    const double h = world.fluid_height(cell);
    return h > 0.0 && p.z < cell.z + h;
}

} // namespace voxelspire
