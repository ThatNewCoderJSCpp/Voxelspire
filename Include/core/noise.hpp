#ifndef VOXELSPIRE_CORE_NOISE_HPP
#define VOXELSPIRE_CORE_NOISE_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include "random.hpp"
#include "types.hpp"

namespace voxelspire {

class SimplexNoise {
public:
    static constexpr int PERIOD = 256;

    explicit SimplexNoise(std::uint64_t seed = 0) noexcept;

    double at(double x, double y) const noexcept;

    double at(double x, double y, double z) const noexcept;

private:
    static constexpr double F2       = 0.36602540378443865;
    static constexpr double G2       = 0.21132486540518713;
    static constexpr double F3       = 1.0 / 3.0;
    static constexpr double G3       = 1.0 / 6.0;
    static constexpr double SCALE_2D = 70.0;
    static constexpr double SCALE_3D = 32.0;
    static constexpr double FALLOFF_2D = 0.5;
    static constexpr double FALLOFF_3D = 0.6;

    static int fast_floor(double v) noexcept { const int i = static_cast<int>(v); return v < i ? i - 1 : i; }

    int hash(int i) const noexcept { return m_perm[static_cast<std::size_t>(i & (2 * PERIOD - 1))]; }

    static double corner2(int h, double x, double y) noexcept {
        double t = FALLOFF_2D - x * x - y * y;
        if (t <= 0.0) return 0.0;
        t *= t;
        return t * t * gradient2(h, x, y);
    }

    static double corner3(int h, double x, double y, double z) noexcept {
        double t = FALLOFF_3D - x * x - y * y - z * z;
        if (t <= 0.0) return 0.0;
        t *= t;
        return t * t * gradient3(h, x, y, z);
    }

    static double gradient2(int h, double x, double y) noexcept;

    static double gradient3(int h, double x, double y, double z) noexcept;

    std::array<std::uint8_t, 2 * PERIOD> m_perm{};
};

struct NoiseShape {
    double scale       = 64.0;
    int    octaves     = 4;
    double persistence = 0.5;
    double lacunarity  = 2.0;
};

class FractalNoise {
public:
    static constexpr int    MAX_OCTAVES = 12;
    static constexpr double OFFSET_SPAN = 4096.0;

    FractalNoise() = default;

    FractalNoise(std::uint64_t seed, const NoiseShape& shape) noexcept
;

    double at(double x, double y) const noexcept;

    double at(double x, double y, double z) const noexcept;

private:
    static constexpr std::uint64_t OFFSET_SALT = 0xA5A5F00DCAFEBEEFull;

    SimplexNoise                       m_noise;
    double                             m_frequency   = 1.0;
    int                                m_octaves     = 1;
    double                             m_persistence = 0.5;
    double                             m_lacunarity  = 2.0;
    double                             m_normalize   = 1.0;
    std::array<vector3d, MAX_OCTAVES>  m_offsets{};
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_NOISE_HPP