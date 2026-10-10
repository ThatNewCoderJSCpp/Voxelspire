#ifndef VOXELSPIRE_UI_SETTINGS_MODEL_HPP
#define VOXELSPIRE_UI_SETTINGS_MODEL_HPP

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "../core/limits.hpp"
#include "../core/settings.hpp"

namespace voxelspire {

struct Apply {
    static constexpr std::uint32_t Nothing     = 0;
    static constexpr std::uint32_t Display     = 1u << 0;
    static constexpr std::uint32_t Camera      = 1u << 1;
    static constexpr std::uint32_t Character   = 1u << 2;
    static constexpr std::uint32_t Physics     = 1u << 3;
    static constexpr std::uint32_t Sky         = 1u << 4;
    static constexpr std::uint32_t Lighting    = 1u << 5;
    static constexpr std::uint32_t LightFormat = 1u << 6;
    static constexpr std::uint32_t Terrain     = 1u << 7;
    static constexpr std::uint32_t Streaming   = 1u << 8;
    static constexpr std::uint32_t Particles   = 1u << 9;
    static constexpr std::uint32_t Entities    = 1u << 10;
    static constexpr std::uint32_t Player      = 1u << 12;
    static constexpr std::uint32_t Celestial   = 1u << 13;
    static constexpr std::uint32_t Everything  = ~0u;
};

enum class ControlKind : std::uint8_t { Header = 0, Toggle, Integer, Decimal, Choice, Color, Binding, Button, Text };

struct AltUnit {
    static constexpr double FAHRENHEIT_SCALE  = 1.8;
    static constexpr double FAHRENHEIT_OFFSET = 32.0;

    std::string unit;
    double      scale  = 0.0;
    double      offset = 0.0;

    bool   active()        const noexcept { return scale != 0.0; }
    double to(double v)    const noexcept { return v * scale + offset; }
    double from(double v)  const noexcept { return (v - offset) / scale; }
    Bounds bounds(Bounds b) const noexcept;
};

struct NumberFormat {
    static constexpr int DEFAULT_DECIMALS = 2;

    Bounds      limits;
    double      step     = 0.0;
    int         decimals = DEFAULT_DECIMALS;
    std::string unit;
    bool        log      = false;
    AltUnit     alt;
};

struct SettingControl {
    ControlKind   kind = ControlKind::Header;
    std::string   key;
    std::string   label;
    std::string   description;
    std::uint32_t apply   = Apply::Nothing;
    bool          persist = true;
    std::function<bool()> active;

    std::function<bool()>       get_bool;
    std::function<void(bool)>   set_bool;

    std::function<double()>     get_number;
    std::function<void(double)> set_number;
    std::function<double()>     default_number;
    NumberFormat                number;
    std::function<Bounds()>     live_limits;

    std::vector<std::string>                  options;
    std::function<std::vector<std::string>()> options_source;
    std::function<int()>                      get_choice;
    std::function<void(int)>                  set_choice;

    std::function<Color()>      get_color;
    std::function<void(Color)>  set_color;
    std::function<Color()>      default_color;
    bool                        with_alpha = false;

    std::function<std::string()>      get_text;
    std::function<void(std::string)>  set_text;
    std::size_t                       max_length = 0;

    Action                      action = Action::MoveForward;
    InputBindings*              bindings = nullptr;
    std::function<void()>       press;

    std::function<void()> reset;
    std::function<bool()> is_default;

    bool enabled() const { return !active || active(); }
    bool interactive() const noexcept { return kind != ControlKind::Header; }
    bool is_number() const noexcept { return kind == ControlKind::Integer || kind == ControlKind::Decimal; }
    bool integer() const noexcept { return kind == ControlKind::Integer; }
    int  decimals() const noexcept { return integer() ? 0 : vmax(number.decimals, 0); }

    Bounds limits() const;

    Bounds slider() const { return limits().usual(); }

    double step() const {
        if (number.step > 0.0) return number.step;
        return integer() ? 1.0 : std::pow(10.0, -decimals());
    }

