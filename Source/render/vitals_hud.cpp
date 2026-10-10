#include "render/vitals_hud.hpp"

namespace voxelspire {

void VitalsHud::update(double dt, const PlayerSurvival& ps) {
    const Survival& v = ps.vitals();
    const SurvivalSettings& s = ps.settings();

    for (std::size_t i = 0; i < VITALS; ++i) {
        const Vital vital = static_cast<Vital>(i);
        const double f = v.fraction(vital, s);
        const bool steady = f >= 1.0 && v.rate(vital) >= 0.0;
        m_shown_for[i] = steady ? m_shown_for[i] + dt : 0.0;
    }
}

void VitalsHud::render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs, const PlayerSurvival& ps, double now) {
    const VitalsHudSettings& vs = hs.vitals;
    const SurvivalSettings& s = ps.settings();
    if (!vs.show || !s.enabled || ps.dead()) return;
    const double scale = vs.scale * hs.scale;
    const double size = HEART_SIZE * scale;
    const double group_w = PER_ROW * size * HEART_STEP;
    const double left = static_cast<double>(w) * HALF - group_w - size * GROUP_GAP * HALF;
    const double right = static_cast<double>(w) * HALF + size * GROUP_GAP * HALF;
    const double base = static_cast<double>(h) - vs.bottom * scale - size;
    double top = base;

    if (vs.hearts && Survival::shown(Vital::Health, s)) top = hearts(r, ps, vs, left, base, size, now);
    bars(r, ps, vs, right, base, size, group_w);
    if (Survival::shown(Vital::Breath, s) && visible(vs.breath, Vital::Breath, vs)) bubbles(r, ps, vs, right, top - size, size);
}

void VitalsHud::render_panel(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs, const PlayerSurvival& ps, const VitalsContext& ctx, const VitalsArea* area) {
    m_rows.clear();
    build_rows(ps, ctx, hs.vitals);
    const double scale = hs.scale * hs.vitals.scale;
    const double size = hs.text_size * scale;
    const fizmo::text::TextStyle label = style(hs, size, hs.label_color);
    const fizmo::text::TextStyle value = style(hs, size, hs.text_color);
    const fizmo::text::TextStyle head  = style(hs, size * TITLE_SCALE, hs.header_color, true);
    const fizmo::text::TextStyle sub   = style(hs, size, hs.header_color, hs.bold_headers);
    const int pad = px(hs.padding * scale * PANEL_PADDING), line = px(size * hs.line_spacing), gap = px(hs.column_gap * scale);
    const int bar_w = px(size * PANEL_BAR), bar_h = px(size * PANEL_BAR_HEIGHT);
    const int title_h = area ? 0 : px(size * TITLE_SCALE * hs.line_spacing);
    int label_w = 0, value_w = 0, height = title_h + line;

    for (const Row& row : m_rows) {
        if (row.header) { height += line + px(size * hs.section_spacing); continue; }
        label_w = vmax(label_w, text_width(r, row.label, label));
        value_w = vmax(value_w, text_width(r, row.value, value));
        height += line;
    }

    const int inner = vmax(label_w + gap + value_w + gap + bar_w, text_width(r, TITLE, head));
    const int box_w = inner + pad * 2, box_h = height + pad * 2;
    const int x0 = area ? area->x - pad : (static_cast<int>(w) - box_w) / 2;
    const int y0 = area ? area->y - pad : vmax((static_cast<int>(h) - box_h) / 2, 0);
    if (!area) r.draw_rect(x0, y0, static_cast<unsigned int>(box_w), static_cast<unsigned int>(box_h), fizmo::graphics::Paint::fill(hs.vitals.panel_back));
    int y = y0 + pad;
    if (!area) r.draw_text(x0 + pad, y, TITLE, head);
    y += title_h;
    const int lx = x0 + pad, vx = lx + label_w + gap, bx = vx + value_w + gap;

    for (const Row& row : m_rows) {
        if (row.header) {
            y += px(size * hs.section_spacing);
            r.draw_text(lx, y, row.label, sub);
            y += line;
            continue;
        }

        r.draw_text(lx, y, row.label, label);
        r.draw_text(vx, y, row.value, value);

        if (row.fill >= 0.0) {
            const int by = y + (line - bar_h) / 2;
            r.draw_rect(bx, by, static_cast<unsigned int>(bar_w), static_cast<unsigned int>(bar_h), fizmo::graphics::Paint::fill(hs.vitals.bar_back));
            const int filled = px(bar_w * vclamp(row.fill, 0.0, 1.0));
            if (filled > 0) r.draw_rect(bx, by, static_cast<unsigned int>(filled), static_cast<unsigned int>(bar_h), fizmo::graphics::Paint::fill(row.color));
        }

        y += line;
    }

    if (!area) r.draw_text(lx, y + line / 2, "Press " + ctx.close_key + " to close", label);
}

