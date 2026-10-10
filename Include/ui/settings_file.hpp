#ifndef VOXELSPIRE_UI_SETTINGS_FILE_HPP
#define VOXELSPIRE_UI_SETTINGS_FILE_HPP

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include "color_math.hpp"
#include "settings_model.hpp"

namespace voxelspire {

struct SettingsLoad {
    bool                     found   = false;
    std::uint32_t            applied = Apply::Nothing;
    std::vector<std::string> problems;
};

class SettingsFile {
public:
    static constexpr const char* KEY_SEPARATOR = " | ";
    static constexpr const char* UNBOUND       = "none";
    static constexpr int         CHANNELS      = 4;
    static constexpr const char* TEMP_SUFFIX   = ".tmp";

    using Values = std::unordered_map<std::string, std::string>;

    static bool save(const std::vector<SettingsTab>& tabs, const std::string& path);

    static void write(const std::vector<SettingsTab>& tabs, std::ostream& out);

    static Values read(std::istream& in);

    static SettingsLoad load(std::vector<SettingsTab>& tabs, const std::string& path);

    static SettingsLoad apply(std::vector<SettingsTab>& tabs, const Values& values);

private:
    static std::string value_of(const SettingControl& c);

    static bool assign(SettingControl& c, const std::string& text, std::string& problem);

    static bool assign_color(SettingControl& c, const std::string& text, std::string& problem);
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_FILE_HPP