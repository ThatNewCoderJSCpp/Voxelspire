#ifndef VOXELSPIRE_UI_SETTINGS_PAGES_HPP
#define VOXELSPIRE_UI_SETTINGS_PAGES_HPP

#include <functional>
#include <string>
#include <vector>
#include "settings_model.hpp"

namespace voxelspire {

struct SettingsHooks {
    std::function<double()>     hour;
    std::function<void(double)> set_hour;
    std::function<void()>       save;
    std::function<void()>       reload;
    std::function<void()>       respawn;
};

class SettingsPages {
public:
    static std::vector<SettingsTab> build(GameSettings& live, GameSettings& defaults, const SettingsHooks& hooks) {
        std::vector<SettingsTab> tabs;
        tabs.reserve(TAB_COUNT);
        general(add(tabs, "General", "Display, camera, mouse and this menu"), live, defaults, hooks);
        controls(add(tabs, "Controls", "Change, add or remove key bindings"), live, defaults);
        character(add(tabs, "Character", "Body size, movement speeds and the hand light"), live, defaults);
        physics(add(tabs, "Physics", "Gravity, air resistance, water and entity pushing"), live, defaults);
        time(add(tabs, "Time & Sky", "Day length, tick rate, sun path and sky colors"), live, defaults, hooks);
        celestial(add(tabs, "Sun, Moon & Stars", "Look, detail and light of the sun, moon and stars"), live, defaults);
        lighting(add(tabs, "Lighting", "Light engine, brightness and light sources"), live, defaults);
        shadows(add(tabs, "Shadows", "Sun and lamp shadows"), live, defaults);
        atmosphere(add(tabs, "Atmosphere", "Fog, hazy air and light rays"), live, defaults);
        reflections(add(tabs, "Reflections & Water", "Mirrors, water, glass and shine"), live, defaults);
        rendering(add(tabs, "Rendering", "Distance, terrain detail, outline, player model and particles"), live, defaults);
        simulation(add(tabs, "Simulation", "World loading, background work and timing limits"), live, defaults);
        hud(add(tabs, "HUD", "The debug panel and crosshair"), live, defaults);
        return tabs;
    }

private:
    static constexpr std::size_t TAB_COUNT = 13;
    static constexpr double      BYTES_PER_MB = 1024.0 * 1024.0;

    static SettingsTab& add(std::vector<SettingsTab>& tabs, const char* name, const char* summary) {
        tabs.push_back({ name, summary, {} });
        return tabs.back();
    }

    static std::vector<std::string> preset_names() {
        std::vector<std::string> names;
        for (const LightingSettings& p : LightingSettings::presets()) names.push_back(p.name);
        return names;
    }

    static void general(SettingsTab& tab, GameSettings& live, GameSettings& defaults, const SettingsHooks& hooks) {
        SettingsPage p(tab, live, defaults);
        p.header("Display").applies(Apply::Display);
        p.toggle("VSync", "Match the frame rate to the monitor to stop screen tearing.", field(&GameSettings::display, &DisplaySettings::vsync));
        p.number("Max FPS", "Frame rate cap. 0 means unlimited.", field(&GameSettings::display, &DisplaySettings::max_fps), { 0.0, 500.0, 5.0, 0, "fps" });

        p.header("Camera").applies(Apply::Camera);
        p.number("Field of view", "How wide the camera sees, measured vertically.", field(&GameSettings::camera, &CameraSettings::fov_y), { 30.0, 130.0, 1.0, 0, "deg" });
        p.number("Near clip", "Closest distance the camera draws. Lower sees closer walls, higher reduces flicker far away.", field(&GameSettings::camera, &CameraSettings::near_plane), { 0.01, 1.0, 0.01, 2, "blocks" });
        p.number("Third-person distance", "How far behind or in front of you the third-person camera sits.", field(&GameSettings::camera, &CameraSettings::third_person_distance), { 1.0, 16.0, 0.5, 1, "blocks" });
        p.number("Camera wall margin", "Gap kept between the third-person camera and walls.", field(&GameSettings::camera, &CameraSettings::collision_margin), { 0.0, 1.0, 0.05, 2, "blocks" });
        p.toggle("Body in first person", "Keep your body in first person so it casts a shadow and shows in mirrors and water.", field(&GameSettings::render, &RenderSettings::first_person_body));

        p.header("Mouse").applies(Apply::Nothing);
        p.number("Mouse sensitivity", "How far the view turns per pixel of mouse movement.", field(&GameSettings::controls, &ControlSettings::mouse_sensitivity), { 0.01, 1.0, 0.01, 2, "" });
        p.toggle("Invert Y", "Moving the mouse up looks down.", field(&GameSettings::controls, &ControlSettings::invert_y));

        p.header("This menu");
        p.toggle("Pause while open", "Stop the world while the menu is open.", field(&GameSettings::menu, &MenuSettings::pause_game));
        p.toggle("Save when closing", "Write your settings to the settings file every time the menu closes.", field(&GameSettings::menu, &MenuSettings::save_on_close));
        p.number("Menu size", "Scale of this menu.", field(&GameSettings::menu, &MenuSettings::scale), { 0.6, 2.0, 0.05, 2, "x" });
        p.color("Accent color", "Color of sliders, the selected tab and section titles.", field(&GameSettings::menu, &MenuSettings::accent));
        p.color("Toggle color", "Color of switched-on checkboxes.", field(&GameSettings::menu, &MenuSettings::toggle_on));
        p.button("Save now", "Write the current settings to the settings file.", hooks.save);
        p.button("Load saved settings", "Throw away unsaved changes and load the settings file again.", hooks.reload);
        p.button("Respawn", "Go back to the spawn point.", hooks.respawn);
    }

    static void controls(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        SettingsPage p(tab, live, defaults);
        const std::vector<ActionInfo>& actions = all_actions();
        p.header("Movement");
        for (const ActionInfo& a : actions) if (movement(a.action)) p.binding(a);
        p.header("Game");
        for (const ActionInfo& a : actions) if (!movement(a.action)) p.binding(a);
    }

    static bool movement(Action a) noexcept {
        switch (a) {
            case Action::MoveForward: case Action::MoveBack: case Action::MoveLeft: case Action::MoveRight:
            case Action::Jump: case Action::Sprint: case Action::Crouch: case Action::Crawl: case Action::Swim: case Action::Alt:
                return true;
            default:
                return false;
        }
    }

