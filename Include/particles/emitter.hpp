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

    void emit(ParticleSink& out, const ParticleTypeRegistry& types, double dt, SeededRandom& rng) override {
        if (m_carry.size() != m_streams.size()) m_carry.resize(m_streams.size(), 0.0);

        if (m_resolved.size() != m_streams.size()) {
            m_resolved.clear();
            for (const EmissionStream& s : m_streams) m_resolved.push_back(types.require(s.type));
        }

        for (std::size_t i = 0; i < m_streams.size(); ++i) {
            const EmissionStream& s = m_streams[i];
            m_carry[i] += s.rate * dt;
            const double whole = std::floor(m_carry[i]);
            m_carry[i] -= whole;

            for (int n = 0; n < static_cast<int>(whole); ++n) {
                const vector3d p = position() + s.offset + jitter(s.spread, rng);
                out.spawn(m_resolved[i], p, s.velocity + jitter(s.velocity_spread, rng), rng);
            }
        }
    }

    std::vector<EmissionStream>&       streams()       noexcept { m_resolved.clear(); return m_streams; }
    const std::vector<EmissionStream>& streams() const noexcept { return m_streams; }

private:
    static vector3d jitter(const vector3d& amount, SeededRandom& rng) noexcept {
        return { amount.x * rng.signed_unit(), amount.y * rng.signed_unit(), amount.z * rng.signed_unit() };
    }

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
    static std::vector<EmissionStream> streams_for(const CampfireRates& r) {
        EmissionStream smoke{ Particles::Smoke, r.smoke, { 0.0, 0.0, 0.35 }, { 0.2, 0.2, 0.05 }, { 0.0, 0.0, 1.4 }, { 0.15, 0.15, 0.3 } };
        EmissionStream flame{ Particles::Flame, r.flame, { 0.0, 0.0, 0.05 }, { 0.25, 0.25, 0.05 }, { 0.0, 0.0, 0.8 }, { 0.1, 0.1, 0.3 } };
        EmissionStream spark{ Particles::Spark, r.spark, { 0.0, 0.0, 0.2 }, { 0.1, 0.1, 0.0 }, { 0.0, 0.0, 3.0 }, { 1.2, 1.2, 1.0 } };
        return { smoke, flame, spark };
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_PARTICLES_PARTICLE_EMITTER_HPP