#include "ui/start_menu.hpp"

namespace voxelspire {

void StartMenu::set_worlds(std::vector<WorldRecord> worlds) {
    const std::string keep = m_selected < m_worlds.size() ? m_worlds[m_selected].folder : std::string();
    m_worlds = std::move(worlds);
    m_selected = 0;
    select(keep);
    m_confirm = 0.0;
}

bool StartMenu::on_event(const fizmo::windows::WindowEvent& e) {
    using fizmo::windows::WindowEventType;

    if (e.type == WindowEventType::MouseScroll) {
        m_scroll_steps += e.scroll_delta;
        return true;
    }

    if (e.button == LEFT_MOUSE && (e.type == WindowEventType::MouseClick || e.type == WindowEventType::MouseDoubleClick)) {
        click(static_cast<int>(e.x), static_cast<int>(e.y), e.type == WindowEventType::MouseDoubleClick);
        return true;
    }

    if (e.type != WindowEventType::KeyPress) return false;
    if (e.key_name == "Enter") { if (selected()) m_action = StartAction::Play; return true; }
    if (e.key_name == "UpArrow" && m_selected > 0) { --m_selected; m_reveal = true; return true; }
    if (e.key_name == "DownArrow" && m_selected + 1 < m_worlds.size()) { ++m_selected; m_reveal = true; return true; }
    if (e.key_name == "Delete") { press_delete(); return true; }
    return false;
}

void StartMenu::update(const fizmo::windows::InputManager& input, double dt) {
    m_mouse_x = input.mouse_x();
    m_mouse_y = input.mouse_y();
    if (m_confirm > 0.0) m_confirm -= dt;
    if (m_status_time > 0.0) m_status_time -= dt;
    m_scroll -= m_scroll_steps * SCROLL_ROWS * m_row_h;
    m_scroll_steps = 0;
}

void StartMenu::render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style) {
    m_hits.clear();
    const double fit = vmin(1.0, vmin((w - MARGIN * 2.0) / PANEL_W, (h - MARGIN * 2.0 - TITLE_SPACE) / PANEL_H));
    m_paint.begin(r, vmax(style.scale * vmax(fit, MIN_FIT), MIN_SCALE));
    MenuPainter& p = m_paint;
    backdrop(static_cast<int>(w), static_cast<int>(h));

    const int pw = vmin(p.px(PANEL_W), static_cast<int>(w) - p.px(MARGIN) * 2);
    const int ph = vmin(p.px(PANEL_H), static_cast<int>(h) - p.px(MARGIN) * 2 - p.px(TITLE_SPACE));
    const int x0 = (static_cast<int>(w) - pw) / 2;
    const int y0 = (static_cast<int>(h) - ph + p.px(TITLE_SPACE)) / 2;
    p.text_centered(static_cast<int>(w) / 2, y0 - p.px(TITLE_SPACE) / 2 - p.px(SUBTITLE_GAP), TITLE, p.px(TITLE_SIZE), style.text, true);
    p.text_centered(static_cast<int>(w) / 2, y0 - p.px(TITLE_SPACE) / 2 + p.px(SUBTITLE_DROP), SUBTITLE, p.px(SMALL_SIZE), style.muted);
    p.rect(x0, y0, pw, ph, style.panel);

    const int pad = p.px(PAD), foot = p.px(FOOTER_H);
    const int list_x = x0 + pad, list_y = y0 + pad, list_w = pw - pad * 2, list_h = ph - pad * 2 - foot;
    draw_list(list_x, list_y, list_w, list_h, style);
    draw_buttons(x0 + pad, y0 + ph - foot, pw - pad * 2, foot, style);
}

void StartMenu::backdrop(int w, int h) {
    for (int i = 0; i < BANDS; ++i) {
        const double t = static_cast<double>(i) / (BANDS - 1);
        auto mix = [t](std::uint8_t a, std::uint8_t b) { return static_cast<std::uint8_t>(a + (b - a) * t); };
        const Color c(mix(SKY_TOP.red(), SKY_BOTTOM.red()), mix(SKY_TOP.green(), SKY_BOTTOM.green()), mix(SKY_TOP.blue(), SKY_BOTTOM.blue()));
        const int y = h * i / BANDS, next = h * (i + 1) / BANDS;
        m_paint.rect(0, y, w, next - y, c);
    }
}

