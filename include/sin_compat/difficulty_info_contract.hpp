#pragma once

// Original SiN 2006 DDInfo typed field-operation contract only.
// Not engine message framing, gameplay scoring, or exact x87 calculation.
#include "difficulty_message_contract.hpp"

namespace sin_compat::difficulty {

// Original bit-buffer writes and client inlined reads operate on single bits.
// Separate interfaces keep the existing DDDecisionMade contract unchanged.
class DDInfoReader : public TypedReader {
public:
    virtual bool read_bit(bool& out) = 0;
};
class DDInfoWriter : public TypedWriter {
public:
    virtual bool write_bit(bool value) = 0;
};

inline constexpr std::size_t ddinfo_field_bit_count = 57;
inline constexpr std::size_t ddinfo_advisor_flag_count = 6;

struct DDInfoFields {
    std::array<bool, 2> first_flags{}; // server this+4 / this+5
    float computed_float = 0.0f;      // original server performs a subtraction
    std::int16_t integer = 0;        // server this+0x40, 16-bit on wire
    bool shared_flag = false;        // server global byte
    std::array<bool, ddinfo_advisor_flag_count> advisor_flags{};
};

// Pure typed operations; no invented bit endianness, framing, or packet
// padding. Failure leaves output unchanged.
inline bool read_ddinfo_fields(DDInfoReader& reader, DDInfoFields& out) {
    DDInfoFields next{};
    if (!reader.read_bit(next.first_flags[0]) ||
        !reader.read_bit(next.first_flags[1]) ||
        !reader.read_f32(next.computed_float) ||
        !reader.read_i16(next.integer) ||
        !reader.read_bit(next.shared_flag)) return false;
    for (auto& flag : next.advisor_flags)
        if (!reader.read_bit(flag)) return false;
    out = next;
    return true;
}

inline bool write_ddinfo_fields(DDInfoWriter& writer, const DDInfoFields& data) {
    if (!writer.write_bit(data.first_flags[0]) ||
        !writer.write_bit(data.first_flags[1]) ||
        !writer.write_f32(data.computed_float) ||
        !writer.write_i16(data.integer) ||
        !writer.write_bit(data.shared_flag)) return false;
    for (bool flag : data.advisor_flags)
        if (!writer.write_bit(flag)) return false;
    return true;
}

// Original client reads all six bits, but substitutes the shared flag for
// positions 2 and 3 in its six state bytes at this+0x28..this+0x2D.
// Preserve original transmitted data separately.
inline std::array<bool, ddinfo_advisor_flag_count>
client_effective_ddinfo_flags(const DDInfoFields& data) {
    auto flags = data.advisor_flags;
    flags[2] = data.shared_flag;
    flags[3] = data.shared_flag;
    return flags;
}

} // namespace sin_compat::difficulty
