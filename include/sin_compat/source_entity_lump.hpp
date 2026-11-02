#pragma once
// Bounded parser for the plaintext entity lump of Source BSP maps.
// Map geometry, compressed BSP handling and Source engine calls remain outside
// this standalone compatibility library.
#include "sin_compat/scenario_entity_world.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sin_compat {

struct ScenarioLumpParseResult {
    std::vector<ScenarioEntitySpec> entities;
    std::string error;
    std::size_t error_offset = 0;
    explicit operator bool() const { return error.empty(); }
};

ScenarioLumpParseResult parse_source_entity_text(std::string_view text);

struct ScenarioLumpLoadReport {
    std::size_t parsed = 0;
    std::size_t spawned = 0;
    std::size_t unsupported = 0;
    std::string error;
    std::vector<EntityRef> created;
    explicit operator bool() const { return error.empty(); }
};

// All-or-nothing for supported entities: parse the entire text first and roll
// back every new entity if any supported entity contains invalid I/O syntax.
// Unsupported classes remain listed as skips, not silently "implemented".
ScenarioLumpLoadReport load_source_entity_text(
    ScenarioEntityWorld& world, std::string_view text);

} // namespace sin_compat
