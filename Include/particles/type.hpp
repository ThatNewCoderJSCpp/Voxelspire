#ifndef VOXELSPIRE_PARTICLES_PARTICLE_TYPE_HPP
#define VOXELSPIRE_PARTICLES_PARTICLE_TYPE_HPP

#include <cstdint>
#include <stdexcept>
#include <vector>
#include "../core/identifier.hpp"
#include "../core/types.hpp"

namespace voxelspire {

enum class ParticleCollision : std::uint8_t { None = 0, Stop, Kill };

struct ParticleType {
    Identifier        id;
    double            lifetime_min  = 1.0;
    double            lifetime_max  = 2.0;
    double            gravity_scale = 0.0;
    double            drag          = 0.0;
    float             size_start    = 0.2f;
    float             size_end      = 0.2f;
    Color             color_start   = Color(255, 255, 255);
    Color             color_end     = Color(255, 255, 255);
    ParticleCollision collision     = ParticleCollision::None;
    bool              emissive      = false;

    bool translucent() const noexcept { return color_start.alpha() < 255 || color_end.alpha() < 255; }
};

namespace Particles {
    inline const Identifier Smoke  = core_id(Kind::Particle, "smoke");
    inline const Identifier Flame  = core_id(Kind::Particle, "flame");
    inline const Identifier Spark  = core_id(Kind::Particle, "spark");
    inline const Identifier Splash = core_id(Kind::Particle, "splash");
} // namespace Particles

using ParticleTypeIndex = std::uint16_t;

class ParticleTypeRegistry {
public:
    static constexpr std::size_t MAX_TYPES = 0x10000;

    ParticleTypeIndex add(const ParticleType& type);

    void replace(const ParticleType& type) {
        if (const ParticleTypeIndex* i = m_index.find(type.id)) m_types[*i] = type;
        else add(type);
    }

    const ParticleTypeIndex* find(Identifier id) const noexcept { return m_index.find(id); }

    ParticleTypeIndex require(Identifier id) const;

    const ParticleType& get(ParticleTypeIndex i) const noexcept { return m_types[i]; }
    std::size_t size() const noexcept { return m_types.size(); }

    static void register_defaults(ParticleTypeRegistry& r);

private:
    std::vector<ParticleType>          m_types;
    IdentifierTable<ParticleTypeIndex> m_index;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PARTICLES_PARTICLE_TYPE_HPP