#ifndef VOXELSPIRE_UI_SETTINGS_PAGES_HPP
#define VOXELSPIRE_UI_SETTINGS_PAGES_HPP

#include <functional>
#include <string>
#include <vector>
#include "../lighting/presets.hpp"
#include "settings_registry.hpp"

namespace voxelspire {

struct SettingsHooks {
    std::function<double()>     hour;
    std::function<void(double)> set_hour;
    std::function<void()>       save;
    std::function<void()>       reload;
    std::function<void()>       respawn;
};

struct SettingsTabs {
    static constexpr const char* GENERAL     = "general";
    static constexpr const char* CONTROLS    = "controls";
    static constexpr const char* CHARACTER   = "character";
    static constexpr const char* PHYSICS     = "physics";
    static constexpr const char* TIME        = "time_sky";
    static constexpr const char* CELESTIAL   = "sun_moon_stars";
    static constexpr const char* LIGHTING    = "lighting";
    static constexpr const char* SHADOWS     = "shadows";
    static constexpr const char* ATMOSPHERE  = "atmosphere";
    static constexpr const char* REFLECTIONS = "reflections_water";
    static constexpr const char* RENDERING   = "rendering";
    static constexpr const char* SIMULATION  = "simulation";
    static constexpr const char* HUD         = "hud";
};

class SettingsPages {
public:
    static constexpr const char* CUSTOM = "custom";

    static void register_all(SettingsRegistry& registry, const SettingsHooks& hooks, const LightingPresets& presets) {
        const LightingPresets* list = &presets;
        registry.add_tab(SettingsTabs::GENERAL, "General", "Display, camera, mouse and this menu", [hooks](SettingsPage& p) { general(p, hooks); });
        registry.add_tab(SettingsTabs::CONTROLS, "Controls", "Change, add or remove key bindings", controls);
        registry.add_tab(SettingsTabs::CHARACTER, "Character", "Body size, movement speeds and the hand light", character);
        registry.add_tab(SettingsTabs::PHYSICS, "Physics", "Gravity, air resistance, water and entity pushing", physics);
        registry.add_tab(SettingsTabs::TIME, "Time & Sky", "Day length, tick rate, sun path and sky colors", [hooks](SettingsPage& p) { time(p, hooks); });
        registry.add_tab(SettingsTabs::CELESTIAL, "Sun, Moon & Stars", "Look, detail and light of the sun, moon and stars", celestial);
        registry.add_tab(SettingsTabs::LIGHTING, "Lighting", "Presets, light engine, brightness and light sources", [list](SettingsPage& p) { lighting(p, *list); });
        registry.add_tab(SettingsTabs::SHADOWS, "Shadows", "Sun and lamp shadows", shadows);
        registry.add_tab(SettingsTabs::ATMOSPHERE, "Atmosphere", "Fog, hazy air and light rays", atmosphere);
        registry.add_tab(SettingsTabs::REFLECTIONS, "Reflections & Water", "Mirrors, water, glass and shine", reflections);
        registry.add_tab(SettingsTabs::RENDERING, "Rendering", "Distance, terrain detail, outline, player model and particles", rendering);
        registry.add_tab(SettingsTabs::SIMULATION, "Simulation", "World loading, background work and timing limits", simulation);
        registry.add_tab(SettingsTabs::HUD, "HUD", "The debug panel and crosshair", hud);
    }

private:
    static constexpr double BYTES_PER_MB = 1024.0 * 1024.0;
    static constexpr double MIN_SECONDS  = 1e-6;

    static void mark_custom(SettingsPage& p) {
        GameSettings* g = &p.live();
        p.touching([g] { g->lighting.name = CUSTOM; });
    }

    static void general(SettingsPage& p, const SettingsHooks& hooks) {
        using CL = CameraLimits;
        auto cam = [](double CameraSettings::*m) { return field(&GameSettings::camera, m); };

        p.header("Display").applies(Apply::Display);
        p.toggle("VSync", "Match the frame rate to the monitor to stop screen tearing.", field(&GameSettings::display, &DisplaySettings::vsync));
        p.integer("Max FPS", "Frame rate cap. 0 means unlimited.", field(&GameSettings::display, &DisplaySettings::max_fps), DisplayLimits::max_fps).unit("fps");

        p.header("Camera").applies(Apply::Camera);
        p.decimal("Field of view", "How wide the camera sees, measured vertically.", cam(&CameraSettings::fov_y), CL::fov_y).unit("deg").decimals(1);
        p.decimal("Near clip", "Closest distance the camera draws. Lower sees closer walls, higher reduces flicker far away.", cam(&CameraSettings::near_plane), CL::near_plane).unit("blocks");
        p.decimal("Third-person distance", "How far behind or in front of you the third-person camera sits.", cam(&CameraSettings::third_person_distance), CL::third_person_distance).unit("blocks").decimals(1);
        p.decimal("Camera wall margin", "Gap kept between the third-person camera and walls.", cam(&CameraSettings::collision_margin), CL::collision_margin).unit("blocks");
        p.toggle("Body in first person", "Keep your body in first person so it casts a shadow and shows in mirrors and water.", field(&GameSettings::render, &RenderSettings::first_person_body));

        p.header("Mouse").applies(Apply::Nothing);
        p.decimal("Mouse sensitivity", "How far the view turns per pixel of mouse movement.", field(&GameSettings::controls, &ControlSettings::mouse_sensitivity), ControlLimits::mouse_sensitivity).decimals(3);
        p.toggle("Invert Y", "Moving the mouse up looks down.", field(&GameSettings::controls, &ControlSettings::invert_y));

        p.header("This menu");
        p.toggle("Pause while open", "Stop the world while the menu is open.", field(&GameSettings::menu, &MenuSettings::pause_game));
        p.toggle("Save when closing", "Write your settings to the settings file every time the menu closes.", field(&GameSettings::menu, &MenuSettings::save_on_close));
        p.decimal("Menu size", "Scale of this menu.", field(&GameSettings::menu, &MenuSettings::scale), MenuLimits::scale).unit("x");
        p.color("Accent color", "Color of sliders, the selected tab and section titles.", field(&GameSettings::menu, &MenuSettings::accent));
        p.color("Toggle color", "Color of switched-on checkboxes.", field(&GameSettings::menu, &MenuSettings::toggle_on));
        p.color("Error color", "Color of error messages.", field(&GameSettings::menu, &MenuSettings::error));
        p.button("Save now", "Write the current settings to the settings file.", hooks.save);
        p.button("Load saved settings", "Throw away unsaved changes and load the settings file again.", hooks.reload);
        p.button("Respawn", "Go back to the spawn point.", hooks.respawn);
    }

