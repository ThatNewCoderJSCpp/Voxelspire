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
;

    void render(
        fizmo::windows::Renderer& renderer, 
        const Player& player, 
        double alpha,
        fizmo::graphics::BakedLight light = fizmo::graphics::BakedLight::full_sky(),
        fizmo::graphics::View3D view = fizmo::graphics::View3D::Everywhere,
        bool casts_shadow = true
    );

private:
    struct Ring { double z, radius, nz, nxy; bool visor_band; };

    const fizmo::graphics::Mesh3D& mesh_for(double radius, double height);

    void build(fizmo::graphics::Mesh3D& mesh, double r, double h) const;

    int   m_segments, m_rings;
    Color m_body, m_visor;
    std::map<std::pair<long, long>, fizmo::graphics::Mesh3D> m_meshes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_CAPSULE_RENDERER_HPP