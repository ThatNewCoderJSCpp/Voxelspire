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
    const DamageType& add(DamageType type);

    const DamageType* find(Identifier id) const noexcept {
        const auto* slot = m_by_id.find(id);
        return slot ? slot->get() : nullptr;
    }

    const DamageType& get(Identifier id) const;

    static void register_defaults(DamageTypeRegistry& r);

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

    void add(Identifier id, int order, Step step);

    bool remove(const Identifier& id);

    Damage run(Damage d, const Entity& target) const;

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