    static void controls(SettingsPage& p) {
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

    static void character(SettingsPage& p) {
        using C  = CharacterSettings;
        using CL = CharacterLimits;
        using DL = DynamicLightLimits;
        auto c = [](double C::*m) { return field(&GameSettings::character, m); };
        auto hand = [](auto DynamicLight::*m) { return field(&GameSettings::hand_light, m); };

        p.header("Body").applies(Apply::Character);
        p.decimal("Width", "How wide the player is.", c(&C::width), CL::width).unit("blocks");
        p.decimal("Reach", "How far away you can target blocks.", c(&C::reach), CL::reach).unit("blocks").decimals(1);
        p.decimal("Standing height", "Height while standing.", c(&C::standing_height), CL::standing_height).unit("blocks");
        p.decimal("Crouching height", "Height while crouching.", c(&C::crouching_height), CL::crouching_height).unit("blocks");
        p.decimal("Crawling height", "Height while crawling.", c(&C::crawling_height), CL::crawling_height).unit("blocks");
        p.decimal("Swimming height", "Height while doing swim strokes.", c(&C::swimming_height), CL::swimming_height).unit("blocks");
        p.decimal("Standing eye height", "Camera height above your feet while standing.", c(&C::standing_eye_height), CL::standing_eye_height).unit("blocks");
        p.decimal("Crouching eye height", "Camera height while crouching.", c(&C::crouching_eye_height), CL::crouching_eye_height).unit("blocks");
        p.decimal("Crawling eye height", "Camera height while crawling.", c(&C::crawling_eye_height), CL::crawling_eye_height).unit("blocks");
        p.decimal("Swimming eye height", "Camera height while doing swim strokes.", c(&C::swimming_eye_height), CL::swimming_eye_height).unit("blocks");

        p.header("Speeds");
        p.decimal("Walk", "Normal walking speed.", c(&C::walk_speed), CL::walk_speed).unit("b/s");
        p.decimal("Sprint", "Speed while sprinting.", c(&C::sprint_speed), CL::sprint_speed).unit("b/s");
        p.decimal("Crouch", "Speed while crouching.", c(&C::crouch_speed), CL::crouch_speed).unit("b/s");
        p.decimal("Crawl", "Speed while crawling.", c(&C::crawl_speed), CL::crawl_speed).unit("b/s");
        p.decimal("Swim", "Speed while treading water.", c(&C::swim_speed), CL::swim_speed).unit("b/s");
        p.decimal("Swim stroke", "Speed while doing swim strokes.", c(&C::stroke_speed), CL::stroke_speed).unit("b/s");
        p.decimal("Fly", "Speed of a flying mode, if one is enabled.", c(&C::fly_speed), CL::fly_speed).unit("b/s");

        p.header("Alternate speeds");
        p.decimal("Walk (alt)", "Walking speed while holding the alternate speed key.", c(&C::alt_walk_speed), CL::alt_walk_speed).unit("b/s");
        p.decimal("Sprint (alt)", "Sprint speed while holding the alternate speed key.", c(&C::alt_sprint_speed), CL::alt_sprint_speed).unit("b/s");
        p.decimal("Crouch (alt)", "Crouch speed while holding the alternate speed key.", c(&C::alt_crouch_speed), CL::alt_crouch_speed).unit("b/s");
        p.decimal("Crawl (alt)", "Crawl speed while holding the alternate speed key.", c(&C::alt_crawl_speed), CL::alt_crawl_speed).unit("b/s");
        p.decimal("Swim (alt)", "Swimming speed while holding the alternate speed key.", c(&C::alt_swim_speed), CL::alt_swim_speed).unit("b/s");
        p.decimal("Swim stroke (alt)", "Stroke speed while holding the alternate speed key.", c(&C::alt_stroke_speed), CL::alt_stroke_speed).unit("b/s");
        p.decimal("Fly (alt)", "Flying speed while holding the alternate speed key.", c(&C::alt_fly_speed), CL::alt_fly_speed).unit("b/s");

        p.header("Jumping and acceleration");
        p.decimal("Jump strength", "Upward speed when you jump.", c(&C::jump_velocity), CL::jump_velocity).unit("b/s");
        p.decimal("Ground acceleration", "How quickly you reach full speed on the ground.", c(&C::ground_acceleration), CL::ground_acceleration).unit("b/s2").decimals(1);
        p.decimal("Air acceleration", "How much you can steer in the air.", c(&C::air_acceleration), CL::air_acceleration).unit("b/s2").decimals(1);
        p.decimal("Swim acceleration", "How quickly you speed up while treading water.", c(&C::swim_acceleration), CL::swim_acceleration).unit("b/s2").decimals(1);
        p.decimal("Stroke acceleration", "How quickly swim strokes speed you up.", c(&C::stroke_acceleration), CL::stroke_acceleration).unit("b/s2").decimals(1);

        p.header("Swimming");
        p.decimal("Rise speed", "How fast you swim up while holding jump.", c(&C::swim_rise_speed), CL::swim_rise_speed).unit("b/s");
        p.decimal("Sink speed", "How fast you slowly sink when not swimming.", c(&C::swim_sink_speed), CL::swim_sink_speed).unit("b/s");
        p.decimal("Vertical acceleration", "How quickly rising and sinking change.", c(&C::swim_vertical_accel), CL::swim_vertical_accel).unit("b/s2").decimals(1);
        p.decimal("Surface leap", "Jump strength when leaping out of water, as a fraction of a normal jump.", c(&C::surface_leap), CL::surface_leap).unit("x");
        p.decimal("Stroke float", "How fast you drift up when not stroking.", c(&C::stroke_buoyancy), CL::stroke_buoyancy).unit("b/s");

        p.header("Flying");
        p.decimal("Vertical fly speed", "Up and down speed of a flying mode, if one is enabled.", c(&C::fly_vertical_speed), CL::fly_vertical_speed).unit("b/s");
        p.decimal("Vertical fly acceleration", "How quickly flying up and down changes.", c(&C::fly_vertical_accel), CL::fly_vertical_accel).unit("b/s2").decimals(1);

        p.header("Hand light").applies(Apply::HandLight);
        p.color("Color", "Color of the light you carry.", hand(&DynamicLight::color));
        p.decimal("Brightness", "How strong the hand light is.", hand(&DynamicLight::intensity), DL::intensity);
        p.decimal("Radius", "How far the hand light reaches.", hand(&DynamicLight::radius), DL::radius).unit("blocks").decimals(1);
        p.integer("Light level", "Light level it counts as for gameplay.", hand(&DynamicLight::level), DL::level);
        p.toggle("Counts as light", "Whether the hand light raises the light level used by gameplay.", hand(&DynamicLight::affects_light_level));
        p.toggle("Casts shadows", "Whether the hand light casts shadows (needs point shadows).", hand(&DynamicLight::casts_shadows));
    }

    static void physics(SettingsPage& p) {
        using P  = PhysicsSettings;
        using PL = PhysicsLimits;
        using WL = WorldLimits;
        using EL = EntityLimits;
        GameSettings* g = &p.live();
        auto ph = [](auto P::*m) { return field(&GameSettings::physics, m); };
        auto world = [](double WorldSettings::*m) { return field(&GameSettings::world, m); };
        auto ent = [](auto EntitySettings::*m) { return field(&GameSettings::entities, m); };

        p.header("Gravity").applies(Apply::Physics);
        p.decimal("Gravity", "How fast things fall. Earth-like is about 32 in blocks.", world(&WorldSettings::gravity), WL::gravity).unit("b/s2").decimals(1);

        p.header("Air");

        p.choice(
            "Air resistance", 
            "How falling slows down in air. World default keeps the model the world was created with.", 
            ph(&P::air_model),
            { "World default", "None", "Speed cap", "World height", "Linear drag", "Quadratic drag" }
        );

        p.when([g] { const AirModel m = g->physics.air_model; return m == AirModel::TerminalCap || m == AirModel::Linear || m == AirModel::Quadratic; });
        p.decimal("Terminal velocity", "Top falling speed in air.", ph(&P::terminal_velocity), PL::terminal_velocity).unit("b/s").decimals(1);

        p.always().header("Water");

        p.choice(
            "Water resistance", 
            "How water slows things down. Realistic uses water drag, Minecraft-like slows a fixed amount per tick.", 
            ph(&P::fluid_model),
            { "World default", "None", "Linear", "Minecraft-like", "Realistic" }
        );
        
        p.when([g] { return g->physics.fluid_model == FluidModel::Quadratic; });
        p.decimal("Water density", "Heavier fluids slow you more.", ph(&P::fluid_density), PL::fluid_density).unit("kg/m3").decimals(1);
        p.decimal("Drag coefficient", "How much your body shape resists water.", ph(&P::drag_coefficient), PL::drag_coefficient);
        p.decimal("Body mass", "Heavier bodies are slowed less.", ph(&P::body_mass), PL::body_mass).unit("kg").decimals(1);
        p.when([g] { return g->physics.fluid_model == FluidModel::Linear; });
        p.decimal("Linear drag", "Fraction of extra speed lost per second.", ph(&P::fluid_linear_drag), PL::fluid_linear_drag).unit("/s");
        p.when([g] { return g->physics.fluid_model == FluidModel::TickDamping; });
        p.decimal("Speed kept per tick", "Minecraft keeps 0.8 of its speed each tick in water.", ph(&P::tick_damping), PL::tick_damping);
        p.integer("Damping ticks per second", "How many of those ticks happen each second.", ph(&P::damping_ticks), PL::damping_ticks).unit("/s");
        p.always();
        p.decimal("Buoyancy", "How much of gravity the water cancels when fully under.", world(&WorldSettings::fluid_buoyancy), WL::fluid_buoyancy);
        p.decimal("Free sinking speed", "Fastest you sink without swimming down.", world(&WorldSettings::fluid_sink_speed), WL::fluid_sink_speed).unit("b/s");

        p.header("Entities").applies(Apply::Entities);
        p.decimal("Push strength", "How hard overlapping entities push each other apart.", ent(&EntitySettings::push_acceleration), EL::push_acceleration).unit("b/s2").decimals(1);
        p.decimal("Max push speed", "Top speed entities are pushed apart at.", ent(&EntitySettings::max_push_speed), EL::max_push_speed).unit("b/s");
        p.integer("Octree node size", "Entities per octree node before it splits. Affects performance only.", ent(&EntitySettings::octree_max_per_node), EL::octree_max_per_node);
        p.integer("Octree depth", "Deepest octree level. Affects performance only.", ent(&EntitySettings::octree_max_depth), EL::octree_max_depth);
        p.decimal("Octree looseness", "How much octree cells overlap. Affects performance only.", ent(&EntitySettings::octree_looseness), EL::octree_looseness);
        p.decimal("Octree margin", "Extra space around the octree. Affects performance only.", ent(&EntitySettings::octree_margin), EL::octree_margin).unit("blocks");
    }

    template <typename Seconds>
    static SettingsPage& linked_ticks(SettingsPage& p, const std::string& label, const std::string& description, bool whole, Seconds seconds) {
        GameSettings* g = &p.live();
        GameSettings* base = &p.base_settings();
        const Bounds widest{ 0.0, SimulationLimits::tick_rate.max * DayCycleLimits::real_day_minutes.max * DayCycleSettings::REAL_SECONDS_PER_MINUTE };
        auto get = [g, seconds] { return g->simulation.tick_rate * seconds(*g); };
        auto set = [g, seconds](double ticks) { g->simulation.tick_rate = SimulationLimits::tick_rate.clamp(ticks / vmax(seconds(*g), MIN_SECONDS)); };
        
        if (whole) p.custom_integer(label, description, get, set, widest);
        else p.custom_decimal(label, description, get, set, widest);

        return p.transient()
                .live_limits([g, seconds] { return Bounds{ SimulationLimits::tick_rate.min * seconds(*g), SimulationLimits::tick_rate.max * seconds(*g) }; })
                .defaults([g, base] { g->simulation.tick_rate = base->simulation.tick_rate; }, [g, base] { return g->simulation.tick_rate == base->simulation.tick_rate; })
                .logarithmic();
    }

    static void time(SettingsPage& p, const SettingsHooks& hooks) {
        using D  = DayCycleSettings;
        using DL = DayCycleLimits;
        using SL = SimulationLimits;
        using LL = LightingLimits;
        GameSettings* g = &p.live();
        auto d = [](auto D::*m) { return field(&GameSettings::day_cycle, m); };
        auto sim = [](auto SimulationSettings::*m) { return field(&GameSettings::simulation, m); };
        auto l = [](double LightingSettings::*m) { return field(&GameSettings::lighting, m); };
 
        p.header("Time").applies(Apply::Nothing);
        p.toggle("Day cycle", "Let time pass. Turn off to freeze the time of day.", d(&D::enabled));
        
        p.custom_decimal(
            "Time of day", 
            "Set the current time.", 
            hooks.hour, 
            hooks.set_hour, 
            { 0.0, DL::hours_per_day.max }
        ).unit("h").transient().live_limits([g] { return Bounds{ 0.0, g->day_cycle.hours_per_day }; });
        
        p.decimal(
            "Real day length", 
            "How long one full day lasts in real minutes at normal game speed. Changing ticks per second never changes this.",
            d(&D::real_day_minutes), 
            DL::real_day_minutes
        ).unit("min").decimals(1).logarithmic();

        p.integer("Hours per day", "How many hours the clock shows in one day. Only changes the clock, not how long a day lasts.", d(&D::hours_per_day), DL::hours_per_day).unit("h");
        p.integer("Minutes per hour", "How many minutes the clock shows in one hour. Only changes the clock, not how long a day lasts.", d(&D::minutes_per_hour), DL::minutes_per_hour).unit("min");
        p.integer("Seconds per minute", "How many seconds the clock shows in one minute. Only changes the clock, not how long a day lasts.", d(&D::seconds_per_minute), DL::seconds_per_minute).unit("s");
 
        p.header("Ticks");

        p.decimal(
            "Ticks per second", 
            "Simulation ticks per real second. Higher is smoother physics. The day keeps the same real length.",
            sim(&SimulationSettings::tick_rate), 
            SL::tick_rate
        ).unit("ticks/s").logarithmic();

        linked_ticks(
            p, 
            "Ticks per day", 
            "Ticks in one full day. Changing it changes ticks per second, so the day keeps the same real length.", 
            true,
            [](const GameSettings& s) { return s.day_cycle.real_day_seconds(); }
        ).unit("ticks");
        linked_ticks(
            p, 
            "Ticks per hour", 
            "Ticks in one clock hour. Changing it changes ticks per second, so the day keeps the same real length.", 
            false,
            [](const GameSettings& s) { return s.day_cycle.real_day_seconds() / s.day_cycle.hours_per_day; }
        ).unit("ticks").decimals(2);

        linked_ticks(
            p, 
            "Ticks per minute", 
            "Ticks in one clock minute. Changing it changes ticks per second, so the day keeps the same real length.", 
            false,
            [](const GameSettings& s) { return s.day_cycle.real_day_seconds() / s.day_cycle.minutes_per_day(); }
        ).unit("ticks").decimals(2);

        linked_ticks(
            p, 
            "Ticks per clock second", 
            "Ticks in one clock second. Changing it changes ticks per second, so the day keeps the same real length.", 
            false,
            [](const GameSettings& s) { return s.day_cycle.real_day_seconds() / s.day_cycle.seconds_per_day(); }
        ).unit("ticks").decimals(3);
        
        p.decimal("Game speed", "Speeds up or slows down everything: movement, physics and time.", sim(&SimulationSettings::game_speed), SL::game_speed).unit("x").logarithmic();
        p.integer("Max ticks per frame", "Limit on catch-up ticks after a slow frame. Raise it with high tick rates.", sim(&SimulationSettings::max_ticks_per_frame), SL::max_ticks_per_frame);
 
        p.header("Sun path").applies(Apply::Sky);
        p.decimal("Sun tilt", "How far the sun's path leans away from straight overhead.", d(&D::sun_tilt), DL::sun_tilt);
        p.decimal("Night brightness", "How bright the sky light stays at night.", d(&D::night_brightness), DL::night_brightness);
        p.decimal("Horizon fade", "How gradually the sun fades out at the horizon.", d(&D::horizon_fade), DL::horizon_fade);
        p.decimal("Twilight length", "How long dawn and dusk last.", d(&D::twilight), DL::twilight);
        p.decimal("Dusk color strength", "How orange the sky gets at sunrise and sunset.", d(&D::dusk_sky_mix), DL::dusk_sky_mix);
 
        p.header("Sky glow").applies(Apply::Lighting);
        p.decimal("Glow strength", "Strength of the sky glow around the sun (with atmosphere on).", l(&LightingSettings::sky_glow), LL::sky_glow);
        p.decimal("Glow spread", "Width of the soft outer sky glow. Lower is wider.", l(&LightingSettings::sun_glow_spread), LL::sun_glow_spread).decimals(1);
        p.integer("Glow focus", "Tightness of the bright inner sky glow. Higher is tighter.", l(&LightingSettings::sun_glow_focus), LL::sun_glow_focus).logarithmic();
 
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

    static void celestial(SettingsPage& p) {
        using C = CelestialSettings;
        GameSettings* g = &p.live();
        auto c = [](auto C::*m) { return field(&GameSettings::celestial, m); };
        auto sun = [](auto SunSettings::*m) { return field(&GameSettings::celestial, &C::sun, m); };
        auto moon = [](auto MoonSettings::*m) { return field(&GameSettings::celestial, &C::moon, m); };
        auto star = [](auto StarSettings::*m) { return field(&GameSettings::celestial, &C::stars, m); };
        auto sun_glow = [](auto GlowSettings::*m) { return [m](GameSettings& s) -> auto& { return s.celestial.sun.glow.*m; }; };
        auto moon_glow = [](auto GlowSettings::*m) { return [m](GameSettings& s) -> auto& { return s.celestial.moon.glow.*m; }; };

        p.header("Placement").applies(Apply::Celestial);
        p.decimal("Distance", "How far away the sky objects are drawn, as a fraction of the view distance.", c(&C::distance), CelestialLimits::distance);
        p.decimal("Horizon fade", "How softly things fade out below the horizon. 0 keeps them visible below it.", c(&C::horizon_fade), CelestialLimits::horizon_fade).decimals(3);
        p.toggle("Hide underwater", "Hide the sun, moon and stars while your head is underwater.", c(&C::hide_underwater));

        p.header("Sun");
        p.toggle("Show sun", "Draw the sun.", sun(&SunSettings::visible));
        p.when([g] { return g->celestial.sun.visible; });
        p.decimal("Sun size", "How big the sun looks across the sky.", sun(&SunSettings::size), SunLimits::size).unit("deg").decimals(1);
        p.integer("Sun roundness", "Detail: points around the sun's edge.", sun(&SunSettings::segments), SunLimits::segments);
        p.integer("Sun shading detail", "Detail: rings used for edge darkening.", sun(&SunSettings::rings), SunLimits::rings);
        p.color("Sun color", "Color of the sun during the day.", sun(&SunSettings::color));
        p.color("Sunset sun color", "Color of the sun near the horizon.", sun(&SunSettings::dusk_color));
        p.decimal("Sun brightness", "How bright the sun looks. It is added on top of the sky, so it always stands out.", sun(&SunSettings::brightness), SunLimits::brightness);
        p.decimal("Edge darkening", "How much darker the sun is at its edge.", sun(&SunSettings::limb_darkening), SunLimits::limb_darkening);
        p.decimal("Sun edge softness", "How blurry the sun's edge is.", sun(&SunSettings::edge_softness), SunLimits::edge_softness);
        p.toggle("Sun glow", "A soft glow around the sun.", sun_glow(&GlowSettings::enabled));
        p.when([g] { return g->celestial.sun.visible && g->celestial.sun.glow.enabled; });
        p.decimal("Sun glow size", "How far the glow reaches, compared to the sun's size.", sun_glow(&GlowSettings::size), GlowLimits::size).unit("x");
        p.decimal("Sun glow strength", "How bright the glow is.", sun_glow(&GlowSettings::strength), GlowLimits::strength);
        p.integer("Sun glow detail", "Detail: rings used to draw the glow.", sun_glow(&GlowSettings::rings), GlowLimits::rings);

        p.always().header("Sunlight").applies(Apply::Lighting);
        mark_custom(p);

        p.toggle(
            "Direct light engine", 
            "Same as Direct sun and moon light in Lighting. Needed for the sun and moon to light up the world.",
            field(&GameSettings::lighting, &LightingSettings::sun_lighting)
        ).transient();

        p.applies(Apply::Celestial).touching({});
        p.toggle("Sun gives off light", "Whether the sun lights up the world.", sun(&SunSettings::emits_light));
        p.when([g] { return g->celestial.sun.emits_light; });
        p.decimal("Sunlight strength", "How strong sunlight is.", sun(&SunSettings::light_strength), SunLimits::light_strength);
        p.color("Sunlight color", "Color of sunlight during the day.", sun(&SunSettings::light_color));
        p.color("Sunset light color", "Color of sunlight near the horizon.", sun(&SunSettings::dusk_light_color));

        p.always().header("Moon");
        p.toggle("Show moon", "Draw the moon.", moon(&MoonSettings::visible));
        p.when([g] { return g->celestial.moon.visible; });
        p.decimal("Moon size", "How big the moon looks across the sky.", moon(&MoonSettings::size), MoonLimits::size).unit("deg").decimals(1);
        p.integer("Moon roundness", "Detail: points around the moon's edge.", moon(&MoonSettings::segments), MoonLimits::segments);
        p.integer("Moon shading detail", "Detail: rings used to shade the moon's phase. More is smoother.", moon(&MoonSettings::rings), MoonLimits::rings);
        p.color("Moon color", "Color of the lit part of the moon.", moon(&MoonSettings::color));
        p.decimal("Moon brightness", "How bright the moon looks.", moon(&MoonSettings::brightness), MoonLimits::brightness);
        p.decimal("Moon edge softness", "How blurry the moon's edge is.", moon(&MoonSettings::edge_softness), MoonLimits::edge_softness);
        p.decimal("Daytime visibility", "How visible the moon is during the day. 0 hides it in daylight.", moon(&MoonSettings::day_visibility), MoonLimits::day_visibility);
        p.toggle("Moon glow", "A soft glow around the moon.", moon_glow(&GlowSettings::enabled));
        p.when([g] { return g->celestial.moon.visible && g->celestial.moon.glow.enabled; });
        p.decimal("Moon glow size", "How far the glow reaches, compared to the moon's size.", moon_glow(&GlowSettings::size), GlowLimits::size).unit("x");
        p.decimal("Moon glow strength", "How bright the glow is.", moon_glow(&GlowSettings::strength), GlowLimits::strength);
        p.integer("Moon glow detail", "Detail: rings used to draw the glow.", moon_glow(&GlowSettings::rings), GlowLimits::rings);

        p.always().header("Moon phases");
        p.toggle("Phases", "The moon waxes and wanes over several days. Off keeps it full.", moon(&MoonSettings::phases));
        p.when([g] { return g->celestial.moon.phases; });
        p.decimal("Days per cycle", "How many days from one full moon to the next.", moon(&MoonSettings::cycle_days), MoonLimits::cycle_days).unit("days").decimals(1);
        p.decimal("Phase offset", "Shifts the cycle. 0 is a full moon on day 0, 0.5 a new moon.", moon(&MoonSettings::phase_offset), MoonLimits::phase_offset);
        p.decimal("Earthshine", "How visible the dark part of the moon is.", moon(&MoonSettings::earthshine), MoonLimits::earthshine);
        p.decimal("Dark side opacity", "How much the dark part of the moon hides the sky and stars behind it at night.", moon(&MoonSettings::dark_opacity), MoonLimits::dark_opacity);
        p.decimal("Shadow edge softness", "How soft the line between the lit and dark part is.", moon(&MoonSettings::terminator), MoonLimits::terminator);

        p.always().header("Moonlight");
        p.toggle("Moon gives off light", "Whether the moon lights up the world at night.", moon(&MoonSettings::emits_light));
        p.when([g] { return g->celestial.moon.emits_light; });
        p.decimal("Moonlight strength", "How strong moonlight is compared to sunlight.", moon(&MoonSettings::light_strength), MoonLimits::light_strength);
        p.color("Moonlight color", "Color of moonlight.", moon(&MoonSettings::light_color));

        p.always().header("Stars");
        p.toggle("Show stars", "Draw stars at night.", star(&StarSettings::visible));
        p.when([g] { return g->celestial.stars.visible; });
        p.integer("Star count", "How many stars there are.", star(&StarSettings::count), StarLimits::count);
        p.decimal("Star size", "How big the largest stars look.", star(&StarSettings::size), StarLimits::size).unit("deg");
        p.decimal("Size variation", "How much smaller some stars are.", star(&StarSettings::size_variation), StarLimits::size_variation);
        p.decimal("Star brightness", "How bright the stars are.", star(&StarSettings::brightness), StarLimits::brightness);
        p.decimal("Brightness variation", "How much dimmer some stars are.", star(&StarSettings::brightness_variation), StarLimits::brightness_variation);
        p.color("Star color", "Main color of the stars.", star(&StarSettings::color));
        p.color("Warm star color", "Color of the warmer stars.", star(&StarSettings::warm_color));
        p.color("Cool star color", "Color of the cooler stars.", star(&StarSettings::cool_color));
        p.decimal("Color variation", "How strongly stars take on the warm or cool color.", star(&StarSettings::color_variation), StarLimits::color_variation);
        p.decimal("Twinkle", "How much stars flicker.", star(&StarSettings::twinkle), StarLimits::twinkle);
        p.decimal("Twinkle speed", "How fast stars flicker.", star(&StarSettings::twinkle_speed), StarLimits::twinkle_speed).unit("/s").decimals(1);
        p.decimal("Daytime stars", "How visible stars are during the day.", star(&StarSettings::day_visibility), StarLimits::day_visibility);
        p.toggle("Turn with the sky", "Stars move across the sky with the sun and moon.", star(&StarSettings::rotate));
        p.integer("Star seed", "Changes the star pattern.", star(&StarSettings::seed), StarLimits::seed);
    }

    static void lighting(SettingsPage& p, const LightingPresets& presets) {
        using L  = LightingSettings;
        using LL = LightingLimits;
        GameSettings* g = &p.live();
        GameSettings* base = &p.base_settings();
        const LightingPresets* list = &presets;
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };

        p.header("Preset").applies(Apply::Lighting);

        p.custom_choice(
            "Preset", "Load a whole lighting setup at once. Changing anything below makes it Custom. Mods can add more presets.",
            [list] { return list->names(); },
            [g, list] { return list->index_of(g->lighting.name); },
            [g, list](int i) { if (const L* preset = list->at(i)) g->lighting = *preset; }
        ).defaults([g, base] { g->lighting = base->lighting; }, [g, base] { return g->lighting.name == base->lighting.name; });

        p.header("Light engine").applies(Apply::LightFormat);
        
        p.choice(
            "Light type", 
            "Colored lets lamps tint the world. Plain is white light and uses less memory. None turns light off.",
            field(&GameSettings::world, &WorldSettings::light_format), 
            { "None", "Plain", "Colored" }
        );

        p.applies(Apply::Lighting);
        mark_custom(p);
        p.decimal("Update budget", "Time per frame spent spreading light after changes.", l(&L::update_budget_ms), LL::update_budget_ms).unit("ms").decimals(1);

        p.header("Block light");
        p.toggle("Baked light", "Light stored in the world (sky light and lamps).", l(&L::baked_light));
        p.when([g] { return g->lighting.baked_light; });
        p.toggle("Smooth lighting", "Blend light between neighboring blocks.", l(&L::smooth_lighting));
        p.always();
        p.decimal("Ambient occlusion", "Darkening in corners and creases.", l(&L::ambient_occlusion), LL::ambient_occlusion);
        p.decimal("Occlusion step", "How much each blocking neighbor darkens a corner.", l(&L::occlusion_step), LL::occlusion_step);
        p.decimal("Face shading", "How much darker sides and bottoms of blocks are.", l(&L::face_shading), LL::face_shading);
        p.decimal("Falloff", "How quickly light fades with distance. Higher keeps it brighter longer.", l(&L::falloff), LL::falloff);
        p.decimal("Minimum light", "Darkest anything can get.", l(&L::min_light), LL::min_light);
        p.decimal("Maximum light", "Brightest anything can get.", l(&L::max_light), LL::max_light);
        p.decimal("Ambient", "Extra light added everywhere.", l(&L::ambient), LL::ambient);
        p.decimal("Sky light", "Strength of light from the sky.", l(&L::sky_light), LL::sky_light);
        p.decimal("Block light", "Strength of light from lamps and other glowing blocks.", l(&L::block_light), LL::block_light);
        p.color("Block light tint", "Color of block light when using plain light.", l(&L::block_tint));

        p.header("Sunlight");
        p.toggle("Direct sun and moon light", "Sunlight and moonlight that shine from a direction and cast shadows.", l(&L::sun_lighting));
        p.when([g] { return g->lighting.sun_lighting; });
        p.decimal("Direct light strength", "How strong direct sun and moon light is overall.", l(&L::sun_strength), LL::sun_strength);
        p.decimal("Sun exposure", "How much sky light a spot needs before sunlight reaches it.", l(&L::sun_exposure), LL::sun_exposure).decimals(1);

        p.always().header("Moving lights");
        p.toggle("Dynamic lights", "Lights that move, like the one you carry.", l(&L::dynamic_lights));
        p.when([g] { return g->lighting.dynamic_lights; });
        p.integer("Max dynamic lights", "Most moving lights drawn at once.", l(&L::max_dynamic_lights), LL::max_dynamic_lights);
        p.integer("Dynamic light distance", "Moving lights farther than this are skipped.", l(&L::dynamic_light_distance), LL::dynamic_light_distance).unit("blocks");

        p.always().header("Lamp lights");
        p.toggle("Lamp lights", "Lamps also shine real light (needed for lamp shadows).", l(&L::block_point_lights));
        p.when([g] { return g->lighting.block_point_lights; });
        p.integer("Max lamp lights", "Most lamps shining at once.", l(&L::max_block_point_lights), LL::max_block_point_lights);
        p.integer("Lamp light distance", "Lamps farther than this don't shine.", l(&L::block_point_light_distance), LL::block_point_light_distance).unit("blocks");
        p.decimal("Lamp brightness", "How strong lamp light is.", l(&L::block_point_light_intensity), LL::block_point_light_intensity);
        p.decimal("Lamp radius", "How far lamp light reaches.", l(&L::block_point_light_radius), LL::block_point_light_radius).unit("blocks").decimals(1);
        p.decimal("Lamp fade", "Distance over which far lamps fade in and out instead of popping.", l(&L::point_light_fade), LL::point_light_fade).unit("blocks").decimals(1);

        p.always().header("Color grading");
        p.toggle("Tone mapping", "Film-like handling of very bright light.", l(&L::tone_mapping));
        p.when([g] { return g->lighting.tone_mapping; });
        p.decimal("Exposure", "Overall brightness.", l(&L::exposure), LL::exposure);
        p.decimal("Saturation", "How colorful the image is.", l(&L::saturation), LL::saturation);

        p.always().header("Underwater");
        p.toggle("Underwater fog", "Tint and fog when your head is underwater.", l(&L::underwater_fog));
        p.when([g] { return g->lighting.underwater_fog; });
        p.decimal("Underwater fog density", "How thick the underwater fog is.", l(&L::underwater_density), LL::underwater_density);
    }

    static void shadows(SettingsPage& p) {
        using L  = LightingSettings;
        using LL = LightingLimits;
        GameSettings* g = &p.live();
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };
        mark_custom(p);

        p.header("Sun shadows").applies(Apply::Lighting);
        p.toggle("Sun shadows", "Shadows from the sun and moon (needs direct sunlight).", l(&L::sun_shadows));
        p.when([g] { return g->lighting.sun_shadows; });
        power_choice(p, "Resolution", "Shadow sharpness. Higher is sharper but slower.", &L::sun_shadow_resolution, { 512u, 1024u, 2048u, 4096u, 8192u });
        p.integer("Distance", "How far from you shadows are drawn.", l(&L::sun_shadow_distance), LL::sun_shadow_distance).unit("blocks");
        p.decimal("Darkness", "How dark shadows are.", l(&L::shadow_strength), LL::shadow_strength);
        p.decimal("Edge blur", "Minimum blur on shadow edges.", l(&L::shadow_softness), LL::shadow_softness).unit("px").decimals(1);
        p.toggle("Soft shadows", "Shadows get blurrier the farther they are from what casts them.", l(&L::soft_shadows));
        p.decimal("Redraw distance", "How far you can walk before sun shadows are redrawn, as a share of the shadow distance. Higher redraws less often but makes shadows a little blurrier. 0 redraws whenever you move.", l(&L::sun_shadow_redraw), LL::sun_shadow_redraw).decimals(3);
        p.decimal("Sun size for soft shadows", "Bigger makes soft shadows blurrier.", l(&L::soft_shadow_sun_size), LL::soft_shadow_sun_size).unit("deg");
        p.decimal("Max blur", "Largest blur a soft shadow can have.", l(&L::max_shadow_softness), LL::max_shadow_softness).unit("px").decimals(1);
        p.integer("Blur quality", "Samples per side for soft shadows. Higher is smoother but slower.", l(&L::shadow_filter_taps), LL::shadow_filter_taps);
        p.decimal("Update step", "How far the sun moves before shadows are recalculated.", l(&L::shadow_angle_step), LL::shadow_angle_step).unit("deg");
        p.toggle("Smooth movement", "Blend between shadow updates so they move smoothly instead of jumping.", l(&L::shadow_crossfade));

        p.always().header("Lamp shadows");
        p.toggle("Lamp shadows", "Shadows from lamps and your hand light (needs lamp lights or dynamic lights).", l(&L::point_shadows));
        p.when([g] { return g->lighting.point_shadows; });
        p.integer("Shadowed lamps", "How many of the nearest lamps cast shadows.", l(&L::max_point_shadows), LL::max_point_shadows);
        power_choice(p, "Lamp shadow resolution", "Sharpness of lamp shadows.", &L::point_shadow_resolution, { 256u, 512u, 768u, 1024u, 2048u });
        p.decimal("Shadow fade", "Distance over which lamp shadows fade in and out.", l(&L::point_shadow_fade), LL::point_shadow_fade).unit("blocks").decimals(1);
        p.toggle("Hide lamps without shadows", "Lamps that don't get a shadow don't shine, so light can't leak through walls.", l(&L::hide_unshadowed_point_lights));

        p.always().header("Player shadow");
        p.toggle("Smooth player shadow", "Draws your shadow from your exact shape so it stays steady while you move.", l(&L::capsule_shadows));
        p.when([g] { return g->lighting.capsule_shadows; });
        p.decimal("Sun blur", "How soft your shadow from the sun and moon gets with distance.", l(&L::capsule_shadow_sun_size), LL::capsule_shadow_sun_size).unit("deg");
        p.decimal("Lamp blur", "How soft your shadow from lamps gets. This is the lamp's size.", l(&L::capsule_shadow_lamp_size), LL::capsule_shadow_lamp_size).unit("blocks");
    }

