#ifndef VOXELSPIRE_WORLD_BIOME_HPP
#define VOXELSPIRE_WORLD_BIOME_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "../block/block_ids.hpp"

namespace voxelspire {

using BiomeId = std::uint16_t;

struct ClimateRange {
    static constexpr double OPEN = std::numeric_limits<double>::infinity();

    double min = -OPEN;
    double max =  OPEN;

    static constexpr ClimateRange any() noexcept { return {}; }
    static constexpr ClimateRange of(double lo, double hi) noexcept { return { lo, hi }; }
    static constexpr ClimateRange below(double hi) noexcept { return { -OPEN, hi }; }
    static constexpr ClimateRange above(double lo) noexcept { return { lo, OPEN }; }

    static constexpr double DEEPEST = 10.0;

    constexpr double distance(double v) const noexcept { return v < min ? min - v : (v > max ? v - max : 0.0); }

    double signed_distance(double v, double grow) const noexcept {
        const double lo = min - grow, hi = max + grow;
        if (v < lo) return lo - v;
        if (v > hi) return v - hi;
        return -vmin(vmin(v - lo, hi - v), DEEPEST);
    }
};

struct Climate {
    double temperature     = 0.0;
    double humidity        = 0.0;
    double continentalness = 0.0;
    double erosion         = 0.0;
    double weirdness       = 0.0;
    double river           = 0.0;
    double elevation       = 0.0;
};

struct BiomeNiche {
    ClimateRange temperature;
    ClimateRange humidity;
    ClimateRange continentalness;
    ClimateRange erosion;
    ClimateRange elevation;
    ClimateRange river = ClimateRange::below(RIVER_EDGE);

    static constexpr double RIVER_EDGE      = 0.55;
    static constexpr double ELEVATION_WEIGHT = 2.0;

    double score(const Climate& c, double grow) const noexcept {
        const double d[] = {
            temperature.signed_distance(c.temperature, grow),
            humidity.signed_distance(c.humidity, grow),
            continentalness.signed_distance(c.continentalness, grow),
            erosion.signed_distance(c.erosion, grow),
            elevation.signed_distance(c.elevation, grow / ELEVATION_WEIGHT) * ELEVATION_WEIGHT,
            river.signed_distance(c.river, grow)
        };

        double outside = 0.0, nearest_edge = -ClimateRange::DEEPEST;

        for (double v : d) {
            if (v > 0.0) outside += v * v;
            else nearest_edge = vmax(nearest_edge, v);
        }

        return outside > 0.0 ? outside : nearest_edge;
    }

    double distance(const Climate& c) const noexcept {
        const double t = temperature.distance(c.temperature);
        const double h = humidity.distance(c.humidity);
        const double k = continentalness.distance(c.continentalness);
        const double e = erosion.distance(c.erosion);
        const double z = elevation.distance(c.elevation) * ELEVATION_WEIGHT;
        const double r = river.distance(c.river);
        return t * t + h * h + k * k + e * e + z * z + r * r;
    }
};

struct SurfaceStyle {
    Identifier top          = BlockIds::GRASS;
    Identifier filler       = BlockIds::DIRT;
    Identifier underwater   = BlockIds::SAND;
    Identifier cliff        = BlockIds::STONE;
    Identifier deep         = BlockIds::STONE;
    int        filler_depth = 3;
    bool       freezes      = false;
};

struct BiomeClimate {
    double temperature  = 14.0;
    double daily_swing  = 10.0;
    double season_swing = 18.0;
    double rainfall     = 1.0;
    double waves        = 0.35;
    Color  water        = Color(46, 96, 205);
};

class Biome {
public:
    Biome(Identifier id, std::string name, SurfaceStyle surface, std::vector<BiomeNiche> niches, BiomeClimate climate = BiomeClimate{})
        : m_id(id), m_name(std::move(name)), m_surface(std::move(surface)), m_niches(std::move(niches)), m_climate(climate) {}

    virtual ~Biome() = default;

    Identifier                     id()      const noexcept { return m_id; }
    const std::string&             name()    const noexcept { return m_name; }
    const SurfaceStyle&            surface() const noexcept { return m_surface; }
    const std::vector<BiomeNiche>& niches()  const noexcept { return m_niches; }
    const BiomeClimate&            climate() const noexcept { return m_climate; }

