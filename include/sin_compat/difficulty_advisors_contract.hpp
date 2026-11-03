#pragma once

// Clean-room typed-operation model for original 2006 SiN DDAdvisorScores and
// DDAdvisorChannels. NOT a Source usermessage bitstream/transport implementation.
#include "difficulty_message_contract.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::difficulty {
inline constexpr std::size_t advisor_count = 6;
inline constexpr std::size_t channel_count = 4;
inline constexpr std::size_t advisor_scores_field_bits = advisor_count * (32 + 16);
inline constexpr std::size_t advisor_channels_field_bits = advisor_count * channel_count * 32;

struct AdvisorScore {
    float value = 0.0f;       // original sender reads advisor pointer+0x1C
    std::int16_t code = 0;    // serialized as signed 16-bit, client stores int32
};
struct AdvisorScores {
    std::array<AdvisorScore, advisor_count> entries{};
};
struct AdvisorChannels {
    // On the wire: advisor 0's four floats, then advisor 1's, and so on.
    std::array<std::array<float, channel_count>, advisor_count> entries{};
};

// Return the original *in-memory* integer which the server passes to its
// signed-16 writer. Do not assume out-of-range raw integers encode losslessly.
// Original branch family: server.dll 0x222BA714..0x222BA750.
inline std::int32_t select_advisor_score_code(std::size_t index,
                                              std::int32_t state,
                                              std::int32_t raw_code) {
    if (index >= 4) return raw_code;
    if (state < 2 && raw_code < 2 && state <= raw_code) return 2;
    if (state > 2 && raw_code > 2 && state >= raw_code) return 2;
    return raw_code;
}

inline bool read_advisor_scores(TypedReader& reader, AdvisorScores& out) {
    AdvisorScores tmp{};
    for (auto& x : tmp.entries)
        if (!reader.read_f32(x.value) || !reader.read_i16(x.code)) return false;
    out = tmp; // No partial output on a failed typed read.
    return true;
}
inline bool write_advisor_scores(TypedWriter& writer, const AdvisorScores& x) {
    for (const auto& row : x.entries)
        if (!writer.write_f32(row.value) || !writer.write_i16(row.code)) return false;
    return true;
}
inline bool read_advisor_channels(TypedReader& reader, AdvisorChannels& out) {
    AdvisorChannels tmp{};
    for (auto& advisor : tmp.entries)
        for (auto& value : advisor)
            if (!reader.read_f32(value)) return false;
    out = tmp;
    return true;
}
inline bool write_advisor_channels(TypedWriter& writer, const AdvisorChannels& x) {
    for (const auto& advisor : x.entries)
        for (float value : advisor)
            if (!writer.write_f32(value)) return false;
    return true;
}

struct ClientAdvisorPresentation {
    std::array<float, advisor_count> scores{}; // client vector at this+0x30
    std::array<std::int32_t, advisor_count> codes{}; // this+0x10..0x24
    // Client stores four vectors at this+0x44/+0x58/+0x6C/+0x80.
    std::array<std::array<float, advisor_count>, channel_count> channels{};
};
inline void apply_advisor_scores(ClientAdvisorPresentation& client,
                                 const AdvisorScores& packet) {
    for (std::size_t i=0; i<advisor_count; ++i) {
        client.scores[i] = packet.entries[i].value;
        client.codes[i] = packet.entries[i].code;
    }
}
inline void apply_advisor_channels(ClientAdvisorPresentation& client,
                                   const AdvisorChannels& packet) {
    for (std::size_t a=0; a<advisor_count; ++a)
        for (std::size_t c=0; c<channel_count; ++c)
            client.channels[c][a] = packet.entries[a][c];
}
} // namespace sin_compat::difficulty