    static void power_choice(
        SettingsPage& p, 
        const std::string& label, 
        const std::string& description,
        unsigned int LightingSettings::*member, 
        std::vector<unsigned int> values
    ) {
        GameSettings* g = &p.live();
        GameSettings* base = &p.base_settings();
        std::vector<std::string> names;
        for (unsigned int v : values) names.push_back(std::to_string(v));

        p.custom_choice(
            label, 
            description, 
            [names] { return names; },
            [g, member, values] { for (std::size_t i = 0; i < values.size(); ++i) if (values[i] == g->lighting.*member) return static_cast<int>(i); return -1; },
            [g, member, values](int i) { if (i >= 0 && static_cast<std::size_t>(i) < values.size()) g->lighting.*member = values[static_cast<std::size_t>(i)]; }
        ).defaults([g, base, member] { g->lighting.*member = base->lighting.*member; }, [g, base, member] { return g->lighting.*member == base->lighting.*member; });
    }

    static void atmosphere(SettingsPage& p) {
        using L  = LightingSettings;
        using LL = LightingLimits;
        GameSettings* g = &p.live();
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };
        mark_custom(p);

        p.header("Sky and fog").applies(Apply::Lighting);
        p.toggle("Atmosphere", "A real sky with a sun and moon, and distance fog.", l(&L::atmosphere));
        p.when([g] { return g->lighting.atmosphere; });
        p.decimal("Fog density", "How thick distant fog is.", l(&L::fog_density), LL::fog_density).decimals(4);
        p.integer("Fog start", "Distance where fog begins.", l(&L::fog_start), LL::fog_start).unit("blocks");