    virtual double fit(const Climate& c, double grow = 0.0) const noexcept {
        double best = std::numeric_limits<double>::max();
        for (const BiomeNiche& n : m_niches) best = vmin(best, n.score(c, grow));
        return best;
    }

private:
    Identifier              m_id;
    std::string             m_name;
    SurfaceStyle            m_surface;
    std::vector<BiomeNiche> m_niches;
    BiomeClimate            m_climate;
};

struct Biomes {
    static constexpr double FREEZING   = -0.55;
    static constexpr double COOL       = -0.25;
    static constexpr double WARM       =  0.35;
    static constexpr double HOT_OCEAN  =  0.55;
    static constexpr double SEA_FLOOR  = -0.02;
    static constexpr double DEEP_FLOOR = -0.35;
    static constexpr double SHALLOW    = -0.13;
    static constexpr double SHORE      =  0.07;
    static constexpr double LOWLAND    =  0.7;
    static constexpr double HIGHLAND   =  1.25;
    static constexpr double COAST      = -0.15;
    static constexpr double OPEN_COAST = -0.35;
    static constexpr double DRY        = -0.1;
    static constexpr double LUSH       =  0.1;
    static constexpr double ARID       = -0.5;
    static constexpr double DAMP       =  0.35;
    static constexpr double MARSH      =  0.45;
    static constexpr double MARSH_TOP  =  0.2;
    static constexpr double FLAT       =  0.1;
    static constexpr double RIVER_BED  = -0.2;
    static constexpr double RIVER_TOP  =  0.04;

    using R = ClimateRange;

    struct ClimateEntry {
        const char*  id;
        BiomeClimate climate;
    };

    static constexpr std::array<ClimateEntry, 21> CLIMATES{ {
        { "shallow_sea",  {  18.0,  4.0, 10.0, 1.00, 0.70, Color( 38, 140, 210) } },
        { "deep_ocean",   {  12.0,  3.0,  6.0, 1.00, 1.00, Color( 28,  62, 170) } },
        { "ocean",        {  15.0,  3.0,  8.0, 1.00, 1.00, Color( 40,  92, 200) } },
        { "warm_ocean",   {  25.0,  3.0,  4.0, 1.10, 1.00, Color( 34, 150, 200) } },
        { "frozen_ocean", {  -6.0,  4.0, 12.0, 0.70, 0.60, Color( 58,  88, 160) } },
        { "river",        {  12.0,  9.0, 22.0, 1.00, 0.25, Color( 52, 110, 200) } },
        { "frozen_river", {  -5.0,  9.0, 18.0, 0.70, 0.15, Color( 60,  92, 165) } },
        { "beach",        {  16.0,  8.0, 16.0, 0.90, 0.80, Color( 44, 120, 205) } },
        { "snowy_beach",  {  -3.0,  8.0, 14.0, 0.70, 0.60, Color( 58,  92, 165) } },
        { "plains",       {  12.0, 12.0, 26.0, 1.00, 0.35, Color( 46,  96, 205) } },
        { "forest",       {  11.0, 10.0, 26.0, 1.20, 0.35, Color( 44,  92, 195) } },
        { "swamp",        {  20.0,  8.0, 14.0, 1.60, 0.10, Color( 64,  92,  70) } },
        { "taiga",        {   2.0, 12.0, 30.0, 1.00, 0.35, Color( 44,  86, 185) } },
        { "snowy_plains", {  -9.0, 12.0, 22.0, 0.70, 0.35, Color( 56,  88, 165) } },
        { "desert",       {  26.0, 20.0, 16.0, 0.08, 0.35, Color( 40, 120, 200) } },
        { "badlands",     {  25.0, 24.0, 16.0, 0.20, 0.35, Color( 60, 100, 180) } },
        { "savanna",      {  26.0, 16.0,  6.0, 0.60, 0.35, Color( 44, 118, 200) } },
        { "jungle",       {  27.0,  8.0,  3.0, 2.00, 0.35, Color( 36, 130, 170) } },
        { "hills",        {  11.0, 12.0, 24.0, 1.10, 0.35, Color( 46,  96, 205) } },
        { "stony_peaks",  {  10.0, 14.0, 18.0, 1.00, 0.35, Color( 46,  96, 205) } },
        { "snowy_peaks",  {   0.0, 14.0, 18.0, 0.90, 0.35, Color( 58,  88, 165) } }
    } };

    static BiomeClimate climate_of(std::string_view id) noexcept {
        for (const ClimateEntry& e : CLIMATES) if (id == e.id) return e.climate;
        return BiomeClimate{};
    }

