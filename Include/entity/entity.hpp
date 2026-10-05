#ifndef VOXELSPIRE_ENTITY_ENTITY_HPP
#define VOXELSPIRE_ENTITY_ENTITY_HPP

#include <cmath>
#include <cstdint>
#include "../core/types.hpp"
#include "../lighting/dynamic_light.hpp"
#include "../physics/aerodynamics.hpp"

namespace voxelspire {

class World;
class EntityPhysics;
class EntityManager;

using EntityId = std::uint64_t;
constexpr EntityId NO_ENTITY = 0;

struct SimulationArea {
    vector3d center{};
    double   radius = 0.0;
    const World* world = nullptr;

    bool contains(const vector3d& p) const noexcept;
};

struct TickContext {
    World&                world;
    EntityPhysics&        physics;
    EntityManager&        entities;
    double                dt;
    const SimulationArea* simulation = nullptr;
};

struct EntityCollision {
    bool   collides_with_entities = true;
    bool   pushable               = true;
    double push_weight            = 1.0;
};

class Entity {
public:
    Entity(double width, double height) noexcept : m_width(width), m_height(height) {}
    virtual ~Entity() = default;

    virtual void tick(TickContext& ctx) = 0;
    virtual void on_touch(Entity&, TickContext&) {}
    virtual bool despawns_in_void() const noexcept { return true; }
    virtual bool emits_light(DynamicLight& /*out*/, double /*alpha*/) const { return false; }
    virtual bool casts_capsule_shadow(fizmo::graphics::CapsuleOccluder3D&, double) const { return false; }

    EntityId id()        const noexcept { return m_id; }
    bool     removed()   const noexcept { return m_removed; }
    bool     simulated() const noexcept { return m_simulated; }

    const EntityCollision& collision() const noexcept { return m_collision; }
    void set_collision(const EntityCollision& c) noexcept { m_collision = c; }

    const Aerodynamics& aerodynamics() const noexcept { return m_aerodynamics; }
    void set_aerodynamics(const Aerodynamics& a) noexcept { m_aerodynamics = a; }
    double drag_factor() const noexcept { return m_aerodynamics.drag_factor(m_height); }

    double fall_distance() const noexcept { return m_fall_distance; }
    void set_fall_distance(double d) noexcept { m_fall_distance = vmax(d, 0.0); }

    static constexpr double MAX_PITCH = 89.9;

public:
    const vector3d& position()          const noexcept { return m_position; }
    const vector3d& previous_position() const noexcept { return m_prev_position; }
    const vector3d& velocity()          const noexcept { return m_velocity; }

    void set_position(const vector3d& p) noexcept { m_position = p; m_prev_position = p; }
    void move_by(const vector3d& d) noexcept      { m_position += d; }
    void set_velocity(const vector3d& v) noexcept { m_velocity = v; }

    void begin_tick() noexcept { m_prev_position = m_position; }
    vector3d interpolated_position(double alpha) const noexcept { return lerp(m_prev_position, m_position, alpha); }

public:
    static constexpr double DEFAULT_DENSITY = 985.0;

    virtual double mass() const noexcept { return m_mass > 0.0 ? m_mass : m_width * m_width * m_height * DEFAULT_DENSITY; }
    void set_mass(double kilograms) noexcept { m_mass = vmax(kilograms, 0.0); }

    double width()  const noexcept { return m_width; }
    double height() const noexcept { return m_height; }

    void set_dimensions(double width, double height) noexcept { m_width = width; m_height = height; }

    AABB bounding_box() const noexcept { return bounding_box_at(m_position); }
    AABB bounding_box_at(const vector3d& feet) const noexcept {
        const double h = m_width * 0.5;
        return { { feet.x - h, feet.y - h, feet.z }, { feet.x + h, feet.y + h, feet.z + m_height } };
    }

    bool on_ground() const noexcept { return m_on_ground; }
    void set_on_ground(bool v) noexcept { m_on_ground = v; }

public:
    double yaw()   const noexcept { return m_yaw; }
    double pitch() const noexcept { return m_pitch; }

    void set_look(double yaw_degrees, double pitch_degrees) noexcept {
        m_yaw = std::remainder(yaw_degrees, 360.0);
        m_pitch = vclamp(pitch_degrees, -MAX_PITCH, MAX_PITCH);
    }

    void add_look(double dyaw, double dpitch) noexcept { set_look(m_yaw + dyaw, m_pitch + dpitch); }

    vector3d look_direction() const noexcept {
        const double y = deg_to_rad(m_yaw), p = deg_to_rad(m_pitch);
        return { -std::sin(y) * std::cos(p), std::cos(y) * std::cos(p), std::sin(p) };
    }

    vector3d flat_forward() const noexcept { const double y = deg_to_rad(m_yaw); return { -std::sin(y), std::cos(y), 0.0 }; }
    vector3d flat_right()   const noexcept { const double y = deg_to_rad(m_yaw); return {  std::cos(y), std::sin(y), 0.0 }; }

protected:
    EntityCollision m_collision;
    Aerodynamics    m_aerodynamics;
    double          m_fall_distance = 0.0;
    vector3d m_position{};
    vector3d m_prev_position{};
    vector3d m_velocity{};
    double   m_width;
    double   m_height;
    bool     m_on_ground   = false;
    double   m_yaw         = 0.0;
    double   m_pitch       = 0.0;
    double   m_mass        = 0.0;

private:
    friend class EntityManager;
    EntityId m_id      = NO_ENTITY;
    bool     m_removed = false;
    bool     m_simulated = true;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_ENTITY_HPP