        p.always().header("Hazy air");
        p.toggle("Volumetric light", "Sunlight lights up the air, so shadows show in the haze.", l(&L::volumetric_light));
        p.when([g] { return g->lighting.volumetric_light; });
        p.integer("Quality", "Depth layers in the light volume. Higher is smoother but slower.", l(&L::volumetric_steps), LL::volumetric_steps);
        p.integer("Cell size", "Screen pixels per light volume cell. Smaller is sharper but slower.", l(&L::volumetric_cell_size), LL::volumetric_cell_size).unit("px");
        p.decimal("Haze density", "How concentrated the haze is near you.", l(&L::volumetric_density), LL::volumetric_density).decimals(3);
        p.decimal("Haze brightness", "How bright the haze is.", l(&L::volumetric_intensity), LL::volumetric_intensity);
        p.decimal("Forward glow", "How much brighter haze is when looking toward the sun.", l(&L::volumetric_anisotropy), LL::volumetric_anisotropy);
        p.integer("Haze distance", "How far the haze is calculated.", l(&L::volumetric_distance), LL::volumetric_distance).unit("blocks");
        p.decimal("Near detail", "Puts more samples close to you. 1 is even spacing.", l(&L::volumetric_near_bias), LL::volumetric_near_bias).decimals(1);