    static std::shared_ptr<Biome> make(const char* id, const char* name, SurfaceStyle s, std::vector<BiomeNiche> niches) {
        return std::make_shared<Biome>(core_id(Kind::Biome, id), name, std::move(s), std::move(niches), climate_of(id));
    }

    static BiomeNiche niche(R temperature, R humidity, R elevation) {
        BiomeNiche n;
        n.temperature = temperature;
        n.humidity    = humidity;
        n.elevation   = elevation;
        return n;
    }

    static SurfaceStyle surface(Identifier top, Identifier filler, Identifier underwater, Identifier cliff = BlockIds::STONE, int depth = 3, bool freezes = false) {
        SurfaceStyle s;
        s.top          = top;
        s.filler       = filler;
        s.underwater   = underwater;
        s.cliff        = cliff;
        s.filler_depth = depth;
        s.freezes      = freezes;
        return s;
    }

    static std::shared_ptr<Biome> deep_ocean() {
        return make("deep_ocean", "Deep Ocean", surface(BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::GRAVEL), { niche(R::above(FREEZING), R::any(), R::below(DEEP_FLOOR)) });
    }

    static std::shared_ptr<Biome> ocean() {
        return make("ocean", "Ocean", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND), {
            niche(R::of(FREEZING, HOT_OCEAN), R::any(), R::of(DEEP_FLOOR, SHALLOW)),
            niche(R::of(FREEZING, COOL), R::any(), R::of(SHALLOW, SEA_FLOOR))
        });
    }

    static std::shared_ptr<Biome> shallow_sea() {
        return make("shallow_sea", "Shallow Sea", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND, BlockIds::SANDSTONE, 4), { niche(R::of(COOL, HOT_OCEAN), R::any(), R::of(SHALLOW, SEA_FLOOR)) });
    }

    static std::shared_ptr<Biome> warm_ocean() {
        return make("warm_ocean", "Warm Ocean", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND, BlockIds::SANDSTONE, 4), { niche(R::above(HOT_OCEAN), R::any(), R::of(DEEP_FLOOR, SEA_FLOOR)) });
    }

    static std::shared_ptr<Biome> frozen_ocean() {
        return make("frozen_ocean", "Frozen Ocean", surface(BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { niche(R::below(FREEZING), R::any(), R::below(SEA_FLOOR)) });
    }

    static std::shared_ptr<Biome> river() {
        BiomeNiche n = niche(R::above(FREEZING), R::any(), R::of(RIVER_BED, RIVER_TOP));
        n.river = R::above(BiomeNiche::RIVER_EDGE);
        return make("river", "River", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND), { n });
    }

    static std::shared_ptr<Biome> frozen_river() {
        BiomeNiche n = niche(R::below(FREEZING), R::any(), R::of(RIVER_BED, RIVER_TOP));
        n.river = R::above(BiomeNiche::RIVER_EDGE);
        return make("frozen_river", "Frozen River", surface(BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { n });
    }

    static std::shared_ptr<Biome> beach() {
        BiomeNiche n = niche(R::of(FREEZING, WARM), R::any(), R::of(SEA_FLOOR, SHORE));
        n.continentalness = R::of(OPEN_COAST, COAST);
        return make("beach", "Beach", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND, BlockIds::SANDSTONE, 4), { n });
    }

    static std::shared_ptr<Biome> snowy_beach() {
        BiomeNiche n = niche(R::below(FREEZING), R::any(), R::of(SEA_FLOOR, SHORE));
        n.continentalness = R::of(OPEN_COAST, COAST);
        return make("snowy_beach", "Snowy Beach", surface(BlockIds::SNOW, BlockIds::SAND, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { n });
    }

    static std::shared_ptr<Biome> plains() {
        return make("plains", "Plains", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::SAND), {
            niche(R::of(COOL, WARM), R::below(LUSH), R::of(0.0, LOWLAND)),
            niche(R::of(FREEZING, COOL), R::below(DRY), R::of(0.0, LOWLAND))
        });
    }

    static std::shared_ptr<Biome> forest() {
        return make("forest", "Forest", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::SAND), { niche(R::of(COOL, WARM), R::above(LUSH), R::of(0.0, LOWLAND)) });
    }

    static std::shared_ptr<Biome> swamp() {
        BiomeNiche n = niche(R::of(0.0, HOT_OCEAN), R::above(MARSH), R::of(SEA_FLOOR, MARSH_TOP));
        n.erosion = R::above(FLAT);
        return make("swamp", "Swamp", surface(BlockIds::MUD, BlockIds::MUD, BlockIds::MUD, BlockIds::STONE, 4), { n });
    }

    static std::shared_ptr<Biome> taiga() {
        return make("taiga", "Taiga", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::GRAVEL), { niche(R::of(FREEZING, COOL), R::above(DRY), R::of(0.0, HIGHLAND)) });
    }

    static std::shared_ptr<Biome> snowy_plains() {
        return make("snowy_plains", "Snowy Plains", surface(BlockIds::SNOW, BlockIds::DIRT, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { niche(R::below(FREEZING), R::any(), R::of(0.0, HIGHLAND)) });
    }

    static std::shared_ptr<Biome> desert() {
        return make("desert", "Desert", surface(BlockIds::SAND, BlockIds::SANDSTONE, BlockIds::SAND, BlockIds::SANDSTONE, 5), { niche(R::above(WARM), R::of(ARID, DRY), R::of(0.0, LOWLAND)) });
    }

    static std::shared_ptr<Biome> badlands() {
        return make("badlands", "Badlands", surface(BlockIds::RED_SAND, BlockIds::SANDSTONE, BlockIds::RED_SAND, BlockIds::SANDSTONE, 5), { niche(R::above(WARM), R::below(ARID), R::of(0.0, HIGHLAND)) });
    }

    static std::shared_ptr<Biome> savanna() {
        return make("savanna", "Savanna", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::SAND), { niche(R::above(WARM), R::of(DRY, DAMP), R::of(0.0, LOWLAND)) });
    }

    static std::shared_ptr<Biome> jungle() {
        return make("jungle", "Jungle", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::CLAY), { niche(R::above(WARM), R::above(DAMP), R::of(0.0, LOWLAND)) });
    }

    static std::shared_ptr<Biome> hills() {
        return make("hills", "Windswept Hills", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::GRAVEL, BlockIds::STONE, 2), { niche(R::above(FREEZING), R::any(), R::of(LOWLAND, HIGHLAND)) });
    }

    static std::shared_ptr<Biome> stony_peaks() {
        return make("stony_peaks", "Stony Peaks", surface(BlockIds::STONE, BlockIds::STONE, BlockIds::GRAVEL, BlockIds::STONE, 1), { niche(R::above(COOL), R::any(), R::above(HIGHLAND)) });
    }

    static std::shared_ptr<Biome> snowy_peaks() {
        return make("snowy_peaks", "Snowy Peaks", surface(BlockIds::SNOW, BlockIds::STONE, BlockIds::GRAVEL, BlockIds::STONE, 1, true), { niche(R::below(COOL), R::any(), R::above(HIGHLAND)) });
    }
};

