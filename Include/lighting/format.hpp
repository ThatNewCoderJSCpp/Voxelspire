#ifndef VOXELSPIRE_LIGHTING_LIGHT_FORMAT_HPP
#define VOXELSPIRE_LIGHTING_LIGHT_FORMAT_HPP

#include <cstdint>
#include "../block/block.hpp"

namespace voxelspire {

enum class LightFormat : std::uint8_t { None = 0, Plain, Colored };

constexpr const char* light_format_name(LightFormat f) noexcept {
    switch (f) {
        case LightFormat::None:    return "none";
        case LightFormat::Plain:   return "plain";
        case LightFormat::Colored: return "colored";
    }
    return "?";
}

struct LightLevel {
    static constexpr int BYTE_SCALE = 17;

    std::uint8_t sky   = LightLimits::MAX;
    std::uint8_t red   = 0;
    std::uint8_t green = 0;
    std::uint8_t blue  = 0;

    static constexpr LightLevel open_sky() noexcept { return {}; }
    static constexpr LightLevel dark() noexcept { return { 0, 0, 0, 0 }; }

    constexpr int block() const noexcept { return vmax(vmax(static_cast<int>(red), static_cast<int>(green)), static_cast<int>(blue)); }
    constexpr int brightest() const noexcept { return vmax(static_cast<int>(sky), block()); }
    constexpr int channel(int c) const noexcept { return c == 0 ? red : (c == 1 ? green : blue); }

    void raise_block(int r, int g, int b) noexcept {
        red   = static_cast<std::uint8_t>(vmax(static_cast<int>(red), vclamp(r, 0, LightLimits::MAX)));
        green = static_cast<std::uint8_t>(vmax(static_cast<int>(green), vclamp(g, 0, LightLimits::MAX)));
        blue  = static_cast<std::uint8_t>(vmax(static_cast<int>(blue), vclamp(b, 0, LightLimits::MAX)));
    }

    fizmo::graphics::BakedLight baked() const noexcept {
        auto b = [](int v) { return static_cast<std::uint8_t>(v * BYTE_SCALE); };
        return fizmo::graphics::BakedLight(b(red), b(green), b(blue), b(sky));
    }

    constexpr bool operator==(const LightLevel& o) const noexcept { return sky == o.sky && red == o.red && green == o.green && blue == o.blue; }
    constexpr bool operator!=(const LightLevel& o) const noexcept { return !(*this == o); }
};

struct PackedLight {
    static constexpr int SKY_SHIFT = 12, RED_SHIFT = 8, GREEN_SHIFT = 4, BLUE_SHIFT = 0;
    static constexpr std::uint16_t MASK = 0xF;

    static constexpr std::uint16_t pack(const LightLevel& l) noexcept {
        return static_cast<std::uint16_t>((l.sky << SKY_SHIFT) | (l.red << RED_SHIFT) | (l.green << GREEN_SHIFT) | (l.blue << BLUE_SHIFT));
    }

    static constexpr LightLevel unpack(std::uint16_t v) noexcept {
        return { static_cast<std::uint8_t>((v >> SKY_SHIFT) & MASK), static_cast<std::uint8_t>((v >> RED_SHIFT) & MASK),
                 static_cast<std::uint8_t>((v >> GREEN_SHIFT) & MASK), static_cast<std::uint8_t>((v >> BLUE_SHIFT) & MASK) };
    }

    static constexpr int sky(std::uint16_t v) noexcept { return (v >> SKY_SHIFT) & MASK; }
    static constexpr int channel(std::uint16_t v, int c) noexcept { return (v >> (RED_SHIFT - c * GREEN_SHIFT)) & MASK; }

    static constexpr std::uint16_t OPEN_SKY = static_cast<std::uint16_t>(LightLimits::MAX << SKY_SHIFT);
};

class PlainLight {
public:
    using Cell = std::uint8_t;

    static constexpr LightFormat FORMAT   = LightFormat::Plain;
    static constexpr int         SKY      = 0;
    static constexpr int         CHANNELS = 2;
    static constexpr int         BLOCK_SHIFT = 0, SKY_SHIFT = 4;
    static constexpr Cell        MASK = 0xF;

    static constexpr int get(Cell c, int ch) noexcept { return ch == SKY ? (c >> SKY_SHIFT) & MASK : (c >> BLOCK_SHIFT) & MASK; }

    static constexpr Cell set(Cell c, int ch, int v) noexcept {
        const int shift = ch == SKY ? SKY_SHIFT : BLOCK_SHIFT;
        return static_cast<Cell>((c & ~(MASK << shift)) | ((v & MASK) << shift));
    }

    static constexpr Cell sky_only(int sky) noexcept { return static_cast<Cell>((sky & MASK) << SKY_SHIFT); }
    static constexpr int  emission(const LightEmission& e, int ch) noexcept { return ch == SKY ? 0 : e.level(); }

    static constexpr LightLevel level(Cell c) noexcept {
        const auto b = static_cast<std::uint8_t>(get(c, 1));
        return { static_cast<std::uint8_t>(get(c, SKY)), b, b, b };
    }

    static constexpr std::uint16_t packed(Cell c) noexcept { return PackedLight::pack(level(c)); }
};

class ColoredLight {
public:
    using Cell = std::uint16_t;

    static constexpr LightFormat FORMAT   = LightFormat::Colored;
    static constexpr int         SKY      = 0;
    static constexpr int         CHANNELS = 4;
    static constexpr Cell        MASK     = 0xF;

    static constexpr int shift_of(int ch)    noexcept { return ch == SKY ? PackedLight::SKY_SHIFT : PackedLight::RED_SHIFT - (ch - 1) * PackedLight::GREEN_SHIFT; }
    static constexpr int get(Cell c, int ch) noexcept { return (c >> shift_of(ch)) & MASK; }

    static constexpr Cell set(Cell c, int ch, int v) noexcept {
        const int shift = shift_of(ch);
        return static_cast<Cell>((c & ~(MASK << shift)) | ((v & MASK) << shift));
    }

    static constexpr Cell sky_only(int sky)                        noexcept { return static_cast<Cell>((sky & MASK) << PackedLight::SKY_SHIFT); }
    static constexpr int  emission(const LightEmission& e, int ch) noexcept { return ch == SKY ? 0 : e.channel(ch - 1); }
    static constexpr LightLevel level(Cell c)                      noexcept { return PackedLight::unpack(c); }
    static constexpr std::uint16_t packed(Cell c)                  noexcept { return c; }
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_LIGHT_FORMAT_HPP