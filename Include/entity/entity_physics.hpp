#ifndef VOXELSPIRE_ENTITY_PHYSICS_HPP
#define VOXELSPIRE_ENTITY_PHYSICS_HPP

#include <cmath>
#include <vector>
#include "../world/block_reader.hpp"
#include "entity.hpp"

namespace voxelspire {

struct CollisionResult {
    bool hit_x = false, hit_y = false, hit_z = false;
    bool   landed = false;
    double landed_after_falling = 0.0;
};

class EntityPhysics {
public:
    explicit EntityPhysics(const WorldSettings& settings) : m_settings(settings.validated()) {}

    const WorldSettings& settings() const noexcept { return m_settings; }
    void set_settings(const WorldSettings& settings) { m_settings = settings.validated(); }

    void apply_gravity(Entity& e, double dt, double gravity_scale = 1.0, double drag_scale = 1.0) const {
        if (gravity_scale == 0.0) return;
        const AirContext air = air_context(e, dt, gravity_scale, drag_scale);
        vector3d v = e.velocity();
        v.z = m_settings.air_resistance->apply(v.z - air.gravity * dt, air);
        e.set_velocity(v);
    }

    AirContext air_context(const Entity& e, double dt, double gravity_scale = 1.0, double drag_scale = 1.0) const noexcept {
        return { m_settings.gravity * gravity_scale, dt, m_settings.fall_height(), e.drag_factor() * vmax(drag_scale, 0.0) };
    }

    double terminal_velocity(const Entity& e, double gravity_scale = 1.0, double drag_scale = 1.0) const {
        return m_settings.air_resistance->terminal_velocity(air_context(e, 0.0, gravity_scale, drag_scale));
    }

    double submerged_fraction(const Entity& e, const World& world) const {
        const AABB box = e.bounding_box();
        const double height = box.max.z - box.min.z;
        if (height <= 0.0) return 0.0;
        const vector3d center = box.center();
        const int x = floor_to_int(center.x), y = floor_to_int(center.y);
        double inside = 0.0;

        for (int z = floor_to_int(box.min.z); z <= floor_to_int(box.max.z - EPS); ++z) {
            if (!world.traits_at({ x, y, z }).fluid) continue;
            inside += vmin(box.max.z, z + 1.0) - vmax(box.min.z, static_cast<double>(z));
        }

        return vclamp(inside / height, 0.0, 1.0);
    }

    double buoyancy_scale(double submerged) const noexcept { return vmax(0.0, 1.0 - vclamp(m_settings.fluid_buoyancy, 0.0, 1.0) * submerged); }

    void apply_fluid(Entity& e, double submerged, double free_horizontal, double free_up, double free_down, double dt) const {
        FluidContext ctx;
        ctx.dt        = dt;
        ctx.submerged = submerged;
        if (ctx.submerged <= 0.0) return;
        free_down = vmin(free_down, m_settings.fluid_sink_speed);
        const AABB box = e.bounding_box();
        ctx.width  = box.max.x - box.min.x;
        ctx.height = box.max.z - box.min.z;
        ctx.free_speed_horizontal = free_horizontal;
        ctx.free_speed_up         = free_up;
        ctx.free_speed_down       = free_down;
        e.set_velocity(m_settings.fluid_resistance->apply(e.velocity(), ctx));
    }

    double ground_distance(const Entity& e, const World& world, double max_depth) {
        const AABB box = e.bounding_box();
        const double feet = box.min.z;
        gather(world, { { box.min.x, box.min.y, feet - max_depth }, { box.max.x, box.max.y, feet } });
        double best = max_depth;

        for (const AABB& b : m_boxes) {
            if (!box.overlaps_on(0, b) || !box.overlaps_on(1, b)) continue;
            if (b.max.z > feet + EPS) continue;
            best = vmin(best, feet - b.max.z);
        }

        return vmax(best, 0.0);
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
        const double start_z = e.position().z;
        const AABB start = e.bounding_box();
        gather(world, start.united(start.translated(delta)).inflated(SWEEP_MARGIN));

        for (int i = 0; i < steps; ++i) {
            const double want_z = d.z;
            const double got_z = move_axis(e, 2, want_z);
            if (got_z != want_z) { r.hit_z = true; if (want_z < 0.0) r.landed = true; }
            if (move_axis(e, 0, d.x) != d.x) r.hit_x = true;
            if (move_axis(e, 1, d.y) != d.y) r.hit_y = true;
        }

        vector3d v = e.velocity();
        if (r.hit_x) v.x = 0.0;
        if (r.hit_y) v.y = 0.0;
        if (r.hit_z) v.z = 0.0;
        e.set_velocity(v);
        e.set_on_ground(r.landed || (r.hit_z && delta.z < 0.0) || (delta.z == 0.0 && is_supported(e, world)));
        const double fallen = e.fall_distance() + vmax(start_z - e.position().z, 0.0);
        track_fall(e, e.position().z - start_z);
        if (e.on_ground() && fallen > 0.0) r.landed_after_falling = fallen;
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
    static constexpr double SWEEP_MARGIN = 1e-3;

    static void track_fall(Entity& e, double moved_z) noexcept {
        if (e.on_ground() || moved_z > 0.0) { e.set_fall_distance(0.0); return; }
        e.set_fall_distance(e.fall_distance() - moved_z);
    }

    double move_axis(Entity& e, int axis, double d) {
        if (d == 0.0) return 0.0;
        const AABB box = e.bounding_box();
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

        BlockReader reader(world);
        const BlockRegistry& blocks = world.blocks();

        for (int y = y0; y <= y1; ++y)
        for (int z = z0; z <= z1; ++z)
        for (int x = x0; x <= x1; ++x) {
            const BlockPos p{ x, y, z };
            if (!world.in_horizontal_bounds(p)) { m_boxes.push_back(AABB::unit_block(p)); continue; }
            const BlockId id = reader.id_at(p);
            if (blocks.traits(id).solid) m_boxes.push_back(reader.collision_box(p, id));
        }
    }

    WorldSettings     m_settings;
    std::vector<AABB> m_boxes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_PHYSICS_HPP