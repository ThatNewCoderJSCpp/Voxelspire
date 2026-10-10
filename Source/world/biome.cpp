#include "world/biome.hpp"

namespace voxelspire {

double ClimateRange::signed_distance(double v, double grow) const noexcept {
    const double lo = min - grow, hi = max + grow;
    if (v < lo) return lo - v;
    if (v > hi) return v - hi;
    return -vmin(vmin(v - lo, hi - v), DEEPEST);
}

double BiomeNiche::score(const Climate& c, double grow) const noexcept {
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

double BiomeNiche::distance(const Climate& c) const noexcept {
    const double t = temperature.distance(c.temperature);
    const double h = humidity.distance(c.humidity);
    const double k = continentalness.distance(c.continentalness);
    const double e = erosion.distance(c.erosion);
    const double z = elevation.distance(c.elevation) * ELEVATION_WEIGHT;
    const double r = river.distance(c.river);
    return t * t + h * h + k * k + e * e + z * z + r * r;
}

Biome::Biome(Identifier id, std::string name, SurfaceStyle surface, std::vector<BiomeNiche> niches, BiomeClimate climate) : m_id(id), m_name(std::move(name)), m_surface(std::move(surface)), m_niches(std::move(niches)), m_climate(climate) {}

double Biome::fit(const Climate& c, double grow) const noexcept {
    double best = std::numeric_limits<double>::max();
    for (const BiomeNiche& n : m_niches) best = vmin(best, n.score(c, grow));
    return best;
}

std::shared_ptr<Biome> Biomes::make(const char* id, const char* name, SurfaceStyle s, std::vector<BiomeNiche> niches) {
    return std::make_shared<Biome>(core_id(Kind::Biome, id), name, std::move(s), std::move(niches), climate_of(id));
}

BiomeNiche Biomes::niche(R temperature, R humidity, R elevation) {
    BiomeNiche n;
    n.temperature = temperature;
    n.humidity    = humidity;
    n.elevation   = elevation;
    return n;
}

SurfaceStyle Biomes::surface(Identifier top, Identifier filler, Identifier underwater, Identifier cliff, int depth, bool freezes) {
    SurfaceStyle s;
    s.top          = top;
    s.filler       = filler;
    s.underwater   = underwater;
    s.cliff        = cliff;
    s.filler_depth = depth;
    s.freezes      = freezes;
    return s;
}

std::shared_ptr<Biome> Biomes::deep_ocean() {
    return make("deep_ocean", "Deep Ocean", surface(BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::GRAVEL), { niche(R::above(FREEZING), R::any(), R::below(DEEP_FLOOR)) });
}

std::shared_ptr<Biome> Biomes::ocean() {
    return make("ocean", "Ocean", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND), {
        niche(R::of(FREEZING, HOT_OCEAN), R::any(), R::of(DEEP_FLOOR, SHALLOW)),
        niche(R::of(FREEZING, COOL), R::any(), R::of(SHALLOW, SEA_FLOOR))
    });
}

std::shared_ptr<Biome> Biomes::shallow_sea() {
    return make("shallow_sea", "Shallow Sea", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND, BlockIds::SANDSTONE, 4), { niche(R::of(COOL, HOT_OCEAN), R::any(), R::of(SHALLOW, SEA_FLOOR)) });
}

std::shared_ptr<Biome> Biomes::warm_ocean() {
    return make("warm_ocean", "Warm Ocean", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND, BlockIds::SANDSTONE, 4), { niche(R::above(HOT_OCEAN), R::any(), R::of(DEEP_FLOOR, SEA_FLOOR)) });
}

std::shared_ptr<Biome> Biomes::frozen_ocean() {
    return make("frozen_ocean", "Frozen Ocean", surface(BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { niche(R::below(FREEZING), R::any(), R::below(SEA_FLOOR)) });
}

std::shared_ptr<Biome> Biomes::river() {
    BiomeNiche n = niche(R::above(FREEZING), R::any(), R::of(RIVER_BED, RIVER_TOP));
    n.river = R::above(BiomeNiche::RIVER_EDGE);
    return make("river", "River", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND), { n });
}

std::shared_ptr<Biome> Biomes::frozen_river() {
    BiomeNiche n = niche(R::below(FREEZING), R::any(), R::of(RIVER_BED, RIVER_TOP));
    n.river = R::above(BiomeNiche::RIVER_EDGE);
    return make("frozen_river", "Frozen River", surface(BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { n });
}

std::shared_ptr<Biome> Biomes::beach() {
    BiomeNiche n = niche(R::of(FREEZING, WARM), R::any(), R::of(SEA_FLOOR, SHORE));
    n.continentalness = R::of(OPEN_COAST, COAST);
    return make("beach", "Beach", surface(BlockIds::SAND, BlockIds::SAND, BlockIds::SAND, BlockIds::SANDSTONE, 4), { n });
}

std::shared_ptr<Biome> Biomes::snowy_beach() {
    BiomeNiche n = niche(R::below(FREEZING), R::any(), R::of(SEA_FLOOR, SHORE));
    n.continentalness = R::of(OPEN_COAST, COAST);
    return make("snowy_beach", "Snowy Beach", surface(BlockIds::SNOW, BlockIds::SAND, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { n });
}

std::shared_ptr<Biome> Biomes::plains() {
    return make("plains", "Plains", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::SAND), {
        niche(R::of(COOL, WARM), R::below(LUSH), R::of(0.0, LOWLAND)),
        niche(R::of(FREEZING, COOL), R::below(DRY), R::of(0.0, LOWLAND))
    });
}

std::shared_ptr<Biome> Biomes::forest() {
    return make("forest", "Forest", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::SAND), { niche(R::of(COOL, WARM), R::above(LUSH), R::of(0.0, LOWLAND)) });
}

