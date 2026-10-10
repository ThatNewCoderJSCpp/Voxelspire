#include "ui/color_math.hpp"

namespace voxelspire {

Hsv ColorMath::to_hsv(const Color& c) noexcept {
    const double r = c.red() / CHANNEL, g = c.green() / CHANNEL, b = c.blue() / CHANNEL;
    const double hi = vmax(r, vmax(g, b)), lo = vmin(r, vmin(g, b)), d = hi - lo;
    Hsv out;
    out.v = hi;
    out.s = hi > 0.0 ? d / hi : 0.0;
    if (d <= 0.0) return out;
    double h = 0.0;
    if (hi == r) h = std::fmod((g - b) / d, HUE_SECTORS);
    else if (hi == g) h = (b - r) / d + 2.0;
    else h = (r - g) / d + 4.0;
    if (h < 0.0) h += HUE_SECTORS;
    out.h = h / HUE_SECTORS;
    return out;
}

Color ColorMath::from_hsv(const Hsv& hsv, std::uint8_t alpha) noexcept {
    const double h = (hsv.h - std::floor(hsv.h)) * HUE_SECTORS;
    const double s = vclamp(hsv.s, 0.0, 1.0), v = vclamp(hsv.v, 0.0, 1.0);
    const double c = v * s, x = c * (1.0 - std::fabs(std::fmod(h, 2.0) - 1.0)), m = v - c;
    double r = 0.0, g = 0.0, b = 0.0;

    switch (static_cast<int>(h) % static_cast<int>(HUE_SECTORS)) {
        case 0:  r = c; g = x; break;
        case 1:  r = x; g = c; break;
        case 2:  g = c; b = x; break;
        case 3:  g = x; b = c; break;
        case 4:  r = x; b = c; break;
        default: r = c; b = x; break;
    }
        
    return Color(byte(r + m), byte(g + m), byte(b + m), alpha);
}

std::string ColorMath::to_hex(const Color& c, bool alpha) {
    char buf[16];
    if (alpha) std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c.red(), c.green(), c.blue(), c.alpha());
    else std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.red(), c.green(), c.blue());
    return buf;
}

bool ColorMath::parse_hex(const std::string& raw, bool alpha, const Color& current, Color& out, std::string& error) {
    std::string text = raw;
    if (!text.empty() && text[0] == '#') text.erase(0, 1);
    const int n = static_cast<int>(text.size());
    const std::string shown = "\"" + raw + "\"";

    for (char ch : text) {
        if (std::isxdigit(static_cast<unsigned char>(ch))) continue;
        error = shown + " is not a hex color.";
        return false;
    }

    const bool fits = n == HEX_RGB || n == SHORT_RGB || (alpha && (n == HEX_RGBA || n == SHORT_RGBA));

    if (!fits) {
        error = shown + (alpha ? " needs 3, 4, 6 or 8 hex digits." : " needs 3 or 6 hex digits.");
        return false;
    }

    const bool short_form = n == SHORT_RGB || n == SHORT_RGBA;
    const int width = short_form ? 1 : 2;
    const int channels = n / width;
    int v[4] = { 0, 0, 0, current.alpha() };

    for (int k = 0; k < channels; ++k) {
        const int value = std::stoi(text.substr(static_cast<std::size_t>(k * width), static_cast<std::size_t>(width)), nullptr, HEX_BASE);
        v[k] = short_form ? value * HEX_BASE + value : value;
    }

    out = Color(static_cast<std::uint8_t>(v[0]), static_cast<std::uint8_t>(v[1]), static_cast<std::uint8_t>(v[2]), static_cast<std::uint8_t>(v[3]));
    return true;
}

int ColorMath::channel(const Color& c, int k) noexcept {
    switch (k) {
        case 0:  return c.red();
        case 1:  return c.green();
        case 2:  return c.blue();
        default: return c.alpha();
    }
}

Color ColorMath::with_channel(const Color& c, int k, int v) noexcept {
    const auto b = static_cast<std::uint8_t>(vclamp(v, 0, static_cast<int>(CHANNEL)));

    switch (k) {
        case 0:  return Color(b, c.green(), c.blue(), c.alpha());
        case 1:  return Color(c.red(), b, c.blue(), c.alpha());
        case 2:  return Color(c.red(), c.green(), b, c.alpha());
        default: return Color(c.red(), c.green(), c.blue(), b);
    }
}

} // namespace voxelspire
