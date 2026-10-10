#ifndef VOXELSPIRE_ENTITY_PHYSICS_HPP
#define VOXELSPIRE_ENTITY_PHYSICS_HPP

#include <cmath>
#include <vector>
#include "../core/limits/engine.hpp"
#include "../world/block_reader.hpp"
#include "entity.hpp"

namespace voxelspire {

struct CollisionResult {
    bool   hit_x                = false;
    bool   hit_y                = false;
    bool   hit_z                = false;
    bool   landed               = false;
    double landed_after_falling = 0.0;
};

class EntityPhysics {
public:
    explicit EntityPhysics(const WorldSettings& settings) : m_settings(settings.validated()) {}

    const WorldSettings& settings() const noexcept { return m_settings; }
    void set_settings(const WorldSettings& settings) { m_settings = settings.validated(); }

    void apply_gravity(Entity& e, double dt, double gravity_scale = 1.0, double drag_scale = 1.0) const;

    AirContext air_context(const Entity& e, double dt, double gravity_scale = 1.0, double drag_scale = 1.0) const noexcept;

    double terminal_velocity(const Entity& e, double gravity_scale = 1.0, double drag_scale = 1.0) const;

    double submerged_fraction(const Entity& e, const World& world) const;

        vector3d fluid_current(const Entity& e, const World& world) const;

    void apply_current(Entity& e, const World& world, double submerged, double dt) const;

    vector3d current_drift(const Entity& e, const World& world, double submerged) const;

    vector3d approach_drift(const vector3d& drift, const vector3d& target, double dt) const noexcept;

    void apply_falling_water(Entity& e, const World& world, double submerged, double dt) const;

    double wade_factor(double submerged) const noexcept {
        return vmax(0.0, 1.0 - vclamp(m_settings.wade_slowdown, 0.0, 1.0) * submerged);
    }

    double water_depth_below(const Entity& e, const World& world) const;

    double absorb_fall(Entity& e, const World& world, double submerged, bool was_submerged) const;

    double buoyancy_scale(double submerged) const noexcept { return vmax(0.0, 1.0 - vclamp(m_settings.fluid_buoyancy, 0.0, 1.0) * submerged); }

    void apply_fluid(Entity& e, double submerged, double free_horizontal, double free_up, double free_down, double dt) const;

    double ground_distance(const Entity& e, const World& world, double max_depth);

    bool collides(const World& world, const AABB& box) {
        gather(world, box);
        for (const AABB& b : m_boxes) if (b.intersects(box)) return true;
        return false;
    }

    CollisionResult move(Entity& e, const World& world, vector3d delta);

    bool is_supported(const Entity& e, const World& world);

private:
    static constexpr double MAX_STEP     = 0.45;
    static constexpr double EPS          = 1e-7;
    static constexpr double GROUND_PROBE = 0.01;
    static constexpr double SWEEP_MARGIN = 1e-3;
    static constexpr double CURRENT_GRIP = 2.0;

    static void push_along(vector3d& v, const vector3d& dir, double target, double gain) noexcept;

    static void track_fall(Entity& e, double moved_z) noexcept;

    double move_axis(Entity& e, int axis, double d);

    void gather(const World& world, const AABB& region);

    WorldSettings     m_settings;
    std::vector<AABB> m_boxes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_PHYSICS_HPP