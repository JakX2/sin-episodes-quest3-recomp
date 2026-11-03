#pragma once

// Clean-room, host-agnostic typed-call contract derived from the verified
// original SiN Episodes 2006 client/server disassembly documented in PR #72.
// This is NOT a byte-level Source usermessage codec, SourceVR integration,
// or a reconstruction of difficulty scoring/action semantics.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::difficulty {

struct Registration {
    std::string_view name;
    std::int32_t declared_length; // -1 means variable length, not byte count.
    bool client_hook_verified_in_dd_init;
};

inline constexpr std::array<Registration, 9> registrations{{
    {"GamePlayVarValue", 6, true},
    {"Progression", 4, true},
    {"DDInfo", -1, true},
    {"DDAdvisorScores", -1, true},
    {"DDAdvisorChannels", -1, true},
    {"DDAdvisorsUpdate", -1, false},
    {"DDDecisionMade", -1, true},
    {"DDDecisionsPerf", -1, true},
    {"DDSuccessModifiers", -1, true},
}};

// Operations correspond to the confirmed *types and order* of original
// x86 read/write CALLs. They DO NOT define bit widths, byte order, frame
// boundaries or a compatible network transport.
class TypedReader {
public:
    virtual ~TypedReader() = default;
    virtual bool read_f32(float& out) = 0;
    virtual bool read_i32(std::int32_t& out) = 0;
};
class TypedWriter {
public:
    virtual ~TypedWriter() = default;
    virtual bool write_f32(float value) = 0;
    virtual bool write_i32(std::int32_t value) = 0;
};

// Anonymous fields: their gameplay meanings have NOT been established.
struct DecisionOrdinaryRecord {
    float first = 0.0f;
    std::array<std::int32_t, 3> integers{};
    std::array<float, 4> floats{};
};

enum class DecisionPrefixKind {
    OrdinaryRecord,
    ZeroSentinel,
    MinusOneSentinel,
};

struct DecisionPrefix {
    DecisionPrefixKind kind = DecisionPrefixKind::OrdinaryRecord;
    DecisionOrdinaryRecord ordinary{};
};

// This function examines only the VERIFIED read sequence. For a sentinel it
// intentionally STOPS after the first f32. Original sentinel branch tails and
// packet completion remain unknown; caller MUST NOT interpret this result as
// a fully consumed/handled Source usermessage.
inline bool read_decision_known_prefix(TypedReader& reader, DecisionPrefix& out) {
    DecisionPrefix next{};
    if (!reader.read_f32(next.ordinary.first)) return false;
    if (next.ordinary.first == 0.0f) {
        next.kind = DecisionPrefixKind::ZeroSentinel;
        out = next;
        return true;
    }
    if (next.ordinary.first == -1.0f) {
        next.kind = DecisionPrefixKind::MinusOneSentinel;
        out = next;
        return true;
    }
    for (auto& v : next.ordinary.integers) if (!reader.read_i32(v)) return false;
    for (auto& v : next.ordinary.floats) if (!reader.read_f32(v)) return false;
    out = next; // No partially initialized result on read failure.
    return true;
}

// Ordinary server writer path ONLY. Sentinel messages have separate original
// paths that have not been completely recovered.
inline bool write_decision_ordinary(TypedWriter& writer,
                                    const DecisionOrdinaryRecord& record) {
    // Guard against accidentally writing a sentinel as a full record.
    if (record.first == 0.0f || record.first == -1.0f) return false;
    if (!writer.write_f32(record.first)) return false;
    for (auto v : record.integers) if (!writer.write_i32(v)) return false;
    for (auto v : record.floats) if (!writer.write_f32(v)) return false;
    return true;
}

inline constexpr std::size_t success_pair_count = 10;
struct SuccessModifiers {
    // Original pair members are associated with selector 1 and selector 3;
    // neither the index meanings nor the gameplay formulas are recovered.
    std::array<std::array<float, 2>, success_pair_count> pairs{};
};

inline bool read_success_modifiers(TypedReader& reader, SuccessModifiers& out) {
    SuccessModifiers next{};
    for (auto& pair : next.pairs) {
        if (!reader.read_f32(pair[0]) || !reader.read_f32(pair[1])) return false;
    }
    out = next;
    return true;
}

inline bool write_success_modifiers(TypedWriter& writer,
                                    const SuccessModifiers& values) {
    for (const auto& pair : values.pairs) {
        if (!writer.write_f32(pair[0]) || !writer.write_f32(pair[1])) return false;
    }
    return true;
}

} // namespace sin_compat::difficulty
