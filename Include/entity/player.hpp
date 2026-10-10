#ifndef VOXELSPIRE_ENTITY_PLAYER_HPP
#define VOXELSPIRE_ENTITY_PLAYER_HPP

#include <cmath>
#include "attributes.hpp"
#include "entity_body.hpp"
#include "entity_physics.hpp"
#include "movement_mode.hpp"

namespace voxelspire {

struct MovementReport {
    Identifier activity;
    bool       moving      = false;
    bool       jumped      = false;
    bool       straining   = false;
    bool       in_fluid    = false;
    bool       head_under  = false;
    double     landed_fall = 0.0;
};

class Player final : public Entity {
public:
    Player(const AttributeRegistry& attributes, const MovementModeRegistry& modes, EntityBody body = EntityBody::player())
;

    AttributeMap&       attributes()       noexcept { return m_attributes; }
    const AttributeMap& attributes() const noexcept { return m_attributes; }
    const EntityBody&   body()       const noexcept { return m_body; }

    const MovementMode* movement_mode() const noexcept { return m_mode; }

    void forget_movement_mode() noexcept { m_mode = nullptr; }

    void set_body(EntityBody body) {
        m_body = std::move(body);
        apply_pose(m_body.has(m_pose) ? m_pose : Poses::Standing);
    }

    Identifier pose() const noexcept { return m_pose; }

    double eye_height() const noexcept { return m_eye_height; }
    double eye_height(double alpha) const noexcept { return m_prev_eye_height + (m_eye_height - m_prev_eye_height) * alpha; }
    vector3d eye_position() const noexcept { return m_position + vector3d{ 0.0, 0.0, m_eye_height }; }
    vector3d eye_position(double alpha) const noexcept { return interpolated_position(alpha) + vector3d{ 0.0, 0.0, eye_height(alpha) }; }

    double capsule_radius() const noexcept { return m_width * 0.5; }

    bool despawns_in_void() const noexcept override { return false; }

    bool casts_capsule_shadow(fizmo::graphics::CapsuleOccluder3D& out, double alpha) const override;

    void set_intent(const MovementIntent& intent) noexcept { m_intent = intent; }
    void set_limits(const MovementLimits& limits) noexcept { m_limits = limits; }

    const MovementLimits& limits() const noexcept { return m_limits; }
    const MovementReport& report() const noexcept { return m_report; }
    const MovementIntent& intent() const noexcept { return m_intent; }

    void respawn(const vector3d& feet) noexcept;

    void tick(TickContext& ctx) override;

    MovementStats movement_stats() const;

    double effective_gravity_scale() const;

    double effective_drag_scale() const;

private:
    static constexpr double EYE_SMOOTHING = 30.0;

    void apply_pose(Identifier pose) noexcept {
        const PoseDimensions& d = m_body.get(pose);
        set_dimensions(d.width, d.height);
        m_pose = pose;
    }

    MovementIntent limited(MovementIntent intent, bool fluid) const noexcept;

    void weigh_down(vector3d& v, double rise_from, double submerged, double dt) const noexcept;

    bool fits(TickContext& ctx, Identifier pose) const;

    static bool under_surface(const World& world, const vector3d& p);

    bool head_in_fluid(const World& world) const { return under_surface(world, eye_position()); }
    bool in_fluid(const World& world) const { return under_surface(world, m_position + vector3d{ 0.0, 0.0, m_height * 0.5 }); }

    AttributeMap                m_attributes;
    const MovementModeRegistry* m_modes;
    EntityBody                  m_body;
    MovementIntent              m_intent;
    MovementLimits              m_limits;
    MovementReport              m_report;
    const MovementMode*         m_mode = nullptr;
    Identifier                  m_pose = Poses::Standing;
    double                      m_eye_height;
    double                      m_prev_eye_height;
    bool                        m_was_submerged  = false;
    vector3d                    m_drift{};
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_PLAYER_HPP