void VitalsHud::render_death(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs, const PlayerSurvival& ps, const VitalsContext& ctx) {
    r.draw_rect(0, 0, w, h, fizmo::graphics::Paint::fill(hs.vitals.death_tint));
    const double size = hs.text_size * hs.scale * hs.vitals.scale;
    const fizmo::text::TextStyle big  = style(hs, size * DEATH_SCALE, hs.text_color, true);
    const fizmo::text::TextStyle body = style(hs, size, hs.text_color);
    const fizmo::text::TextStyle hint = style(hs, size, hs.label_color);
    const std::string message = ps.death_message();
    const std::string press = "Press " + ctx.respawn_key + " to respawn";
    const int cx = static_cast<int>(w) / 2, cy = static_cast<int>(h) / 2;
    r.draw_text(cx - text_width(r, DEATH_TITLE, big) / 2, cy - px(size * DEATH_SCALE * DEATH_RISE), DEATH_TITLE, big);
    r.draw_text(cx - text_width(r, message, body) / 2, cy, message, body);
    r.draw_text(cx - text_width(r, press, hint) / 2, cy + px(size * HINT_DROP), press, hint);
}

std::string VitalsHud::format(const char* f, ...) {
    char buf[TEXT_BUFFER];
    va_list args;
    va_start(args, f);
    std::vsnprintf(buf, sizeof(buf), f, args);
    va_end(args);
    return buf;
}

fizmo::text::TextStyle VitalsHud::style(const HudSettings& hs, double size, const Color& c, bool bold) {
    fizmo::text::TextStyle st(size, c);
    const double offset = hs.shadow_offset * hs.scale;
    if (offset > 0.0) st.set_shadow(offset, offset, 0.0, hs.text_shadow);
    if (bold) st.set_bold();
    return st;
}

bool VitalsHud::visible(VitalsShown mode, Vital v, const VitalsHudSettings& vs) const noexcept {
    if (mode == VitalsShown::Never) return false;
    if (mode == VitalsShown::Always) return true;
    return m_shown_for[static_cast<std::size_t>(v)] < vs.linger;
}

std::vector<fizmo::windows::RenderPoint> VitalsHud::heart_shape(double x, double y, double size, bool half) {
    std::vector<fizmo::windows::RenderPoint> out;
    const double span = CURVE_X * 2.0, tall = CURVE_TOP + CURVE_BOTTOM;
    const double k = size / vmax(span, tall), cx = x + size * HALF, cy = y + CURVE_TOP * k;
    const int last = half ? HEART_POINTS / 2 : HEART_POINTS;

    for (int i = 0; i <= last; ++i) {
        const double t = PI + 2.0 * PI * i / HEART_POINTS;
        const double sx = std::sin(t);
        const double hx = CURVE_X * sx * sx * sx;
        const double hy = CURVE_A * std::cos(t) - CURVE_B * std::cos(2.0 * t) - CURVE_C * std::cos(3.0 * t) - CURVE_D * std::cos(4.0 * t);
        out.push_back({ cx + hx * k, cy - hy * k });
    }

    return out;
}

double VitalsHud::hearts(fizmo::windows::Renderer& r, const PlayerSurvival& ps, const VitalsHudSettings& vs, double left, double base, double size, double now) const {
    const Survival& v = ps.vitals();
    const SurvivalSettings& s = ps.settings();
    const double per = vmax(s.health.per_heart, SurvivalLimits::per_heart.min);
    const int count = vmax(1, static_cast<int>(std::ceil(s.health.max / per - EPSILON)));
    const double hp = v.value(Vital::Health);
    const bool low = v.fraction(Vital::Health, s) <= vs.low_flash;
    const double pulse = low ? HALF + HALF * std::sin(now * PULSE_SPEED) : 0.0;
    double top = base;

    for (int i = 0; i < count; ++i) {
        const int row = i / PER_ROW, col = i % PER_ROW;
        const double x = left + col * size * HEART_STEP;
        const double y = base - row * size * ROW_STEP + (low ? pulse * size * BUBBLE_SHARE * HALF * ((col % 2) ? 1.0 : -1.0) : 0.0);
        top = vmin(top, y);
        const double share = vclamp((hp - i * per) / per, 0.0, 1.0);
        const std::vector<fizmo::windows::RenderPoint> shape = heart_shape(x, y, size, false);
        r.draw_polygon(shape, fizmo::graphics::Paint::fill(vs.heart_empty));
        if (share >= FULL_SHARE) r.draw_polygon(shape, fizmo::graphics::Paint::fill(vs.heart));
        else if (share >= HALF_SHARE) r.draw_polygon(heart_shape(x, y, size, true), fizmo::graphics::Paint::fill(vs.heart));
        r.draw_polyline(shape, fizmo::graphics::Paint::stroke(vs.heart_outline, static_cast<unsigned int>(vmax(1L, std::lround(OUTLINE * size / HEART_SIZE)))), true);
    }

    return top;
}

