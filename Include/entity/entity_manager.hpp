#ifndef VOXELSPIRE_ENTITY_ENTITY_MANAGER_HPP
#define VOXELSPIRE_ENTITY_ENTITY_MANAGER_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../core/settings.hpp"
#include "../world/world.hpp"
#include "entity.hpp"

namespace voxelspire {



class EntityManager {
public:
    using EntityList = std::vector<std::unique_ptr<Entity>>;

    explicit EntityManager(const EntitySettings& settings = EntitySettings{}) : m_settings(settings) {}

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;

    const EntitySettings& settings() const noexcept { return m_settings; }
    EntitySettings&       settings()       noexcept { return m_settings; }

    template <typename T, typename... Args>
    T& spawn(Args&&... args) {
        static_assert(std::is_base_of<Entity, T>::value, "EntityManager::spawn requires an Entity subclass");
        auto owned = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *owned;
        ref.m_id = ++m_last_id;
        m_by_id.emplace(ref.m_id, &ref);
        m_entities.push_back(std::move(owned));
        return ref;
    }

    void remove(Entity& e) noexcept { e.m_removed = true; }

    void remove(EntityId id) noexcept { if (Entity* e = find(id)) remove(*e); }

    Entity* find(EntityId id) const noexcept;

    std::size_t size() const noexcept { return m_entities.size(); }
    const EntityList& entities() const noexcept { return m_entities; }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (const auto& e : m_entities) if (!e->removed()) fn(*e);
    }

    void tick(TickContext& ctx);

    void rebuild_index();

    void query(const AABB& region, std::vector<Entity*>& out) const;

    std::vector<Entity*> query(const AABB& region) const {
        std::vector<Entity*> out;
        query(region, out);
        return out;
    }

    void query_radius(const vector3d& center, double radius, std::vector<Entity*>& out) const;

    std::vector<Entity*> query_radius(const vector3d& center, double radius) const {
        std::vector<Entity*> out;
        query_radius(center, radius, out);
        return out;
    }

    std::size_t last_contact_count() const noexcept { return m_contacts; }
    std::size_t index_node_count()   const noexcept { return m_tree.node_count(); }
    std::size_t index_depth()        const noexcept { return m_tree.depth(); }

private:
    static fizmo::physics::AABB3D to_fizmo(const AABB& b) noexcept { return { b.min, b.max }; }

    void despawn_fallen(const World& world) noexcept;

    void resolve_contacts(TickContext& ctx);

    void push_apart(Entity& a, Entity& b, double dt) const noexcept;

    void nudge(Entity& e, const vector3d& dir, double amount) const noexcept;

    void flush_removed();

    static constexpr double        SEPARATION_EPS    = 1e-6;
    static constexpr std::uint64_t GOLDEN_STEP       = 137;
    static constexpr std::uint64_t FULL_TURN_DEGREES = 360;

    EntitySettings                            m_settings;
    EntityList                                m_entities;
    std::unordered_map<EntityId, Entity*>     m_by_id;
    EntityId                                  m_last_id = NO_ENTITY;
    fizmo::physics::Octree<Entity>            m_tree;
    std::vector<Entity*>                      m_items;
    std::vector<fizmo::physics::AABB3D>       m_boxes;
    mutable std::vector<Entity*>              m_scratch;
    std::size_t                               m_contacts = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_ENTITY_MANAGER_HPP