        p.always().header("Light rays");
        p.toggle("Light rays", "Beams of light around things in front of the sun.", l(&L::light_shafts));
        p.when([g] { return g->lighting.light_shafts; });
        p.decimal("Ray strength", "How bright the rays are.", l(&L::light_shaft_strength), LL::light_shaft_strength);
        p.integer("Ray quality", "Samples per ray. Higher is smoother but slower.", l(&L::light_shaft_samples), LL::light_shaft_samples);
        p.decimal("Ray length", "How far the rays stretch.", l(&L::light_shaft_length), LL::light_shaft_length);
        p.decimal("Ray fade", "How quickly rays fade along their length.", l(&L::light_shaft_decay), LL::light_shaft_decay).decimals(3);
        p.decimal("Ray focus", "How closely the rays hug the sun. Higher is tighter.", l(&L::light_shaft_focus), LL::light_shaft_focus).decimals(1);
    }

    static void reflections(SettingsPage& p) {
        using L  = LightingSettings;
        using LL = LightingLimits;
        GameSettings* g = &p.live();
        auto l = [](auto L::*m) { return field(&GameSettings::lighting, m); };
        mark_custom(p);

        p.header("Shine").applies(Apply::Lighting);
        p.toggle("Reflections", "Water, glass and mirrors reflect the sky and shine in the sun.", l(&L::reflections));
        p.when([g] { return g->lighting.reflections; });
        p.decimal("Reflectivity", "How strong reflections are.", l(&L::reflectivity), LL::reflectivity);
        p.decimal("Sun highlights", "Brightness of sun sparkles on shiny surfaces.", l(&L::specular), LL::specular).decimals(1);
        p.integer("Highlight sharpness", "Higher makes the sparkles smaller and sharper.", l(&L::specular_power), LL::specular_power).logarithmic();

        p.always().header("Mirrors and water");
        p.toggle("Real reflections", "Mirrors and water show the world, including you. Each reflecting surface draws the world again.", l(&L::planar_reflections));
        p.when([g] { return g->lighting.planar_reflections; });
        p.integer("Reflecting surfaces", "How many mirrors or water surfaces can reflect at once.", l(&L::max_reflection_planes), LL::max_reflection_planes);
        p.decimal("Reflection resolution", "Detail of reflections. 1 is full screen resolution.", l(&L::reflection_resolution), LL::reflection_resolution).unit("x");
        p.integer("Search distance", "How far away a mirror or water can be to reflect.", l(&L::reflection_plane_distance), LL::reflection_plane_distance).unit("blocks");
        p.integer("Reflected view distance", "How far the world is drawn inside reflections.", l(&L::reflection_view_chunks), LL::reflection_view_chunks).unit("chunks");
        p.decimal("Ripple distortion", "How much waves bend water reflections.", l(&L::reflection_distortion), LL::reflection_distortion).decimals(3);
        p.toggle("Water reflects the world", "Let water use real reflections too, not just mirrors.", l(&L::water_planar_reflections));

        p.always().header("Screen reflections");
        p.toggle("Screen reflections", "Cheaper reflections of things already on screen (used where real reflections aren't).", l(&L::screen_reflections));
        p.when([g] { return g->lighting.screen_reflections; });
        p.integer("Screen reflection quality", "Steps traced per reflection.", l(&L::reflection_steps), LL::reflection_steps);
        p.integer("Screen reflection distance", "How far reflections are traced.", l(&L::reflection_distance), LL::reflection_distance).unit("blocks");

        p.always().header("Water");
        p.decimal("Wave height", "How rough the water surface is.", l(&L::wave_strength), LL::wave_strength);
        p.decimal("Wave size", "Higher makes smaller, busier waves.", l(&L::wave_scale), LL::wave_scale);
        p.decimal("Wave speed", "How fast waves move.", l(&L::wave_speed), LL::wave_speed);
        p.toggle("See-through water", "Water bends and tints what's under it.", l(&L::refraction));
        p.when([g] { return g->lighting.refraction; });
        p.decimal("Bending", "How much water bends the view underneath.", l(&L::refraction_strength), LL::refraction_strength);
        p.decimal("Color absorption", "How quickly deep water turns blue.", l(&L::water_absorption), LL::water_absorption);
        p.decimal("Murkiness", "How cloudy water gets with depth.", l(&L::water_scattering), LL::water_scattering);
    }

