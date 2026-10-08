#ifndef VOXELSPIRE_SURVIVAL_DAMAGE_HPP
#define VOXELSPIRE_SURVIVAL_DAMAGE_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "../core/identifier.hpp"
#include "../entity/entity.hpp"

namespace voxelspire {

namespace DamageTypes {
    inline const Identifier Generic     = core_id(Kind::Damage, "generic");
    inline const Identifier Fall        = core_id(Kind::Damage, "fall");
    inline const Identifier Starving    = core_id(Kind::Damage, "starving");
    inline const Identifier Dehydration = core_id(Kind::Damage, "dehydration");
    inline const Identifier Exhaustion  = core_id(Kind::Damage, "exhaustion");
    inline const Identifier Drowning    = core_id(Kind::Damage, "drowning");
    inline const Identifier Freezing    = core_id(Kind::Damage, "freezing");
    inline const Identifier Overheating = core_id(Kind::Damage, "overheating");
    inline const Identifier Void        = core_id(Kind::Damage, "void");
} // namespace DamageTypes

struct DamageType {
    Identifier  id;
    std::string name;
    std::string death_message;
    bool        ignores_scale = false;
};

class DamageTypeRegistry {
public:
    const DamageType& add(DamageType type) {
        if (!type.id) throw std::runtime_error("damage type needs an id");
        if (m_by_id.contains(type.id)) throw std::runtime_error("damage type already registered: " + type.id.str());
        auto owned = std::make_unique<DamageType>(std::move(type));
        const DamageType& ref = *owned;
        m_by_id.emplace(ref.id, std::move(owned));
        return ref;
    }

    const DamageType* find(Identifier id) const noexcept {
        const auto* slot = m_by_id.find(id);
        return slot ? slot->get() : nullptr;
    }

    const DamageType& get(Identifier id) const {
        if (const DamageType* t = find(id)) return *t;
        if (const DamageType* t = find(DamageTypes::Generic)) return *t;
        throw std::runtime_error("unknown damage type: " + id.str());
    }

    static void register_defaults(DamageTypeRegistry& r) {
        r.add({ DamageTypes::Generic,     "Damage",      "You died" });
        r.add({ DamageTypes::Fall,        "Fall",        "You hit the ground too hard" });
        r.add({ DamageTypes::Starving,    "Starving",    "You starved" });
        r.add({ DamageTypes::Dehydration, "Thirst",      "You died of thirst" });
        r.add({ DamageTypes::Exhaustion,  "Exhaustion",  "You pushed yourself too far" });
        r.add({ DamageTypes::Drowning,    "Drowning",    "You drowned" });
        r.add({ DamageTypes::Freezing,    "Freezing",    "You froze to death" });
        r.add({ DamageTypes::Overheating, "Overheating", "You overheated" });
        r.add({ DamageTypes::Void,        "Void",        "You fell out of the world", true });
    }

private:
    IdentifierTable<std::unique_ptr<DamageType>> m_by_id;
};

struct Damage {
    Identifier type      = DamageTypes::Generic;
    double     amount    = 0.0;
    double     floor     = 0.0;
    bool       has_place = false;
    vector3d   place{};
    EntityId   source    = NO_ENTITY;

    static Damage of(Identifier type, double amount, double floor = 0.0) {
        Damage d;
        d.type   = type;
        d.amount = amount;
        d.floor  = floor;
        return d;
    }
};

class DamagePipeline {
public:
    using Step = std::function<void(Damage&, const Entity& target)>;

    void add(Identifier id, int order, Step step) {
        remove(id);
        m_steps.push_back({ std::move(id), order, std::move(step) });
        std::stable_sort(m_steps.begin(), m_steps.end(), [](const Entry& a, const Entry& b) { return a.order < b.order; });
    }

    bool remove(const Identifier& id) {
        auto it = std::find_if(m_steps.begin(), m_steps.end(), [&](const Entry& e) { return e.id == id; });
        if (it == m_steps.end()) return false;
        m_steps.erase(it);
        return true;
    }

    Damage run(Damage d, const Entity& target) const {
        for (const Entry& e : m_steps) {
            if (d.amount <= 0.0) break;
            e.step(d, target);
        }

        d.amount = d.amount > 0.0 ? d.amount : 0.0;
        return d;
    }

    std::size_t size() const noexcept { return m_steps.size(); }

private:
    struct Entry {
        Identifier id;
        int        order = 0;
        Step       step;
    };

    std::vector<Entry> m_steps;
};

struct DamageRecord {
    Identifier type;
    double     amount = 0.0;
    double     time   = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_SURVIVAL_DAMAGE_HPP