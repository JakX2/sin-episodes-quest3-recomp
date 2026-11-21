#pragma once
// Source-grounded, original-free 2006 client VClient011 lifecycle decisions.
// This is a bounded C++17 reference for unit tests, NOT the engine interface,
// no original game's C++ sources or proprietary runtime code are included.
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace sin_compat::original_client_level_lifecycle {
inline constexpr std::uintptr_t kClientVtable = 0x24296C6Cu;
inline constexpr std::uintptr_t kClientShutdown = 0x240FF3D0u;
inline constexpr std::uintptr_t kPreEntity = 0x240FDB40u;
inline constexpr std::uintptr_t kPostEntity = 0x240FDC00u;
inline constexpr std::uintptr_t kLevelShutdown = 0x240FF4A0u;
inline constexpr std::uintptr_t kMapActiveOriginalAddress = 0x2433F680u;
inline constexpr std::uintptr_t kCommandInterfaceOriginalAddress = 0x2433F684u;
inline constexpr std::size_t kClientCallbackArrayOffset = 4u;
inline constexpr std::size_t kClientCallbackCountOffset = 16u;
inline constexpr std::size_t kClientCallbackVtableByteOffset = 0x34u;

// Original client virtual slot +8 @ 0x240FDB40:
//   if (engineGlobal+0x14 > 1) {
//      if (!(engineGlobal->vslot+0x148)()) command = "cl_predict 1";
//      else command = "cl_predict 0";
//   } else command="cl_predict 0";
// The meaning of engineGlobal+0x14 and virtual+0x148 is NOT identified.
inline constexpr std::string_view prediction_command(std::int32_t native_field_0x14,
                                                     bool native_virtual_0x148_result) noexcept {
    return (native_field_0x14 > 1 && !native_virtual_0x148_result)
               ? std::string_view("cl_predict 1")
               : std::string_view("cl_predict 0");
}

// The same original global byte gates level start (+8) and level shutdown
// (+0x10). This captures only the short-circuit and flag mutation, not the
// complex unmodeled engine calls that follow the original guard.
inline bool enter_level_once(bool& native_active_flag) noexcept {
    if (native_active_flag) return false;
    native_active_flag = true;
    return true;
}
inline bool exit_level_once(bool& native_active_flag) noexcept {
    if (!native_active_flag) return false;
    native_active_flag = false;
    return true;
}

// Original whole shutdown and level shutdown both walk CHLClient-owned callbacks
// in reverse, using count owner+0x10 and data pointer owner+0x04, dispatch
// native virtual byte+0x34 per element, then clear count. These must NOT
// be assumed to be identical to the separate global AutoGameSystem list at
// 0x242FBFA8. Deliberately no
// native calling convention or object pointer casts in this portable model.
template<class Callback>
void for_each_client_callback_reverse(std::size_t& active_count, Callback&& callback) {
    while (active_count) {
        --active_count;
        std::forward<Callback>(callback)(active_count);
    }
}
} // namespace sin_compat::original_client_level_lifecycle