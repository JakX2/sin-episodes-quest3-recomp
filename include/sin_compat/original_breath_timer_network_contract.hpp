#pragma once
// Original 2006 SiN Episodes PE32 DT_HL2Local.m_breathTimer evidence.
// These are the *original Windows 32-bit* networked object offsets,
// not the layout to use in recompiled Android/Quest modules.
#include "sin_compat/original_poison_timer_network_contract.hpp"
#include <cstddef>
#include <string_view>

namespace sin_compat::original_breath_timer_network {
inline constexpr std::string_view kTable = "DT_HL2Local";
inline constexpr std::string_view kProperty = "m_breathTimer";
inline constexpr std::size_t kNestedOffset = 0x28U;
inline constexpr std::size_t kClientPlayerOffset =
    sin_compat::original_poison_timer_network::kOriginalClientPlayerLocalOffset
    + kNestedOffset;
inline constexpr std::size_t kServerPlayerOffset =
    sin_compat::original_poison_timer_network::kOriginalServerPlayerLocalOffset
    + kNestedOffset;
static_assert(kClientPlayerOffset == 0x11F0U,
              "original CHudBreathTimer client read must be at 0x11F0");
static_assert(kServerPlayerOffset == 0x13A4U,
              "original server SendProp must be at 0x13A4");
static_assert(kClientPlayerOffset != kServerPlayerOffset,
              "never reinterpret server entity memory as client layout");
} // namespace sin_compat::original_breath_timer_network
