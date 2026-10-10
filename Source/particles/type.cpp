#include "particles/type.hpp"

namespace voxelspire {

ParticleTypeIndex ParticleTypeRegistry::add(const ParticleType& type) {
    if (!type.id) throw std::runtime_error("particle type needs an id");
    if (m_index.contains(type.id)) throw std::runtime_error("particle type already registered: " + type.id.str());
    if (m_types.size() >= MAX_TYPES) throw std::runtime_error("too many particle types");
    const auto index = static_cast<ParticleTypeIndex>(m_types.size());
    m_types.push_back(type);
    m_index.emplace(type.id, index);
    return index;
}

ParticleTypeIndex ParticleTypeRegistry::require(Identifier id) const {
    if (const ParticleTypeIndex* i = m_index.find(id)) return *i;
    throw std::runtime_error("unknown particle type: " + id.str());
}

void ParticleTypeRegistry::register_defaults(ParticleTypeRegistry& r) {
    ParticleType smoke;
    smoke.id            = Particles::Smoke;
    smoke.lifetime_min  = 2.5;
    smoke.lifetime_max  = 4.5;
    smoke.gravity_scale = -0.035;
    smoke.drag          = 0.8;
    smoke.size_start    = 0.18f;
    smoke.size_end      = 0.55f;
    smoke.color_start   = Color(90, 90, 90, 200);
    smoke.color_end     = Color(170, 170, 170, 0);
    r.add(smoke);

    ParticleType flame;
    flame.id            = Particles::Flame;
    flame.lifetime_min  = 0.35;
    flame.lifetime_max  = 0.7;
    flame.gravity_scale = -0.05;
    flame.drag          = 2.0;
    flame.size_start    = 0.16f;
    flame.size_end      = 0.04f;
    flame.color_start   = Color(255, 210, 90);
    flame.color_end     = Color(230, 70, 20);
    flame.emissive      = true;
    r.add(flame);

    ParticleType spark;
    spark.id            = Particles::Spark;
    spark.lifetime_min  = 0.6;
    spark.lifetime_max  = 1.4;
    spark.gravity_scale = 0.5;
    spark.drag          = 0.3;
    spark.size_start    = 0.05f;
    spark.size_end      = 0.03f;
    spark.color_start   = Color(255, 190, 80);
    spark.color_end     = Color(200, 60, 20);
    spark.collision     = ParticleCollision::Kill;
    spark.emissive      = true;
    r.add(spark);

    ParticleType splash;
    splash.id            = Particles::Splash;
    splash.lifetime_min  = 0.5;
    splash.lifetime_max  = 1.1;
    splash.gravity_scale = 1.0;
    splash.drag          = 0.4;
    splash.size_start    = 0.08f;
    splash.size_end      = 0.05f;
    splash.color_start   = Color(200, 225, 255, 200);
    splash.color_end     = Color(220, 235, 255, 60);
    splash.collision     = ParticleCollision::Kill;
    r.add(splash);
}

} // namespace voxelspire
