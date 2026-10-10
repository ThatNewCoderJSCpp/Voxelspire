#ifndef VOXELSPIRE_GAME_APP_HPP
#define VOXELSPIRE_GAME_APP_HPP

#include <memory>
#include <string>
#include "../ui/settings_menu.hpp"
#include "../ui/start_menu.hpp"
#include "../ui/world_store.hpp"
#include "game.hpp"

namespace voxelspire {

class App {
public:
    explicit App(GameSettings settings = GameSettings{})
;

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void start(fizmo::windows::Window& window, unsigned int width, unsigned int height) noexcept {
        m_window = &window;
        m_width  = width;
        m_height = height;
    }

    void handle_event(const fizmo::windows::WindowEvent& e);

    void update(double dt, const fizmo::windows::InputManager& input);

    void render(fizmo::windows::Renderer& r, double fps_average);

    void shutdown() {
        if (!m_game) return;
        m_game->save_world();
        m_game->shutdown();
        m_game.reset();
    }

    Color sky_color() const { return m_game ? m_game->sky_color() : TITLE_CLEAR; }

    const GameSettings& settings() const noexcept { return m_game ? m_game->settings() : m_settings; }

    bool take_display_change() noexcept;

    bool take_quit() noexcept { const bool q = m_quit; m_quit = false; return q; }
    bool playing() const noexcept { return m_game != nullptr; }
    Game*       game()       noexcept { return m_game.get(); }
    WorldStore& store()      noexcept { return m_store; }
    StartMenu&  start_menu() noexcept { return m_start; }

    bool play(const std::string& folder);

private:
    static constexpr const char* ESCAPE_KEY  = "Escape";
    static constexpr const char* CREATE_HINT = "These shape the new world. Each world gets its own random seed.";
    static constexpr const char* EDIT_HINT   = "Terrain changes reshape every place you have not built in.";
    inline static const Color TITLE_CLEAR{ 18, 30, 54 };

    SettingsHooks title_hooks();

    void rebuild_settings();

    void refresh() { m_start.set_worlds(m_store.list()); }

    void act(StartAction action);

    void open_options(const WorldRecord* existing);

    void confirm_options();

    void close_settings();

    void leave_world() {
        shutdown();
        rebuild_settings();
        refresh();
        m_start.select(m_last_world);
    }

    GameSettings            m_settings;
    GameSettings            m_defaults;
    GameSettings            m_draft;
    GameSettings            m_draft_defaults;
    WorldRecord             m_editing;
    LightingPresets         m_lighting = LightingPresets::builtin();
    SettingsRegistry        m_pages;
    SettingsMenu            m_settings_menu;
    SettingsMenu            m_options;
    WorldStore              m_store;
    StartMenu               m_start;
    std::unique_ptr<Game>   m_game;
    fizmo::windows::Window* m_window = nullptr;
    unsigned int            m_width  = 1;
    unsigned int            m_height = 1;
    std::string             m_last_world;
    bool                    m_display_dirty = false;
    bool                    m_quit = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_GAME_APP_HPP