#ifndef VOXELSPIRE_WEATHER_PRECIPITATION_HPP
#define VOXELSPIRE_WEATHER_PRECIPITATION_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "../core/random.hpp"
#include "../render/greedy_mesher.hpp"
#include "../world/block_reader.hpp"
#include "system.hpp"

namespace voxelspire {

struct DropStyle {
    vector3d size;
    double   speed  = 10.0;
    double   spread = 0.1;
    double   sway   = 0.0;
    double   drift  = 1.0;
    double   share  = 1.0;
    double   above  = 26.0;
    double   shade  = 1.0;
    Color    color;
};

struct DropStyles {
    static constexpr std::size_t COUNT = 6;

    static std::array<DropStyle, COUNT> builtin();

    static std::size_t index(Precipitation p) noexcept { return static_cast<std::size_t>(p); }
};

class PrecipitationRenderer {
public:
    static constexpr double SPAWN_BELOW    = 8.0;
    static constexpr double SCAN_ABOVE     = 64.0;
    static constexpr double FILL_RATE      = 3.0;
    static constexpr double CACHE_SECONDS  = 1.5;
    static constexpr double ESCAPE_MARGIN  = 1.25;
    static constexpr int    ATTEMPTS       = 3;

    PrecipitationRenderer();

    std::array<DropStyle, DropStyles::COUNT>& styles() noexcept { return m_styles; }

    void rebuild_meshes() { for (std::size_t i = 0; i < DropStyles::COUNT; ++i) m_meshes[i] = box(m_styles[i]); }

    std::size_t count() const noexcept { return m_drops.size(); }

    void clear() noexcept { m_drops.clear(); m_ceilings.clear(); }

    void update(
        double dt, 
        double seconds, 
        const vector3d& eye, 
        const World& world, 
        const LocalWeather& weather,
        const WeatherViewSettings& view, 
        const vector3d& wind
    );

    void render(fizmo::windows::Renderer& renderer, double brightness);

private:
    static constexpr double HALF        = 0.5;
    static constexpr double SWAY_SPEED  = 1.7;
    static constexpr double SWAY_CROSS  = 0.6;
    static constexpr double RECENTER    = 256.0;
    static constexpr double NEAR_HIDE   = 1.6;
    static constexpr double CHANNEL     = 255.0;

    struct Drop {
        float        x = 0.0f, y = 0.0f, z = 0.0f;
        float        vx = 0.0f, vy = 0.0f, vz = 0.0f;
        float        phase = 0.0f;
        std::uint8_t style = 0;
    };

    struct Ceiling {
        int    top   = 0;
        double stamp = 0.0;
    };

    static fizmo::graphics::Mesh3D box(const DropStyle& style);

    void recenter(const vector3d& eye);

    double ceiling(BlockReader& reader, double x, double y, double eye_z);

    void prune();

    static constexpr std::size_t MAX_CACHED = 16384;

    std::array<DropStyle, DropStyles::COUNT>                             m_styles;
    std::array<fizmo::graphics::Mesh3D, DropStyles::COUNT>               m_meshes;
    std::array<std::vector<fizmo::graphics::Instance3D>, DropStyles::COUNT> m_lists;
    std::vector<Drop>                                                    m_drops;
    std::unordered_map<std::uint64_t, Ceiling>                           m_ceilings;
    SeededRandom                                                         m_rng;
    vector3d                                                             m_anchor{};
    vector3d                                                             m_eye{};
    double                                                               m_seconds = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_PRECIPITATION_HPP