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

    std::string describe() const {
        std::string out = seasons ? std::string(Seasons::name(season)) + ", " : std::string();
        return out + "month " + std::to_string(month) + " day " + std::to_string(day_of_month) + ", year " + std::to_string(year);
    }
};

class Calendar {
public:
    static constexpr double SUMMER_PEAK = 0.375;
    static constexpr double FULL_TURN   = 2.0 * PI;

    static double warmth_of(double year_fraction) noexcept { return std::cos(FULL_TURN * (year_fraction - SUMMER_PEAK)); }

    static double middle_of(Season s) noexcept { return (static_cast<double>(s) + HALF) / Seasons::COUNT; }

    static CalendarDate at(std::int64_t day, double time_of_day, const SeasonSettings& s) noexcept {
        CalendarDate d;
        const int per_month  = vmax(s.days_per_month, 1);
        const int per_season = per_month * vmax(s.months_per_season, 1);
        const int per_year   = per_season * Seasons::COUNT;
        const double total   = static_cast<double>(day) + time_of_day + static_cast<double>(s.start_season) * per_season;
        const double years   = std::floor(total / per_year);
        const double into    = total - years * per_year;
        const int    whole   = vclamp(static_cast<int>(std::floor(into)), 0, per_year - 1);

        d.year            = static_cast<std::int64_t>(years) + 1;
        d.month           = whole / per_month + 1;
        d.day_of_month    = whole % per_month + 1;
        d.month_of_season = (whole % per_season) / per_month + 1;
        d.year_fraction   = into / per_year;
        d.season          = static_cast<Season>(vclamp(whole / per_season, 0, Seasons::COUNT - 1));
        d.season_progress = (into - static_cast<double>(static_cast<int>(d.season)) * per_season) / per_season;

        switch (s.mode) {
            case SeasonMode::Cycle:
                d.warmth = warmth_of(d.year_fraction);
                break;
            case SeasonMode::Fixed:
                d.season = s.fixed_season;
                d.warmth = warmth_of(middle_of(s.fixed_season));
                d.season_progress = HALF;
                break;
            case SeasonMode::Off:
                d.seasons = false;
                d.warmth  = 0.0;
                break;
        }

        return d;
    }

private:
    static constexpr double HALF = 0.5;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_CALENDAR_HPP