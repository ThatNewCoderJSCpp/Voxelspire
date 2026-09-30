#ifndef VOXELSPIRE_ENTITY_PLAYER_HPP
#define VOXELSPIRE_ENTITY_PLAYER_HPP

#include <cmath>
#include "attributes.hpp"
#include "entity_body.hpp"
#include "entity_physics.hpp"
#include "movement_mode.hpp"

namespace voxelspire {

class Player final : public Entity {
public:
    Player(const AttributeRegistry& attributes, const MovementModeRegistry& modes, EntityBody body = EntityBody::player())
        : Entity(body.standing().width, body.standing().height),
          m_attributes(attributes), m_modes(&modes), m_body(std::move(body)),
          m_eye_height(m_body.standing().eye_height), m_prev_eye_height(m_eye_height) {}

    AttributeMap&       attributes()       noexcept { return m_attributes; }
    const AttributeMap& attributes() const noexcept { return m_attributes; }
    const EntityBody&   body()       const noexcept { return m_body; }

    const MovementMode* movement_mode() const noexcept { return m_mode; }
    Identifier          pose()          const noexcept { return m_pose; }

    double eye_height() const noexcept { return m_eye_height; }
    double eye_height(double alpha) const noexcept { return m_prev_eye_height + (m_eye_height - m_prev_eye_height) * alpha; }
    vector3d eye_position() const noexcept { return m_position + vector3d{ 0.0, 0.0, m_eye_height }; }
    vector3d eye_position(double alpha) const noexcept { return interpolated_position(alpha) + vector3d{ 0.0, 0.0, eye_height(alpha) }; }

    double capsule_radius() const noexcept { return m_width * 0.5; }

    bool despawns_in_void() const noexcept override { return false; }

    static constexpr double HAND_FORWARD = 0.45;
    static constexpr double HAND_DROP    = 0.35;

    void set_hand_light(const DynamicLight* light) {
        m_has_hand_light = light != nullptr;
        if (light) m_hand_light = *light;
    }

    bool has_hand_light() const noexcept { return m_has_hand_light; }
    const DynamicLight& hand_light() const noexcept { return m_hand_light; }

    bool emits_light(DynamicLight& out, double alpha) const override {
        if (!m_has_hand_light) return false;
        out = m_hand_light;
        out.position = eye_position(alpha) + flat_forward() * HAND_FORWARD - vector3d{ 0.0, 0.0, HAND_DROP };
        return true;
    }

    void set_intent(const MovementIntent& intent) noexcept { m_intent = intent; }
    const MovementIntent& intent() const noexcept { return m_intent; }

    void respawn(const vector3d& feet) noexcept {
        set_position(feet);
        set_velocity({});
        set_on_ground(false);
        apply_pose(Poses::Standing);
        m_eye_height = m_prev_eye_height = m_body.standing().eye_height;
    }

    void tick(TickContext& ctx) override {
        begin_tick();
        m_prev_eye_height = m_eye_height;

        auto fits_pose = [&](Identifier pose) { return fits(ctx, pose); };
        auto ground    = [&](double max_depth) { return ctx.physics.ground_distance(*this, ctx.world, max_depth); };
        const MovementContext mctx{ m_intent, ctx.world, m_on_ground, in_fluid(ctx.world), fits_pose, m_velocity.z, m_fall_distance, ground,
                                    head_in_fluid(ctx.world), look_direction(), flat_right() };

        m_mode = m_modes->select(mctx);
        if (!m_mode) return;
        if (m_mode->pose() != m_pose && fits(ctx, m_mode->pose())) apply_pose(m_mode->pose());
        vector3d wish = flat_forward() * m_intent.forward + flat_right() * m_intent.strafe;
        const double len = wish.magnitude();
        if (len > 1.0) wish = wish / len;
        const MovementStats stats = movement_stats();
        const double submerged = ctx.physics.submerged_fraction(*this, ctx.world);
        ctx.physics.apply_fluid(*this, submerged, stats.speed * m_mode->speed_multiplier(m_intent), m_mode->rising_speed_limit(stats, m_intent),
                                m_mode->sinking_speed_limit(stats, m_intent), ctx.dt);
        vector3d v = m_velocity;
        m_mode->update_velocity(v, wish, mctx, stats, ctx.dt);
        if (v.z > 0.0 && m_on_ground) m_on_ground = false;
        m_velocity = v;
        ctx.physics.apply_gravity(*this, ctx.dt, effective_gravity_scale() * ctx.physics.buoyancy_scale(submerged), effective_drag_scale());
        ctx.physics.move(*this, ctx.world, m_velocity * ctx.dt);
        const double target_eye = m_body.get(m_pose).eye_height;
        m_eye_height += (target_eye - m_eye_height) * vmin(1.0, EYE_SMOOTHING * ctx.dt);
        if (std::fabs(target_eye - m_eye_height) < 1e-4) m_eye_height = target_eye;
    }

    MovementStats movement_stats() const {
        MovementStats s;
        s.speed         = m_attributes.value(Attributes::MovementSpeed);
        s.ground_accel  = m_attributes.value(Attributes::GroundAcceleration);
        s.air_accel     = m_attributes.value(Attributes::AirAcceleration);
        s.jump_velocity = m_attributes.value(Attributes::JumpVelocity);
        s.gravity_scale = m_attributes.value(Attributes::GravityScale);
        s.drag_scale    = m_attributes.value(Attributes::DragScale);
        return s;
    }

    double effective_gravity_scale() const {
        return m_attributes.value(Attributes::GravityScale) * (m_mode ? m_mode->gravity_scale() : 1.0);
    }

    double effective_drag_scale() const {
        return m_attributes.value(Attributes::DragScale) * (m_mode ? m_mode->drag_scale(m_intent) : 1.0);
    }

private:
    static constexpr double EYE_SMOOTHING = 30.0;

    void apply_pose(Identifier pose) noexcept {
        const PoseDimensions& d = m_body.get(pose);
        set_dimensions(d.width, d.height);
        m_pose = pose;
    }

    bool fits(TickContext& ctx, Identifier pose) const {
        const PoseDimensions& d = m_body.get(pose);
        const double h = d.width * 0.5;
        const AABB box{ { m_position.x - h, m_position.y - h, m_position.z }, { m_position.x + h, m_position.y + h, m_position.z + d.height } };
        return !ctx.physics.collides(ctx.world, box);
    }

    bool head_in_fluid(const World& world) const {
        return world.traits_at(BlockPos::containing(eye_position())).fluid;
    }

    bool in_fluid(const World& world) const {
        const vector3d center = m_position + vector3d{ 0.0, 0.0, m_height * 0.5 };
        return world.traits_at(BlockPos::containing(center)).fluid;
    }

    AttributeMap                m_attributes;
    const MovementModeRegistry* m_modes;
    EntityBody                  m_body;
    MovementIntent              m_intent;
    const MovementMode*         m_mode = nullptr;
    Identifier                  m_pose = Poses::Standing;
    double                      m_eye_height;
    double                      m_prev_eye_height;
    DynamicLight                m_hand_light;
    bool                        m_has_hand_light = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_PLAYER_HPP