void VitalsHud::bars(fizmo::windows::Renderer& r, const PlayerSurvival& ps, const VitalsHudSettings& vs, double x, double base, double size, double width) const {
    const Survival& v = ps.vitals();
    const SurvivalSettings& s = ps.settings();
    const struct { Vital vital; VitalsShown mode; Color color; } list[] = {
        { Vital::Hunger,  vs.hunger,  vs.hunger_color },
        { Vital::Thirst,  vs.thirst,  vs.thirst_color },
        { Vital::Stamina, vs.stamina, vs.stamina_color },
    };
    const double bar_h = size * BAR_HEIGHT, step = bar_h + size * BAR_GAP;
    double y = base + size - bar_h;

    for (const auto& b : list) {
        if (!Survival::shown(b.vital, s) || !visible(b.mode, b.vital, vs)) continue;
        r.draw_rect(px(x), px(y), static_cast<unsigned int>(px(width)), static_cast<unsigned int>(px(bar_h)), fizmo::graphics::Paint::fill(vs.bar_back));
        const int filled = px(width * v.fraction(b.vital, s));
        if (filled > 0) r.draw_rect(px(x), px(y), static_cast<unsigned int>(filled), static_cast<unsigned int>(px(bar_h)), fizmo::graphics::Paint::fill(b.color));
        r.draw_rect(px(x), px(y), static_cast<unsigned int>(px(width)), static_cast<unsigned int>(px(bar_h)), fizmo::graphics::Paint::stroke(vs.heart_outline, 1));
        y -= step;
    }
}

void VitalsHud::bubbles(fizmo::windows::Renderer& r, const PlayerSurvival& ps, const VitalsHudSettings& vs, double x, double y, double size) const {
    const Survival& v = ps.vitals();
    const double f = v.fraction(Vital::Breath, ps.settings());
    const int full = static_cast<int>(std::ceil(f * PER_ROW - EPSILON));
    const unsigned int radius = static_cast<unsigned int>(vmax(1L, std::lround(size * BUBBLE_SHARE)));

    for (int i = 0; i < full; ++i) {
        const int cx = px(x + (i + HALF) * size * HEART_STEP), cy = px(y + size * HALF);
        r.draw_circle(cx, cy, radius, fizmo::graphics::Paint::fill(vs.breath_color));
        r.draw_circle(cx, cy, radius, fizmo::graphics::Paint::stroke(vs.heart_outline, 1));
    }
}

std::string VitalsHud::rate_text(double per_min, bool per_second) {
    const double r = per_second ? per_min / SECONDS_PER_MIN : per_min;
    if (std::fabs(r) < 0.005) return "steady";
    return format("%+.2f per %s", r, per_second ? "second" : "minute");
}

std::string VitalsHud::temperature(double c, TemperatureUnit unit) const {
    if (unit == TemperatureUnit::Fahrenheit) return format("%.1f F", c * FAHRENHEIT_SCALE + FAHRENHEIT_ZERO);
    return format("%.1f C", c);
}

void VitalsHud::meter(const Survival& v, const SurvivalSettings& s, Vital vital, const char* name, const char* unit, bool per_second, const Color& color, const std::string& note) {
    const double max = Survival::max_of(vital, s);
    std::string text = format("%.1f / %.0f%s, %s", v.value(vital), max, unit, rate_text(v.rate(vital), per_second).c_str());
    if (!note.empty()) text += ", " + note;
    m_rows.push_back({ name, text, v.fraction(vital, s), color });
}

