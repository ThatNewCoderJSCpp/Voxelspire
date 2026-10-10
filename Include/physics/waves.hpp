#ifndef VOXELSPIRE_PHYSICS_WAVES_HPP
#define VOXELSPIRE_PHYSICS_WAVES_HPP

#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include "../core/settings.hpp"
#include "../core/types.hpp"
#include "../fizmo.hpp"

namespace voxelspire {

struct WaterSample {
    double waves = 1.0;
    Color  tint  = Color(46, 96, 205);
};

using WaterLook    = std::function<WaterSample(int, int)>;
using WaterLookPtr = std::shared_ptr<const WaterLook>;

class WaveField {
public:
    using Swell = fizmo::graphics::Swell3D;

    static constexpr std::size_t COUNT   = Swell::COUNT;
    static constexpr double      GRAVITY = 9.81;
    static constexpr double      TURN    = 2.0 * PI;

    void update(const WaveSettings& s, double storminess, const vector3d& wind, double seconds) noexcept;

    double height() const noexcept { return m_height; }

    double at(double x, double y, double scale = 1.0) const noexcept;

    double crest(double scale = 1.0) const noexcept;

    Swell swell(const vector3d& camera) const noexcept;

private:
    static constexpr double MIN_WAVELENGTH = 1.0;
    static constexpr double MIN_WIND       = 1e-6;
    static constexpr double MAX_STORM      = 1.5;

    void direction(std::size_t i, double& dx, double& dy) const noexcept;

    double omega(double k) const noexcept { return std::sqrt(GRAVITY * k) * m_speed; }

    double m_height  = 0.0;
    double m_k       = TURN / WaveSettings{}.wavelength;
    double m_speed   = 1.0;
    double m_seconds = 0.0;
    double m_dir_x   = 1.0;
    double m_dir_y   = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_WAVES_HPP