struct BiomeTuning {
    static constexpr double DEFAULT_WEIGHT = 1.0;
    static constexpr double DEFAULT_SIZE   = 1.0;

    double weight      = DEFAULT_WEIGHT;
    double size        = DEFAULT_SIZE;
    double temperature = 0.0;
    double humidity    = 0.0;
};

class BiomeRegistry {
public:
    static constexpr int    NOT_FOUND      = -1;
    static constexpr double DEFAULT_WEIGHT = BiomeTuning::DEFAULT_WEIGHT;
    static constexpr double WEIGHT_REACH   = 0.12;
    static constexpr double MIN_SIZE       = 0.05;

    static BiomeRegistry builtin() {
        BiomeRegistry r;

        for (const auto& b : {
            Biomes::deep_ocean(), Biomes::ocean(), Biomes::shallow_sea(), Biomes::warm_ocean(), Biomes::frozen_ocean(),
            Biomes::river(), Biomes::frozen_river(), Biomes::beach(), Biomes::snowy_beach(),
            Biomes::plains(), Biomes::forest(), Biomes::swamp(), Biomes::taiga(), Biomes::snowy_plains(),
            Biomes::desert(), Biomes::badlands(), Biomes::savanna(), Biomes::jungle(),
            Biomes::hills(), Biomes::stony_peaks(), Biomes::snowy_peaks()
        }) r.add(b);

        return r;
    }

    BiomeId add(std::shared_ptr<const Biome> biome, BiomeTuning tuning = BiomeTuning{}) {
        const int at = index_of(biome->id());

        if (at != NOT_FOUND) {
            m_biomes[static_cast<std::size_t>(at)] = std::move(biome);
            m_tuning[static_cast<std::size_t>(at)] = tuning;
            refresh_scales();
            return static_cast<BiomeId>(at);
        }

        m_biomes.push_back(std::move(biome));
        m_tuning.push_back(tuning);
        refresh_scales();
        return static_cast<BiomeId>(m_biomes.size() - 1);
    }