void StartMenu::draw_list(int x, int y, int w, int h, const MenuSettings& style) {
    MenuPainter& p = m_paint;
    m_row_h = p.px(ROW_H);
    const int total = static_cast<int>(m_worlds.size()) * m_row_h;

    if (m_reveal && selected()) {
        const int top = static_cast<int>(m_selected) * m_row_h;
        if (top < m_scroll) m_scroll = top;
        else if (top + m_row_h > m_scroll + h) m_scroll = top + m_row_h - h;
        m_reveal = false;
    }

    m_scroll = vclamp(m_scroll, 0, vmax(0, total - h));
    p.rect(x, y, w, h, MenuPainter::faded(style.sidebar, HALF_ALPHA));

    if (m_worlds.empty()) {
        p.text_centered(x + w / 2, y + h / 2 - p.px(LINE_LIFT), "No worlds yet.", p.px(NAME_SIZE), style.text, true);
        p.text_centered(x + w / 2, y + h / 2 + p.px(LINE_DROP), "Press Create new world to make your first one.", p.px(SMALL_SIZE), style.muted);
        return;
    }

    p.renderer().set_clip_rect(x, y, static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    const std::int64_t now = WorldStore::now();

    for (std::size_t i = 0; i < m_worlds.size(); ++i) {
        const int ry = y - m_scroll + static_cast<int>(i) * m_row_h;
        if (ry + m_row_h < y || ry > y + h) continue;
        const WorldRecord& world = m_worlds[i];
        const bool on = i == m_selected;
            
        if (on) p.rect(x, ry, w, m_row_h, MenuPainter::faded(style.accent, DIM_ALPHA));
        else if (hot(x, ry, w, m_row_h)) p.rect(x, ry, w, m_row_h, style.row_hover);
            
        if (on) p.rect(x, ry, p.px(ACCENT_BAR), m_row_h, style.accent);
        const int tx = x + p.px(PAD);
        p.text(tx, ry + m_row_h / 2 - p.px(LINE_LIFT), world.name, p.px(NAME_SIZE), style.text, true);
        const std::string details = "Played " + ago(now, world.played) + "   -   Created " + date(world.created) + "   -   " + world.folder;
        p.text(tx, ry + m_row_h / 2 + p.px(LINE_DROP), details, p.px(SMALL_SIZE), style.muted);
        const int clip_top = vmax(ry, y), clip_bottom = vmin(ry + m_row_h, y + h);
        if (clip_bottom > clip_top) m_hits.push_back({ x, clip_top, w, clip_bottom - clip_top, Button::Row, i });
    }

    p.renderer().reset_clip_rect();

    if (total > h) {
        const int bar_h = vmax(m_row_h / 2, h * h / total);
        const int bar_y = y + (h - bar_h) * m_scroll / vmax(1, total - h);
        p.rect(x + w - p.px(SCROLLBAR_W), bar_y, p.px(SCROLLBAR_W), bar_h, MenuPainter::faded(style.muted, HALF_ALPHA));
    }
}

void StartMenu::draw_buttons(int x, int y, int w, int h, const MenuSettings& style) {
    MenuPainter& p = m_paint;
    const int gap = p.px(GAP), bh = p.px(BUTTON_H), size = p.px(TEXT_SIZE);
    const int row1 = y + (h / 2 - bh) / 2 + gap / 2, row2 = y + h / 2 + (h / 2 - bh) / 2 - gap / 2;
    const int half = (w - gap) / 2, quarter = (w - gap * 3) / 4;
    const bool any = selected() != nullptr;
    const Color white(255, 255, 255);

    auto button = [&](int bx, int by, int bw, Button which, const std::string& label, const Color& bg, const Color& fg, bool enabled) {
        p.button(bx, by, bw, bh, label, enabled ? bg : MenuPainter::faded(bg, DIM_ALPHA), enabled ? fg : style.muted, enabled && hot(bx, by, bw, bh), size);
        if (enabled) m_hits.push_back({ bx, by, bw, bh, which, 0 });
    };

    button(x, row1, half, Button::Play, any ? "Play " + selected()->name : std::string("Play"), style.accent, white, any);
    button(x + half + gap, row1, half, Button::Create, "Create new world", style.control, style.text, true);
    button(x, row2, quarter, Button::Edit, "Edit world", style.control, style.text, any);
    button(x + (quarter + gap), row2, quarter, Button::Delete, m_confirm > 0.0 ? "Really delete?" : "Delete world", m_confirm > 0.0 ? style.danger : style.control, style.text, any);
    button(x + (quarter + gap) * 2, row2, quarter, Button::Settings, "Settings", style.control, style.text, true);
    button(x + (quarter + gap) * 3, row2, quarter, Button::Quit, "Quit", style.control, style.text, true);

    if (m_status_time > 0.0 && !m_status.empty())
        p.text_centered(x + w / 2, y + h + p.px(PAD) / 2, m_status, p.px(SMALL_SIZE), m_status_problem ? style.error : style.muted);
}

void StartMenu::click(int x, int y, bool twice) {
    m_mouse_x = x;
    m_mouse_y = y;

    for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
        if (!it->contains(x, y)) continue;

        switch (it->button) {
            case Button::Row:
                if (it->row != m_selected) m_confirm = 0.0;
                m_selected = it->row;
                if (twice) m_action = StartAction::Play;
                break;
            case Button::Play:     m_action = StartAction::Play; break;
            case Button::Create:   m_action = StartAction::Create; break;
            case Button::Edit:     m_action = StartAction::Edit; break;
            case Button::Delete:   press_delete(); break;
            case Button::Settings: m_action = StartAction::Settings; break;
            case Button::Quit:     m_action = StartAction::Quit; break;
        }

        return;
    }
}

void StartMenu::press_delete() {
    if (!selected()) return;
    if (m_confirm > 0.0) { m_confirm = 0.0; m_action = StartAction::Delete; return; }
    m_confirm = CONFIRM_SECONDS;
}

std::string StartMenu::ago(std::int64_t now, std::int64_t then) {
    if (then <= 0) return "never";
    const std::int64_t s = vmax<std::int64_t>(0, now - then);
    if (s < MINUTE) return "just now";
    if (s < HOUR) return plural(s / MINUTE, "minute") + " ago";
    if (s < DAY) return plural(s / HOUR, "hour") + " ago";
    if (s < WEEK) return plural(s / DAY, "day") + " ago";
    return "on " + date(then);
}

std::string StartMenu::date(std::int64_t when) {
    if (when <= 0) return "unknown";
    const std::time_t t = static_cast<std::time_t>(when);
    std::tm parts{};
#ifdef OS_WINDOWS
    localtime_s(&parts, &t);
#elif defined(OS_LINUX)
    localtime_r(&t, &parts);
#endif
    char buf[DATE_LENGTH];
    std::strftime(buf, sizeof(buf), "%b %d, %Y", &parts);
    return buf;
}

} // namespace voxelspire
