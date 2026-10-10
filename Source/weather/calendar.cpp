#include "weather/calendar.hpp"

namespace voxelspire {

std::string CalendarDate::describe() const {
    std::string out = seasons ? std::string(Seasons::name(season)) + ", " : std::string();
    return out + "month " + std::to_string(month) + " day " + std::to_string(day_of_month) + ", year " + std::to_string(year);
}

CalendarDate Calendar::at(std::int64_t day, double time_of_day, const SeasonSettings& s) noexcept {
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

} // namespace voxelspire
