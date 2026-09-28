#ifndef VOXELSPIRE_ENTITY_PHYSICS_HPP
#define VOXELSPIRE_ENTITY_PHYSICS_HPP

#include <cmath>
#include <vector>
#include "../world/world.hpp"
#include "entity.hpp"

namespace voxelspire {

struct CollisionResult {
    bool hit_x = false, hit_y = false, hit_z = false;
    bool landed = false;
};

class EntityPhysics {
public:
    explicit EntityPhysics(const WorldSettings& settings) noexcept : m_settings(settings) {}

    const WorldSettings& settings() const noexcept { return m_settings; }

    void apply_gravity(Entity& e, double dt, double scale = 1.0) const noexcept {
        if (scale == 0.0) return;
        vector3d v = e.velocity();
        v.z = vmax(v.z - m_settings.gravity * scale * dt, -m_settings.terminal_velocity);
        e.set_velocity(v);
    }

    bool collides(const World& world, const AABB& box) {
        gather(world, box);
        for (const AABB& b : m_boxes) if (b.intersects(box)) return true;
        return false;
    }

    CollisionResult move(Entity& e, const World& world, const vector3d& delta) {
        CollisionResult r;
        const double largest = vmax(std::fabs(delta.x), vmax(std::fabs(delta.y), std::fabs(delta.z)));
        const int steps = vmax(1, static_cast<int>(std::ceil(largest / MAX_STEP)));
        const vector3d d = delta / static_cast<double>(steps);

        for (int i = 0; i < steps; ++i) {
            const double want_z = d.z;
            const double got_z = move_axis(e, world, 2, want_z);
            if (got_z != want_z) { r.hit_z = true; if (want_z < 0.0) r.landed = true; }
            if (move_axis(e, world, 0, d.x) != d.x) r.hit_x = true;
            if (move_axis(e, world, 1, d.y) != d.y) r.hit_y = true;
        }

        vector3d v = e.velocity();
        if (r.hit_x) v.x = 0.0;
        if (r.hit_y) v.y = 0.0;
        if (r.hit_z) v.z = 0.0;
        e.set_velocity(v);
        e.set_on_ground(r.landed || (r.hit_z && delta.z < 0.0) || (delta.z == 0.0 && is_supported(e, world)));
        return r;
    }

    bool is_supported(const Entity& e, const World& world) {
        const AABB probe = e.bounding_box().translated({ 0.0, 0.0, -GROUND_PROBE });
        gather(world, probe);
        for (const AABB& b : m_boxes) if (b.intersects(probe)) return true;
        return false;
    }

private:
    static constexpr double MAX_STEP     = 0.45;
    static constexpr double EPS          = 1e-7;
    static constexpr double GROUND_PROBE = 0.01;

    double move_axis(Entity& e, const World& world, int axis, double d) {
        if (d == 0.0) return 0.0;
        const AABB box = e.bounding_box();
        vector3d shift{};
        set_component(shift, axis, d);
        gather(world, box.united(box.translated(shift)));

        double allowed = d;
        for (const AABB& b : m_boxes) {
            bool overlaps_others = true;
            for (int o = 0; o < 3; ++o) if (o != axis && !box.overlaps_on(o, b)) overlaps_others = false;
            if (!overlaps_others) continue;

            if (allowed > 0.0 && component(b.min, axis) >= component(box.max, axis) - EPS) {
                allowed = vmin(allowed, component(b.min, axis) - component(box.max, axis));
            } else if (allowed < 0.0 && component(b.max, axis) <= component(box.min, axis) + EPS) {
                allowed = vmax(allowed, component(b.max, axis) - component(box.min, axis));
            }
        }

        if (std::fabs(allowed) < EPS) allowed = 0.0;
        vector3d move{};
        set_component(move, axis, allowed);
        e.move_by(move);
        return allowed;
    }

    void gather(const World& world, const AABB& region) {
        m_boxes.clear();
        const int x0 = floor_to_int(region.min.x - EPS), x1 = floor_to_int(region.max.x + EPS);
        const int y0 = floor_to_int(region.min.y - EPS), y1 = floor_to_int(region.max.y + EPS);
        const int z0 = floor_to_int(region.min.z - EPS), z1 = floor_to_int(region.max.z + EPS);

        for (int y = y0; y <= y1; ++y)
        for (int z = z0; z <= z1; ++z)
        for (int x = x0; x <= x1; ++x) {
            const BlockPos p{ x, y, z };
            const Block& b = world.block_at(p);
            if (b.is_solid()) m_boxes.push_back(b.collision_box(p));
        }
    }

    WorldSettings     m_settings;
    std::vector<AABB> m_boxes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_PHYSICS_HPP