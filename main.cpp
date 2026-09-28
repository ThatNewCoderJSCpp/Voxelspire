#include "Include/game/game.hpp"

int main() {
    using namespace fizmo::windows;

    voxelspire::GameSettings settings;
    Application app(1280, 720, "Voxelspire", settings.render.sky_color);
    app.renderer().set_vsync(settings.display.vsync);
    app.set_max_fps(settings.display.max_fps);

    voxelspire::Game game(settings);
    app.on_startup([&](Application& a) { game.start(a.window(), a.width(), a.height()); });
    app.on_event([&](const WindowEvent& e) { game.handle_event(e); });
    app.on_update([&](double dt) { game.update(dt, app.input()); });
    app.on_render([&](Renderer& r) { game.render(r, app.fps_average()); });
    app.on_shutdown([&]() { game.shutdown(); });
    return app.run();
}