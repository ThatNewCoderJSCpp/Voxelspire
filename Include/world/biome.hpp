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

    double signed_distance(double v, double grow) const noexcept;
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

    double score(const Climate& c, double grow) const noexcept;

    double distance(const Climate& c) const noexcept;
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
;

    virtual ~Biome() = default;

    Identifier                     id()      const noexcept { return m_id; }
    const std::string&             name()    const noexcept { return m_name; }
    const SurfaceStyle&            surface() const noexcept { return m_surface; }
    const std::vector<BiomeNiche>& niches()  const noexcept { return m_niches; }
    const BiomeClimate&            climate() const noexcept { return m_climate; }

    virtual double fit(const Climate& c, double grow = 0.0) const noexcept;

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

    static std::shared_ptr<Biome> make(const char* id, const char* name, SurfaceStyle s, std::vector<BiomeNiche> niches);

    static BiomeNiche niche(R temperature, R humidity, R elevation);

    static SurfaceStyle surface(Identifier top, Identifier filler, Identifier underwater, Identifier cliff = BlockIds::STONE, int depth = 3, bool freezes = false);

    static std::shared_ptr<Biome> deep_ocean();

    static std::shared_ptr<Biome> ocean();

    static std::shared_ptr<Biome> shallow_sea();

    static std::shared_ptr<Biome> warm_ocean();

    static std::shared_ptr<Biome> frozen_ocean();

    static std::shared_ptr<Biome> river();

    static std::shared_ptr<Biome> frozen_river();

    static std::shared_ptr<Biome> beach();

    static std::shared_ptr<Biome> snowy_beach();

    static std::shared_ptr<Biome> plains();

    static std::shared_ptr<Biome> forest();

    static std::shared_ptr<Biome> swamp();

    static std::shared_ptr<Biome> taiga();

    static std::shared_ptr<Biome> snowy_plains();

    static std::shared_ptr<Biome> desert();

    static std::shared_ptr<Biome> badlands();

    static std::shared_ptr<Biome> savanna();

    static std::shared_ptr<Biome> jungle();

    static std::shared_ptr<Biome> hills();

    static std::shared_ptr<Biome> stony_peaks();

    static std::shared_ptr<Biome> snowy_peaks();
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

    static BiomeRegistry builtin();

    BiomeId add(std::shared_ptr<const Biome> biome, BiomeTuning tuning = BiomeTuning{});

    bool remove(Identifier id);

    void set_tuning(Identifier id, BiomeTuning tuning);

    void set_weight(Identifier id, double weight);

    double             weight(BiomeId id) const noexcept { return id < m_tuning.size() ? m_tuning[id].weight : 0.0; }
    const BiomeTuning& tuning(BiomeId id) const noexcept { return m_tuning[vmin<std::size_t>(id, m_tuning.size() - 1)]; }

    int index_of(Identifier id) const noexcept;

    const Biome* find(Identifier id) const noexcept;

    const Biome& at(BiomeId id) const noexcept { return *m_biomes[vmin<std::size_t>(id, m_biomes.size() - 1)]; }

    const std::vector<double>& scales() const noexcept { return m_scales; }

    BiomeId select(const Climate& c) const noexcept { return pick([&c](std::size_t) -> const Climate& { return c; }); }

    BiomeId select(const Climate* by_scale) const noexcept;

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

    void refresh_scales();

    std::vector<std::shared_ptr<const Biome>> m_biomes;
    std::vector<BiomeTuning>                  m_tuning;
    std::vector<double>                       m_scales{ BiomeTuning::DEFAULT_SIZE };
    std::vector<std::size_t>                  m_scale_slot;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_BIOME_HPP