    std::vector<std::string> choices() const { return options_source ? options_source() : options; }
};

enum class SettingsScope : std::uint8_t { Personal = 0, World };

struct SettingsTab {
    std::string                 id;
    std::string                 name;
    std::string                 summary;
    std::vector<SettingControl> controls;
    std::string                 group;
    SettingsScope               scope = SettingsScope::Personal;
};

struct NumberCheck {
    bool        ok = false;
    double      value = 0.0;
    std::string error;
};

std::string trimmed(const std::string& s);

std::string format_number(double v, int decimals);

inline std::string format_limit(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.10g", v);
    return buf;
}

bool looks_numeric(const std::string& text) noexcept;

NumberCheck check_value(const std::string& raw, Bounds b, bool whole);

inline NumberCheck check_number(const SettingControl& c, const std::string& raw) { return check_value(raw, c.limits(), c.integer()); }

template <typename A>
auto field(A GameSettings::*a) {
    return [a](GameSettings& g) -> A& { return g.*a; };
}

template <typename A, typename B>
auto field(A GameSettings::*a, B A::*b) {
    return [a, b](GameSettings& g) -> B& { return (g.*a).*b; };
}

template <typename A, typename B, typename C>
auto field(A GameSettings::*a, B A::*b, C B::*c) {
    return [a, b, c](GameSettings& g) -> C& { return ((g.*a).*b).*c; };
}

template <typename A, typename B, typename C, typename D>
auto field(A GameSettings::*a, B A::*b, C B::*c, D C::*d) {
    return [a, b, c, d](GameSettings& g) -> D& { return (((g.*a).*b).*c).*d; };
}

template <typename T>
struct Value {
    T* target = nullptr;
    T  fallback{};
};

template <typename T>
Value<T> value(T& target, T fallback) { return { &target, fallback }; }

template <typename F>
struct FieldSlot {
    F             at;
    GameSettings* live;
    GameSettings* defaults;

    decltype(auto) get() const { return at(*live); }
    auto fallback() const { return at(*defaults); }
};

template <typename T>
struct ValueSlot {
    T* target;
    T  initial;

    T& get() const { return *target; }
    T  fallback() const { return initial; }
};

std::string slug(const std::string& text);

class SettingsPage {
public:
    SettingsPage(SettingsTab& tab, GameSettings& live, GameSettings& defaults)
        : m_tab(tab), m_live(live), m_defaults(defaults) {}

    SettingsPage& applies(std::uint32_t groups) { m_apply = groups; return *this; }
    SettingsPage& when(std::function<bool()> active) { m_active = std::move(active); return *this; }
    SettingsPage& always() { m_active = {}; return *this; }
    SettingsPage& touching(std::function<void()> touch) { m_touch = std::move(touch); return *this; }

    SettingsPage& header(const std::string& label);

    template <typename S>
    SettingsPage& toggle(const std::string& label, const std::string& description, S source) {
        SettingControl c = base(ControlKind::Toggle, label, description);
        const auto s = slot(source);
        c.get_bool   = [s] { return static_cast<bool>(s.get()); };
        c.set_bool   = [s](bool v) { s.get() = v; };
        c.reset      = [s] { s.get() = s.fallback(); };
        c.is_default = [s] { return s.get() == s.fallback(); };
        return add(std::move(c));
    }

    SettingsPage& custom_toggle(
        const std::string& label,
        const std::string& description,
        std::function<bool()> get,
        std::function<void(bool)> set
    );

    template <typename S>
    SettingsPage& integer(const std::string& label, const std::string& description, S source, Bounds limits) {
        return number(ControlKind::Integer, label, description, slot(source), limits);
    }

    template <typename S>
    SettingsPage& decimal(const std::string& label, const std::string& description, S source, Bounds limits) {
        using T = std::decay_t<decltype(slot(source).get())>;
        static_assert(std::is_floating_point_v<T>, "decimal() needs a floating point setting, use integer() for whole numbers");
        return number(ControlKind::Decimal, label, description, slot(source), limits);
    }

    SettingsPage& custom_integer(
        const std::string& label, 
        const std::string& description, 
        std::function<double()> get,
        std::function<void(double)> set, 
        Bounds limits
    );

    SettingsPage& custom_decimal(
        const std::string& label, 
        const std::string& description, 
        std::function<double()> get,
        std::function<void(double)> set, 
        Bounds limits
    );

    template <typename S>
    SettingsPage& choice(
        const std::string& label, 
        const std::string& description, 
        S source, 
        std::vector<std::string> options
    ) {
        SettingControl c = base(ControlKind::Choice, label, description);
        const auto s = slot(source);
        using T = std::decay_t<decltype(s.get())>;
        const int last = static_cast<int>(options.size()) - 1;
        c.options    = std::move(options);
        c.get_choice = [s] { return static_cast<int>(s.get()); };
        c.set_choice = [s, last](int v) { s.get() = static_cast<T>(vclamp(v, 0, vmax(last, 0))); };
        c.reset      = [s] { s.get() = s.fallback(); };
        c.is_default = [s] { return s.get() == s.fallback(); };
        return add(std::move(c));
    }

    SettingsPage& custom_choice(
        const std::string& label, 
        const std::string& description, 
        std::function<std::vector<std::string>()> options,
        std::function<int()> get, 
        std::function<void(int)> set
    );

    template <typename S>
    SettingsPage& color(const std::string& label, const std::string& description, S source, bool with_alpha = false) {
        SettingControl c = base(ControlKind::Color, label, description);
        const auto s = slot(source);
        c.with_alpha    = with_alpha;
        c.get_color     = [s] { return s.get(); };
        c.set_color     = [s, with_alpha](Color v) { s.get() = with_alpha ? v : Color(v.red(), v.green(), v.blue(), s.get().alpha()); };
        c.default_color = [s] { return s.fallback(); };
        c.reset         = [s] { s.get() = s.fallback(); };
        c.is_default    = [s] { return s.get() == s.fallback(); };
        return add(std::move(c));
    }

