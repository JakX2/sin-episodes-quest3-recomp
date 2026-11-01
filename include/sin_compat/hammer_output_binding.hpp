#pragma once

// Independently authored parser for an extracted Source/Hammer output value.
// This is NOT a VMF reader, Source event queue, engine ABI, or game module.
#include "sin_compat/entity_io_runtime.hpp"

#include <string_view>

namespace sin_compat {

enum class HammerOutputError {
    None,
    TooLong,
    WrongFieldCount,
    EmptyTarget,
    EmptyInput,
    InvalidDelay,
    InvalidMaxFires,
};

struct HammerOutputParseResult {
    EntityOutputConnection connection{};
    HammerOutputError error = HammerOutputError::None;
    explicit operator bool() const noexcept { return error == HammerOutputError::None; }
};

// Parse the five-field value "target,input,parameter,delay,maxfires" or its
// ESC (0x1B) delimited BSP entity-lump variant, without the output key.
// The host is responsible for wildcard, !activator/!caller and name lookup.
HammerOutputParseResult parse_hammer_output_value(std::string_view value);

} // namespace sin_compat
