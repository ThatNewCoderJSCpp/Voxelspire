#ifndef VOXELSPIRE_UI_TEXT_FIELD_HPP
#define VOXELSPIRE_UI_TEXT_FIELD_HPP

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

namespace voxelspire {

enum class TextFilter : std::uint8_t { Integer = 0, Decimal, Hex, Text };

enum class TextResult : std::uint8_t { Ignored = 0, Edited, Blocked, Commit, Cancel, Next, Previous };

class TextField {
public:
    static constexpr std::size_t DEFAULT_LENGTH = 16;
    static constexpr char        MINUS          = '-';
    static constexpr char        DOT            = '.';
    static constexpr char        HASH           = '#';
    static constexpr char        FIRST_PRINTABLE = ' ';
    static constexpr char        LAST_PRINTABLE  = '~';
    static constexpr const char* SPACE_KEY       = "Space";

    void begin(std::string text, TextFilter filter, bool allow_negative, std::size_t max_length = DEFAULT_LENGTH);

    void end() noexcept { m_active = false; m_all = false; }

    bool               active()       const noexcept { return m_active; }
    const std::string& text()         const noexcept { return m_text; }
    std::size_t        caret()        const noexcept { return m_caret; }
    bool               all_selected() const noexcept { return m_all; }
    TextFilter         filter()       const noexcept { return m_filter; }

    TextResult key(const std::string& name, bool shift, bool control);

private:
    TextResult insert(char ch);

    bool allowed(const std::string& text, std::size_t at, char ch) const noexcept;

    TextResult erase_back();

    TextResult erase_forward();

    std::string m_text;
    TextFilter  m_filter   = TextFilter::Decimal;
    bool        m_negative = false;
    std::size_t m_limit    = DEFAULT_LENGTH;
    std::size_t m_caret    = 0;
    bool        m_all      = false;
    bool        m_active   = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_TEXT_FIELD_HPP