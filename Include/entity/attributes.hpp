#ifndef VOXELSPIRE_ENTITY_ATTRIBUTES_HPP
#define VOXELSPIRE_ENTITY_ATTRIBUTES_HPP

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "../core/identifier.hpp"
#include "../core/types.hpp"
#include "player_defaults.hpp"

namespace voxelspire {

class Attribute {
public:
    Attribute(Identifier id, double default_value, double min_value, double max_value)
        : m_id(id), m_default(default_value), m_min(min_value), m_max(max_value) {}

    const Identifier&  id()            const noexcept { return m_id; }
    double             default_value() const noexcept { return m_default; }
    double             min_value()     const noexcept { return m_min; }
    double             max_value()     const noexcept { return m_max; }
    double             clamp(double v) const noexcept { return vclamp(v, m_min, m_max); }

private:
    Identifier  m_id;
    double      m_default, m_min, m_max;
};

namespace Attributes {
    inline const Identifier MovementSpeed      = core_id(Kind::Attribute, "movement_speed");
    inline const Identifier JumpVelocity       = core_id(Kind::Attribute, "jump_velocity");
    inline const Identifier GroundAcceleration = core_id(Kind::Attribute, "ground_acceleration");
    inline const Identifier AirAcceleration    = core_id(Kind::Attribute, "air_acceleration");
    inline const Identifier GravityScale       = core_id(Kind::Attribute, "gravity_scale");
    inline const Identifier DragScale          = core_id(Kind::Attribute, "drag_scale");
    inline const Identifier BlockReach         = core_id(Kind::Attribute, "block_reach");
} // namespace Attributes

class AttributeRegistry {
public:
    struct defaults {
        static constexpr double movement_speed_max      = 1024.0;
        static constexpr double jump_velocity_max       = 1024.0;
        static constexpr double ground_acceleration_max = 4096.0;
        static constexpr double air_acceleration_max    = 4096.0;
        static constexpr double gravity_scale_max       =   16.0;
        static constexpr double drag_scale_max          = 1024.0;
        static constexpr double reach_max               =   64.0;
    };

    const Attribute& add(Identifier id, double default_value, double min_value, double max_value) {
        if (!id) throw std::runtime_error("attribute needs an id");
        if (m_by_id.contains(id)) throw std::runtime_error("attribute already registered: " + id.str());
        auto attr = std::make_unique<Attribute>(id, default_value, min_value, max_value);
        const Attribute& ref = *attr;
        m_by_id.emplace(id, std::move(attr));
        return ref;
    }

    const Attribute* find(Identifier id) const noexcept {
        const auto* slot = m_by_id.find(id);
        return slot ? slot->get() : nullptr;
    }

    const Attribute& get(Identifier id) const {
        if (const Attribute* a = find(id)) return *a;
        throw std::runtime_error("unknown attribute: " + id.str());
    }

    template <typename Fn>
    void for_each(Fn&& fn) const { m_by_id.for_each([&fn](const std::unique_ptr<Attribute>& a) { fn(*a); }); }

    static void register_defaults(AttributeRegistry& r) {
        r.add(Attributes::MovementSpeed,      PlayerDefaults::movement::movement_speed,      0.0, defaults::movement_speed_max);
        r.add(Attributes::JumpVelocity,       PlayerDefaults::movement::jump_velocity,       0.0, defaults::jump_velocity_max);
        r.add(Attributes::GroundAcceleration, PlayerDefaults::movement::ground_acceleration, 0.0, defaults::ground_acceleration_max);
        r.add(Attributes::AirAcceleration,    PlayerDefaults::movement::air_acceleration,    0.0, defaults::air_acceleration_max);
        r.add(Attributes::BlockReach,         PlayerDefaults::reach,                         0.0, defaults::reach_max);
        r.add(Attributes::GravityScale,       1.0,  -defaults::gravity_scale_max,                 defaults::gravity_scale_max);
        r.add(Attributes::DragScale,          1.0,  0.0,                                          defaults::drag_scale_max);
    }

private:
    IdentifierTable<std::unique_ptr<Attribute>> m_by_id;
};

enum class ModifierOp : std::uint8_t { Add = 0, AddMultipliedBase, MultiplyTotal };

struct AttributeModifier {
    Identifier  id;
    double      amount = 0.0;
    ModifierOp  op     = ModifierOp::Add;
};

class AttributeInstance {
public:
    explicit AttributeInstance(const Attribute& type) noexcept : m_type(&type), m_base(type.default_value()) {}

    const Attribute& type() const noexcept { return *m_type; }

    double base() const noexcept { return m_base; }
    void set_base(double v) noexcept { m_base = v; m_dirty = true; }

    void add_modifier(AttributeModifier m) {
        remove_modifier(m.id);
        m_modifiers.push_back(std::move(m));
        m_dirty = true;
    }

    bool remove_modifier(Identifier id) {
        auto it = std::find_if(m_modifiers.begin(), m_modifiers.end(), [&](const AttributeModifier& m) { return m.id == id; });
        if (it == m_modifiers.end()) return false;
        m_modifiers.erase(it);
        m_dirty = true;
        return true;
    }

    bool has_modifier(Identifier id) const noexcept {
        for (const auto& m : m_modifiers) if (m.id == id) return true;
        return false;
    }

    const std::vector<AttributeModifier>& modifiers() const noexcept { return m_modifiers; }

    double value() const noexcept {
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

private:
    const Attribute*               m_type;
    double                         m_base;
    std::vector<AttributeModifier> m_modifiers;
    mutable double                 m_cached = 0.0;
    mutable bool                   m_dirty  = true;
};

class AttributeMap {
public:
    explicit AttributeMap(const AttributeRegistry& registry) : m_registry(&registry) {
        registry.for_each([this](const Attribute& a) { m_instances.emplace(a.id(), a); });
    }

    bool has(Identifier id) const noexcept { return m_instances.contains(id); }

    AttributeInstance& get(Identifier id) {
        if (AttributeInstance* i = m_instances.find(id)) return *i;
        return m_instances.emplace(id, m_registry->get(id));
    }

    double value(Identifier id) const {
        if (const AttributeInstance* i = m_instances.find(id)) return i->value();
        return m_registry->get(id).default_value();
    }

    void set_base(Identifier id, double v) { get(id).set_base(v); }

private:
    const AttributeRegistry*           m_registry;
    IdentifierTable<AttributeInstance> m_instances;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_ATTRIBUTES_HPP