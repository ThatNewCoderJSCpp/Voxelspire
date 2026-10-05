#ifndef VOXELSPIRE_UI_WORLD_PAGES_HPP
#define VOXELSPIRE_UI_WORLD_PAGES_HPP

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "../world/biome.hpp"
#include "settings_registry.hpp"

namespace voxelspire {

struct WorldTabs {
    static constexpr const char* WORLD  = "world";
    static constexpr const char* LAND   = "land";
    static constexpr const char* BIOMES = "biomes";
    static constexpr const char* RIVERS = "rivers";
    static constexpr const char* CAVES  = "caves";
    static constexpr const char* BIOME  = "biome.";
};

struct WorldGroups {
    static constexpr const char* WORLD  = "World";
    static constexpr const char* BIOMES = "Biomes";
};

struct WorldOptionLimits {
    static constexpr std::size_t NAME_LENGTH = 48;
    static constexpr int         MIN_HEIGHT  = 2 * EngineLimits::CHUNK_SIZE;

    static constexpr Bounds bottom { EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z - MIN_HEIGHT };
    static constexpr Bounds top    { EngineLimits::WORLD_MIN_Z + MIN_HEIGHT, EngineLimits::WORLD_MAX_Z };
    static constexpr Bounds radius { EngineLimits::CHUNK_SIZE, EngineLimits::WORLD_MAX_HORIZONTAL };
};

class WorldPages {
public:
    using BlockList = std::vector<Identifier>;

    static BlockList surface_blocks() {
        return {
            BlockIds::GRASS, BlockIds::DIRT, BlockIds::STONE, BlockIds::BEDROCK, BlockIds::SAND, BlockIds::RED_SAND,
            BlockIds::SANDSTONE, BlockIds::GRAVEL, BlockIds::CLAY, BlockIds::SNOW, BlockIds::MUD, BlockIds::ICE, BlockIds::GLASS
        };
    }

    static void register_all(SettingsRegistry& registry, const BiomeRegistry& biomes, BlockList blocks = surface_blocks()) {
        const BiomeRegistry* list = &biomes;
        auto palette = std::make_shared<const std::vector<std::string>>(names(blocks));
        registry.set_group(WorldGroups::WORLD);
        registry.add_tab(WorldTabs::WORLD, "World", "Name, height and size of the world", world);
        registry.add_tab(WorldTabs::LAND, "Land and sea", "Continents, oceans, mountains and hills", land);
        registry.add_tab(WorldTabs::RIVERS, "Rivers", "Rivers that wind down to the sea", rivers);
        registry.add_tab(WorldTabs::CAVES, "Caves", "Tunnels, caverns, entrances, canyons and underwater caves", caves);
        registry.set_group(WorldGroups::BIOMES);
        registry.add_tab(WorldTabs::BIOMES, "All biomes", "Biome size, climate, and which biomes this world has", [list](SettingsPage& p) { biome_page(p, *list); });

        for (const auto& biome : biomes.all()) {
            std::shared_ptr<const Biome> keep = biome;
            registry.add_tab(WorldTabs::BIOME + biome->id().str(), biome->name(), "Everything about " + biome->name() + " in this world",
                             [keep, palette](SettingsPage& p) { biome_tab(p, *keep, *palette); });
        }

        registry.end_group();
    }

private:
    static void world(SettingsPage& p) {
        using W  = WorldSettings;
        using WL = WorldOptionLimits;
        GameSettings* g = &p.live();
        auto w = [](auto W::*m) { return field(&GameSettings::world, m); };

        p.header("World");
        p.text("Name", "What this world is called in the world list.", w(&W::name), WL::NAME_LENGTH);

        p.header("Height and size");
        p.integer("Bottom of the world", "The lowest block. Bedrock is placed here.", w(&W::min_z), WL::bottom).unit("blocks")
         .live_limits([g] { return Bounds{ WL::bottom.min, static_cast<double>(g->world.max_z - WL::MIN_HEIGHT) }; });
        p.integer("Top of the world", "Nothing can be built at or above this height.", w(&W::max_z), WL::top).unit("blocks")
         .live_limits([g] { return Bounds{ static_cast<double>(g->world.min_z + WL::MIN_HEIGHT), WL::top.max }; });
        p.integer("World radius", "How far the world reaches from the center in every direction.", w(&W::horizontal_limit), WL::radius).unit("blocks").logarithmic();
    }

