#include "ui/text_field.hpp"

namespace voxelspire {

void TextField::begin(std::string text, TextFilter filter, bool allow_negative, std::size_t max_length) {
    m_text     = std::move(text);
    m_filter   = filter;
    m_negative = allow_negative;
    m_limit    = max_length;
    m_caret    = m_text.size();
    m_all      = true;
    m_active   = true;
}

TextResult TextField::key(const std::string& name, bool shift, bool control) {
    if (!m_active) return TextResult::Ignored;
    if (name == "Enter") return TextResult::Commit;
    if (name == "Escape") return TextResult::Cancel;
    if (name == "Tab") return shift ? TextResult::Previous : TextResult::Next;
    if (control && (name == "a" || name == "A")) { m_all = true; return TextResult::Edited; }
    if (name == "Backspace") return erase_back();
    if (name == "Delete") return erase_forward();
    if (name == "LeftArrow") { m_caret = m_all ? 0 : (m_caret > 0 ? m_caret - 1 : 0); m_all = false; return TextResult::Edited; }
    if (name == "RightArrow") { m_caret = m_all ? m_text.size() : (m_caret < m_text.size() ? m_caret + 1 : m_caret); m_all = false; return TextResult::Edited; }
    if (name == "Home") { m_caret = 0; m_all = false; return TextResult::Edited; }
    if (name == "End") { m_caret = m_text.size(); m_all = false; return TextResult::Edited; }
    if (name == SPACE_KEY && m_filter == TextFilter::Text) return insert(' ');
    if (name.size() != 1) return TextResult::Blocked;
    return insert(name[0]);
}

TextResult TextField::insert(char ch) {
    std::string base = m_all ? std::string() : m_text;
    std::size_t at = m_all ? 0 : m_caret;
    if (!allowed(base, at, ch) || base.size() >= m_limit) return TextResult::Blocked;
    base.insert(base.begin() + static_cast<std::ptrdiff_t>(at), ch);
    m_text  = std::move(base);
    m_caret = at + 1;
    m_all   = false;
    return TextResult::Edited;
}

bool TextField::allowed(const std::string& text, std::size_t at, char ch) const noexcept {
    const bool digit = std::isdigit(static_cast<unsigned char>(ch)) != 0;
    if (m_filter == TextFilter::Text) return ch >= FIRST_PRINTABLE && ch <= LAST_PRINTABLE;
    const bool leads_with_sign = !text.empty() && (text[0] == MINUS || text[0] == HASH);
    if (at == 0 && leads_with_sign) return false;

    switch (m_filter) {
        case TextFilter::Integer:
            return digit || (ch == MINUS && m_negative && at == 0);
        case TextFilter::Decimal:
            return digit || (ch == MINUS && m_negative && at == 0) || (ch == DOT && text.find(DOT) == std::string::npos);
        case TextFilter::Hex:
            return std::isxdigit(static_cast<unsigned char>(ch)) != 0 || (ch == HASH && at == 0);
        case TextFilter::Text:
            return true;
    }

    return false;
}

TextResult TextField::erase_back() {
    if (m_all) { m_text.clear(); m_caret = 0; m_all = false; return TextResult::Edited; }
    if (m_caret == 0) return TextResult::Blocked;
    m_text.erase(--m_caret, 1);
    return TextResult::Edited;
}

TextResult TextField::erase_forward() {
    if (m_all) { m_text.clear(); m_caret = 0; m_all = false; return TextResult::Edited; }
    if (m_caret >= m_text.size()) return TextResult::Blocked;
    m_text.erase(m_caret, 1);
    return TextResult::Edited;
}

} // namespace voxelspire
