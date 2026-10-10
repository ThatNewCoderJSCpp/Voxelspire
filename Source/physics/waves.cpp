#include "physics/waves.hpp"

namespace voxelspire {

void WaveField::update(const WaveSettings& s, double storminess, const vector3d& wind, double seconds) noexcept {
    m_height = s.enabled ? s.calm_height + (s.storm_height - s.calm_height) * vclamp(storminess, 0.0, MAX_STORM) : 0.0;
    m_k = TURN / vmax(s.wavelength, MIN_WAVELENGTH);
    m_speed = s.speed;
    m_seconds = seconds;
    const double w = std::sqrt(wind.x * wind.x + wind.y * wind.y);
    if (w > MIN_WIND) { m_dir_x = wind.x / w; m_dir_y = wind.y / w; }
}

double WaveField::at(double x, double y, double scale) const noexcept {
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

double WaveField::crest(double scale) const noexcept {
    double total = 0.0;
    for (std::size_t i = 0; i < COUNT; ++i) total += Swell::AMPLITUDE[i];
    return total * m_height * scale;
}

auto WaveField::swell(const vector3d& camera) const noexcept -> Swell {
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

void WaveField::direction(std::size_t i, double& dx, double& dy) const noexcept {
    const double c = std::cos(Swell::ANGLES[i]), s = std::sin(Swell::ANGLES[i]);
    dx = m_dir_x * c - m_dir_y * s;
    dy = m_dir_x * s + m_dir_y * c;
}

} // namespace voxelspire
