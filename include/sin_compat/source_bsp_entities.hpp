#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sin_compat::map {

enum class SourceBspEntityParseStatus : std::uint8_t {
    Ok,
    NullInput,
    TruncatedHeader,
    BadMagic,
    UnsupportedVersion,
    InvalidEntityLumpRange,
    UnsupportedEntityLumpEncoding,
    EmptyEntityLump,
    NoEntities,
    MalformedEntityText
};

struct SourceMapKeyValue {
    std::string key;
    std::string value;
};

struct SourceMapEntity {
    // Preserve source order and duplicate keys. Do not silently collapse map data.
    std::vector<SourceMapKeyValue> properties;

    const std::string* find_first(std::string_view key) const noexcept;
};

struct SourceBspEntityDocument {
    std::uint32_t bsp_version = 0;
    std::int32_t map_revision = 0;
    std::vector<SourceMapEntity> entities;
};

struct SourceBspEntityParseResult {
    SourceBspEntityParseStatus status = SourceBspEntityParseStatus::NullInput;
    std::size_t error_offset = 0;

    bool ok() const noexcept { return status == SourceBspEntityParseStatus::Ok; }
};

// Reads only VBSP header metadata and LUMP_ENTITIES (lump 0). Supports the
// uncompressed little-endian v19/v20 header and KeyValues-style entity text.
// Output is replaced only after the entire lump has parsed successfully.
SourceBspEntityParseResult parse_source_bsp_entities(
    const std::uint8_t* bytes,
    std::size_t byte_count,
    SourceBspEntityDocument& output);

} // namespace sin_compat::map