    static void land(SettingsPage& p) {
        using T  = TerrainSettings;
        using TL = TerrainLimits;
        auto t = [](auto T::*m) { return field(&GameSettings::terrain, m); };

        p.header("Sea");
        p.integer("Sea level", "Height of the oceans.", t(&T::sea_level), TL::sea_level).unit("blocks");
        p.decimal("Ocean amount", "How much of the world is ocean. 1 is normal, lower gives more land, higher gives more sea.", t(&T::ocean_amount), TL::ocean_amount);
        p.decimal("Ocean depth", "How deep the oceans go.", t(&T::ocean_depth), TL::ocean_depth).unit("blocks").decimals(1);
        p.decimal("Shelf width", "How wide the shallow sea along coasts is before the sea floor drops away. Higher makes long, gentle shallows.", t(&T::shelf_width), TL::shelf_width).unit("x").logarithmic();
        p.decimal("Continent size", "How big continents and seas are. Lower makes many smaller seas and islands.", t(&T::continent_size), TL::continent_size).unit("x").logarithmic();

        p.header("Land");
        p.decimal("Land height", "How high ordinary land rises above the sea.", t(&T::land_height), TL::land_height).unit("blocks").decimals(1);
        p.decimal("Flatness", "How much of the land is flat or gently rolling. 1 is normal.", t(&T::flatness), TL::flatness);
        p.decimal("Hill height", "How tall rolling hills are.", t(&T::hill_height), TL::hill_height).unit("blocks").decimals(1);
        p.decimal("Roughness", "Small bumps on the ground. 0 is smooth.", t(&T::roughness), TL::roughness);
        p.integer("Bedrock layers", "How thick and uneven the bedrock floor is.", t(&T::bedrock_layers), TL::bedrock_layers);

        p.header("Mountains");
        p.decimal("Mountain amount", "How much of the land is mountains. 1 is normal, 0 is none.", t(&T::mountain_amount), TL::mountain_amount);
        p.decimal("Mountain height", "How tall mountain ranges get.", t(&T::mountain_height), TL::mountain_height).unit("blocks").decimals(1);
        p.decimal("Snow line", "How high mountains reach before they turn snowy. Higher keeps more mountains bare.", t(&T::snow_line), TL::snow_line).unit("x");
    }

    static void biome_page(SettingsPage& p, const BiomeRegistry& list) {
        using T  = TerrainSettings;
        using TL = TerrainLimits;
        GameSettings* g = &p.live();
        auto t = [](auto T::*m) { return field(&GameSettings::terrain, m); };

        p.header("Climate");
        p.decimal("Biome size", "How big every biome is. Each biome can also be made bigger or smaller on its own page.", t(&T::biome_size), TL::biome_size).unit("x").logarithmic();
        p.decimal("Size variation", "How much each new world randomly grows or shrinks its biomes and continents.", t(&T::size_variation), TL::size_variation);
        p.decimal("Climate variation", "How much each new world is randomly warmer, colder, wetter or drier.", t(&T::climate_shift), TL::climate_shift);

        p.header("Biomes in this world");

        for (const auto& biome : list.all()) {
            const std::string id = biome->id().str();
            p.custom_toggle(
                biome->name(),
                "Whether " + biome->name() + " can appear. Open its own page to change how common it is, its size, its climate and its blocks.",
                [g, id] { return g->terrain.biome(id).enabled; },
                [g, id](bool v) { g->terrain.edit_biome(id).enabled = v; g->terrain.tidy_biome(id); }
            ).transient();
        }

        p.button("Turn every biome on", "Let every biome appear again.", [g] {
            for (auto& kv : g->terrain.biomes) kv.second.enabled = true;
            tidy_all(*g);
        });

        p.button("Reset every biome", "Put every biome back to how it normally is.", [g] { g->terrain.biomes.clear(); });
    }

