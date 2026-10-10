#ifndef VOXELSPIRE_UI_COLOR_MATH_HPP
#define VOXELSPIRE_UI_COLOR_MATH_HPP

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include "../core/types.hpp"

namespace voxelspire {

struct Hsv {
    double h = 0.0;
    double s = 0.0;
    double v = 0.0;
};

struct ColorMath {
    static constexpr double CHANNEL     = 255.0;
    static constexpr double HUE_SECTORS = 6.0;
    static constexpr int    HEX_RGB     = 6;
    static constexpr int    HEX_RGBA    = 8;
    static constexpr int    SHORT_RGB   = 3;
    static constexpr int    SHORT_RGBA  = 4;
    static constexpr int    HEX_BASE    = 16;

    static Hsv to_hsv(const Color& c) noexcept;

    static Color from_hsv(const Hsv& hsv, std::uint8_t alpha) noexcept;

    static std::uint8_t byte(double unit) noexcept { return static_cast<std::uint8_t>(std::lround(vclamp(unit, 0.0, 1.0) * CHANNEL)); }

    static std::string to_hex(const Color& c, bool alpha);

    static bool parse_hex(const std::string& raw, bool alpha, const Color& current, Color& out, std::string& error);

    static int channel(const Color& c, int k) noexcept;

    static Color with_channel(const Color& c, int k, int v) noexcept;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_COLOR_MATH_HPP