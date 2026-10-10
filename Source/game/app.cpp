#include "game/app.hpp"

namespace voxelspire {

App::App(GameSettings settings) : m_settings(std::move(settings)), m_defaults(m_settings), m_store(m_settings.saves.folder) {
    SettingsPages::register_personal(m_pages, title_hooks(), m_lighting);
    rebuild_settings();
    refresh();
}

void App::handle_event(const fizmo::windows::WindowEvent& e) {
    using fizmo::windows::WindowEventType;
    if (e.type == WindowEventType::WindowResize && e.x > 0 && e.y > 0) { m_width = e.x; m_height = e.y; }
    if (m_game) { m_game->handle_event(e); return; }
    const bool escape = e.type == WindowEventType::KeyPress && e.key_name == ESCAPE_KEY;

    if (m_options.is_open()) {
        if (m_options.on_event(e)) return;
        if (escape) m_options.close();
        return;
    }

    if (m_settings_menu.is_open()) {
        if (m_settings_menu.on_event(e)) return;
        if (escape) close_settings();
        return;
    }

    m_start.on_event(e);
}

void App::update(double dt, const fizmo::windows::InputManager& input) {
    if (m_game) {
        m_game->update(dt, input);
        if (m_game->take_quit_request()) leave_world();
        return;
    }

    if (m_options.is_open()) {
        m_options.update(input, dt);
        m_options.take_changes();
        if (m_options.take_cancel_request()) m_options.close();
        else if (m_options.take_close_request()) confirm_options();
        return;
    }

    if (m_settings_menu.is_open()) {
        m_settings_menu.update(input, dt);
        if (m_settings_menu.take_changes() & Apply::Display) m_display_dirty = true;
        if (m_settings_menu.take_close_request()) close_settings();
        return;
    }

    m_start.update(input, dt);
    act(m_start.take_action());
}

void App::render(fizmo::windows::Renderer& r, double fps_average) {
    if (m_game) { m_game->render(r, fps_average); return; }
    m_start.render(r, m_width, m_height, m_settings.menu);
    if (m_options.is_open()) m_options.render(r, m_width, m_height, m_settings.menu);
    if (m_settings_menu.is_open()) m_settings_menu.render(r, m_width, m_height, m_settings.menu);
}

bool App::take_display_change() noexcept {
    bool changed = m_display_dirty;
    m_display_dirty = false;
    if (m_game) changed = m_game->take_display_change() || changed;
    return changed;
}

bool App::play(const std::string& folder) {
    GameSettings world = m_settings;
    WorldRecord record;
    WorldSlot slot;

    if (!m_window || !m_store.load(folder, world, &record, &slot.state)) {
        m_start.set_status("That world could not be opened. Its world.cfg may be missing or damaged.", true);
        return false;
    }

    m_store.touch(folder);
    slot.folder      = m_store.folder_path(folder);
    slot.name        = record.name;
    slot.write_state   = [this, folder](const WorldState& state) { return m_store.save_state(folder, state); };
    slot.write_options = [this, folder](const GameSettings& options, const WorldState& state) { return m_store.save_options(folder, options, state); };
    m_game = std::make_unique<Game>(world, WorldGeneratorFactory{}, std::move(slot));
    m_game->start(*m_window, m_width, m_height);
    m_last_world = folder;
    return true;
}

SettingsHooks App::title_hooks() {
    SettingsHooks h;
    h.save     = [this] { SettingsFile::save(m_settings_menu.tabs(), m_settings.menu.file); };
    h.reload   = [this] { rebuild_settings(); };
    return h;
}

void App::rebuild_settings() {
    m_settings_menu.set_tabs(m_pages.build(m_settings, m_defaults));
    const SettingsLoad loaded = SettingsFile::load(m_settings_menu.tabs(), m_settings.menu.file);
    m_settings_menu.report_problems(loaded.problems);
}

void App::act(StartAction action) {
    const WorldRecord* selected = m_start.selected();

    switch (action) {
        case StartAction::None: break;
        case StartAction::Play:     if (selected) play(selected->folder); break;
        case StartAction::Create:   open_options(nullptr); break;
        case StartAction::Edit:     if (selected) open_options(selected); break;
        case StartAction::Settings: m_settings_menu.open(); break;
        case StartAction::Quit:     m_quit = true; break;
        case StartAction::Delete:
            if (!selected) break;
            if (m_store.remove(selected->folder)) m_start.set_status("Deleted " + selected->name + ".");
            else m_start.set_status("Could not delete " + selected->name + ".", true);
            refresh();
            break;
    }
}

void App::open_options(const WorldRecord* existing) {
    m_draft = m_settings;
    m_draft.world   = WorldSettings{};
    m_draft.terrain = TerrainSettings{};
    m_draft_defaults = m_draft;
    m_editing = existing ? *existing : WorldRecord{};

    if (existing && !m_store.load(existing->folder, m_draft, &m_editing)) {
        m_start.set_status("That world's settings could not be read.", true);
        return;
    }

    m_options.set_tabs(m_store.pages().build(m_draft, m_draft_defaults));
    m_options.set_title(existing ? "Edit world" : "Create world");
    m_options.set_actions(existing ? "Save" : "Create", "Cancel");
    m_options.set_hint(existing ? EDIT_HINT : CREATE_HINT);
    m_options.select_tab(0);
    m_options.open();
}

void App::confirm_options() {
    m_options.close();

    if (m_editing.folder.empty()) {
        const auto created = m_store.create(m_draft);
        if (!created) { m_start.set_status("The world could not be created. Check that the worlds folder can be written to.", true); return; }
        refresh();
        m_start.select(created->folder);
        m_start.set_status("Created " + created->name + ". Press Play to start.");
        return;
    }

    m_editing.name = m_draft.world.name;
    if (m_store.save(m_editing, m_draft)) m_start.set_status("Saved " + m_editing.name + ". Terrain changes reshape every place you have not built in.");
    else m_start.set_status("Could not save " + m_editing.name + ".", true);
    refresh();
}

void App::close_settings() {
    m_settings_menu.close();
    if (m_settings.menu.save_on_close) SettingsFile::save(m_settings_menu.tabs(), m_settings.menu.file);
}

} // namespace voxelspire
