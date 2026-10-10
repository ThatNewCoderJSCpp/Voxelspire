#ifndef VOXELSPIRE_WEATHER_CALENDAR_HPP
#define VOXELSPIRE_WEATHER_CALENDAR_HPP

#include <cmath>
#include <cstdint>
#include <string>
#include "../core/settings.hpp"

namespace voxelspire {

struct CalendarDate {
    bool         seasons         = true;
    Season       season          = Season::Spring;
    std::int64_t year            = 1;
    int          month           = 1;
    int          month_of_season = 1;
    int          day_of_month    = 1;
    double       year_fraction   = 0.0;
    double       season_progress = 0.0;
    double       warmth          = 0.0;

    std::string describe() const;
};

class Calendar {
public:
    static constexpr double SUMMER_PEAK = 0.375;
    static constexpr double FULL_TURN   = 2.0 * PI;

    static double warmth_of(double year_fraction) noexcept { return std::cos(FULL_TURN * (year_fraction - SUMMER_PEAK)); }

    static double middle_of(Season s) noexcept { return (static_cast<double>(s) + HALF) / Seasons::COUNT; }

    static CalendarDate at(std::int64_t day, double time_of_day, const SeasonSettings& s) noexcept;

private:
    static constexpr double HALF = 0.5;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_CALENDAR_HPP