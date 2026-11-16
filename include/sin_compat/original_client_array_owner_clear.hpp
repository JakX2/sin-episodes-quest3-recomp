#pragma once
// Phase246: host-neutral C++17 behavioral model of retail client VA 0x24105FE0.
// The original 39-byte thiscall routine uses owner fields at +0, +4, +8.
// This model intentionally owns its own memory; it never invokes game allocators.
#include <cstdint>
#include <vector>
namespace sin_compat::original_client_array_owner_clear {
inline constexpr std::uint32_t kNativeVA=0x24105FE0u;
inline constexpr unsigned kNativeBytes=39u;
struct Owner {
    std::vector<std::uint8_t> allocation;
    std::int32_t count{};
    std::int32_t guard{};
    unsigned release_events{};
};
// Native guard == -1 skips every write. Otherwise, non-null storage is freed,
// storage pointer is cleared, and count becomes zero. Null storage still clears
// count. release_events is a synthetic observation, not a native owner field.
inline void clear_if_enabled(Owner& owner) {
    if(owner.guard==-1) return;
    if(!owner.allocation.empty()) {
        owner.allocation.clear();
        ++owner.release_events;
    }
    owner.count=0;
}
} // namespace sin_compat::original_client_array_owner_clear
