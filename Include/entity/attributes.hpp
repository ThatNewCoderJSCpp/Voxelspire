#ifndef VOXELSPIRE_ENTITY_ATTRIBUTES_HPP
#define VOXELSPIRE_ENTITY_ATTRIBUTES_HPP

#include <algorithm>
#include <limits>
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
    static constexpr double NO_LIMIT = std::numeric_limits<double>::infinity();

    const Attribute& add(Identifier id, double default_value, double min_value, double max_value);

    const Attribute* find(Identifier id) const noexcept {
        const auto* slot = m_by_id.find(id);
        return slot ? slot->get() : nullptr;
    }

    const Attribute& get(Identifier id) const;

    template <typename Fn>
    void for_each(Fn&& fn) const { m_by_id.for_each([&fn](const std::unique_ptr<Attribute>& a) { fn(*a); }); }

    static void register_defaults(AttributeRegistry& r);

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

    bool remove_modifier(Identifier id);

    bool has_modifier(Identifier id) const noexcept {
        for (const auto& m : m_modifiers) if (m.id == id) return true;
        return false;
    }

    const std::vector<AttributeModifier>& modifiers() const noexcept { return m_modifiers; }

    double value() const noexcept;

private:
    const Attribute*               m_type;
    double                         m_base;
    std::vector<AttributeModifier> m_modifiers;
    mutable double                 m_cached = 0.0;
    mutable bool                   m_dirty  = true;
};

class AttributeMap {
public:
    explicit AttributeMap(const AttributeRegistry& registry);

    bool has(Identifier id) const noexcept { return m_instances.contains(id); }

    AttributeInstance& get(Identifier id);

    double value(Identifier id) const;

    void set_base(Identifier id, double v) { get(id).set_base(v); }

private:
    const AttributeRegistry*           m_registry;
    IdentifierTable<AttributeInstance> m_instances;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_ATTRIBUTES_HPP