    static void biome_tab(SettingsPage& p, const Biome& biome, const std::vector<std::string>& palette) {
        using BL = BiomeOptionLimits;
        GameSettings* g = &p.live();
        const std::string id = biome.id().str();
        const SurfaceStyle look = biome.surface();
        auto get = [g, id]() -> const BiomeOptions& { return g->terrain.biome(id); };
        auto edit = [g, id]() -> BiomeOptions& { return g->terrain.edit_biome(id); };
        auto tidy = [g, id] { g->terrain.tidy_biome(id); };

        p.header("Biome");
        p.custom_toggle("Enabled", "Whether " + biome.name() + " can appear in this world.",
            [get] { return get().enabled; }, [edit, tidy](bool v) { edit().enabled = v; tidy(); })
         .defaults([edit, tidy] { edit().enabled = true; tidy(); }, [get] { return get().enabled; });

        p.when([get] { return get().enabled; });
        number(p, "How common", "How often it shows up. 1 is normal, 2 is about twice as much, 0 means it never appears.", BL::weight, BiomeOptions::DEFAULT_WEIGHT, get, edit, tidy, &BiomeOptions::weight).unit("x");
        number(p, "Size", "How big each patch of it is. Bigger patches also win where they overlap other biomes, so it covers more ground; lower How common to balance that.", BL::size, BiomeOptions::DEFAULT_SIZE, get, edit, tidy, &BiomeOptions::size).unit("x").logarithmic();

        p.header("Climate");
        number(p, "Warmer or colder", "Moves it toward warmer places (above 0) or colder places (below 0).", BL::shift, 0.0, get, edit, tidy, &BiomeOptions::temperature);
        number(p, "Wetter or drier", "Moves it toward wetter places (above 0) or drier places (below 0).", BL::shift, 0.0, get, edit, tidy, &BiomeOptions::humidity);

        p.header("Blocks");
        block(p, "Top block", "The block on the surface above the sea.", palette, look.top, get, edit, tidy, &BiomeOptions::top);
        block(p, "Filler block", "The blocks just under the surface.", palette, look.filler, get, edit, tidy, &BiomeOptions::filler);
        block(p, "Underwater block", "The ground under seas, lakes and rivers.", palette, look.underwater, get, edit, tidy, &BiomeOptions::underwater);
        block(p, "Cliff block", "The block on steep slopes.", palette, look.cliff, get, edit, tidy, &BiomeOptions::cliff);
        block(p, "Deep block", "The block that fills everything further down.", palette, look.deep, get, edit, tidy, &BiomeOptions::deep);

        const int depth = look.filler_depth;
        p.custom_integer("Filler depth", "How many blocks of filler sit under the top block.",
            [get, depth] { return static_cast<double>(get().filler_depth.value_or(depth)); },
            [edit, tidy, depth](double v) { const int d = static_cast<int>(v); if (d == depth) edit().filler_depth.reset(); else edit().filler_depth = d; tidy(); },
            BL::filler_depth
        ).unit("blocks").defaults([edit, tidy] { edit().filler_depth.reset(); tidy(); }, [get] { return !get().filler_depth; });

        const bool freezes = look.freezes;
        p.custom_toggle("Frozen water", "Turns the top of seas and rivers here into ice.",
            [get, freezes] { return get().freezes.value_or(freezes); },
            [edit, tidy, freezes](bool v) { if (v == freezes) edit().freezes.reset(); else edit().freezes = v; tidy(); }
        ).defaults([edit, tidy] { edit().freezes.reset(); tidy(); }, [get] { return !get().freezes; });

        const BiomeClimate climate = biome.climate();
        p.header("Weather");
        optional_number(p, "Average temperature", "The average temperature here at sea level, across a whole day and year.", BL::temperature, climate.temperature, get, edit, tidy, &BiomeOptions::mean_temperature).unit("C").decimals(1);
        optional_number(p, "Day and night swing", "How much warmer the afternoon is than the night. Deserts swing a lot.", BL::swing, climate.daily_swing, get, edit, tidy, &BiomeOptions::daily_swing).unit("C").decimals(1);
        optional_number(p, "Summer and winter swing", "How much warmer summer is than winter.", BL::swing, climate.season_swing, get, edit, tidy, &BiomeOptions::season_swing).unit("C").decimals(1);
        optional_number(p, "Rain amount", "How much rain or snow falls here. 1 is normal, 0 is completely dry, and above 1 brings heavier rain and extra showers.", BL::rainfall, climate.rainfall, get, edit, tidy, &BiomeOptions::rainfall).unit("x");
        optional_number(p, "Wave size", "How big waves get on water here. Oceans have 1, small lakes and rivers much less.", BL::waves, climate.waves, get, edit, tidy, &BiomeOptions::waves).unit("x");
        p.always();
    }

