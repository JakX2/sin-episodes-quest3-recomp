#pragma once
// Independent C++17 reference for *bounded*, source-authenticated original
// 2006 CVoiceStatus decisions. Does not invoke original client.dll vtables.
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace sin_compat::original_client_voice_status {
inline constexpr std::uintptr_t kNativeUpdateServerState = 0x241E5880u;
inline constexpr std::uintptr_t kNativeHandleReqStateMsg = 0x241E5A90u;
inline constexpr std::uintptr_t kNativeMask14Getter = 0x241E5AC0u;
inline constexpr std::uintptr_t kNativeMask10Getter = 0x241E5AF0u;
inline constexpr std::uintptr_t kMapInitializedGlobal = 0x2433F680u;
inline constexpr std::uintptr_t kEngineClientGlobal = 0x2433F684u;
inline constexpr std::size_t kNativeMaxPlayersInUpdate = 32u;
inline constexpr std::string_view kModEnableFormat = "VModEnable %d";
inline constexpr std::string_view kVoiceBanPrefix = "vban";

// Original 0x241E5AC0 and 0x241E5AF0 subtract 1 from the one-based
// input, take its five low bits as the bit index, and (signed-)shift five
// for the 32-bit word index. The original has NO proven bounds guard;
// this independent reference uses a conservative synthetic input bound.
inline bool bounded_original_player_mask_bit(const std::uint32_t* words,
                                             std::size_t word_count,
                                             std::int32_t player_one_based) noexcept {
    if (!words || player_one_based <= 0) return false; // host-only safety
    const std::uint32_t bit_index = static_cast<std::uint32_t>(player_one_based - 1);
    const std::size_t word = static_cast<std::size_t>(bit_index >> 5);
    if (word >= word_count) return false; // host-only safety
    return (words[word] & (std::uint32_t{1} << (bit_index & 31u))) != 0;
}

// Original UpdateServerState: when the map flag is clear, the code skips
// the normal voice command update. During a level, a forced request or a
// difference between the new enable bit and this+8 triggers VModEnable.
// This only describes one of multiple command branches within the method.
inline constexpr bool should_send_mod_enable(bool map_initialized,
                                             bool force,
                                             bool stored_enable_bit,
                                             bool current_enable_bit) noexcept {
    return map_initialized && (force || stored_enable_bit != current_enable_bit);
}
} // namespace sin_compat::original_client_voice_status