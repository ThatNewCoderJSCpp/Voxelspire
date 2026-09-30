#ifndef VOXELSPIRE_RENDER_CAPSULE_RENDERER_HPP
#define VOXELSPIRE_RENDER_CAPSULE_RENDERER_HPP

#include <cmath>
#include <map>
#include <utility>
#include <vector>
#include "../entity/player.hpp"

namespace voxelspire {

class CapsuleRenderer {
public:
    CapsuleRenderer(int segments, int rings_per_cap, Color body, Color visor)
        : m_segments(vmax(segments, 6)), m_rings(vmax(rings_per_cap, 2)), m_body(body), m_visor(visor) {}

    void render(fizmo::windows::Renderer& renderer, const Player& player, double alpha,
                fizmo::graphics::BakedLight light = fizmo::graphics::BakedLight::full_sky(),
                fizmo::graphics::View3D view = fizmo::graphics::View3D::Everywhere) {
        const vector3d feet = player.interpolated_position(alpha);
        const double yaw = deg_to_rad(player.yaw());
        const double c = std::cos(yaw), s = std::sin(yaw);
        fizmo::math::Matrix4d model = fizmo::math::Matrix4d::identity();
        model.data[0] = c;  model.data[1] = -s;  model.data[3]  = feet.x;
        model.data[4] = s;  model.data[5] = c;   model.data[7]  = feet.y;
        model.data[11] = feet.z;
        renderer.draw_mesh(mesh_for(player.capsule_radius(), player.height()), model, fizmo::graphics::Material3D().with_light(light).with_view(view));
    }

private:
    struct Ring { double z, radius, nz, nxy; bool visor_band; };

    const fizmo::graphics::Mesh3D& mesh_for(double radius, double height) {
        const auto key = std::make_pair(std::lround(radius * 1000.0), std::lround(height * 1000.0));
        auto it = m_meshes.find(key);

        if (it == m_meshes.end()) {
            it = m_meshes.emplace(key, fizmo::graphics::Mesh3D{}).first;
            build(it->second, radius, height);
        }

        return it->second;
    }

    void build(fizmo::graphics::Mesh3D& mesh, double r, double h) const {
        const double cyl_bottom = r, cyl_top = vmax(r, h - r);
        std::vector<Ring> ring_list;

        for (int i = 0; i <= m_rings; ++i) {
            const double a = deg_to_rad(-90.0 + 90.0 * i / m_rings);
            ring_list.push_back({ cyl_bottom + r * std::sin(a), r * std::cos(a), std::sin(a), std::cos(a), false });
        }

        for (int i = 0; i <= m_rings; ++i) {
            const double a = deg_to_rad(90.0 * i / m_rings);
            ring_list.push_back({ cyl_top + r * std::sin(a), r * std::cos(a), std::sin(a), std::cos(a), i == 0 });
        }

        using fizmo::graphics::Vertex3D;

        for (std::size_t k = 0; k + 1 < ring_list.size(); ++k) {
            const Ring& lo = ring_list[k];
            const Ring& hi = ring_list[k + 1];
            if (hi.z - lo.z < 1e-9 && std::fabs(hi.radius - lo.radius) < 1e-9) continue;

            for (int j = 0; j < m_segments; ++j) {
                const double t0 = 2.0 * PI * j / m_segments, t1 = 2.0 * PI * (j + 1) / m_segments, tm = (t0 + t1) * 0.5;
                auto pos = [](const Ring& rg, double t) { return vector3d{ rg.radius * std::cos(t), rg.radius * std::sin(t), rg.z }; };
                auto nrm = [](const Ring& rg, double t) { return vector3d{ rg.nxy * std::cos(t), rg.nxy * std::sin(t), rg.nz }; };
                const double off = std::fabs(std::remainder(rad_to_deg(tm) - 90.0, 360.0));
                const Color col = (lo.visor_band && off < 50.0) ? m_visor : m_body;

                mesh.add_quad(
                    Vertex3D(pos(lo, t0), nrm(lo, t0), col), Vertex3D(pos(lo, t1), nrm(lo, t1), col),
                    Vertex3D(pos(hi, t1), nrm(hi, t1), col), Vertex3D(pos(hi, t0), nrm(hi, t0), col)
                );
            }
        }
    }

    int   m_segments, m_rings;
    Color m_body, m_visor;
    std::map<std::pair<long, long>, fizmo::graphics::Mesh3D> m_meshes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_CAPSULE_RENDERER_HPP