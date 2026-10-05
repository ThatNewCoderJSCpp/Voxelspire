#include "Include/game/app.hpp"

int main() {
    using namespace fizmo::windows;
    voxelspire::GameSettings settings;
    Application app(1280, 720, "Voxelspire", settings.render.sky_color);
    voxelspire::App game(settings);
    app.renderer().set_vsync(game.settings().display.vsync);
    app.set_max_fps(game.settings().display.max_fps);
    app.on_startup([&](Application& a) { game.start(a.window(), a.width(), a.height()); });
    app.on_event([&](const WindowEvent& e) { game.handle_event(e); });

    app.on_update([&](double dt) {
        game.update(dt, app.input());
        app.set_clear_color(game.sky_color());
        if (game.take_quit()) app.quit();

        if (game.take_display_change()) {
            app.renderer().set_vsync(game.settings().display.vsync);
            app.set_max_fps(game.settings().display.max_fps);
        }
    });

    app.on_render([&](Renderer& r) { game.render(r, app.fps_average()); });
    app.on_shutdown([&]() { game.shutdown(); });
    return app.run();
}