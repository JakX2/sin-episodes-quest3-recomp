#include "sin_compat/source_bsp_entities.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace sin_compat::map;

namespace {
constexpr std::size_t kHeader = 1036U;

void put_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::vector<std::uint8_t> make_bsp(const std::string& entities,
                                   std::uint32_t version = 19U,
                                   std::uint32_t encoding = 0U) {
    std::vector<std::uint8_t> bytes(kHeader + entities.size(), 0U);
    bytes[0] = 'V'; bytes[1] = 'B'; bytes[2] = 'S'; bytes[3] = 'P';
    put_u32(bytes, 4U, version);
    put_u32(bytes, 8U, static_cast<std::uint32_t>(kHeader));
    put_u32(bytes, 12U, static_cast<std::uint32_t>(entities.size()));
    put_u32(bytes, 16U, 0U);
    put_u32(bytes, 20U, encoding);
    put_u32(bytes, kHeader - 4U, 37U);
    if (!entities.empty()) std::memcpy(bytes.data() + kHeader, entities.data(), entities.size());
    return bytes;
}

void test_reads_multiple_entities_preserving_order_duplicates_and_escapes() {
    const std::string text =
        "// world first\n"
        "{ \"classname\" \"worldspawn\" \"message\" \"A \\\"quoted\\\" map\" }\n"
        "/* comment */ { \"classname\" \"logic_relay\" \"OnTrigger\" \"target,Use,,0,-1\" \"dup\" \"one\" \"dup\" \"two\" }\n";
    const auto bytes = make_bsp(text, 19U);
    SourceBspEntityDocument doc;
    const auto result = parse_source_bsp_entities(bytes.data(), bytes.size(), doc);
    assert(result.ok());
    assert(doc.bsp_version == 19U && doc.map_revision == 37);
    assert(doc.entities.size() == 2U);
    assert(*doc.entities[0].find_first("classname") == "worldspawn");
    assert(*doc.entities[0].find_first("message") == "A \"quoted\" map");
    assert(*doc.entities[1].find_first("classname") == "logic_relay");
    assert(*doc.entities[1].find_first("dup") == "one");
    assert(doc.entities[1].properties.size() == 4U);
}

void test_v20_and_nul_terminated_entity_lump() {
    std::string entity_text = "{\"classname\" \"info_player_start\"}";
    entity_text.push_back('\0');
    const auto bytes = make_bsp(entity_text, 20U);
    SourceBspEntityDocument doc;
    const auto result = parse_source_bsp_entities(bytes.data(), bytes.size(), doc);
    assert(result.ok() && doc.bsp_version == 20U);
    assert(doc.entities.size() == 1U);
}

void test_unknown_escape_is_preserved() {
    const auto bytes = make_bsp("{\"classname\" \"test\" \"path\" \"a\\qb\"}");
    SourceBspEntityDocument doc;
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).ok());
    assert(*doc.entities[0].find_first("path") == "a\\qb");
}

void test_rejects_invalid_header_and_ranges() {
    SourceBspEntityDocument doc;
    auto bytes = make_bsp("{\"classname\" \"worldspawn\"}");
    assert(parse_source_bsp_entities(nullptr, 0U, doc).status == SourceBspEntityParseStatus::NullInput);
    assert(parse_source_bsp_entities(bytes.data(), 100U, doc).status == SourceBspEntityParseStatus::TruncatedHeader);
    bytes[0] = 'X';
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::BadMagic);
    bytes[0] = 'V';
    put_u32(bytes, 4U, 21U);
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::UnsupportedVersion);
    put_u32(bytes, 4U, 19U);
    put_u32(bytes, 8U, 0xFFFFFFF0U);
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::InvalidEntityLumpRange);
    put_u32(bytes, 8U, 0x80000000U);
    put_u32(bytes, 12U, 1U);
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::InvalidEntityLumpRange);
    put_u32(bytes, 8U, static_cast<std::uint32_t>(kHeader));
    put_u32(bytes, 12U, 0xFFFFFFFFU);
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::InvalidEntityLumpRange);
}

void test_rejects_compressed_or_empty_entity_lump() {
    SourceBspEntityDocument doc;
    auto bytes = make_bsp("{\"classname\" \"worldspawn\"}", 19U, 123U);
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::UnsupportedEntityLumpEncoding);
    bytes = make_bsp("");
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::EmptyEntityLump);
    bytes = make_bsp("  \n\t");
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::NoEntities);
}

void test_rejects_malformed_entity_text() {
    SourceBspEntityDocument doc;
    for (const std::string text : {
        "{ \"classname\" \"worldspawn\"",
        "{ classname \"worldspawn\" }",
        "{ \"classname\" }",
        "{ { \"classname\" \"nested\" } }",
        "{ \"classname\" \"worldspawn\" } garbage",
        "/* never closed"}) {
        const auto bytes = make_bsp(text);
        assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::MalformedEntityText);
    }
}

void test_nul_padding_and_atomic_output() {
    std::string padded = "{\"classname\" \"worldspawn\"}";
    padded.append("\0\0 \t", 4U);
    auto bytes = make_bsp(padded);
    SourceBspEntityDocument doc;
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).ok());
    const auto previous = doc.entities.size();
    std::string junk = "{\"classname\" \"worldspawn\"}";
    junk.append("\0junk", 5U);
    bytes = make_bsp(junk);
    assert(parse_source_bsp_entities(bytes.data(), bytes.size(), doc).status == SourceBspEntityParseStatus::MalformedEntityText);
    assert(doc.entities.size() == previous);
}

} // namespace

int main() {
    test_reads_multiple_entities_preserving_order_duplicates_and_escapes();
    test_v20_and_nul_terminated_entity_lump();
    test_unknown_escape_is_preserved();
    test_rejects_invalid_header_and_ranges();
    test_rejects_compressed_or_empty_entity_lump();
    test_rejects_malformed_entity_text();
    test_nul_padding_and_atomic_output();
    return 0;
}
