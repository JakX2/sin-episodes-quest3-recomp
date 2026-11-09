#include "sin_compat/source_bsp_entities.hpp"

#include <cstring>
#include <limits>
#include <utility>

namespace sin_compat::map {
namespace {

constexpr std::size_t kBspHeaderBytes = 4U + 4U + 64U * 16U + 4U;
constexpr std::size_t kEntityLumpDirectoryOffset = 8U;
constexpr std::size_t kEntityLumpOffsetField = 0U;
constexpr std::size_t kEntityLumpLengthField = 4U;
constexpr std::size_t kEntityLumpEncodingField = 12U;

std::uint32_t read_u32_le(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8U) |
           (static_cast<std::uint32_t>(p[2]) << 16U) |
           (static_cast<std::uint32_t>(p[3]) << 24U);
}

bool is_space(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
           c == '\v' || c == '\f';
}

bool is_trailing_padding(std::uint8_t c) noexcept {
    return c == 0U || is_space(static_cast<char>(c));
}

class EntityTextParser {
public:
    EntityTextParser(const char* data, std::size_t size) : data_(data), size_(size) {}

    bool parse(std::vector<SourceMapEntity>& entities, std::size_t& error_offset) {
        if (size_ >= 3U && static_cast<unsigned char>(data_[0]) == 0xEFU &&
            static_cast<unsigned char>(data_[1]) == 0xBBU &&
            static_cast<unsigned char>(data_[2]) == 0xBFU) {
            position_ = 3U;
        }

        if (!skip_ignored()) {
            error_offset = position_;
            return false;
        }
        while (position_ < size_) {
            if (data_[position_] != '{') {
                error_offset = position_;
                return false;
            }
            ++position_;
            SourceMapEntity entity;
            bool closed = false;
            while (position_ < size_) {
                if (!skip_ignored()) {
                    error_offset = position_;
                    return false;
                }
                if (position_ >= size_) break;
                if (data_[position_] == '}') {
                    ++position_;
                    closed = true;
                    break;
                }
                if (data_[position_] == '{') {
                    error_offset = position_;
                    return false;
                }

                SourceMapKeyValue property;
                if (!read_quoted(property.key) || !skip_ignored() ||
                    !read_quoted(property.value)) {
                    error_offset = position_;
                    return false;
                }
                entity.properties.push_back(std::move(property));
            }
            if (!closed) {
                error_offset = position_;
                return false;
            }
            entities.push_back(std::move(entity));
            if (!skip_ignored()) {
                error_offset = position_;
                return false;
            }
        }
        error_offset = position_;
        return !entities.empty();
    }

private:
    bool skip_ignored() {
        for (;;) {
            while (position_ < size_ && is_space(data_[position_])) ++position_;
            if (position_ + 1U < size_ && data_[position_] == '/' &&
                data_[position_ + 1U] == '/') {
                position_ += 2U;
                while (position_ < size_ && data_[position_] != '\n') ++position_;
                continue;
            }
            if (position_ + 1U < size_ && data_[position_] == '/' &&
                data_[position_ + 1U] == '*') {
                position_ += 2U;
                bool closed = false;
                while (position_ + 1U < size_) {
                    if (data_[position_] == '*' && data_[position_ + 1U] == '/') {
                        position_ += 2U;
                        closed = true;
                        break;
                    }
                    ++position_;
                }
                if (!closed) return false;
                continue;
            }
            return true;
        }
    }

    bool read_quoted(std::string& output) {
        if (position_ >= size_ || data_[position_] != '"') return false;
        ++position_;
        output.clear();
        while (position_ < size_) {
            const char c = data_[position_++];
            if (c == '"') return true;
            if (c != '\\') {
                output.push_back(c);
                continue;
            }
            if (position_ >= size_) return false;
            const char escaped = data_[position_++];
            switch (escaped) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            default:
                // Preserve unknown escapes verbatim instead of rewriting map data.
                output.push_back('\\');
                output.push_back(escaped);
                break;
            }
        }
        return false;
    }

    const char* data_;
    std::size_t size_;
    std::size_t position_ = 0;
};

} // namespace

