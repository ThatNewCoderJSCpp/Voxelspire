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

    void update(const WaveSettings& s, double storminess, const vector3d& wind, double seconds) noexcept {
        m_height = s.enabled ? s.calm_height + (s.storm_height - s.calm_height) * vclamp(storminess, 0.0, MAX_STORM) : 0.0;
        m_k = TURN / vmax(s.wavelength, MIN_WAVELENGTH);
        m_speed = s.speed;
        m_seconds = seconds;
        const double w = std::sqrt(wind.x * wind.x + wind.y * wind.y);
        if (w > MIN_WIND) { m_dir_x = wind.x / w; m_dir_y = wind.y / w; }
    }

    double height() const noexcept { return m_height; }

    double at(double x, double y, double scale = 1.0) const noexcept {
        if (m_height <= 0.0 || scale <= 0.0) return 0.0;
        double h = 0.0;

        for (std::size_t i = 0; i < COUNT; ++i) {
            double dx, dy;
            direction(i, dx, dy);
            const double k = m_k * Swell::WAVENUMBER[i];
            h += Swell::AMPLITUDE[i] * std::sin(k * (dx * x + dy * y) - omega(k) * m_seconds);
        }

        return h * m_height * scale;
    }

    double crest(double scale = 1.0) const noexcept {
        double total = 0.0;
        for (std::size_t i = 0; i < COUNT; ++i) total += Swell::AMPLITUDE[i];
        return total * m_height * scale;
    }

    Swell swell(const vector3d& camera) const noexcept {
        Swell out;
        out.height     = static_cast<float>(m_height);
        out.wavenumber = static_cast<float>(m_k);
        out.dir_x      = static_cast<float>(m_dir_x);
        out.dir_y      = static_cast<float>(m_dir_y);

        for (std::size_t i = 0; i < COUNT; ++i) {
            double dx, dy;
            direction(i, dx, dy);
            const double k = m_k * Swell::WAVENUMBER[i];
            const double phase = k * (dx * camera.x + dy * camera.y) - omega(k) * m_seconds;
            out.phase[i] = static_cast<float>(phase - TURN * std::floor(phase / TURN));
        }

        return out;
    }

private:
    static constexpr double MIN_WAVELENGTH = 1.0;
    static constexpr double MIN_WIND       = 1e-6;
    static constexpr double MAX_STORM      = 1.5;

    void direction(std::size_t i, double& dx, double& dy) const noexcept {
        const double c = std::cos(Swell::ANGLES[i]), s = std::sin(Swell::ANGLES[i]);
        dx = m_dir_x * c - m_dir_y * s;
        dy = m_dir_x * s + m_dir_y * c;
    }

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