    static void character(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using C = CharacterSettings;
        SettingsPage p(tab, live, defaults);
        const NumberRange size{ 0.1, 4.0, 0.02, 2, "blocks" };
        const NumberRange speed{ 0.0, 50.0, 0.1, 2, "b/s" };
        const NumberRange accel{ 1.0, 300.0, 1.0, 0, "b/s2" };
        auto c = [](double C::*m) { return field(&GameSettings::character, m); };

        p.header("Body").applies(Apply::Character);
        p.number("Width", "How wide the player is.", c(&C::width), { 0.2, 2.0, 0.02, 2, "blocks" });
        p.number("Reach", "How far away you can target blocks.", c(&C::reach), { 1.0, 20.0, 0.5, 1, "blocks" });
        p.number("Standing height", "Height while standing.", c(&C::standing_height), size);
        p.number("Crouching height", "Height while crouching.", c(&C::crouching_height), size);
        p.number("Crawling height", "Height while crawling.", c(&C::crawling_height), size);
        p.number("Swimming height", "Height while doing swim strokes.", c(&C::swimming_height), size);
        p.number("Standing eye height", "Camera height above your feet while standing.", c(&C::standing_eye_height), size);
        p.number("Crouching eye height", "Camera height while crouching.", c(&C::crouching_eye_height), size);
        p.number("Crawling eye height", "Camera height while crawling.", c(&C::crawling_eye_height), size);
        p.number("Swimming eye height", "Camera height while doing swim strokes.", c(&C::swimming_eye_height), size);

        p.header("Speeds");
        p.number("Walk", "Normal walking speed.", c(&C::walk_speed), speed);
        p.number("Sprint", "Speed while sprinting.", c(&C::sprint_speed), speed);
        p.number("Crouch", "Speed while crouching.", c(&C::crouch_speed), speed);
        p.number("Crawl", "Speed while crawling.", c(&C::crawl_speed), speed);
        p.number("Swim", "Speed while treading water.", c(&C::swim_speed), speed);
        p.number("Swim stroke", "Speed while doing swim strokes.", c(&C::stroke_speed), speed);
        p.number("Fly", "Speed of a flying mode, if one is enabled.", c(&C::fly_speed), speed);

        p.header("Alternate speeds");
        p.number("Walk (alt)", "Walking speed while holding the alternate speed key.", c(&C::alt_walk_speed), speed);
        p.number("Sprint (alt)", "Sprint speed while holding the alternate speed key.", c(&C::alt_sprint_speed), speed);
        p.number("Crouch (alt)", "Crouch speed while holding the alternate speed key.", c(&C::alt_crouch_speed), speed);
        p.number("Crawl (alt)", "Crawl speed while holding the alternate speed key.", c(&C::alt_crawl_speed), speed);
        p.number("Swim (alt)", "Swimming speed while holding the alternate speed key.", c(&C::alt_swim_speed), speed);
        p.number("Swim stroke (alt)", "Stroke speed while holding the alternate speed key.", c(&C::alt_stroke_speed), speed);
        p.number("Fly (alt)", "Flying speed while holding the alternate speed key.", c(&C::alt_fly_speed), speed);

        p.header("Jumping and acceleration");
        p.number("Jump strength", "Upward speed when you jump.", c(&C::jump_velocity), { 0.0, 40.0, 0.5, 1, "b/s" });
        p.number("Ground acceleration", "How quickly you reach full speed on the ground.", c(&C::ground_acceleration), accel);
        p.number("Air acceleration", "How much you can steer in the air.", c(&C::air_acceleration), accel);
        p.number("Swim acceleration", "How quickly you speed up while treading water.", c(&C::swim_acceleration), accel);
        p.number("Stroke acceleration", "How quickly swim strokes speed you up.", c(&C::stroke_acceleration), accel);

        p.header("Swimming");
        p.number("Rise speed", "How fast you swim up while holding jump.", c(&C::swim_rise_speed), { 0.0, 10.0, 0.1, 1, "b/s" });
        p.number("Sink speed", "How fast you slowly sink when not swimming.", c(&C::swim_sink_speed), { 0.0, 10.0, 0.1, 1, "b/s" });
        p.number("Vertical acceleration", "How quickly rising and sinking change.", c(&C::swim_vertical_accel), { 0.5, 60.0, 0.5, 1, "b/s2" });
        p.number("Surface leap", "Jump strength when leaping out of water, as a fraction of a normal jump.", c(&C::surface_leap), { 0.0, 3.0, 0.05, 2, "x" });
        p.number("Stroke float", "How fast you drift up when not stroking.", c(&C::stroke_buoyancy), { 0.0, 2.0, 0.05, 2, "b/s" });

        p.header("Flying");
        p.number("Vertical fly speed", "Up and down speed of a flying mode, if one is enabled.", c(&C::fly_vertical_speed), { 0.0, 40.0, 0.5, 1, "b/s" });
        p.number("Vertical fly acceleration", "How quickly flying up and down changes.", c(&C::fly_vertical_accel), accel);

        p.header("Hand light").applies(Apply::HandLight);
        p.color("Color", "Color of the light you carry.", field(&GameSettings::hand_light, &DynamicLight::color));
        p.number("Brightness", "How strong the hand light is.", field(&GameSettings::hand_light, &DynamicLight::intensity), { 0.0, 4.0, 0.05, 2, "" });
        p.number("Radius", "How far the hand light reaches.", field(&GameSettings::hand_light, &DynamicLight::radius), { 1.0, 40.0, 0.5, 1, "blocks" });
        p.number("Light level", "Light level it counts as for gameplay.", field(&GameSettings::hand_light, &DynamicLight::level), { 0.0, 15.0, 1.0, 0, "" });
        p.toggle("Counts as light", "Whether the hand light raises the light level used by gameplay.", field(&GameSettings::hand_light, &DynamicLight::affects_light_level));
        p.toggle("Casts shadows", "Whether the hand light casts shadows (needs point shadows).", field(&GameSettings::hand_light, &DynamicLight::casts_shadows));
    }

