#include "sin_compat/hammer_output_binding.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <string>
#include <system_error>

namespace sin_compat {
namespace {
constexpr std::size_t kMaxOutputLength = 16384;

std::string_view trim(std::string_view s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos) return {};
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

bool parse_delay(std::string_view field, double& out) {
    if (field.empty()) return false;
    std::istringstream stream{std::string(field)};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> out;
    return !stream.fail() && stream.peek() == std::char_traits<char>::eof() &&
           std::isfinite(out) && out >= 0.0;
}

bool parse_maxfires(std::string_view field, int& out) {
    if (field.empty()) return false;
    auto result = std::from_chars(field.data(), field.data() + field.size(), out);
    return result.ec == std::errc{} && result.ptr == field.data() + field.size() &&
           out >= -1;
}
} // namespace

HammerOutputParseResult parse_hammer_output_value(std::string_view value) {
    HammerOutputParseResult parsed;
    if (value.size() > kMaxOutputLength) {
        parsed.error = HammerOutputError::TooLong;
        return parsed;
    }
    const char delimiter = value.find('\x1b') == std::string_view::npos ? ',' : '\x1b';
    std::array<std::string_view, 5> fields{};
    std::size_t offset = 0;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto pos = value.find(delimiter, offset);
        if (i < fields.size() - 1 && pos == std::string_view::npos) {
            parsed.error = HammerOutputError::WrongFieldCount;
            return parsed;
        }
        if (i == fields.size() - 1 && pos != std::string_view::npos) {
            parsed.error = HammerOutputError::WrongFieldCount;
            return parsed;
        }
        fields[i] = trim(value.substr(offset, pos == std::string_view::npos
                                               ? std::string_view::npos : pos - offset));
        if (pos != std::string_view::npos) offset = pos + 1;
    }
    if (fields[0].empty()) parsed.error = HammerOutputError::EmptyTarget;
    else if (fields[1].empty()) parsed.error = HammerOutputError::EmptyInput;
    if (parsed.error != HammerOutputError::None) return parsed;

    if (!parse_delay(fields[3], parsed.connection.delay_seconds)) {
        parsed.error = HammerOutputError::InvalidDelay;
        return parsed;
    }
    if (!parse_maxfires(fields[4], parsed.connection.max_fires)) {
        parsed.error = HammerOutputError::InvalidMaxFires;
        return parsed;
    }
    parsed.connection.target = std::string(fields[0]);
    parsed.connection.input = std::string(fields[1]);
    parsed.connection.parameter = std::string(fields[2]);
    return parsed;
}

} // namespace sin_compat