const char* VitalsHud::need_note(const NeedSettings& n, double value, bool lacking) {
    if (lacking) return "empty, hurting you";
    if (Survival::weak(n, value)) return "low, slowing you";
    return "";
}

void VitalsHud::build_rows(const PlayerSurvival& ps, const VitalsContext& ctx, const VitalsHudSettings& vs) {
    const Survival& v = ps.vitals();
    const SurvivalSettings& s = ps.settings();

    if (!s.enabled) {
        line("Survival", "off in this world");
        return;
    }

    header("Body");

    if (s.health.enabled) {
        const char* note = v.regenerating() ? "healing" : "";
        meter(v, s, Vital::Health, "Health", "", false, vs.heart, note);
    }

    if (s.hunger.enabled) meter(v, s, Vital::Hunger, "Hunger", "", false, vs.hunger_color, need_note(s.hunger, v.value(Vital::Hunger), v.starving()));
    if (s.thirst.enabled) meter(v, s, Vital::Thirst, "Thirst", "", false, vs.thirst_color, need_note(s.thirst, v.value(Vital::Thirst), v.parched()));

    if (s.digestion.enabled && (s.hunger.enabled || s.thirst.enabled)) {
        const Stomach& st = v.stomach();
        const std::string text = st.empty() ? std::string("empty") : format("%.0f / %.0f full, done in %.1f min", st.bulk, s.digestion.capacity, v.digest_seconds(s) / SECONDS_PER_MIN);
        m_rows.push_back({ "Digesting", text + (v.stuffed(s) ? ", stuffed" : ""), s.digestion.capacity > 0.0 ? st.bulk / s.digestion.capacity : 0.0, vs.hunger_color });
    }

    if (s.stamina.enabled) meter(v, s, Vital::Stamina, "Stamina", "", true, vs.stamina_color, v.exhausted() ? "exhausted" : "");
    if (s.breath.enabled) meter(v, s, Vital::Breath, "Breath", " s", true, vs.breath_color, v.drowning(s) ? "drowning" : "");
    if (ctx.heat_shown) line("Body temperature", temperature(ctx.body_temperature, ctx.unit) + ", " + ctx.body_state);

    if (s.weight.enabled) {
        header("Load");
        const double load = ps.load();
        const double level = Survival::load_level(load, s.weight);
        const char* state = level > 1.0 ? "overloaded" : level > 0.0 ? "heavy" : "comfortable";
        m_rows.push_back({ "Carrying", format("%.1f kg, %s (comfortable up to %.0f, most %.0f)", load, state, s.weight.comfortable, s.weight.max), s.weight.max > 0.0 ? load / s.weight.max : 0.0, vs.stamina_color });
        for (const LoadPart& part : ps.load_parts()) line("  " + part.name, format("%.1f kg", part.kilograms));
    }

    header("Right now");
    line("Doing", ctx.activity);
    const std::vector<std::string> effects = effect_list(ps);
    line("Effects", effects.empty() ? std::string("none") : join(effects));
    if (s.thirst.enabled && s.drink.from_water) line("Drinking", ctx.water_in_reach ? "hold " + ctx.drink_key + " to drink" : "look at water and hold " + ctx.drink_key);

    bool any = false;

    for (const DamageRecord& d : v.recent()) {
        const double ago = ctx.now - d.time;
        if (ago > RECENT_SECONDS) continue;
        if (!any) header("Recent damage");
        any = true;
        const DamageType* t = ps.damage_types().find(d.type);
        line(t ? t->name : d.type.str(), format("-%.1f, %.0f s ago", d.amount, ago));
    }
}

std::vector<std::string> VitalsHud::effect_list(const PlayerSurvival& ps) {
    const Survival& v = ps.vitals();
    const SurvivalSettings& s = ps.settings();
    std::vector<std::string> out;
    if (s.stamina.enabled && v.exhausted()) out.push_back("exhausted: no sprinting, sinking in water");
    if (Survival::weak(s.hunger, v.value(Vital::Hunger))) out.push_back("hungry: slower");
    if (Survival::weak(s.thirst, v.value(Vital::Thirst))) out.push_back("thirsty: slower");
    if (v.stuffed(s)) out.push_back("stuffed: a little slower");
    const double level = s.weight.enabled ? Survival::load_level(ps.load(), s.weight) : 0.0;
    if (level > 1.0) out.push_back("overloaded: much slower");
    else if (level > 0.0) out.push_back("heavy load: slower");
    return out;
}

} // namespace voxelspire