    SettingsPage& binding(const ActionInfo& info);

    template <typename S>
    SettingsPage& text(const std::string& label, const std::string& description, S source, std::size_t max_length) {
        SettingControl c = base(ControlKind::Text, label, description);
        const auto s = slot(source);
        c.max_length = max_length;
        c.get_text   = [s] { return s.get(); };
        c.set_text   = [s](std::string v) { s.get() = std::move(v); };
        c.reset      = [s] { s.get() = s.fallback(); };
        c.is_default = [s] { return s.get() == s.fallback(); };
        return add(std::move(c));
    }

    SettingsPage& button(const std::string& label, const std::string& description, std::function<void()> press);

    SettingsPage& unit(const std::string& text) { last().number.unit = text; return *this; }
    SettingsPage& decimals(int count) { last().number.decimals = vmax(count, 0); return *this; }
    SettingsPage& step(double size) { last().number.step = vmax(size, 0.0); return *this; }
    SettingsPage& logarithmic() { last().number.log = true; return *this; }
    SettingsPage& also_in(const std::string& text, double scale, double offset = 0.0) { last().number.alt = { text, scale, offset }; return *this; }
    SettingsPage& celsius() { unit("C"); return also_in("F", AltUnit::FAHRENHEIT_SCALE, AltUnit::FAHRENHEIT_OFFSET); }
    SettingsPage& celsius_change() { unit("C"); return also_in("F", AltUnit::FAHRENHEIT_SCALE); }
    SettingsPage& live_limits(std::function<Bounds()> limits) { last().live_limits = std::move(limits); return *this; }
    SettingsPage& transient() { last().persist = false; return *this; }

    SettingsPage& defaults(std::function<void()> reset, std::function<bool()> is_default);

    SettingControl& last() { return m_tab.controls.back(); }
    SettingsTab&    tab() noexcept { return m_tab; }
    GameSettings&   live() noexcept { return m_live; }
    GameSettings&   base_settings() noexcept { return m_defaults; }

private:
    static constexpr double SAME_TOLERANCE = 1e-7;

    static bool same(double a, double b) noexcept { return std::fabs(a - b) <= SAME_TOLERANCE * vmax(1.0, std::fabs(b)); }

    template <typename F>
    FieldSlot<F> slot(F at) const { return { at, &m_live, &m_defaults }; }

    template <typename T>
    ValueSlot<T> slot(Value<T> v) const { return { v.target, v.fallback }; }

    template <typename T>
    static T convert(double v) {
        if constexpr (std::is_same_v<T, bool>) return v != 0.0;
        else if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_unsigned_v<T>) return static_cast<T>(std::llround(vmax(v, 0.0)));
            else return static_cast<T>(std::llround(v));
        } else {
            return static_cast<T>(v);
        }
    }

    template <typename Slot>
    SettingsPage& number(
        ControlKind kind, 
        const std::string& label, 
        const std::string& description, 
        Slot s, 
        Bounds limits
    ) {
        using T = std::decay_t<decltype(s.get())>;
        static_assert(std::is_arithmetic_v<T>, "number settings must be arithmetic");
        SettingControl c = base(kind, label, description);
        const bool whole = kind == ControlKind::Integer;
        c.number.limits  = limits;
        c.get_number     = [s] { return static_cast<double>(s.get()); };
        c.set_number     = [s, limits, whole](double v) { const double k = limits.clamp(v); s.get() = convert<T>(whole ? std::round(k) : k); };
        c.default_number = [s] { return static_cast<double>(s.fallback()); };
        c.reset          = [s] { s.get() = s.fallback(); };
        c.is_default     = [s] { return same(static_cast<double>(s.get()), static_cast<double>(s.fallback())); };
        return add(std::move(c));
    }

    SettingsPage& custom_number(
        ControlKind kind, 
        const std::string& label, 
        const std::string& description, 
        std::function<double()> get,
        std::function<void(double)> set, 
        Bounds limits
    );

    SettingControl base(ControlKind kind, const std::string& label, const std::string& description) const;

    SettingsPage& add(SettingControl c) {
        if (m_touch) wrap(c, m_touch);
        m_tab.controls.push_back(std::move(c));
        return *this;
    }

    static void wrap(SettingControl& c, const std::function<void()>& touch);

    SettingsTab&          m_tab;
    GameSettings&         m_live;
    GameSettings&         m_defaults;
    std::uint32_t         m_apply = Apply::Nothing;
    std::function<bool()> m_active;
    std::function<void()> m_touch;
    std::string           m_section;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_MODEL_HPP