std::shared_ptr<Biome> Biomes::swamp() {
    BiomeNiche n = niche(R::of(0.0, HOT_OCEAN), R::above(MARSH), R::of(SEA_FLOOR, MARSH_TOP));
    n.erosion = R::above(FLAT);
    return make("swamp", "Swamp", surface(BlockIds::MUD, BlockIds::MUD, BlockIds::MUD, BlockIds::STONE, 4), { n });
}

std::shared_ptr<Biome> Biomes::taiga() {
    return make("taiga", "Taiga", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::GRAVEL), { niche(R::of(FREEZING, COOL), R::above(DRY), R::of(0.0, HIGHLAND)) });
}

std::shared_ptr<Biome> Biomes::snowy_plains() {
    return make("snowy_plains", "Snowy Plains", surface(BlockIds::SNOW, BlockIds::DIRT, BlockIds::GRAVEL, BlockIds::STONE, 3, true), { niche(R::below(FREEZING), R::any(), R::of(0.0, HIGHLAND)) });
}

std::shared_ptr<Biome> Biomes::desert() {
    return make("desert", "Desert", surface(BlockIds::SAND, BlockIds::SANDSTONE, BlockIds::SAND, BlockIds::SANDSTONE, 5), { niche(R::above(WARM), R::of(ARID, DRY), R::of(0.0, LOWLAND)) });
}

std::shared_ptr<Biome> Biomes::badlands() {
    return make("badlands", "Badlands", surface(BlockIds::RED_SAND, BlockIds::SANDSTONE, BlockIds::RED_SAND, BlockIds::SANDSTONE, 5), { niche(R::above(WARM), R::below(ARID), R::of(0.0, HIGHLAND)) });
}

std::shared_ptr<Biome> Biomes::savanna() {
    return make("savanna", "Savanna", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::SAND), { niche(R::above(WARM), R::of(DRY, DAMP), R::of(0.0, LOWLAND)) });
}

std::shared_ptr<Biome> Biomes::jungle() {
    return make("jungle", "Jungle", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::CLAY), { niche(R::above(WARM), R::above(DAMP), R::of(0.0, LOWLAND)) });
}

std::shared_ptr<Biome> Biomes::hills() {
    return make("hills", "Windswept Hills", surface(BlockIds::GRASS, BlockIds::DIRT, BlockIds::GRAVEL, BlockIds::STONE, 2), { niche(R::above(FREEZING), R::any(), R::of(LOWLAND, HIGHLAND)) });
}

std::shared_ptr<Biome> Biomes::stony_peaks() {
    return make("stony_peaks", "Stony Peaks", surface(BlockIds::STONE, BlockIds::STONE, BlockIds::GRAVEL, BlockIds::STONE, 1), { niche(R::above(COOL), R::any(), R::above(HIGHLAND)) });
}

std::shared_ptr<Biome> Biomes::snowy_peaks() {
    return make("snowy_peaks", "Snowy Peaks", surface(BlockIds::SNOW, BlockIds::STONE, BlockIds::GRAVEL, BlockIds::STONE, 1, true), { niche(R::below(COOL), R::any(), R::above(HIGHLAND)) });
}

BiomeRegistry BiomeRegistry::builtin() {
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

BiomeId BiomeRegistry::add(std::shared_ptr<const Biome> biome, BiomeTuning tuning) {
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

bool BiomeRegistry::remove(Identifier id) {
    const int at = index_of(id);
    if (at == NOT_FOUND || m_biomes.size() <= 1) return false;
    m_biomes.erase(m_biomes.begin() + at);
    m_tuning.erase(m_tuning.begin() + at);
    refresh_scales();
    return true;
}

void BiomeRegistry::set_tuning(Identifier id, BiomeTuning tuning) {
    const int at = index_of(id);
    if (at == NOT_FOUND) return;
    tuning.weight = vmax(tuning.weight, 0.0);
    tuning.size   = vmax(tuning.size, MIN_SIZE);
    m_tuning[static_cast<std::size_t>(at)] = tuning;
    refresh_scales();
}

void BiomeRegistry::set_weight(Identifier id, double weight) {
    const int at = index_of(id);
    if (at == NOT_FOUND) return;
    BiomeTuning t = m_tuning[static_cast<std::size_t>(at)];
    t.weight = weight;
    set_tuning(id, t);
}

int BiomeRegistry::index_of(Identifier id) const noexcept {
    for (std::size_t i = 0; i < m_biomes.size(); ++i) if (m_biomes[i]->id() == id) return static_cast<int>(i);
    return NOT_FOUND;
}

const Biome* BiomeRegistry::find(Identifier id) const noexcept {
    const int at = index_of(id);
    return at == NOT_FOUND ? nullptr : m_biomes[static_cast<std::size_t>(at)].get();
}

BiomeId BiomeRegistry::select(const Climate* by_scale) const noexcept {
    return pick([this, by_scale](std::size_t i) -> const Climate& { return by_scale[m_scale_slot[i]]; });
}

void BiomeRegistry::refresh_scales() {
    m_scales.assign(1, BiomeTuning::DEFAULT_SIZE);
    m_scale_slot.assign(m_tuning.size(), 0);

    for (std::size_t i = 0; i < m_tuning.size(); ++i) {
        const double size = m_tuning[i].size;
        auto it = std::find(m_scales.begin(), m_scales.end(), size);
        if (it == m_scales.end()) it = m_scales.insert(m_scales.end(), size);
        m_scale_slot[i] = static_cast<std::size_t>(it - m_scales.begin());
    }
}

} // namespace voxelspire
