#include "entity/attributes.hpp"

namespace voxelspire {

const Attribute& AttributeRegistry::add(Identifier id, double default_value, double min_value, double max_value) {
    if (!id) throw std::runtime_error("attribute needs an id");
    if (m_by_id.contains(id)) throw std::runtime_error("attribute already registered: " + id.str());
    auto attr = std::make_unique<Attribute>(id, default_value, min_value, max_value);
    const Attribute& ref = *attr;
    m_by_id.emplace(id, std::move(attr));
    return ref;
}

const Attribute& AttributeRegistry::get(Identifier id) const {
    if (const Attribute* a = find(id)) return *a;
    throw std::runtime_error("unknown attribute: " + id.str());
}

void AttributeRegistry::register_defaults(AttributeRegistry& r) {
    r.add(Attributes::MovementSpeed,      PlayerDefaults::movement::movement_speed,      0.0,       NO_LIMIT);
    r.add(Attributes::JumpVelocity,       PlayerDefaults::movement::jump_velocity,       0.0,       NO_LIMIT);
    r.add(Attributes::GroundAcceleration, PlayerDefaults::movement::ground_acceleration, 0.0,       NO_LIMIT);
    r.add(Attributes::AirAcceleration,    PlayerDefaults::movement::air_acceleration,    0.0,       NO_LIMIT);
    r.add(Attributes::BlockReach,         PlayerDefaults::reach,                         0.0,       NO_LIMIT);
    r.add(Attributes::GravityScale,       1.0,                                           -NO_LIMIT, NO_LIMIT);
    r.add(Attributes::DragScale,          1.0,                                           0.0,       NO_LIMIT);
}

bool AttributeInstance::remove_modifier(Identifier id) {
    auto it = std::find_if(m_modifiers.begin(), m_modifiers.end(), [&](const AttributeModifier& m) { return m.id == id; });
    if (it == m_modifiers.end()) return false;
    m_modifiers.erase(it);
    m_dirty = true;
    return true;
}

double AttributeInstance::value() const noexcept {
    if (!m_dirty) return m_cached;
    double v = m_base;
    for (const auto& m : m_modifiers) if (m.op == ModifierOp::Add) v += m.amount;
    double scaled = v;
    for (const auto& m : m_modifiers) if (m.op == ModifierOp::AddMultipliedBase) scaled += v * m.amount;
    for (const auto& m : m_modifiers) if (m.op == ModifierOp::MultiplyTotal) scaled *= 1.0 + m.amount;
    m_cached = m_type->clamp(scaled);
    m_dirty = false;
    return m_cached;
}

AttributeMap::AttributeMap(const AttributeRegistry& registry) : m_registry(&registry) {
    registry.for_each([this](const Attribute& a) { m_instances.emplace(a.id(), a); });
}

AttributeInstance& AttributeMap::get(Identifier id) {
    if (AttributeInstance* i = m_instances.find(id)) return *i;
    return m_instances.emplace(id, m_registry->get(id));
}

double AttributeMap::value(Identifier id) const {
    if (const AttributeInstance* i = m_instances.find(id)) return i->value();
    return m_registry->get(id).default_value();
}

} // namespace voxelspire
