#pragma once
// Original SiN Episodes (2006) verified nested network property crosswalk.
// These are original Windows PE32 offsets, NOT C++ struct definitions and
// NOT valid offsets in a future ARM64 Android / Quest game module.
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_poison_timer_network {
inline constexpr std::string_view kTable = "DT_HL2Local";
inline constexpr std::string_view kParentProperty = "m_HL2Local";
inline constexpr std::string_view kNestedProperty = "m_poisonTimer";

inline constexpr std::size_t kOriginalClientPlayerLocalOffset = 0x11C8U;
inline constexpr std::size_t kOriginalServerPlayerLocalOffset = 0x137CU;
inline constexpr std::size_t kOriginalNestedPoisonTimerOffset = 0x24U;
inline constexpr std::size_t kOriginalClientPlayerPoisonTimerOffset =
    kOriginalClientPlayerLocalOffset + kOriginalNestedPoisonTimerOffset;
inline constexpr std::size_t kOriginalServerPlayerPoisonTimerOffset =
    kOriginalServerPlayerLocalOffset + kOriginalNestedPoisonTimerOffset;

static_assert(kOriginalClientPlayerPoisonTimerOffset == 0x11ECU,
              "Must match original CHudPoisonTimer local-player read");
static_assert(kOriginalServerPlayerPoisonTimerOffset == 0x13A0U,
              "Must match original server DT_HL2Local send property");
static_assert(kOriginalClientPlayerPoisonTimerOffset !=
                  kOriginalServerPlayerPoisonTimerOffset,
              "Never reuse original client entity offsets on server");

struct OriginalX86PoisonTimerField {
    std::string_view network_table = kTable;
    std::string_view parent_property = kParentProperty;
    std::string_view field_property = kNestedProperty;
    std::size_t local_offset = 0;
    std::size_t nested_offset = kOriginalNestedPoisonTimerOffset;
    std::size_t absolute_player_offset = 0;
};

inline constexpr OriginalX86PoisonTimerField original_client_field() noexcept {
    return {kTable,kParentProperty,kNestedProperty,
            kOriginalClientPlayerLocalOffset,kOriginalNestedPoisonTimerOffset,
            kOriginalClientPlayerPoisonTimerOffset};
}
inline constexpr OriginalX86PoisonTimerField original_server_field() noexcept {
    return {kTable,kParentProperty,kNestedProperty,
            kOriginalServerPlayerLocalOffset,kOriginalNestedPoisonTimerOffset,
            kOriginalServerPlayerPoisonTimerOffset};
}
} // namespace sin_compat::original_poison_timer_network
