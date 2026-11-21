#pragma once
// Bounded C++17 transcription of native CVoiceStatus VoiceMask message field
// order. Not original Ritual C++ and not an implementation of engine bf_read.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sin_compat::original_voice_mask_wire {
constexpr std::size_t kAudibleMaskBits=32;
constexpr std::size_t kServerBlockedMaskBits=32;
constexpr std::size_t kStatusBits=8;
constexpr std::size_t kPayloadBits=72;
constexpr std::size_t kAlignedPayloadBytes=9;
constexpr std::size_t kOriginalAudibleMaskOffset=0x14;
constexpr std::size_t kOriginalServerBlockedMaskOffset=0x1C;
constexpr std::size_t kOriginalStatusOffset=0x08;

struct Fields {
    std::uint32_t audible_players = 0;
    std::uint32_t server_banned_players = 0;
    std::uint8_t status = 0;
};

// The original 0x240A4760 reads LSB-first bit ranges from native 32-bit
// little-endian words; here the same field ordering is independently decoded
// without assuming access to the original byte reader's buffer ownership.
class BitReader {
public:
    explicit BitReader(const std::vector<std::uint8_t>& input,
                       std::size_t bit_offset = 0) noexcept
        : input_(input), cursor_(bit_offset) {}

    bool get(std::size_t bits, std::uint32_t& out) noexcept {
        out=0;
        if (bits > 32 || cursor_ > input_.size() * 8 ||
            bits > input_.size() * 8 - cursor_) return false;
        for (std::size_t i=0;i<bits;++i) {
            const std::size_t index=cursor_+i;
            const std::uint32_t bit=std::uint32_t((input_[index/8] >> (index%8))&1u);
            out |= bit << i;
        }
        cursor_+=bits;
        return true;
    }
    std::size_t position() const noexcept {return cursor_;}
private:
    const std::vector<std::uint8_t>& input_;
    std::size_t cursor_;
};

// The native wrappers call ReadSBitLong(32), ReadSBitLong(32), then
// ReadUBitLong(8). For width exactly 32 the sign-extension operation
// preserves the underlying 32-bit pattern, so uint32_t is appropriate here.
// Unlike the retail handler this strict parser leaves no partially written
// state on invalid/truncated data.
inline std::optional<Fields> decode_strict(const std::vector<std::uint8_t>& bits,
                                           std::size_t initial_bit_offset=0) noexcept {
    BitReader reader(bits,initial_bit_offset);
    Fields f{};
    std::uint32_t b=0;
    if (!reader.get(kAudibleMaskBits,f.audible_players))return std::nullopt;
    if (!reader.get(kServerBlockedMaskBits,f.server_banned_players))return std::nullopt;
    if (!reader.get(kStatusBits,b))return std::nullopt;
    f.status=static_cast<std::uint8_t>(b);
    return f;
}
}