    static void rendering(SettingsPage& p) {
        using R  = RenderSettings;
        using RL = RenderLimits;
        using FL = FaceShadingLimits;
        using LD = LodLimits;
        using PL = ParticleLimits;
        GameSettings* g = &p.live();
        auto r = [](auto R::*m) { return field(&GameSettings::render, m); };
        auto shade = [](double FaceShadingSettings::*m) { return field(&GameSettings::render, &R::shading, m); };
        auto lod = [](auto LodSettings::*m) { return field(&GameSettings::lod, m); };
        auto pa = [](auto ParticleSettings::*m) { return field(&GameSettings::particles, m); };

        p.header("Distance").applies(Apply::Streaming);
        p.integer("Render distance", "How far the world is drawn.", r(&R::render_distance), RL::render_distance).unit("chunks").logarithmic();
        p.applies(Apply::Nothing);
        p.integer("Distance key step", "How much the render distance keys change it.", r(&R::render_distance_step), RL::render_distance_step).unit("chunks");

        p.header("Terrain").applies(Apply::Terrain);
        p.toggle("Merge faces", "Join matching block faces into bigger pieces. Much faster.", r(&R::merge_faces));
        p.toggle("Skip hidden floor", "Don't draw the bottom of the world that can never be seen.", r(&R::cull_void_faces));
        p.decimal("Top brightness", "Brightness of block tops before lighting.", shade(&FaceShadingSettings::up), FL::up);
        p.decimal("Bottom brightness", "Brightness of block bottoms.", shade(&FaceShadingSettings::down), FL::down);
        p.decimal("North/south brightness", "Brightness of north and south sides.", shade(&FaceShadingSettings::north_south), FL::north_south);
        p.decimal("East/west brightness", "Brightness of east and west sides.", shade(&FaceShadingSettings::east_west), FL::east_west);
        p.applies(Apply::Nothing);
        p.toggle("Cave culling", "Skip chunks hidden behind solid ground. Faster underground.", r(&R::cave_culling));
        p.toggle("Face culling", "Skip block faces pointing away from you. Faster.", r(&R::face_culling));

        p.header("Level of detail").applies(Apply::Streaming);
        p.toggle("Level of detail", "Draw far terrain with simpler shapes so the view can reach much farther.", lod(&LodSettings::enabled));
        p.when([g] { return g->lod.enabled; });
        p.integer("Detail levels", "How many levels of simpler terrain to use.", lod(&LodSettings::max_level), LD::max_level);
        p.integer("Exact levels", "Levels that keep exact block shapes.", lod(&LodSettings::exact_levels), LD::exact_levels);
        p.integer("Heightmap level", "Level where far terrain switches to a height map.", lod(&LodSettings::heightmap_level), LD::heightmap_level);
        p.decimal("Coverage threshold", "How full a far cell must be to count as solid.", lod(&LodSettings::coverage_threshold), LD::coverage_threshold);
        p.integer("Samples per cell", "Samples used to shape far terrain.", lod(&LodSettings::samples_per_cell), LD::samples_per_cell);
        p.integer("Tile size", "Size of each far terrain tile.", lod(&LodSettings::tile_chunks), LD::tile_chunks).unit("chunks");
        p.integer("Tile jobs", "Far tiles built at once in the background.", lod(&LodSettings::max_tile_jobs), LD::max_tile_jobs);

        p.always().header("Block outline").applies(Apply::Nothing);
        p.color("Outline color", "Color of the box around the block you're looking at.", r(&R::outline_color), true);
        p.integer("Outline width", "Thickness of the outline.", r(&R::outline_width), RL::outline_width).unit("px");
        p.decimal("Outline size", "How far the outline sits outside the block.", r(&R::outline_inflate), RL::outline_inflate).unit("blocks").decimals(3);

        p.header("Player model").applies(Apply::Player);
        p.color("Body color", "Main color of the player.", r(&R::player_color));
        p.color("Visor color", "Color of the visor.", r(&R::player_visor));
        p.integer("Roundness", "Segments around the body. Higher is smoother.", r(&R::capsule_segments), RL::capsule_segments);
        p.integer("Cap detail", "Rings on the rounded ends.", r(&R::capsule_rings), RL::capsule_rings);

        p.header("Particles").applies(Apply::Particles);
        p.toggle("Particles", "Small effects like sparks and dust.", pa(&ParticleSettings::enabled));
        p.when([g] { return g->particles.enabled; });
        p.integer("Max particles", "Most particles alive at once.", pa(&ParticleSettings::max_particles), PL::max_particles);
        p.integer("Spawn distance", "Emitters farther than this don't spawn particles.", pa(&ParticleSettings::emit_distance), PL::emit_distance).unit("blocks");
        p.integer("Draw distance", "Particles farther than this aren't drawn.", pa(&ParticleSettings::draw_distance), PL::draw_distance).unit("blocks");
    }

