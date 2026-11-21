#pragma once
// Independently authored Phase556 reference of *bounded* original server
// CVoiceGameMgr command state transitions. NOT complete original Ritual C++.
// The RETAIL 2006 DLL has fewer bounds guards; these guards are host safety.
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string_view>

namespace sin_compat::original_server_voice_commands {

enum class CommandKind { vban, VModEnable, unknown };

// Original 0x223A5574 chooses a locale-dependent transform or, when the
// native readiness flag is false, the ASCII-only 0x223A8410 fallback.
// These are fallback-only semantics, not a claim about active retail locale.
inline unsigned char ascii_fold(unsigned char c) {
    if (c >= static_cast<unsigned char>('A') &&
        c <= static_cast<unsigned char>('Z')) {
        return static_cast<unsigned char>(c + static_cast<unsigned char>('a' - 'A'));
    }
    return c;
}

inline bool ascii_casefold_equal(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i=0; i<a.size(); ++i) {
        const auto lhs=ascii_fold(static_cast<unsigned char>(a[i]));
        const auto rhs=ascii_fold(static_cast<unsigned char>(b[i]));
        if (lhs != rhs) return false;
    }
    return true;
}

inline CommandKind classify_ascii_fallback(std::string_view cmd) {
    if (ascii_casefold_equal(cmd, "vban")) return CommandKind::vban;
    if (ascii_casefold_equal(cmd, "VModEnable")) return CommandKind::VModEnable;
    return CommandKind::unknown;
}

struct VoiceManagerState {
    std::size_t players;
    std::vector<std::uint32_t> voice_enabled_bits; // original global 0x22590400
    std::vector<std::uint32_t> request_pending_bits; // 0x22590588
    std::vector<std::uint32_t> server_banned_masks; // 0x22590408: one mask per recipient
    std::vector<std::uint32_t> cached_audible; // 0x22590488: per recipient
    std::vector<std::uint32_t> cached_banned; // 0x22590508: per recipient

    explicit VoiceManagerState(std::size_t player_count)
        : players(player_count),
          voice_enabled_bits((player_count + 31U) / 32U, 0),
          request_pending_bits((player_count + 31U) / 32U, 0),
          server_banned_masks(player_count, 0),
          cached_audible(player_count, 0),
          cached_banned(player_count, 0) {}
};

inline bool valid_player(const VoiceManagerState& s, std::size_t index) {
    return index < s.players;
}

inline std::uint32_t bit_mask(std::size_t index) {
    return std::uint32_t{1} << (index & 31U);
}

inline bool test_bit(const std::vector<std::uint32_t>& words, std::size_t index) {
    const auto word = index / 32U;
    return word < words.size() && (words[word] & bit_mask(index)) != 0U;
}

inline void assign_bit(std::vector<std::uint32_t>& words,
                       std::size_t index, bool enabled) {
    const auto word = index / 32U;
    if (word >= words.size()) return; // host-side safety only
    if (enabled) words[word] |= bit_mask(index);
    else words[word] &= ~bit_mask(index);
}

// 0x22293AC0: original engine +0x48 yields 1-based player number, then
// pending state is marked and recipient *cached values* are zeroed.
inline bool mark_refresh(VoiceManagerState& s, std::uint32_t one_based_player) {
    if (one_based_player == 0U) return false; // host guard only
    auto index = static_cast<std::size_t>(one_based_player - 1U);
    if (!valid_player(s,index)) return false; // host guard only
    assign_bit(s.request_pending_bits,index,true);
    s.cached_audible[index] = 0U;
    s.cached_banned[index] = 0U;
    return true;
}

// Bounded state-change model of original 0x22293B10. The original compares
// command names with 0x223A5574 and parses through 0x223A5CD6 / 0x223A56CB.
// Here the classification and parsed numeric arguments are given explicitly;
// the original CRT, engine virtuals, network routing and diagnostic strings
// are NOT falsely replaced by this helper.
inline bool apply_parsed_command(VoiceManagerState& s,
                                 std::int32_t zero_based_sender,
                                 CommandKind command,
                                 const std::vector<std::uint32_t>& args) {
    // Native: invalid recipient returns 'handled' true, after diagnostic.
    if (zero_based_sender < 0 ||
        !valid_player(s,static_cast<std::size_t>(zero_based_sender))) return true;
    const auto sender = static_cast<std::size_t>(zero_based_sender);
    if (command == CommandKind::vban) {
        if (args.empty()) return false; // native argc<2
        // Only argument #1 writes; following args emit a debug message.
        // The original sscanf return is ignored, and its pre-zeroed out value
        // is stored even after a failed parse. Caller supplies parsed value.
        s.server_banned_masks[sender] = args.front();
        return true;
    }
    if (command == CommandKind::VModEnable) {
        if (args.empty()) return false; // native argc<2
        assign_bit(s.voice_enabled_bits,sender,args.front() != 0U);
        assign_bit(s.request_pending_bits,sender,false);
        return true;
    }
    return false;
}
} // namespace sin_compat::original_server_voice_commands