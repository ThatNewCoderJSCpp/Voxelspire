#ifndef VOXELSPIRE_PARTICLES_PARTICLE_EMITTER_HPP
#define VOXELSPIRE_PARTICLES_PARTICLE_EMITTER_HPP

#include <cmath>
#include <vector>
#include "../core/random.hpp"
#include "type.hpp"

namespace voxelspire {

class ParticleSink {
public:
    virtual ~ParticleSink() = default;
    virtual void spawn(ParticleTypeIndex type, const vector3d& position, const vector3d& velocity, SeededRandom& rng) = 0;
};

class ParticleEmitter {
public:
    explicit ParticleEmitter(vector3d position) noexcept : m_position(position) {}
    virtual ~ParticleEmitter() = default;

    virtual void emit(ParticleSink& out, const ParticleTypeRegistry& types, double dt, SeededRandom& rng) = 0;
    virtual bool finished() const noexcept { return false; }

    const vector3d& position() const noexcept { return m_position; }
    void set_position(const vector3d& p) noexcept { m_position = p; }

    bool active() const noexcept { return m_active; }
    void set_active(bool on) noexcept { m_active = on; }

private:
    vector3d m_position;
    bool     m_active = true;
};

struct EmissionStream {
    Identifier type;
    double     rate = 10.0;
    vector3d   offset{};
    vector3d   spread{ 0.1, 0.1, 0.0 };
    vector3d   velocity{};
    vector3d   velocity_spread{};
};

class StreamEmitter : public ParticleEmitter {
public:
    StreamEmitter(vector3d position, std::vector<EmissionStream> streams)
        : ParticleEmitter(position), m_streams(std::move(streams)), m_carry(m_streams.size(), 0.0) {}

    void emit(ParticleSink& out, const ParticleTypeRegistry& types, double dt, SeededRandom& rng) override;

    std::vector<EmissionStream>&       streams()       noexcept { m_resolved.clear(); return m_streams; }
    const std::vector<EmissionStream>& streams() const noexcept { return m_streams; }

private:
    static vector3d jitter(const vector3d& amount, SeededRandom& rng) noexcept;

    std::vector<EmissionStream>    m_streams;
    std::vector<double>            m_carry;
    std::vector<ParticleTypeIndex> m_resolved;
};

struct CampfireRates {
    double smoke = 12.0;
    double flame = 30.0;
    double spark = 2.0;
};

class CampfireEmitter final : public StreamEmitter {
public:
    explicit CampfireEmitter(const vector3d& block_center, CampfireRates rates = CampfireRates{})
        : StreamEmitter(block_center, streams_for(rates)) {}

private:
    static std::vector<EmissionStream> streams_for(const CampfireRates& r);
};

} // namespace voxelspire

#endif // VOXELSPIRE_PARTICLES_PARTICLE_EMITTER_HPP