const std::string* SourceMapEntity::find_first(std::string_view key) const noexcept {
    for (const auto& property : properties) {
        if (property.key == key) return &property.value;
    }
    return nullptr;
}

SourceBspEntityParseResult parse_source_bsp_entities(
    const std::uint8_t* bytes,
    std::size_t byte_count,
    SourceBspEntityDocument& output) {
    SourceBspEntityParseResult result;
    if (bytes == nullptr) {
        result.status = SourceBspEntityParseStatus::NullInput;
        return result;
    }
    if (byte_count < kBspHeaderBytes) {
        result.status = SourceBspEntityParseStatus::TruncatedHeader;
        result.error_offset = byte_count;
        return result;
    }
    if (bytes[0] != 'V' || bytes[1] != 'B' || bytes[2] != 'S' || bytes[3] != 'P') {
        result.status = SourceBspEntityParseStatus::BadMagic;
        return result;
    }

    const std::uint32_t version = read_u32_le(bytes + 4U);
    if (version != 19U && version != 20U) {
        result.status = SourceBspEntityParseStatus::UnsupportedVersion;
        result.error_offset = 4U;
        return result;
    }

    const std::size_t entry = kEntityLumpDirectoryOffset;
    const std::uint32_t lump_offset = read_u32_le(bytes + entry + kEntityLumpOffsetField);
    const std::uint32_t lump_length = read_u32_le(bytes + entry + kEntityLumpLengthField);
    const std::uint32_t lump_encoding = read_u32_le(bytes + entry + kEntityLumpEncodingField);
    if (lump_encoding != 0U) {
        result.status = SourceBspEntityParseStatus::UnsupportedEntityLumpEncoding;
        result.error_offset = entry + kEntityLumpEncodingField;
        return result;
    }
    if (lump_length == 0U) {
        result.status = SourceBspEntityParseStatus::EmptyEntityLump;
        result.error_offset = entry + kEntityLumpLengthField;
        return result;
    }
    if (lump_offset > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
        lump_length > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
        result.status = SourceBspEntityParseStatus::InvalidEntityLumpRange;
        result.error_offset = entry + kEntityLumpOffsetField;
        return result;
    }
    const std::uint64_t end = static_cast<std::uint64_t>(lump_offset) +
                              static_cast<std::uint64_t>(lump_length);
    if (lump_offset < kBspHeaderBytes || end > byte_count) {
        result.status = SourceBspEntityParseStatus::InvalidEntityLumpRange;
        result.error_offset = entry + kEntityLumpOffsetField;
        return result;
    }

    const auto* text_bytes = bytes + lump_offset;
    std::size_t text_length = lump_length;
    for (std::size_t i = 0; i < text_length; ++i) {
        if (text_bytes[i] == 0U) {
            for (std::size_t tail = i; tail < text_length; ++tail) {
                if (!is_trailing_padding(text_bytes[tail])) {
                    result.status = SourceBspEntityParseStatus::MalformedEntityText;
                    result.error_offset = static_cast<std::size_t>(lump_offset) + tail;
                    return result;
                }
            }
            text_length = i;
            break;
        }
    }

    SourceBspEntityDocument parsed;
    parsed.bsp_version = version;
    const std::uint32_t revision_bits = read_u32_le(bytes + kBspHeaderBytes - 4U);
    std::memcpy(&parsed.map_revision, &revision_bits, sizeof(parsed.map_revision));

    std::size_t text_error = 0;
    EntityTextParser parser(reinterpret_cast<const char*>(text_bytes), text_length);
    if (!parser.parse(parsed.entities, text_error)) {
        const bool only_space = [&]() {
            for (std::size_t i = 0; i < text_length; ++i) {
                if (!is_space(static_cast<char>(text_bytes[i]))) return false;
            }
            return true;
        }();
        result.status = only_space ? SourceBspEntityParseStatus::NoEntities
                                   : SourceBspEntityParseStatus::MalformedEntityText;
        result.error_offset = static_cast<std::size_t>(lump_offset) + text_error;
        return result;
    }

    output = std::move(parsed);
    result.status = SourceBspEntityParseStatus::Ok;
    result.error_offset = 0;
    return result;
}

} // namespace sin_compat::map
