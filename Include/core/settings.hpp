#ifndef VOXELSPIRE_CORE_SETTINGS_HPP
#define VOXELSPIRE_CORE_SETTINGS_HPP

#include "limits.hpp"
#include "types.hpp"
#include "../input/input_bindings.hpp"
#include "settings/world.hpp"
#include "settings/camera.hpp"
#include "settings/menu.hpp"
#include "settings/performance.hpp"
#include "settings/render.hpp"
#include "settings/hud.hpp"
#include "settings/character.hpp"
#include "settings/lighting.hpp"
#include "settings/sky.hpp"
#include "settings/physics.hpp"
#include "settings/water.hpp"
#include "settings/weather.hpp"
#include "settings/terrain.hpp"
#include "settings/survival.hpp"
#include "settings/items.hpp"

namespace voxelspire {

struct GameSettings {
    WorldSettings       world;
    TerrainSettings     terrain;
    PhysicsSettings     physics;
    SurvivalSettings    survival;
    ItemRules           items;
    InventorySettings   inventory;
    WaterSettings       water;
    CharacterSettings   character;
    InputBindings       bindings = InputBindings::defaults();
    MenuSettings        menu;
    SaveSettings        saves;
    StreamingSettings   streaming;
    LodSettings         lod;
    LightingSettings    lighting;
    DayCycleSettings    day_cycle;
    CelestialSettings   celestial;
    SeasonSettings      seasons;
    WeatherSettings     weather;
    WeatherViewSettings weather_view;
    ParticleSettings    particles;
    CameraSettings      camera;
    DisplaySettings     display;
    ControlSettings     controls;
    SimulationSettings  simulation;
    EntitySettings      entities;
    RenderSettings      render;
    HudSettings         hud;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_HPP