    static void physics(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using P = PhysicsSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto ph = [](auto P::*m) { return field(&GameSettings::physics, m); };

        p.header("Gravity").applies(Apply::Physics);
        p.number("Gravity", "How fast things fall. Earth-like is about 32 in blocks.", field(&GameSettings::world, &WorldSettings::gravity), { 0.0, 100.0, 0.5, 1, "b/s2" });

        p.header("Air");
        
        p.choice(
            "Air resistance", 
            "How falling slows down in air. World default keeps the model the world was created with.", 
            ph(&P::air_model),
            { "World default", "None", "Speed cap", "World height", "Linear drag", "Quadratic drag" }
        );

        p.when([g] { const AirModel m = g->physics.air_model; return m == AirModel::TerminalCap || m == AirModel::Linear || m == AirModel::Quadratic; });
        p.number("Terminal velocity", "Top falling speed in air.", ph(&P::terminal_velocity), { 5.0, 400.0, 1.0, 1, "b/s" });

        p.always().header("Water");
        
        p.choice(
            "Water resistance", 
            "How water slows things down. Realistic uses water drag, Minecraft-like slows a fixed amount per tick.", 
            ph(&P::fluid_model),
            { "World default", "None", "Linear", "Minecraft-like", "Realistic" }
        );
        
        p.when([g] { return g->physics.fluid_model == FluidModel::Quadratic; });
        p.number("Water density", "Heavier fluids slow you more.", ph(&P::fluid_density), { 100.0, 3000.0, 10.0, 0, "kg/m3" });
        p.number("Drag coefficient", "How much your body shape resists water.", ph(&P::drag_coefficient), { 0.1, 3.0, 0.05, 2, "" });
        p.number("Body mass", "Heavier bodies are slowed less.", ph(&P::body_mass), { 10.0, 300.0, 1.0, 0, "kg" });
        p.when([g] { return g->physics.fluid_model == FluidModel::Linear; });
        p.number("Linear drag", "Fraction of extra speed lost per second.", ph(&P::fluid_linear_drag), { 0.0, 20.0, 0.1, 1, "/s" });
        p.when([g] { return g->physics.fluid_model == FluidModel::TickDamping; });
        p.number("Speed kept per tick", "Minecraft keeps 0.8 of its speed each tick in water.", ph(&P::tick_damping), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Damping ticks per second", "How many of those ticks happen each second.", ph(&P::damping_ticks), { 1.0, 100.0, 1.0, 0, "/s" });
        p.always();
        p.number("Buoyancy", "How much of gravity the water cancels when fully under.", field(&GameSettings::world, &WorldSettings::fluid_buoyancy), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Free sinking speed", "Fastest you sink without swimming down.", field(&GameSettings::world, &WorldSettings::fluid_sink_speed), { 0.0, 20.0, 0.1, 1, "b/s" });

        p.header("Entities").applies(Apply::Entities);
        p.number("Push strength", "How hard overlapping entities push each other apart.", field(&GameSettings::entities, &EntitySettings::push_acceleration), { 0.0, 100.0, 1.0, 0, "b/s2" });
        p.number("Max push speed", "Top speed entities are pushed apart at.", field(&GameSettings::entities, &EntitySettings::max_push_speed), { 0.0, 20.0, 0.1, 1, "b/s" });
        p.number("Octree node size", "Entities per octree node before it splits. Affects performance only.", field(&GameSettings::entities, &EntitySettings::octree_max_per_node), { 1.0, 64.0, 1.0, 0, "" });
        p.number("Octree depth", "Deepest octree level. Affects performance only.", field(&GameSettings::entities, &EntitySettings::octree_max_depth), { 1.0, 16.0, 1.0, 0, "" });
        p.number("Octree looseness", "How much octree cells overlap. Affects performance only.", field(&GameSettings::entities, &EntitySettings::octree_looseness), { 1.0, 4.0, 0.1, 1, "" });
        p.number("Octree margin", "Extra space around the octree. Affects performance only.", field(&GameSettings::entities, &EntitySettings::octree_margin), { 0.0, 4.0, 0.1, 1, "blocks" });
    }

    static void time(SettingsTab& tab, GameSettings& live, GameSettings& defaults, const SettingsHooks& hooks) {
        using D = DayCycleSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto d = [](auto D::*m) { return field(&GameSettings::day_cycle, m); };

        p.header("Time").applies(Apply::Nothing);
        p.toggle("Day cycle", "Let time pass. Turn off to freeze the time of day.", d(&D::enabled));
        p.custom_number("Time of day", "Set the current time.", hooks.hour, hooks.set_hour, { 0.0, 100.0, 0.25, 2, "h" }, false);
        tab.controls.back().live_max = [g] { return g->day_cycle.hours_per_day; };
        p.number("Hours per day", "How many hours the clock shows in one day.", d(&D::hours_per_day), { 1.0, 100.0, 1.0, 0, "h" });
        p.number("Ticks per day", "How many simulation ticks one full day lasts.", d(&D::ticks_per_day), { 100.0, 1000000.0, 100.0, 0, "ticks", true });
        p.number("Tick rate", "Simulation ticks per second. Higher is smoother physics and makes days pass faster in real time.", field(&GameSettings::simulation, &SimulationSettings::tick_rate), { 5.0, 240.0, 1.0, 0, "ticks/s" });
        p.number("Game speed", "Speeds up or slows down everything: movement, physics and time.", field(&GameSettings::simulation, &SimulationSettings::game_speed), { 0.1, 10.0, 0.05, 2, "x", true });
        
        p.custom_number("Real day length", "How long a day lasts in real minutes (ticks per day, tick rate and game speed together).",
                        [g] { const double rate = g->simulation.tick_rate * g->simulation.game_speed; return rate > 0.0 ? g->day_cycle.ticks_per_day / rate / SECONDS_PER_MINUTE : 0.0; },
                        [g](double minutes) { g->day_cycle.ticks_per_day = vmax(1.0, minutes * SECONDS_PER_MINUTE * g->simulation.tick_rate * g->simulation.game_speed); },
                        { 0.5, 240.0, 0.5, 1, "min", true }, false);
        
                        p.number("Max ticks per frame", "Limit on catch-up ticks after a slow frame.", field(&GameSettings::simulation, &SimulationSettings::max_ticks_per_frame), { 1.0, 60.0, 1.0, 0, "" });

        p.header("Sun path").applies(Apply::Sky);
        p.number("Sun tilt", "How far the sun's path leans away from straight overhead.", d(&D::sun_tilt), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Night brightness", "How bright the sky light stays at night.", d(&D::night_brightness), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Horizon fade", "How gradually the sun fades out at the horizon.", d(&D::horizon_fade), { 0.01, 0.5, 0.01, 2, "" });
        p.number("Twilight length", "How long dawn and dusk last.", d(&D::twilight), { 0.01, 0.5, 0.01, 2, "" });
        p.number("Dusk color strength", "How orange the sky gets at sunrise and sunset.", d(&D::dusk_sky_mix), { 0.0, 1.0, 0.01, 2, "" });

        p.header("Sky glow").applies(Apply::Lighting);
        auto l = [](auto LightingSettings::*m) { return field(&GameSettings::lighting, m); };
        p.number("Glow strength", "Strength of the sky glow around the sun (with atmosphere on).", l(&LightingSettings::sky_glow), { 0.0, 2.0, 0.01, 2, "" });
        p.number("Glow spread", "Width of the soft outer sky glow. Lower is wider.", l(&LightingSettings::sun_glow_spread), { 1.0, 32.0, 0.5, 1, "" });
        p.number("Glow focus", "Tightness of the bright inner sky glow. Higher is tighter.", l(&LightingSettings::sun_glow_focus), { 16.0, 4000.0, 1.0, 0, "", true });

        p.header("Sky colors").applies(Apply::Sky);
        p.color("Day sky", "Sky color during the day.", d(&D::day_sky));
        p.color("Night sky", "Sky color at night.", d(&D::night_sky));
        p.color("Dusk sky", "Sky color at sunrise and sunset.", d(&D::dusk_sky));
        p.color("Day zenith", "Color straight up during the day (with atmosphere on).", d(&D::day_zenith));
        p.color("Day horizon", "Color near the horizon during the day.", d(&D::day_horizon));
        p.color("Night zenith", "Color straight up at night.", d(&D::night_zenith));
        p.color("Night horizon", "Color near the horizon at night.", d(&D::night_horizon));
        p.color("Dusk horizon", "Horizon color at sunrise and sunset.", d(&D::dusk_horizon));
        p.color("Sun glow color", "Color of the glow around the sun.", d(&D::sun_glow));
        p.color("Dusk glow color", "Glow color at sunrise and sunset.", d(&D::dusk_glow));
        p.color("Day sky light", "Tint of sky light during the day.", d(&D::day_light_tint));
        p.color("Night sky light", "Tint of sky light at night.", d(&D::night_light_tint));
        p.applies(Apply::Nothing);
        p.color("Sky without lighting", "Background color used when lighting is off.", field(&GameSettings::render, &RenderSettings::sky_color));
    }

    static void celestial(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using C = CelestialSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto c = [](auto C::*m) { return field(&GameSettings::celestial, m); };
        auto sun = [](auto SunSettings::*m) { return field(&GameSettings::celestial, &C::sun, m); };
        auto moon = [](auto MoonSettings::*m) { return field(&GameSettings::celestial, &C::moon, m); };
        auto star = [](auto StarSettings::*m) { return field(&GameSettings::celestial, &C::stars, m); };
        auto sun_glow = [](auto GlowSettings::*m) { return [m](GameSettings& s) -> auto& { return s.celestial.sun.glow.*m; }; };
        auto moon_glow = [](auto GlowSettings::*m) { return [m](GameSettings& s) -> auto& { return s.celestial.moon.glow.*m; }; };
        const NumberRange angle{ 0.1, 20.0, 0.1, 1, "deg" };
        const NumberRange segments{ 3.0, 256.0, 1.0, 0, "" };
        const NumberRange rings{ 1.0, 64.0, 1.0, 0, "" };
        const NumberRange unit{ 0.0, 1.0, 0.01, 2, "" };
        const NumberRange glow_size{ 1.0, 30.0, 0.25, 2, "x" };

        p.header("Placement").applies(Apply::Celestial);
        p.number("Distance", "How far away the sky objects are drawn, as a fraction of the view distance.", c(&C::distance), { 0.05, 0.95, 0.01, 2, "" });
        p.number("Horizon fade", "How softly things fade out below the horizon. 0 keeps them visible below it.", c(&C::horizon_fade), { 0.0, 0.3, 0.005, 3, "" });
        p.toggle("Hide underwater", "Hide the sun, moon and stars while your head is underwater.", c(&C::hide_underwater));

        p.header("Sun");
        p.toggle("Show sun", "Draw the sun.", sun(&SunSettings::visible));
        p.when([g] { return g->celestial.sun.visible; });
        p.number("Sun size", "How big the sun looks across the sky.", sun(&SunSettings::size), angle);
        p.number("Sun roundness", "Detail: points around the sun's edge.", sun(&SunSettings::segments), segments);
        p.number("Sun shading detail", "Detail: rings used for edge darkening.", sun(&SunSettings::rings), rings);
        p.color("Sun color", "Color of the sun during the day.", sun(&SunSettings::color));
        p.color("Sunset sun color", "Color of the sun near the horizon.", sun(&SunSettings::dusk_color));
        p.number("Sun brightness", "How bright the sun looks. It is added on top of the sky, so it always stands out.", sun(&SunSettings::brightness), unit);
        p.number("Edge darkening", "How much darker the sun is at its edge.", sun(&SunSettings::limb_darkening), unit);
        p.number("Sun edge softness", "How blurry the sun's edge is.", sun(&SunSettings::edge_softness), { 0.0, 1.0, 0.01, 2, "" });
        p.toggle("Sun glow", "A soft glow around the sun.", sun_glow(&GlowSettings::enabled));
        p.when([g] { return g->celestial.sun.visible && g->celestial.sun.glow.enabled; });
        p.number("Sun glow size", "How far the glow reaches, compared to the sun's size.", sun_glow(&GlowSettings::size), glow_size);
        p.number("Sun glow strength", "How bright the glow is.", sun_glow(&GlowSettings::strength), unit);
        p.number("Sun glow detail", "Detail: rings used to draw the glow.", sun_glow(&GlowSettings::rings), rings);

        p.always().header("Sunlight").applies(Apply::Lighting).touching([g] { g->lighting.name = CUSTOM; });
        p.toggle("Direct light engine", "Same as Direct sun and moon light in Lighting. Needed for the sun and moon to light up the world.",
                 field(&GameSettings::lighting, &LightingSettings::sun_lighting));
        tab.controls.back().persist = false;
        p.applies(Apply::Celestial).touching({});
        p.toggle("Sun gives off light", "Whether the sun lights up the world.", sun(&SunSettings::emits_light));
        p.when([g] { return g->celestial.sun.emits_light; });
        p.number("Sunlight strength", "How strong sunlight is.", sun(&SunSettings::light_strength), { 0.0, 3.0, 0.05, 2, "" });
        p.color("Sunlight color", "Color of sunlight during the day.", sun(&SunSettings::light_color));
        p.color("Sunset light color", "Color of sunlight near the horizon.", sun(&SunSettings::dusk_light_color));

        p.always().header("Moon");
        p.toggle("Show moon", "Draw the moon.", moon(&MoonSettings::visible));
        p.when([g] { return g->celestial.moon.visible; });
        p.number("Moon size", "How big the moon looks across the sky.", moon(&MoonSettings::size), angle);
        p.number("Moon roundness", "Detail: points around the moon's edge.", moon(&MoonSettings::segments), segments);
        p.number("Moon shading detail", "Detail: rings used to shade the moon's phase. More is smoother.", moon(&MoonSettings::rings), rings);
        p.color("Moon color", "Color of the lit part of the moon.", moon(&MoonSettings::color));
        p.number("Moon brightness", "How bright the moon looks.", moon(&MoonSettings::brightness), unit);
        p.number("Moon edge softness", "How blurry the moon's edge is.", moon(&MoonSettings::edge_softness), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Daytime visibility", "How visible the moon is during the day. 0 hides it in daylight.", moon(&MoonSettings::day_visibility), unit);
        p.toggle("Moon glow", "A soft glow around the moon.", moon_glow(&GlowSettings::enabled));
        p.when([g] { return g->celestial.moon.visible && g->celestial.moon.glow.enabled; });
        p.number("Moon glow size", "How far the glow reaches, compared to the moon's size.", moon_glow(&GlowSettings::size), glow_size);
        p.number("Moon glow strength", "How bright the glow is.", moon_glow(&GlowSettings::strength), unit);
        p.number("Moon glow detail", "Detail: rings used to draw the glow.", moon_glow(&GlowSettings::rings), rings);

        p.always().header("Moon phases");
        p.toggle("Phases", "The moon waxes and wanes over several days. Off keeps it full.", moon(&MoonSettings::phases));
        p.when([g] { return g->celestial.moon.phases; });
        p.number("Days per cycle", "How many days from one full moon to the next.", moon(&MoonSettings::cycle_days), { 1.0, 100.0, 1.0, 0, "days" });
        p.number("Phase offset", "Shifts the cycle. 0 is a full moon on day 0, 0.5 a new moon.", moon(&MoonSettings::phase_offset), unit);
        p.number("Earthshine", "How visible the dark part of the moon is.", moon(&MoonSettings::earthshine), { 0.0, 0.5, 0.01, 2, "" });
        p.number("Dark side opacity", "How much the dark part of the moon hides the sky and stars behind it at night.", moon(&MoonSettings::dark_opacity), unit);
        p.number("Shadow edge softness", "How soft the line between the lit and dark part is.", moon(&MoonSettings::terminator), { 0.0, 1.0, 0.01, 2, "" });

        p.always().header("Moonlight");
        p.toggle("Moon gives off light", "Whether the moon lights up the world at night.", moon(&MoonSettings::emits_light));
        p.when([g] { return g->celestial.moon.emits_light; });
        p.number("Moonlight strength", "How strong moonlight is compared to sunlight.", moon(&MoonSettings::light_strength), { 0.0, 2.0, 0.01, 2, "" });
        p.color("Moonlight color", "Color of moonlight.", moon(&MoonSettings::light_color));

        p.always().header("Stars");
        p.toggle("Show stars", "Draw stars at night.", star(&StarSettings::visible));
        p.when([g] { return g->celestial.stars.visible; });
        p.number("Star count", "How many stars there are.", star(&StarSettings::count), { 0.0, 20000.0, 50.0, 0, "" });
        p.number("Star size", "How big the largest stars look.", star(&StarSettings::size), { 0.01, 2.0, 0.01, 2, "deg" });
        p.number("Size variation", "How much smaller some stars are.", star(&StarSettings::size_variation), unit);
        p.number("Star brightness", "How bright the stars are.", star(&StarSettings::brightness), { 0.0, 2.0, 0.05, 2, "" });
        p.number("Brightness variation", "How much dimmer some stars are.", star(&StarSettings::brightness_variation), unit);
        p.color("Star color", "Main color of the stars.", star(&StarSettings::color));
        p.color("Warm star color", "Color of the warmer stars.", star(&StarSettings::warm_color));
        p.color("Cool star color", "Color of the cooler stars.", star(&StarSettings::cool_color));
        p.number("Color variation", "How strongly stars take on the warm or cool color.", star(&StarSettings::color_variation), unit);
        p.number("Twinkle", "How much stars flicker.", star(&StarSettings::twinkle), unit);
        p.number("Twinkle speed", "How fast stars flicker.", star(&StarSettings::twinkle_speed), { 0.0, 10.0, 0.1, 1, "/s" });
        p.number("Daytime stars", "How visible stars are during the day.", star(&StarSettings::day_visibility), unit);
        p.toggle("Turn with the sky", "Stars move across the sky with the sun and moon.", star(&StarSettings::rotate));
        p.number("Star seed", "Changes the star pattern.", star(&StarSettings::seed), { 0.0, 99999.0, 1.0, 0, "" });
    }

    static void lighting(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using L = LightingSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        GameSettings* base = &defaults;
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };
        const std::vector<std::string> names = preset_names();

        p.header("Preset").applies(Apply::Lighting);
        p.custom_choice("Preset", "Load a whole lighting setup at once. Changing anything below makes it Custom.", names,
                        [g, names] { for (std::size_t i = 0; i < names.size(); ++i) if (names[i] == g->lighting.name) return static_cast<int>(i); return -1; },
                        [g](int i) { const auto presets = L::presets(); if (i >= 0 && static_cast<std::size_t>(i) < presets.size()) g->lighting = presets[static_cast<std::size_t>(i)]; });
        tab.controls.back().reset      = [g, base] { g->lighting = base->lighting; };
        tab.controls.back().is_default = [g, base] { return g->lighting.name == base->lighting.name; };

        p.header("Light engine").applies(Apply::LightFormat);
        p.choice("Light type", "Colored lets lamps tint the world. Plain is white light and uses less memory. None turns light off.",
                 field(&GameSettings::world, &WorldSettings::light_format), { "None", "Plain", "Colored" });
        p.applies(Apply::Lighting).touching([g] { g->lighting.name = CUSTOM; });
        p.number("Update budget", "Time per frame spent spreading light after changes.", l(&L::update_budget_ms), { 0.1, 20.0, 0.1, 1, "ms" });

        p.header("Block light");
        p.toggle("Baked light", "Light stored in the world (sky light and lamps).", l(&L::baked_light));
        p.when([g] { return g->lighting.baked_light; });
        p.toggle("Smooth lighting", "Blend light between neighboring blocks.", l(&L::smooth_lighting));
        p.always();
        p.number("Ambient occlusion", "Darkening in corners and creases.", l(&L::ambient_occlusion), { 0.0, 2.0, 0.05, 2, "" });
        p.number("Occlusion step", "How much each blocking neighbor darkens a corner.", l(&L::occlusion_step), { 0.0, 0.5, 0.01, 2, "" });
        p.number("Face shading", "How much darker sides and bottoms of blocks are.", l(&L::face_shading), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Falloff", "How quickly light fades with distance. Higher keeps it brighter longer.", l(&L::falloff), { 1.0, 4.0, 0.05, 2, "" });
        p.number("Minimum light", "Darkest anything can get.", l(&L::min_light), { 0.0, 0.5, 0.01, 2, "" });
        p.number("Maximum light", "Brightest anything can get.", l(&L::max_light), { 0.5, 4.0, 0.05, 2, "" });
        p.number("Ambient", "Extra light added everywhere.", l(&L::ambient), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Sky light", "Strength of light from the sky.", l(&L::sky_light), { 0.0, 2.0, 0.01, 2, "" });
        p.number("Block light", "Strength of light from lamps and other glowing blocks.", l(&L::block_light), { 0.0, 2.0, 0.01, 2, "" });
        p.color("Block light tint", "Color of block light when using plain light.", l(&L::block_tint));

        p.header("Sunlight");
        p.toggle("Direct sun and moon light", "Sunlight and moonlight that shine from a direction and cast shadows.", l(&L::sun_lighting));
        p.when([g] { return g->lighting.sun_lighting; });
        p.number("Direct light strength", "How strong direct sun and moon light is overall.", l(&L::sun_strength), { 0.0, 3.0, 0.05, 2, "" });
        p.number("Sun exposure", "How much sky light a spot needs before sunlight reaches it.", l(&L::sun_exposure), { 0.0, 8.0, 0.1, 1, "" });

        p.always().header("Moving lights");
        p.toggle("Dynamic lights", "Lights that move, like the one you carry.", l(&L::dynamic_lights));
        p.when([g] { return g->lighting.dynamic_lights; });
        p.number("Max dynamic lights", "Most moving lights drawn at once.", l(&L::max_dynamic_lights), { 0.0, 128.0, 1.0, 0, "" });
        p.number("Dynamic light distance", "Moving lights farther than this are skipped.", l(&L::dynamic_light_distance), { 8.0, 256.0, 1.0, 0, "blocks" });

        p.always().header("Lamp lights");
        p.toggle("Lamp lights", "Lamps also shine real light (needed for lamp shadows).", l(&L::block_point_lights));
        p.when([g] { return g->lighting.block_point_lights; });
        p.number("Max lamp lights", "Most lamps shining at once.", l(&L::max_block_point_lights), { 0.0, 64.0, 1.0, 0, "" });
        p.number("Lamp light distance", "Lamps farther than this don't shine.", l(&L::block_point_light_distance), { 8.0, 128.0, 1.0, 0, "blocks" });
        p.number("Lamp brightness", "How strong lamp light is.", l(&L::block_point_light_intensity), { 0.0, 3.0, 0.05, 2, "" });
        p.number("Lamp radius", "How far lamp light reaches.", l(&L::block_point_light_radius), { 1.0, 32.0, 0.5, 1, "blocks" });
        p.number("Lamp fade", "Distance over which far lamps fade in and out instead of popping.", l(&L::point_light_fade), { 0.0, 16.0, 0.5, 1, "blocks" });

        p.always().header("Color grading");
        p.toggle("Tone mapping", "Film-like handling of very bright light.", l(&L::tone_mapping));
        p.when([g] { return g->lighting.tone_mapping; });
        p.number("Exposure", "Overall brightness.", l(&L::exposure), { 0.1, 3.0, 0.05, 2, "" });
        p.number("Saturation", "How colorful the image is.", l(&L::saturation), { 0.0, 2.0, 0.05, 2, "" });

        p.always().header("Underwater");
        p.toggle("Underwater fog", "Tint and fog when your head is underwater.", l(&L::underwater_fog));
        p.when([g] { return g->lighting.underwater_fog; });
        p.number("Underwater fog density", "How thick the underwater fog is.", l(&L::underwater_density), { 0.0, 1.0, 0.01, 2, "" });
    }

    static void shadows(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using L = LightingSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };
        p.touching([g] { g->lighting.name = CUSTOM; });

        p.header("Sun shadows").applies(Apply::Lighting);
        p.toggle("Sun shadows", "Shadows from the sun and moon (needs direct sunlight).", l(&L::sun_shadows));
        p.when([g] { return g->lighting.sun_shadows; });
        power_choice(p, g, "Resolution", "Shadow sharpness. Higher is sharper but slower.", &L::sun_shadow_resolution, { 512u, 1024u, 2048u, 4096u, 8192u });
        p.number("Distance", "How far from you shadows are drawn.", l(&L::sun_shadow_distance), { 16.0, 256.0, 4.0, 0, "blocks" });
        p.number("Darkness", "How dark shadows are.", l(&L::shadow_strength), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Edge blur", "Minimum blur on shadow edges.", l(&L::shadow_softness), { 0.0, 6.0, 0.1, 1, "px" });
        p.toggle("Soft shadows", "Shadows get blurrier the farther they are from what casts them.", l(&L::soft_shadows));
        p.number("Sun size for soft shadows", "Bigger makes soft shadows blurrier.", l(&L::soft_shadow_sun_size), { 0.1, 5.0, 0.05, 2, "deg" });
        p.number("Max blur", "Largest blur a soft shadow can have.", l(&L::max_shadow_softness), { 1.0, 32.0, 0.5, 1, "px" });
        p.number("Blur quality", "Samples per side for soft shadows. Higher is smoother but slower.", l(&L::shadow_filter_taps), { 1.0, 8.0, 1.0, 0, "" });
        p.number("Update step", "How far the sun moves before shadows are recalculated.", l(&L::shadow_angle_step), { 0.0, 2.0, 0.05, 2, "deg" });
        p.toggle("Smooth movement", "Blend between shadow updates so they move smoothly instead of jumping.", l(&L::shadow_crossfade));

        p.always().header("Lamp shadows");
        p.toggle("Lamp shadows", "Shadows from lamps and your hand light (needs lamp lights or dynamic lights).", l(&L::point_shadows));
        p.when([g] { return g->lighting.point_shadows; });
        p.number("Shadowed lamps", "How many of the nearest lamps cast shadows.", l(&L::max_point_shadows), { 0.0, 8.0, 1.0, 0, "" });
        power_choice(p, g, "Lamp shadow resolution", "Sharpness of lamp shadows.", &L::point_shadow_resolution, { 256u, 512u, 768u, 1024u, 2048u });
        p.number("Shadow fade", "Distance over which lamp shadows fade in and out.", l(&L::point_shadow_fade), { 0.0, 16.0, 0.5, 1, "blocks" });
        p.toggle("Hide lamps without shadows", "Lamps that don't get a shadow don't shine, so light can't leak through walls.", l(&L::hide_unshadowed_point_lights));
    }

    static void power_choice(SettingsPage& p, GameSettings* g, const std::string& label, const std::string& description,
                             unsigned int LightingSettings::*member, std::vector<unsigned int> values) {
        std::vector<std::string> names;
        for (unsigned int v : values) names.push_back(std::to_string(v));
        p.custom_choice(label, description, names,
                        [g, member, values] { for (std::size_t i = 0; i < values.size(); ++i) if (values[i] == g->lighting.*member) return static_cast<int>(i); return -1; },
                        [g, member, values](int i) { if (i >= 0 && static_cast<std::size_t>(i) < values.size()) g->lighting.*member = values[static_cast<std::size_t>(i)]; });
    }

    static void atmosphere(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using L = LightingSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };
        p.touching([g] { g->lighting.name = CUSTOM; });

        p.header("Sky and fog").applies(Apply::Lighting);
        p.toggle("Atmosphere", "A real sky with a sun and moon, and distance fog.", l(&L::atmosphere));
        p.when([g] { return g->lighting.atmosphere; });
        p.number("Fog density", "How thick distant fog is.", l(&L::fog_density), { 0.0, 0.05, 0.0005, 4, "" });
        p.number("Fog start", "Distance where fog begins.", l(&L::fog_start), { 0.0, 256.0, 1.0, 0, "blocks" });

        p.always().header("Hazy air");
        p.toggle("Volumetric light", "Sunlight lights up the air, so shadows show in the haze.", l(&L::volumetric_light));
        p.when([g] { return g->lighting.volumetric_light; });
        p.number("Quality", "Samples along each view ray. Higher is smoother but slower.", l(&L::volumetric_steps), { 4.0, 128.0, 1.0, 0, "" });
        p.number("Haze density", "How concentrated the haze is near you.", l(&L::volumetric_density), { 0.0, 0.2, 0.001, 3, "" });
        p.number("Haze brightness", "How bright the haze is.", l(&L::volumetric_intensity), { 0.0, 5.0, 0.05, 2, "" });
        p.number("Forward glow", "How much brighter haze is when looking toward the sun.", l(&L::volumetric_anisotropy), { -0.9, 0.95, 0.01, 2, "" });
        p.number("Haze distance", "How far the haze is calculated.", l(&L::volumetric_distance), { 16.0, 256.0, 4.0, 0, "blocks" });
        p.number("Near detail", "Puts more samples close to you. 1 is even spacing.", l(&L::volumetric_near_bias), { 1.0, 4.0, 0.1, 1, "" });

        p.always().header("Light rays");
        p.toggle("Light rays", "Beams of light around things in front of the sun.", l(&L::light_shafts));
        p.when([g] { return g->lighting.light_shafts; });
        p.number("Ray strength", "How bright the rays are.", l(&L::light_shaft_strength), { 0.0, 2.0, 0.01, 2, "" });
        p.number("Ray quality", "Samples per ray. Higher is smoother but slower.", l(&L::light_shaft_samples), { 8.0, 128.0, 1.0, 0, "" });
        p.number("Ray length", "How far the rays stretch.", l(&L::light_shaft_length), { 0.1, 1.0, 0.01, 2, "" });
        p.number("Ray fade", "How quickly rays fade along their length.", l(&L::light_shaft_decay), { 0.8, 1.0, 0.005, 3, "" });
        p.number("Ray focus", "How closely the rays hug the sun. Higher is tighter.", l(&L::light_shaft_focus), { 1.0, 100.0, 1.0, 0, "" });
    }

    static void reflections(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using L = LightingSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };
        p.touching([g] { g->lighting.name = CUSTOM; });

        p.header("Shine").applies(Apply::Lighting);
        p.toggle("Reflections", "Water, glass and mirrors reflect the sky and shine in the sun.", l(&L::reflections));
        p.when([g] { return g->lighting.reflections; });
        p.number("Reflectivity", "How strong reflections are.", l(&L::reflectivity), { 0.0, 2.0, 0.05, 2, "" });
        p.number("Sun highlights", "Brightness of sun sparkles on shiny surfaces.", l(&L::specular), { 0.0, 10.0, 0.1, 1, "" });
        p.number("Highlight sharpness", "Higher makes the sparkles smaller and sharper.", l(&L::specular_power), { 8.0, 1000.0, 1.0, 0, "", true });

        p.always().header("Mirrors and water");
        p.toggle("Real reflections", "Mirrors and water show the world, including you. Each reflecting surface draws the world again.", l(&L::planar_reflections));
        p.when([g] { return g->lighting.planar_reflections; });
        p.number("Reflecting surfaces", "How many mirrors or water surfaces can reflect at once.", l(&L::max_reflection_planes), { 0.0, 2.0, 1.0, 0, "" });
        p.number("Reflection resolution", "Detail of reflections. 1 is full screen resolution.", l(&L::reflection_resolution), { 0.25, 1.0, 0.05, 2, "x" });
        p.number("Search distance", "How far away a mirror or water can be to reflect.", l(&L::reflection_plane_distance), { 8.0, 128.0, 1.0, 0, "blocks" });
        p.number("Reflected view distance", "How far the world is drawn inside reflections.", l(&L::reflection_render_distance), { 16.0, 512.0, 8.0, 0, "blocks" });
        p.number("Ripple distortion", "How much waves bend water reflections.", l(&L::reflection_distortion), { 0.0, 0.1, 0.005, 3, "" });
        p.toggle("Water reflects the world", "Let water use real reflections too, not just mirrors.", l(&L::water_planar_reflections));

        p.always().header("Screen reflections");
        p.toggle("Screen reflections", "Cheaper reflections of things already on screen (used where real reflections aren't).", l(&L::screen_reflections));
        p.when([g] { return g->lighting.screen_reflections; });
        p.number("Screen reflection quality", "Steps traced per reflection.", l(&L::reflection_steps), { 4.0, 128.0, 1.0, 0, "" });
        p.number("Screen reflection distance", "How far reflections are traced.", l(&L::reflection_distance), { 8.0, 256.0, 4.0, 0, "blocks" });

        p.always().header("Water");
        p.number("Wave height", "How rough the water surface is.", l(&L::wave_strength), { 0.0, 0.5, 0.01, 2, "" });
        p.number("Wave size", "Higher makes smaller, busier waves.", l(&L::wave_scale), { 0.1, 4.0, 0.05, 2, "" });
        p.number("Wave speed", "How fast waves move.", l(&L::wave_speed), { 0.0, 5.0, 0.05, 2, "" });
        p.toggle("See-through water", "Water bends and tints what's under it.", l(&L::refraction));
        p.when([g] { return g->lighting.refraction; });
        p.number("Bending", "How much water bends the view underneath.", l(&L::refraction_strength), { 0.0, 2.0, 0.05, 2, "" });
        p.number("Color absorption", "How quickly deep water turns blue.", l(&L::water_absorption), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Murkiness", "How cloudy water gets with depth.", l(&L::water_scattering), { 0.0, 1.0, 0.01, 2, "" });
    }

    static void rendering(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using R = RenderSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto r = [](auto R::*m) { return field(&GameSettings::render, m); };

        p.header("Distance").applies(Apply::Streaming);
        p.number("Render distance", "How far the world is drawn.", r(&R::render_distance), { 16.0, 8192.0, 16.0, 0, "blocks", true });
        p.applies(Apply::Nothing);
        p.number("Distance key step", "How much the render distance keys change it.", r(&R::render_distance_step), { 8.0, 512.0, 8.0, 0, "blocks" });

        p.header("Terrain").applies(Apply::Terrain);
        p.toggle("Merge faces", "Join matching block faces into bigger pieces. Much faster.", r(&R::merge_faces));
        p.toggle("Skip hidden floor", "Don't draw the bottom of the world that can never be seen.", r(&R::cull_void_faces));
        p.number("Top brightness", "Brightness of block tops before lighting.", field(&GameSettings::render, &R::shading, &FaceShadingSettings::up), { 0.0, 1.0, 0.01, 2, "" });
        p.number("Bottom brightness", "Brightness of block bottoms.", field(&GameSettings::render, &R::shading, &FaceShadingSettings::down), { 0.0, 1.0, 0.01, 2, "" });
        p.number("North/south brightness", "Brightness of north and south sides.", field(&GameSettings::render, &R::shading, &FaceShadingSettings::north_south), { 0.0, 1.0, 0.01, 2, "" });
        p.number("East/west brightness", "Brightness of east and west sides.", field(&GameSettings::render, &R::shading, &FaceShadingSettings::east_west), { 0.0, 1.0, 0.01, 2, "" });
        p.applies(Apply::Nothing);
        p.toggle("Cave culling", "Skip chunks hidden behind solid ground. Faster underground.", r(&R::cave_culling));
        p.toggle("Face culling", "Skip block faces pointing away from you. Faster.", r(&R::face_culling));

        p.header("Level of detail").applies(Apply::Streaming);
        auto lod = [](auto LodSettings::*m) { return field(&GameSettings::lod, m); };
        p.toggle("Level of detail", "Draw far terrain with simpler shapes so the view can reach much farther.", lod(&LodSettings::enabled));
        p.when([g] { return g->lod.enabled; });
        p.number("Detail levels", "How many levels of simpler terrain to use.", lod(&LodSettings::max_level), { 1.0, 16.0, 1.0, 0, "" });
        p.number("Exact levels", "Levels that keep exact block shapes.", lod(&LodSettings::exact_levels), { 0.0, 8.0, 1.0, 0, "" });
        p.number("Heightmap level", "Level where far terrain switches to a height map.", lod(&LodSettings::heightmap_level), { 0.0, 16.0, 1.0, 0, "" });
        p.number("Coverage threshold", "How full a far cell must be to count as solid.", lod(&LodSettings::coverage_threshold), { 0.0, 1.0, 0.05, 2, "" });
        p.number("Samples per cell", "Samples used to shape far terrain.", lod(&LodSettings::samples_per_cell), { 1.0, 8.0, 1.0, 0, "" });
        p.number("Tile size", "Size of each far terrain tile.", lod(&LodSettings::tile_cells), { 16.0, 128.0, 16.0, 0, "blocks" });
        p.number("Tile jobs", "Far tiles built at once in the background.", lod(&LodSettings::max_tile_jobs), { 1.0, 128.0, 1.0, 0, "" });

        p.always().header("Block outline").applies(Apply::Nothing);
        p.color("Outline color", "Color of the box around the block you're looking at.", r(&R::outline_color), true);
        p.number("Outline width", "Thickness of the outline.", r(&R::outline_width), { 1.0, 8.0, 1.0, 0, "px" });
        p.number("Outline size", "How far the outline sits outside the block.", r(&R::outline_inflate), { 0.0, 0.05, 0.001, 3, "blocks" });

        p.header("Player model").applies(Apply::Player);
        p.color("Body color", "Main color of the player.", r(&R::player_color));
        p.color("Visor color", "Color of the visor.", r(&R::player_visor));
        p.number("Roundness", "Segments around the body. Higher is smoother.", r(&R::capsule_segments), { 6.0, 64.0, 1.0, 0, "" });
        p.number("Cap detail", "Rings on the rounded ends.", r(&R::capsule_rings), { 2.0, 16.0, 1.0, 0, "" });

        p.header("Particles").applies(Apply::Particles);
        auto pa = [](auto ParticleSettings::*m) { return field(&GameSettings::particles, m); };
        p.toggle("Particles", "Small effects like sparks and dust.", pa(&ParticleSettings::enabled));
        p.when([g] { return g->particles.enabled; });
        p.number("Max particles", "Most particles alive at once.", pa(&ParticleSettings::max_particles), { 0.0, 1000000.0, 1000.0, 0, "", false });
        p.number("Spawn distance", "Emitters farther than this don't spawn particles.", pa(&ParticleSettings::emit_distance), { 8.0, 512.0, 8.0, 0, "blocks" });
        p.number("Draw distance", "Particles farther than this aren't drawn.", pa(&ParticleSettings::draw_distance), { 8.0, 512.0, 8.0, 0, "blocks" });
    }

    static void simulation(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using S = StreamingSettings;
        SettingsPage p(tab, live, defaults);
        GameSettings* g = &live;
        auto s = [](auto S::*m) { return field(&GameSettings::streaming, m); };

        p.header("World loading").applies(Apply::Streaming);
        p.number("Simulation distance", "Chunks around you where things move and update.", s(&S::simulation_distance), { 1.0, 32.0, 1.0, 0, "chunks" });
        p.number("Detail distance", "Chunks around you drawn in full detail.", s(&S::detail_distance), { 1.0, 64.0, 1.0, 0, "chunks" });
        p.number("Unload margin", "Extra chunks kept loaded past the edge before unloading.", s(&S::unload_margin), { 0.0, 8.0, 1.0, 0, "chunks" });

        p.header("Background work").applies(Apply::Nothing);
        p.number("World generation jobs", "Columns generated at once.", s(&S::max_column_jobs), { 1.0, 512.0, 1.0, 0, "" });
        p.number("Meshing jobs", "Chunks turned into shapes at once.", s(&S::max_mesh_jobs), { 1.0, 1024.0, 1.0, 0, "" });
        p.custom_number("Upload per frame", "Most mesh data sent to the graphics card each frame.",
                        [g] { return static_cast<double>(g->streaming.upload_bytes_per_frame) / BYTES_PER_MB; },
                        [g](double mb) { g->streaming.upload_bytes_per_frame = static_cast<std::size_t>(vmax(mb, 0.0) * BYTES_PER_MB); },
                        { 1.0, 256.0, 1.0, 0, "MB" });
        tab.controls.back().apply = Apply::Streaming;
        p.number("Result time budget", "Time per frame spent finishing background work.", s(&S::result_time_budget_ms), { 0.5, 33.0, 0.5, 1, "ms" });
    }

    static void hud(SettingsTab& tab, GameSettings& live, GameSettings& defaults) {
        using H = HudSettings;
        SettingsPage p(tab, live, defaults);
        auto h = [](auto H::*m) { return field(&GameSettings::hud, m); };
        auto sec = [](bool HudSections::*m) { return field(&GameSettings::hud, &H::sections, m); };

        p.header("Show").applies(Apply::Nothing);
        p.toggle("Debug panel", "The panel with position, speed and performance numbers.", h(&H::show_debug));
        p.toggle("Crosshair", "The cross in the middle of the screen.", h(&H::show_crosshair));
        p.toggle("Last key", "Show the last key you pressed in the panel.", h(&H::show_last_key));
        p.toggle("Performance section", "FPS, draws and memory.", sec(&HudSections::performance));
        p.toggle("Player section", "Position, speed and target.", sec(&HudSections::player));
        p.toggle("World section", "Seed, time and loaded world.", sec(&HudSections::world));
        p.toggle("Rendering section", "Chunks, quads and reflections.", sec(&HudSections::rendering));
        p.toggle("Lighting section", "Lights and light data.", sec(&HudSections::lighting));

        p.header("Layout");
        p.choice("Corner", "Which corner the panel sits in.", h(&H::corner), { "Top left", "Top right", "Bottom left", "Bottom right" });
        p.number("Size", "Overall HUD scale.", h(&H::scale), { 0.5, 3.0, 0.05, 2, "x" });
        p.number("Text size", "Text size before scaling.", h(&H::text_size), { 8.0, 32.0, 0.5, 1, "px" });
        p.number("Line spacing", "Space between lines.", h(&H::line_spacing), { 1.0, 2.0, 0.05, 2, "x" });
        p.number("Section spacing", "Extra space before each section.", h(&H::section_spacing), { 0.0, 2.0, 0.05, 2, "lines" });
        p.number("Column gap", "Space between names and values.", h(&H::column_gap), { 0.0, 60.0, 1.0, 0, "px" });
        p.number("Margin", "Space from the screen edge.", h(&H::margin), { 0.0, 60.0, 1.0, 0, "px" });
        p.number("Padding", "Space inside the panel.", h(&H::padding), { 0.0, 40.0, 1.0, 0, "px" });
        p.number("Refresh interval", "How often the numbers update.", h(&H::refresh_interval), { 0.0, 2.0, 0.05, 2, "s" });
        p.toggle("Bold headers", "Section titles in bold.", h(&H::bold_headers));
        p.toggle("Fit to screen", "Shrink the panel when it would run off the screen.", h(&H::fit_to_screen));

        p.header("Colors");
        p.color("Text", "Value text color.", h(&H::text_color));
        p.color("Labels", "Name text color.", h(&H::label_color));
        p.color("Headers", "Section title color.", h(&H::header_color));
        p.color("Background", "Panel background (A is opacity).", h(&H::background), true);
        p.color("Text shadow", "Shadow behind text (A is opacity).", h(&H::text_shadow), true);
        p.number("Shadow offset", "How far the text shadow is offset.", h(&H::shadow_offset), { 0.0, 4.0, 0.5, 1, "px" });

        p.header("Crosshair");
        p.color("Crosshair color", "Color of the crosshair (A is opacity).", h(&H::crosshair_color), true);
        p.number("Crosshair size", "Length of each arm.", h(&H::crosshair_size), { 2.0, 40.0, 1.0, 0, "px" });
        p.number("Crosshair gap", "Empty space in the middle.", h(&H::crosshair_gap), { 0.0, 20.0, 1.0, 0, "px" });
        p.number("Crosshair thickness", "Line thickness.", h(&H::crosshair_thickness), { 1.0, 8.0, 1.0, 0, "px" });
    }

    static constexpr double SECONDS_PER_MINUTE = 60.0;
    static constexpr const char* CUSTOM = "custom";
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_PAGES_HPP