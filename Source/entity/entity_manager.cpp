#include "entity/entity_manager.hpp"

namespace voxelspire {

bool SimulationArea::contains(const vector3d& p) const noexcept {
    const double dx = p.x - center.x, dy = p.y - center.y;
    if (dx * dx + dy * dy > radius * radius) return false;
    return !world || world->column_loaded(World::column_of(p));
}

Entity* EntityManager::find(EntityId id) const noexcept {
    auto it = m_by_id.find(id);
    return (it == m_by_id.end() || it->second->removed()) ? nullptr : it->second;
}

void EntityManager::tick(TickContext& ctx) {
    const std::size_t count = m_entities.size();

    for (std::size_t i = 0; i < count; ++i) {
        Entity& e = *m_entities[i];
        if (e.removed()) continue;
        e.m_simulated = !ctx.simulation || ctx.simulation->contains(e.position());
        if (e.m_simulated) e.tick(ctx);
    }

    despawn_fallen(ctx.world);
    flush_removed();
    rebuild_index();
    resolve_contacts(ctx);
}

void EntityManager::rebuild_index() {
    m_items.clear();
    m_boxes.clear();
    bool first = true;
    fizmo::physics::AABB3D bounds;

    for (const auto& owned : m_entities) {
        Entity& e = *owned;
        if (e.removed()) continue;
        const fizmo::physics::AABB3D box = to_fizmo(e.bounding_box());
        m_items.push_back(&e);
        m_boxes.push_back(box);
        if (first) { bounds = box; first = false; continue; }
        bounds.min = fizmo::physics::vec3::min(bounds.min, box.min);
        bounds.max = fizmo::physics::vec3::max(bounds.max, box.max);
    }

    const vector3d center = bounds.center();
    const vector3d size = bounds.max - bounds.min;
    const double half = vmax(size.x, vmax(size.y, size.z)) * 0.5 + m_settings.octree_margin;
    bounds.min = center - vector3d{ half, half, half };
    bounds.max = center + vector3d{ half, half, half };
    m_tree.configure(m_settings.octree_max_per_node, m_settings.octree_max_depth, m_settings.octree_looseness);
    m_tree.rebuild(bounds, m_items.data(), m_boxes.data(), m_items.size());
}

void EntityManager::query(const AABB& region, std::vector<Entity*>& out) const {
    m_scratch.clear();
    m_tree.query(to_fizmo(region), m_scratch);
    for (Entity* e : m_scratch) if (!e->removed() && e->bounding_box().intersects(region)) out.push_back(e);
}

void EntityManager::query_radius(const vector3d& center, double radius, std::vector<Entity*>& out) const {
    const AABB region{ center - vector3d{ radius, radius, radius }, center + vector3d{ radius, radius, radius } };
    m_scratch.clear();
    m_tree.query(to_fizmo(region), m_scratch);

    for (Entity* e : m_scratch) {
        if (e->removed()) continue;
        if (fizmo::physics::overlap_aabb_sphere(to_fizmo(e->bounding_box()), center, radius)) out.push_back(e);
    }
}

void EntityManager::despawn_fallen(const World& world) noexcept {
    const double void_z = world.settings().min_z - world.settings().void_depth;
    for (const auto& e : m_entities) if (e->despawns_in_void() && e->position().z < void_z) remove(*e);
}

void EntityManager::resolve_contacts(TickContext& ctx) {
    m_contacts = 0;
        
    m_tree.find_pairs([&](Entity* a, Entity* b) {
        if (a->removed() || b->removed()) return;
        if (!a->simulated() || !b->simulated()) return;
        if (!a->collision().collides_with_entities || !b->collision().collides_with_entities) return;
        if (!a->bounding_box().intersects(b->bounding_box())) return;
        ++m_contacts;
        a->on_touch(*b, ctx);
        b->on_touch(*a, ctx);
        push_apart(*a, *b, ctx.dt);
    });
}

void EntityManager::push_apart(Entity& a, Entity& b, double dt) const noexcept {
    const bool a_moves = a.collision().pushable, b_moves = b.collision().pushable;
    if (!a_moves && !b_moves) return;
    const AABB ba = a.bounding_box(), bb = b.bounding_box();
    const double overlap_x = vmin(ba.max.x, bb.max.x) - vmax(ba.min.x, bb.min.x);
    const double overlap_y = vmin(ba.max.y, bb.max.y) - vmax(ba.min.y, bb.min.y);
    const double narrowest = vmin(a.width(), b.width());
    if (narrowest <= 0.0) return;
    const double depth = vclamp(vmin(overlap_x, overlap_y) / narrowest, 0.0, 1.0);
    vector3d dir = bb.center() - ba.center();
    dir.z = 0.0;
    double len = dir.magnitude();

    if (len < SEPARATION_EPS) {
        const double angle = static_cast<double>((a.id() * GOLDEN_STEP + b.id()) % FULL_TURN_DEGREES);
        dir = { std::cos(deg_to_rad(angle)), std::sin(deg_to_rad(angle)), 0.0 };
        len = 1.0;
    }

    dir = dir / len;
    const double impulse = m_settings.push_acceleration * depth * dt;
    const double wa = a_moves ? vmax(a.collision().push_weight, 0.0) : 0.0;
    const double wb = b_moves ? vmax(b.collision().push_weight, 0.0) : 0.0;
    const double total = wa + wb;
    if (total <= 0.0) return;
    if (a_moves) nudge(a, dir * -1.0, impulse * (b_moves ? wb / total : 1.0));
    if (b_moves) nudge(b, dir,        impulse * (a_moves ? wa / total : 1.0));
}

void EntityManager::nudge(Entity& e, const vector3d& dir, double amount) const noexcept {
    vector3d v = e.velocity();
    const double along = v.x * dir.x + v.y * dir.y;
    const double room = m_settings.max_push_speed - along;
    if (room <= 0.0) return;
    const double applied = vmin(amount, room);
    v.x += dir.x * applied;
    v.y += dir.y * applied;
    e.set_velocity(v);
}

void EntityManager::flush_removed() {
    for (const auto& e : m_entities) if (e->removed()) m_by_id.erase(e->id());

    m_entities.erase(
        std::remove_if(
            m_entities.begin(), m_entities.end(),
            [](const std::unique_ptr<Entity>& e) { return e->removed(); }
        ),
        m_entities.end()
    );
}

} // namespace voxelspire
