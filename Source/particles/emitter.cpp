#include "particles/emitter.hpp"

namespace voxelspire {

void StreamEmitter::emit(ParticleSink& out, const ParticleTypeRegistry& types, double dt, SeededRandom& rng) {
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

vector3d StreamEmitter::jitter(const vector3d& amount, SeededRandom& rng) noexcept {
    return { amount.x * rng.signed_unit(), amount.y * rng.signed_unit(), amount.z * rng.signed_unit() };
}

std::vector<EmissionStream> CampfireEmitter::streams_for(const CampfireRates& r) {
    EmissionStream smoke{ Particles::Smoke, r.smoke, { 0.0, 0.0, 0.35 }, { 0.2, 0.2, 0.05 }, { 0.0, 0.0, 1.4 }, { 0.15, 0.15, 0.3 } };
    EmissionStream flame{ Particles::Flame, r.flame, { 0.0, 0.0, 0.05 }, { 0.25, 0.25, 0.05 }, { 0.0, 0.0, 0.8 }, { 0.1, 0.1, 0.3 } };
    EmissionStream spark{ Particles::Spark, r.spark, { 0.0, 0.0, 0.2 }, { 0.1, 0.1, 0.0 }, { 0.0, 0.0, 3.0 }, { 1.2, 1.2, 1.0 } };
    return { smoke, flame, spark };
}

} // namespace voxelspire
