#include "particles/system.hpp"

namespace voxelspire {

void ParticleSystem::spawn(ParticleTypeIndex type, const vector3d& position, const vector3d& velocity, SeededRandom& rng) {
    if (m_type.size() >= m_settings.max_particles) return;
    const ParticleType& t = m_types.get(type);
    const vector3d local = position - m_anchor;
    m_px.push_back(static_cast<float>(local.x)); m_py.push_back(static_cast<float>(local.y)); m_pz.push_back(static_cast<float>(local.z));
    m_vx.push_back(static_cast<float>(velocity.x)); m_vy.push_back(static_cast<float>(velocity.y)); m_vz.push_back(static_cast<float>(velocity.z));
    m_age.push_back(0.0f);
    m_life.push_back(static_cast<float>(rng.range(t.lifetime_min, t.lifetime_max)));
    m_type.push_back(type);
}

void ParticleSystem::update(double dt, const vector3d& viewer, const World& world, double gravity) {
    if (!m_settings.enabled || dt <= 0.0) return;
    refresh_params(gravity);
    recenter(viewer);
    const double emit_sq = m_settings.emit_distance * m_settings.emit_distance;

    for (auto it = m_emitters.begin(); it != m_emitters.end();) {
        ParticleEmitter& e = *it->second;
        if (e.finished()) { it = m_emitters.erase(it); continue; }
        const vector3d d = e.position() - viewer;
        if (e.active() && d.x * d.x + d.y * d.y + d.z * d.z <= emit_sq) e.emit(*this, m_types, dt, m_rng);
        ++it;
    }

    simulate(static_cast<float>(dt), world);
}

void ParticleSystem::clear() noexcept {
    m_px.clear(); m_py.clear(); m_pz.clear(); m_vx.clear(); m_vy.clear(); m_vz.clear();
    m_age.clear(); m_life.clear(); m_type.clear();
}

fizmo::graphics::Mesh3D ParticleSystem::unit_cube() {
    using fizmo::graphics::Vertex3D;
    fizmo::graphics::Mesh3D m;
    const Color white(255, 255, 255);

    for (Face f : ALL_FACES) {
        std::array<vector3d, 4> c;
        face_corners(vector3d{ 0.0, 0.0, 0.0 }, vector3d{ 1.0, 1.0, 1.0 }, f, c);
        const vector3d n = face_normal(f);
        m.add_quad(Vertex3D(c[0], n, white), Vertex3D(c[1], n, white), Vertex3D(c[2], n, white), Vertex3D(c[3], n, white));
    }

    return m;
}

std::uint32_t ParticleSystem::lerp_rgba(const float* a, const float* b, float t) noexcept {
    std::uint32_t out = 0;

    for (int k = 0; k < 4; ++k) {
        const float v = a[k] + (b[k] - a[k]) * t;
        const std::uint32_t byte = static_cast<std::uint32_t>(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v + 0.5f));
        out |= byte << (8 * k);
    }

    return out;
}

void ParticleSystem::refresh_params(double gravity) {
    if (m_params.size() == m_types.size() && m_params_gravity == gravity) return;
    m_params.resize(m_types.size());

    for (std::size_t i = 0; i < m_types.size(); ++i) {
        const ParticleType& t = m_types.get(static_cast<ParticleTypeIndex>(i));
        Params& p = m_params[i];
        p.accel = static_cast<float>(-gravity * t.gravity_scale);
        p.drag  = static_cast<float>(t.drag);
        p.size0 = t.size_start;
        p.size1 = t.size_end;
        unpack(t.color_start, p.c0);
        unpack(t.color_end, p.c1);
        p.collision   = t.collision;
        p.translucent = t.translucent();
        p.emissive    = t.emissive;
    }

    m_params_gravity = gravity;
}

void ParticleSystem::recenter(const vector3d& viewer) {
    const vector3d d = viewer - m_anchor;
    if (std::fabs(d.x) < m_settings.recenter_distance && std::fabs(d.y) < m_settings.recenter_distance && std::fabs(d.z) < m_settings.recenter_distance) return;
    const vector3d shift{ std::floor(d.x), std::floor(d.y), std::floor(d.z) };
    const float sx = static_cast<float>(shift.x), sy = static_cast<float>(shift.y), sz = static_cast<float>(shift.z);
    for (std::size_t i = 0; i < m_type.size(); ++i) { m_px[i] -= sx; m_py[i] -= sy; m_pz[i] -= sz; }
    m_anchor += shift;
}

void ParticleSystem::kill(std::size_t i) noexcept {
    const std::size_t last = m_type.size() - 1;

    if (i != last) {
        m_px[i] = m_px[last]; m_py[i] = m_py[last]; m_pz[i] = m_pz[last];
        m_vx[i] = m_vx[last]; m_vy[i] = m_vy[last]; m_vz[i] = m_vz[last];
        m_age[i] = m_age[last]; m_life[i] = m_life[last]; m_type[i] = m_type[last];
    }

    m_px.pop_back(); m_py.pop_back(); m_pz.pop_back();
    m_vx.pop_back(); m_vy.pop_back(); m_vz.pop_back();
    m_age.pop_back(); m_life.pop_back(); m_type.pop_back();
}

void ParticleSystem::simulate(float dt, const World& world) {
    BlockReader reader(world);
    const vector3d a = m_anchor;

    for (std::size_t i = 0; i < m_type.size();) {
        m_age[i] += dt;
        if (m_age[i] >= m_life[i]) { kill(i); continue; }
        const Params& p = m_params[m_type[i]];
        const float damp = 1.0f / (1.0f + p.drag * dt);
        m_vz[i] = (m_vz[i] + p.accel * dt) * damp;
        m_vx[i] *= damp;
        m_vy[i] *= damp;
        const float nx = m_px[i] + m_vx[i] * dt, ny = m_py[i] + m_vy[i] * dt, nz = m_pz[i] + m_vz[i] * dt;

        if (p.collision != ParticleCollision::None) {
            const BlockPos b = BlockPos::containing({ a.x + nx, a.y + ny, a.z + nz });

            if (reader.solid_at(b)) {
                if (p.collision == ParticleCollision::Kill) { kill(i); continue; }
                m_vx[i] = m_vy[i] = m_vz[i] = 0.0f;
                ++i;
                continue;
            }
        }

        m_px[i] = nx; m_py[i] = ny; m_pz[i] = nz;
        ++i;
    }
}

} // namespace voxelspire
