#ifndef VOXELSPIRE_PARTICLES_PARTICLE_SYSTEM_HPP
#define VOXELSPIRE_PARTICLES_PARTICLE_SYSTEM_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../core/settings.hpp"
#include "../render/greedy_mesher.hpp"
#include "../world/block_reader.hpp"
#include "emitter.hpp"

namespace voxelspire {

class ParticleSystem final : public ParticleSink {
public:
    using EmitterId = std::uint64_t;

    ParticleSystem(const ParticleTypeRegistry& types, const ParticleSettings& settings)
        : m_types(types), m_settings(settings), m_cube(unit_cube()) {}

    ParticleSettings&       settings()       noexcept { return m_settings; }
    const ParticleSettings& settings() const noexcept { return m_settings; }

    EmitterId add_emitter(std::unique_ptr<ParticleEmitter> emitter) {
        const EmitterId id = ++m_last_emitter;
        m_emitters.emplace(id, std::move(emitter));
        return id;
    }

    template <typename T, typename... Args>
    EmitterId emplace_emitter(Args&&... args) { return add_emitter(std::make_unique<T>(std::forward<Args>(args)...)); }

    bool remove_emitter(EmitterId id) { return m_emitters.erase(id) != 0; }

    ParticleEmitter* emitter(EmitterId id) noexcept {
        auto it = m_emitters.find(id);
        return it == m_emitters.end() ? nullptr : it->second.get();
    }

    void spawn(ParticleTypeIndex type, const vector3d& position, const vector3d& velocity, SeededRandom& rng) override;

    void update(double dt, const vector3d& viewer, const World& world, double gravity);

    template <typename LightProbe>
    void render(fizmo::windows::Renderer& renderer, const vector3d& viewer, LightProbe&& light_at) {
        for (auto& list : m_lists) list.clear();
        if (!m_settings.enabled || m_type.empty()) return;
        const vector3d v = viewer - m_anchor;
        const float vx = static_cast<float>(v.x), vy = static_cast<float>(v.y), vz = static_cast<float>(v.z);
        const float max_sq = static_cast<float>(m_settings.draw_distance * m_settings.draw_distance);
        const std::size_t n = m_type.size();

        for (std::size_t i = 0; i < n; ++i) {
            const float dx = m_px[i] - vx, dy = m_py[i] - vy, dz = m_pz[i] - vz;
            if (dx * dx + dy * dy + dz * dz > max_sq) continue;
            const Params& p = m_params[m_type[i]];
            const float t = m_age[i] / m_life[i];
            const float size = p.size0 + (p.size1 - p.size0) * t;
            const float half = size * 0.5f;
            fizmo::graphics::Instance3D inst;
            inst.x = m_px[i] - half; inst.y = m_py[i] - half; inst.z = m_pz[i] - half;
            inst.scale = size;
            inst.rgba = lerp_rgba(p.c0, p.c1, t);

            if (!p.emissive) {
                const BlockPos cell = BlockPos::containing(m_anchor + vector3d{ m_px[i], m_py[i], m_pz[i] });
                if (cell != m_probe_cell || !m_probe_valid) { m_probe_light = light_at(cell).packed(); m_probe_cell = cell; m_probe_valid = true; }
                inst.light = m_probe_light;
            }

            m_lists[list_of(p.translucent, p.emissive)].push_back(inst);
        }

        m_probe_valid = false;
        using fizmo::graphics::Material3D;
        using fizmo::graphics::Shadow3D;

        for (std::size_t k = 0; k < LISTS; ++k) {
            if (m_lists[k].empty()) continue;
            const bool translucent = (k & TRANSLUCENT_BIT) != 0, emissive = (k & EMISSIVE_BIT) != 0;
            Material3D m = translucent ? Material3D::transparent() : Material3D().with_shadow(Shadow3D::None);
            m.lit = !emissive;
            renderer.draw_instances(m_cube, m_anchor, m_lists[k], m);
        }
    }

    void clear() noexcept;

    std::size_t count()         const noexcept { return m_type.size(); }
    std::size_t emitter_count() const noexcept { return m_emitters.size(); }
    
    std::size_t drawn() const noexcept {
        std::size_t n = 0;
        for (const auto& list : m_lists) n += list.size();
        return n;
    }

private:
    static constexpr std::size_t TRANSLUCENT_BIT = 1, EMISSIVE_BIT = 2, LISTS = 4;

    static std::size_t list_of(bool translucent, bool emissive) noexcept { return (translucent ? TRANSLUCENT_BIT : 0) | (emissive ? EMISSIVE_BIT : 0); }

    struct Params {
        float             accel = 0.0f;
        float             drag  = 0.0f;
        float             size0 = 0.0f, size1 = 0.0f;
        float             c0[4] = {}, c1[4] = {};
        ParticleCollision collision = ParticleCollision::None;
        bool              translucent = false;
        bool              emissive    = false;
    };

    static fizmo::graphics::Mesh3D unit_cube();

    static void unpack(const Color& c, float* out) noexcept {
        out[0] = c.red(); out[1] = c.green(); out[2] = c.blue(); out[3] = c.alpha();
    }

    static std::uint32_t lerp_rgba(const float* a, const float* b, float t) noexcept;

    void refresh_params(double gravity);

    void recenter(const vector3d& viewer);

    void kill(std::size_t i) noexcept;

    void simulate(float dt, const World& world);

    const ParticleTypeRegistry& m_types;
    ParticleSettings            m_settings;
    fizmo::graphics::Mesh3D     m_cube;
    std::unordered_map<EmitterId, std::unique_ptr<ParticleEmitter>> m_emitters;
    EmitterId                   m_last_emitter = 0;
    SeededRandom                  m_rng;
    vector3d                    m_anchor{};
    std::vector<Params>         m_params;
    double                      m_params_gravity = 0.0;

    std::vector<float>             m_px, m_py, m_pz, m_vx, m_vy, m_vz, m_age, m_life;
    std::vector<ParticleTypeIndex> m_type;
    std::array<std::vector<fizmo::graphics::Instance3D>, LISTS> m_lists;
    BlockPos                       m_probe_cell{};
    std::uint32_t                  m_probe_light = fizmo::graphics::LIGHT_FULL_SKY;
    bool                           m_probe_valid = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PARTICLES_PARTICLE_SYSTEM_HPP