    template <typename Get, typename Edit, typename Tidy>
    static SettingsPage& optional_number(SettingsPage& p, const std::string& label, const std::string& description, Bounds limits, double normal,
                                         Get get, Edit edit, Tidy tidy, std::optional<double> BiomeOptions::*member) {
        return p.custom_decimal(label, description,
            [get, member, normal] { return (get().*member).value_or(normal); },
            [edit, tidy, member, normal](double v) { if (v == normal) (edit().*member).reset(); else edit().*member = v; tidy(); },
            limits
        ).defaults([edit, tidy, member] { (edit().*member).reset(); tidy(); }, [get, member] { return !(get().*member); });
    }

    template <typename Get, typename Edit, typename Tidy>
    static SettingsPage& number(SettingsPage& p, const std::string& label, const std::string& description, Bounds limits, double normal,
                                Get get, Edit edit, Tidy tidy, double BiomeOptions::*member) {
        return p.custom_decimal(label, description,
            [get, member] { return get().*member; },
            [edit, tidy, member](double v) { edit().*member = v; tidy(); },
            limits
        ).defaults([edit, tidy, member, normal] { edit().*member = normal; tidy(); }, [get, member, normal] { return get().*member == normal; });
    }

    template <typename Get, typename Edit, typename Tidy>
    static SettingsPage& block(SettingsPage& p, const std::string& label, const std::string& description, const std::vector<std::string>& palette,
                               Identifier normal, Get get, Edit edit, Tidy tidy, std::string BiomeOptions::*member) {
        const std::string fallback = normal.str();
        auto current = [get, member, fallback] { const std::string& v = get().*member; return v.empty() ? fallback : v; };

        auto options = [palette, current] {
            std::vector<std::string> list = palette;
            const std::string now = current();
            if (std::find(list.begin(), list.end(), now) == list.end()) list.push_back(now);
            return list;
        };

        return p.custom_choice(label, description, options,
            [options, current] {
                const std::vector<std::string> list = options();
                return static_cast<int>(std::find(list.begin(), list.end(), current()) - list.begin());
            },
            [options, edit, tidy, member, fallback](int i) {
                const std::vector<std::string> list = options();
                if (i < 0 || static_cast<std::size_t>(i) >= list.size()) return;
                edit().*member = list[static_cast<std::size_t>(i)] == fallback ? std::string() : list[static_cast<std::size_t>(i)];
                tidy();
            }
        ).defaults([edit, tidy, member] { (edit().*member).clear(); tidy(); }, [get, member] { return (get().*member).empty(); });
    }

    static std::vector<std::string> names(const BlockList& blocks) {
        std::vector<std::string> out;
        for (const Identifier& b : blocks) out.push_back(b.str());
        return out;
    }

    static void tidy_all(GameSettings& g) {
        for (auto it = g.terrain.biomes.begin(); it != g.terrain.biomes.end();) {
            if (it->second.is_default()) it = g.terrain.biomes.erase(it);
            else ++it;
        }
    }