    bool remove(Identifier id) {
        const int at = index_of(id);
        if (at == NOT_FOUND || m_biomes.size() <= 1) return false;
        m_biomes.erase(m_biomes.begin() + at);
        m_tuning.erase(m_tuning.begin() + at);
        refresh_scales();
        return true;
    }

    void set_tuning(Identifier id, BiomeTuning tuning) {
        const int at = index_of(id);
        if (at == NOT_FOUND) return;
        tuning.weight = vmax(tuning.weight, 0.0);
        tuning.size   = vmax(tuning.size, MIN_SIZE);
        m_tuning[static_cast<std::size_t>(at)] = tuning;
        refresh_scales();
    }

    void set_weight(Identifier id, double weight) {
        const int at = index_of(id);
        if (at == NOT_FOUND) return;
        BiomeTuning t = m_tuning[static_cast<std::size_t>(at)];
        t.weight = weight;
        set_tuning(id, t);
    }

    double             weight(BiomeId id) const noexcept { return id < m_tuning.size() ? m_tuning[id].weight : 0.0; }
    const BiomeTuning& tuning(BiomeId id) const noexcept { return m_tuning[vmin<std::size_t>(id, m_tuning.size() - 1)]; }

    int index_of(Identifier id) const noexcept {
        for (std::size_t i = 0; i < m_biomes.size(); ++i) if (m_biomes[i]->id() == id) return static_cast<int>(i);
        return NOT_FOUND;
    }

    const Biome* find(Identifier id) const noexcept {
        const int at = index_of(id);
        return at == NOT_FOUND ? nullptr : m_biomes[static_cast<std::size_t>(at)].get();
    }

    const Biome& at(BiomeId id) const noexcept { return *m_biomes[vmin<std::size_t>(id, m_biomes.size() - 1)]; }

    const std::vector<double>& scales() const noexcept { return m_scales; }

    BiomeId select(const Climate& c) const noexcept { return pick([&c](std::size_t) -> const Climate& { return c; }); }

    BiomeId select(const Climate* by_scale) const noexcept {
        return pick([this, by_scale](std::size_t i) -> const Climate& { return by_scale[m_scale_slot[i]]; });
    }

    std::size_t size() const noexcept { return m_biomes.size(); }
    const std::vector<std::shared_ptr<const Biome>>& all() const noexcept { return m_biomes; }

private:
    template <typename ClimateOf>
    BiomeId pick(ClimateOf&& climate_of) const noexcept {
        BiomeId best = 0;
        double best_fit = std::numeric_limits<double>::max();
        double best_size = 0.0;
        const bool any = std::any_of(m_tuning.begin(), m_tuning.end(), [](const BiomeTuning& t) { return t.weight > 0.0; });

        for (std::size_t i = 0; i < m_biomes.size(); ++i) {
            const BiomeTuning& t = m_tuning[i];
            if (any && t.weight <= 0.0) continue;
            Climate c = climate_of(i);
            c.temperature -= t.temperature;
            c.humidity    -= t.humidity;
            const double fit = m_biomes[i]->fit(c, t.weight > 0.0 ? WEIGHT_REACH * std::log2(t.weight) : 0.0);
            const double size = fit < 0.0 ? t.size : 0.0;
            const bool better = size != best_size ? size > best_size : fit < best_fit;
            if (better) { best_fit = fit; best_size = size; best = static_cast<BiomeId>(i); }
        }

        return best;
    }

    void refresh_scales() {
        m_scales.assign(1, BiomeTuning::DEFAULT_SIZE);
        m_scale_slot.assign(m_tuning.size(), 0);

        for (std::size_t i = 0; i < m_tuning.size(); ++i) {
            const double size = m_tuning[i].size;
            auto it = std::find(m_scales.begin(), m_scales.end(), size);
            if (it == m_scales.end()) it = m_scales.insert(m_scales.end(), size);
            m_scale_slot[i] = static_cast<std::size_t>(it - m_scales.begin());
        }
    }

    std::vector<std::shared_ptr<const Biome>> m_biomes;
    std::vector<BiomeTuning>                  m_tuning;
    std::vector<double>                       m_scales{ BiomeTuning::DEFAULT_SIZE };
    std::vector<std::size_t>                  m_scale_slot;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_BIOME_HPP