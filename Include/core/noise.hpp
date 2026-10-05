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

    explicit SimplexNoise(std::uint64_t seed = 0) noexcept {
        std::array<std::uint8_t, PERIOD> p{};
        for (int i = 0; i < PERIOD; ++i) p[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i);
        SeededRandom rng(seed);

        for (int i = PERIOD - 1; i > 0; --i) {
            const int j = rng.integer(0, i);
            const std::uint8_t t = p[static_cast<std::size_t>(i)];
            p[static_cast<std::size_t>(i)] = p[static_cast<std::size_t>(j)];
            p[static_cast<std::size_t>(j)] = t;
        }

        for (int i = 0; i < 2 * PERIOD; ++i) m_perm[static_cast<std::size_t>(i)] = p[static_cast<std::size_t>(i & (PERIOD - 1))];
    }

    double at(double x, double y) const noexcept {
        const double s = (x + y) * F2;
        const int i = fast_floor(x + s), j = fast_floor(y + s);
        const double t = (i + j) * G2;
        const double x0 = x - (i - t), y0 = y - (j - t);
        const int i1 = x0 > y0 ? 1 : 0, j1 = x0 > y0 ? 0 : 1;
        const double x1 = x0 - i1 + G2, y1 = y0 - j1 + G2;
        const double x2 = x0 - 1.0 + 2.0 * G2, y2 = y0 - 1.0 + 2.0 * G2;
        const int ii = i & (PERIOD - 1), jj = j & (PERIOD - 1);
        const double n0 = corner2(hash(ii + hash(jj)), x0, y0);
        const double n1 = corner2(hash(ii + i1 + hash(jj + j1)), x1, y1);
        const double n2 = corner2(hash(ii + 1 + hash(jj + 1)), x2, y2);
        return SCALE_2D * (n0 + n1 + n2);
    }

    double at(double x, double y, double z) const noexcept {
        const double s = (x + y + z) * F3;
        const int i = fast_floor(x + s), j = fast_floor(y + s), k = fast_floor(z + s);
        const double t = (i + j + k) * G3;
        const double x0 = x - (i - t), y0 = y - (j - t), z0 = z - (k - t);
        int i1, j1, k1, i2, j2, k2;

        if (x0 >= y0) {
            if (y0 >= z0)      { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
            else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
            else               { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
        } else {
            if (y0 < z0)       { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
            else if (x0 < z0)  { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
            else               { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
        }

        const double x1 = x0 - i1 + G3, y1 = y0 - j1 + G3, z1 = z0 - k1 + G3;
        const double x2 = x0 - i2 + 2.0 * G3, y2 = y0 - j2 + 2.0 * G3, z2 = z0 - k2 + 2.0 * G3;
        const double x3 = x0 - 1.0 + 3.0 * G3, y3 = y0 - 1.0 + 3.0 * G3, z3 = z0 - 1.0 + 3.0 * G3;
        const int ii = i & (PERIOD - 1), jj = j & (PERIOD - 1), kk = k & (PERIOD - 1);
        const double n0 = corner3(hash(ii + hash(jj + hash(kk))), x0, y0, z0);
        const double n1 = corner3(hash(ii + i1 + hash(jj + j1 + hash(kk + k1))), x1, y1, z1);
        const double n2 = corner3(hash(ii + i2 + hash(jj + j2 + hash(kk + k2))), x2, y2, z2);
        const double n3 = corner3(hash(ii + 1 + hash(jj + 1 + hash(kk + 1))), x3, y3, z3);
        return SCALE_3D * (n0 + n1 + n2 + n3);
    }

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

    static double gradient2(int h, double x, double y) noexcept {
        switch (h & 7) {
            case 0:  return  x + y;
            case 1:  return -x + y;
            case 2:  return  x - y;
            case 3:  return -x - y;
            case 4:  return  x;
            case 5:  return -x;
            case 6:  return  y;
            default: return -y;
        }
    }

    static double gradient3(int h, double x, double y, double z) noexcept {
        switch (h % 12) {
            case 0:  return  x + y;
            case 1:  return -x + y;
            case 2:  return  x - y;
            case 3:  return -x - y;
            case 4:  return  x + z;
            case 5:  return -x + z;
            case 6:  return  x - z;
            case 7:  return -x - z;
            case 8:  return  y + z;
            case 9:  return -y + z;
            case 10: return  y - z;
            default: return -y - z;
        }
    }

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
        : m_noise(seed), m_frequency(1.0 / (shape.scale > 0.0 ? shape.scale : 1.0)), m_octaves(vclamp(shape.octaves, 1, MAX_OCTAVES)),
          m_persistence(shape.persistence), m_lacunarity(shape.lacunarity) {
        SeededRandom rng(seed ^ OFFSET_SALT);
        double amplitude = 1.0, total = 0.0;

        for (int i = 0; i < m_octaves; ++i) {
            m_offsets[static_cast<std::size_t>(i)] = { rng.range(-OFFSET_SPAN, OFFSET_SPAN), rng.range(-OFFSET_SPAN, OFFSET_SPAN), rng.range(-OFFSET_SPAN, OFFSET_SPAN) };
            total += amplitude;
            amplitude *= m_persistence;
        }

        m_normalize = total > 0.0 ? 1.0 / total : 1.0;
    }

    double at(double x, double y) const noexcept {
        double sum = 0.0, amplitude = 1.0, f = m_frequency;

        for (int i = 0; i < m_octaves; ++i) {
            const vector3d& o = m_offsets[static_cast<std::size_t>(i)];
            sum += amplitude * m_noise.at(x * f + o.x, y * f + o.y);
            amplitude *= m_persistence;
            f *= m_lacunarity;
        }

        return sum * m_normalize;
    }

    double at(double x, double y, double z) const noexcept {
        double sum = 0.0, amplitude = 1.0, f = m_frequency;

        for (int i = 0; i < m_octaves; ++i) {
            const vector3d& o = m_offsets[static_cast<std::size_t>(i)];
            sum += amplitude * m_noise.at(x * f + o.x, y * f + o.y, z * f + o.z);
            amplitude *= m_persistence;
            f *= m_lacunarity;
        }

        return sum * m_normalize;
    }

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