    static void caves(SettingsPage& p) {
        using C  = CaveSettings;
        using CL = CaveLimits;
        GameSettings* g = &p.live();
        auto c = [](auto C::*m) { return field(&GameSettings::terrain, &TerrainSettings::caves, m); };
        auto on = [g] { return g->terrain.caves.enabled; };

        p.header("Caves");
        p.toggle("Caves", "Tunnels, caverns, entrances and canyons.", c(&C::enabled));

        p.when(on).header("Tunnels");
        p.decimal("How common", "How many winding tunnels there are. 0 turns tunnels off.", c(&C::tunnels), CL::amount).unit("x");
        p.decimal("Width", "How wide tunnels are.", c(&C::tunnel_width), CL::size).unit("x");
        p.decimal("Length", "How long and smooth tunnels run before turning.", c(&C::tunnel_length), CL::size).unit("x").logarithmic();
        p.decimal("Flatness", "Higher keeps tunnels more level, lower lets them climb and dive steeply.", c(&C::flatness), CL::flatness).unit("x");

        p.when(on).header("Caverns");
        p.decimal("How common", "How many big open caverns there are. 0 turns caverns off.", c(&C::caverns), CL::amount).unit("x");
        p.decimal("Size", "How big caverns are.", c(&C::cavern_size), CL::size).unit("x").logarithmic();
        p.integer("Cavern roof", "Solid ground always kept above caverns.", c(&C::cavern_roof), CL::cavern_roof).unit("blocks");

        p.when(on).header("Depth");
        p.integer("Start below surface", "Tunnels and caverns stay at least this deep. 0 lets them break through the ground. Entrances and canyons still reach the surface.", c(&C::min_depth), CL::depth).unit("blocks");
        p.integer("Deepest", "How far below the surface caves reach. 0 means all the way down to bedrock.", c(&C::max_depth), CL::depth).unit("blocks");
        p.decimal("Bigger deeper down", "How much wider tunnels and caverns get the deeper you go.", c(&C::deep_growth), CL::deep_growth).unit("x");

        p.when(on).header("Entrances");
        p.decimal("How common", "How many sloped cave mouths open at the surface. 0 turns them off.", c(&C::entrances), CL::amount).unit("x");
        p.integer("Depth", "About how deep each entrance goes.", c(&C::entrance_depth), CL::entrance_depth).unit("blocks");
        p.decimal("Width", "How wide entrances are.", c(&C::entrance_width), CL::size).unit("x");

        p.when(on).header("Canyons");
        p.decimal("How common", "How many long narrow canyons cut into the land. 0 turns them off.", c(&C::canyons), CL::amount).unit("x");
        p.integer("Depth", "How deep canyons cut.", c(&C::canyon_depth), CL::canyon_depth).unit("blocks");
        p.decimal("Width", "How wide canyons are.", c(&C::canyon_width), CL::size).unit("x");

        p.when(on).header("Water");
        p.toggle("Underwater caves", "Caves and canyons also form under seas and rivers, filled with water and open to the sea floor. Caves under land stay dry.", c(&C::underwater));
        p.always();
    }

    static void rivers(SettingsPage& p) {
        using T  = TerrainSettings;
        using TL = TerrainLimits;
        GameSettings* g = &p.live();
        auto t = [](auto T::*m) { return field(&GameSettings::terrain, m); };

        p.header("Rivers");
        p.toggle("Rivers", "Rivers wind across the land down to sea level.", t(&T::rivers));
        p.when([g] { return g->terrain.rivers; });
        p.decimal("River spacing", "How far apart rivers are. Higher means fewer, longer rivers.", t(&T::river_width), TL::river_width).unit("x");
        p.decimal("River depth", "How deep river beds are cut below the sea.", t(&T::river_depth), TL::river_depth).unit("blocks").decimals(1);
        p.always();
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_WORLD_PAGES_HPP