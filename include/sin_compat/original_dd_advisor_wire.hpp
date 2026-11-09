#pragma once

// Independent behavioral reconstruction from SHA-pinned original SiN 2006
// x86 server.dll and client.dll (Phase 148). No original source copied.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace sin_compat::original_dd_advisor {

constexpr std::size_t kAdvisorCount = 6U;
constexpr std::size_t kChannelFields = 4U;
static_assert(sizeof(float) == sizeof(std::uint32_t), "32-bit float required");
static_assert(std::numeric_limits<float>::is_iec559, "IEEE-754 float required");

struct ServerAdvisor {
    float score = 0.0f;                            // Original row +0x1C
    std::int32_t raw_code = 0;                      // Original row +0x20
    std::array<float, kChannelFields> channels{};  // +0x24/+0x28/+0x2C/+0x30
};
struct ServerSnapshot {
    std::int32_t raw_mode = 0;                     // Original manager +0x40
    std::array<ServerAdvisor, kAdvisorCount> advisors{}; // pointers +0x48 onward
};
struct ClientSnapshot {
    std::array<float, kAdvisorCount> scores{};
    std::array<std::int32_t, kAdvisorCount> codes{};
    std::array<std::array<float, kAdvisorCount>, kChannelFields> channels{};
    bool dirty = true;
};

// The real engine owns message setup, bit order, envelope, and lifecycle.
struct Writer {
    virtual ~Writer() = default;
    virtual bool write_float32_bits(std::uint32_t value) = 0;
    virtual bool write_signed16(std::int16_t value) = 0;
};
struct Reader {
    virtual ~Reader() = default;
    virtual bool read_float32_bits(std::uint32_t& value) = 0;
    virtual bool read_signed16(std::int16_t& value) = 0;
};

inline std::uint32_t float_bits(float value) noexcept {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}
inline float float_from_bits(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

// 0x222BA714..0x222BA74F uses signed JGE/JL/JLE comparisons.
// Unknown +0x40 and +0x20 field meanings are NOT guessed as game modes.
inline std::int32_t transmitted_code(std::size_t index,
                                     std::int32_t raw_mode,
                                     std::int32_t raw_code) noexcept {
    if (index >= 4U) return raw_code;
    if (raw_mode < 2 && raw_code < 2 && raw_mode <= raw_code) return 2;
    if (raw_mode > 2 && raw_code > 2 && raw_mode >= raw_code) return 2;
    return raw_code;
}

inline std::int16_t wire_signed16(std::int32_t value) noexcept {
    const auto low = static_cast<std::uint16_t>(value);
    const std::int32_t signed_value = low < 32768U
        ? static_cast<std::int32_t>(low)
        : static_cast<std::int32_t>(low) - 65536;
    return static_cast<std::int16_t>(signed_value);
}

// DDAdvisorScores: six (Float32, Signed16) records, 288 payload bits.
inline bool write_scores(const ServerSnapshot& input, Writer& writer) {
    for (std::size_t i = 0; i < kAdvisorCount; ++i) {
        const auto& row = input.advisors[i];
        if (!writer.write_float32_bits(float_bits(row.score))) return false;
        if (!writer.write_signed16(wire_signed16(
                transmitted_code(i, input.raw_mode, row.raw_code)))) return false;
    }
    return true;
}

// DDAdvisorChannels: six sets of four Float32 values, 768 payload bits.
inline bool write_channels(const ServerSnapshot& input, Writer& writer) {
    for (const auto& row : input.advisors) {
        for (float channel : row.channels) {
            if (!writer.write_float32_bits(float_bits(channel))) return false;
        }
    }
    return true;
}

// Portable snapshot commits atomically on failure, unlike the original
// unverified truncated-packet behavior. Original client resets +0xB8.
inline bool read_scores(Reader& reader, ClientSnapshot& output) {
    auto scores = output.scores;
    auto codes = output.codes;
    for (std::size_t i = 0; i < kAdvisorCount; ++i) {
        std::uint32_t bits = 0;
        std::int16_t code = 0;
        if (!reader.read_float32_bits(bits) || !reader.read_signed16(code))
            return false;
        scores[i] = float_from_bits(bits);
        codes[i] = static_cast<std::int32_t>(code);
    }
    output.scores = scores;
    output.codes = codes;
    output.dirty = false;
    return true;
}
inline bool read_channels(Reader& reader, ClientSnapshot& output) {
    auto channels = output.channels;
    for (std::size_t i = 0; i < kAdvisorCount; ++i) {
        for (std::size_t c = 0; c < kChannelFields; ++c) {
            std::uint32_t bits = 0;
            if (!reader.read_float32_bits(bits)) return false;
            channels[c][i] = float_from_bits(bits);
        }
    }
    output.channels = channels;
    output.dirty = false;
    return true;
}
} // namespace sin_compat::original_dd_advisor