    static void simulation(SettingsPage& p) {
        using S  = StreamingSettings;
        using SL = StreamingLimits;
        GameSettings* g = &p.live();
        auto s = [](auto S::*m) { return field(&GameSettings::streaming, m); };

        p.header("World loading").applies(Apply::Streaming);
        p.integer("Simulation distance", "Chunks around you where things move and update.", s(&S::simulation_distance), SL::simulation_distance).unit("chunks");
        p.integer("Detail distance", "Chunks around you drawn in full detail.", s(&S::detail_distance), SL::detail_distance).unit("chunks");
        p.integer("Unload margin", "Extra chunks kept loaded past the edge before unloading.", s(&S::unload_margin), SL::unload_margin).unit("chunks");

        p.header("Background work").applies(Apply::Nothing);
        p.integer("World generation jobs", "Columns generated at once.", s(&S::max_column_jobs), SL::max_column_jobs);
        p.integer("Meshing jobs", "Chunks turned into shapes at once.", s(&S::max_mesh_jobs), SL::max_mesh_jobs);
        p.applies(Apply::Streaming);

        p.custom_integer(
            "Upload per frame", 
            "Most mesh data sent to the graphics card each frame.",
            [g] { return std::round(static_cast<double>(g->streaming.upload_bytes_per_frame) / BYTES_PER_MB); },
            [g](double mb) { g->streaming.upload_bytes_per_frame = static_cast<std::size_t>(mb * BYTES_PER_MB); },
            SL::upload_megabytes
        ).unit("MB");

        p.applies(Apply::Nothing);
        p.decimal("Result time budget", "Time per frame spent finishing background work.", s(&S::result_time_budget_ms), SL::result_time_budget_ms).unit("ms").decimals(1);
    }

