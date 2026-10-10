#include "survival/damage.hpp"

namespace voxelspire {

const DamageType& DamageTypeRegistry::add(DamageType type) {
    if (!type.id) throw std::runtime_error("damage type needs an id");
    if (m_by_id.contains(type.id)) throw std::runtime_error("damage type already registered: " + type.id.str());
    auto owned = std::make_unique<DamageType>(std::move(type));
    const DamageType& ref = *owned;
    m_by_id.emplace(ref.id, std::move(owned));
    return ref;
}

const DamageType& DamageTypeRegistry::get(Identifier id) const {
    if (const DamageType* t = find(id)) return *t;
    if (const DamageType* t = find(DamageTypes::Generic)) return *t;
    throw std::runtime_error("unknown damage type: " + id.str());
}

void DamageTypeRegistry::register_defaults(DamageTypeRegistry& r) {
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

void DamagePipeline::add(Identifier id, int order, Step step) {
    remove(id);
    m_steps.push_back({ std::move(id), order, std::move(step) });
    std::stable_sort(m_steps.begin(), m_steps.end(), [](const Entry& a, const Entry& b) { return a.order < b.order; });
}

bool DamagePipeline::remove(const Identifier& id) {
    auto it = std::find_if(m_steps.begin(), m_steps.end(), [&](const Entry& e) { return e.id == id; });
    if (it == m_steps.end()) return false;
    m_steps.erase(it);
    return true;
}

Damage DamagePipeline::run(Damage d, const Entity& target) const {
    for (const Entry& e : m_steps) {
        if (d.amount <= 0.0) break;
        e.step(d, target);
    }

    d.amount = d.amount > 0.0 ? d.amount : 0.0;
    return d;
}

} // namespace voxelspire