    static void hud(SettingsPage& p) {
        using H  = HudSettings;
        using HL = HudLimits;
        auto h = [](auto H::*m) { return field(&GameSettings::hud, m); };
        auto sec = [](bool HudSections::*m) { return field(&GameSettings::hud, &H::sections, m); };

        p.header("Show").applies(Apply::Nothing);
        p.toggle("Debug panel", "The panel with position, speed and performance numbers.", h(&H::show_debug));
        p.toggle("Crosshair", "The cross in the middle of the screen.", h(&H::show_crosshair));
        p.toggle("Last key", "Show the last key you pressed in the panel.", h(&H::show_last_key));
        p.toggle("Performance section", "FPS, draws and memory.", sec(&HudSections::performance));
        p.toggle("GPU section", "How long each part of the frame takes on the graphics card.", sec(&HudSections::gpu));
        p.toggle("Player section", "Position, speed and target.", sec(&HudSections::player));
        p.toggle("World section", "Seed, time and loaded world.", sec(&HudSections::world));
        p.toggle("Rendering section", "Chunks, quads and reflections.", sec(&HudSections::rendering));
        p.toggle("Lighting section", "Lights, light data and the light where you stand.", sec(&HudSections::lighting));

        p.header("Layout");
        p.choice("Corner", "Which corner the panel sits in.", h(&H::corner), { "Top left", "Top right", "Bottom left", "Bottom right" });
        p.decimal("Size", "Overall HUD scale.", h(&H::scale), HL::scale).unit("x");
        p.decimal("Text size", "Text size before scaling.", h(&H::text_size), HL::text_size).unit("px").decimals(1);
        p.decimal("Line spacing", "Space between lines.", h(&H::line_spacing), HL::line_spacing).unit("x");
        p.decimal("Section spacing", "Extra space before each section.", h(&H::section_spacing), HL::section_spacing).unit("lines");
        p.integer("Column gap", "Space between names and values.", h(&H::column_gap), HL::column_gap).unit("px");
        p.integer("Margin", "Space from the screen edge.", h(&H::margin), HL::margin).unit("px");
        p.integer("Padding", "Space inside the panel.", h(&H::padding), HL::padding).unit("px");
        p.decimal("Refresh interval", "How often the numbers update.", h(&H::refresh_interval), HL::refresh_interval).unit("s");
        p.toggle("Bold headers", "Section titles in bold.", h(&H::bold_headers));
        p.toggle("Fit to screen", "Shrink the panel when it would run off the screen.", h(&H::fit_to_screen));

        p.header("Colors");
        p.color("Text", "Value text color.", h(&H::text_color));
        p.color("Labels", "Name text color.", h(&H::label_color));
        p.color("Headers", "Section title color.", h(&H::header_color));
        p.color("Background", "Panel background (A is opacity).", h(&H::background), true);
        p.color("Text shadow", "Shadow behind text (A is opacity).", h(&H::text_shadow), true);
        p.decimal("Shadow offset", "How far the text shadow is offset.", h(&H::shadow_offset), HL::shadow_offset).unit("px").decimals(1);

        p.header("Light meters");
        p.decimal("Meter length", "Length of the light level bars, in text heights.", h(&H::meter_width), HL::meter_width).unit("x").decimals(1);
        p.color("Meter background", "Empty part of the light bars (A is opacity).", h(&H::meter_background), true);
        p.color("Light level bar", "Bar for the overall light level where you stand.", h(&H::level_meter));
        p.color("Sky light bar", "Bar for sky light.", h(&H::sky_meter));
        p.color("Red light bar", "Bar for red block light.", h(&H::red_meter));
        p.color("Green light bar", "Bar for green block light.", h(&H::green_meter));
        p.color("Blue light bar", "Bar for blue block light.", h(&H::blue_meter));

        p.header("Crosshair");
        p.color("Crosshair color", "Color of the crosshair (A is opacity).", h(&H::crosshair_color), true);
        p.integer("Crosshair size", "Length of each arm.", h(&H::crosshair_size), HL::crosshair_size).unit("px");
        p.integer("Crosshair gap", "Empty space in the middle.", h(&H::crosshair_gap), HL::crosshair_gap).unit("px");
        p.integer("Crosshair thickness", "Line thickness.", h(&H::crosshair_thickness), HL::crosshair_thickness).unit("px");
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_PAGES_HPP