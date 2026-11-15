#pragma once
// Phase243 independent C++17 reference for original SiN Episodes (2006)
// client PVS map comparator, global thunk and key-composing adapter.
// Native: 0x241056C0, 0x24105CF0, 0x24105760, 0x24108800.
// This is NOT a real engine-owned x86 client object or gameplay DLL.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "sin_compat/original_client_pvs_map_frontends.hpp"

namespace sin_compat::original_client_pvs_comparator_dispatch {

namespace map = sin_compat::original_client_pvs_map_frontends;
inline constexpr std::uint32_t kUnsignedComparatorVA=0x241056C0u;
inline constexpr std::uint32_t kGlobalThunkVA=0x24105CF0u;
inline constexpr std::uint32_t kGlobalGetSetVA=0x24105760u;
inline constexpr std::uint32_t kKeyInsertionAdapterVA=0x24108800u;
inline constexpr std::uint32_t kNativeGlobalPointerVA=0x2433FDB8u;
inline constexpr std::size_t kNewOriginalMethods=4u;
inline constexpr std::size_t kWholeOriginalCodeBytes=97u;
inline constexpr std::size_t kOwnerEmbeddedMapOffset=0x18060u;
inline constexpr std::size_t kMapThunkDword=0x00u;
inline constexpr std::size_t kMapCandidateDword=0x1Cu;
inline constexpr std::size_t kKeyStoredBytes=8u;
inline constexpr std::size_t kNativeKeyInitializedBytes=6u;
inline constexpr std::size_t kNativeKeyUninitializedBytes=2u;

inline std::uint32_t first_dword(const map::Payload& value) noexcept {
    return static_cast<std::uint32_t>(value[0]) |
        (static_cast<std::uint32_t>(value[1])<<8) |
        (static_cast<std::uint32_t>(value[2])<<16) |
        (static_cast<std::uint32_t>(value[3])<<24);
}

// The whole original 17-byte function performs unsigned CMP DWORD,
// SBB EAX,EAX, NEG EAX and returns exactly 0 or 1.
// It DOES NOT compare the second DWORD, including the notifier halfword.
struct UnsignedFirstDwordComparator final:map::Comparator {
    bool less(const map::Payload& a,
              const map::Payload& b) const override {
        return first_dword(a)<first_dword(b);
    }
};

// Native 0x24105760: nonzero caller pointer updates 0x2433FDB8 and
// returns it; zero retrieves that global without changing it.
// Original 0x24105CF0 tail-JMPs through this absolute pointer.
// Host records an x86 VA/token, not an executable native function pointer.
struct GlobalComparatorPointer {
    std::uint32_t target{};
    std::size_t writes{};
    std::uint32_t native_set_or_get(std::uint32_t input) noexcept {
        if(input!=0u) {
            target=input;
            ++writes;
        }
        return target;
    }
};

// Only the owner-original registered comparator 0x241056C0 is modeled.
// The native FF25 thunk could jump to any pointer in its global, which
// is deliberately NOT executable in this host reference.
struct ComparatorThroughThunk final:map::Comparator {
    const GlobalComparatorPointer& global;
    explicit ComparatorThroughThunk(const GlobalComparatorPointer& state):
        global(state){}
    bool less(const map::Payload& a,
              const map::Payload& b) const override {
        if(global.target!=kUnsignedComparatorVA)
            throw std::runtime_error("unresolved original PVS comparator global target");
        return UnsignedFirstDwordComparator{}.less(a,b);
    }
};

// Original adapter 0x24108800 reserves 8 uninitialized native stack
// bytes but writes only a DWORD from first pointer and a WORD from the
// second pointer, then calls map insert 0x24108750 with the key pointer.
// The upper TWO bytes of that temporary key are never written by the
// method. We accept those bytes EXPLICITLY rather than inventing zeroes.
inline map::Payload key_from_native_adapter(
    std::uint32_t identity_dword,
    std::uint16_t notifier_word,
    const std::array<std::uint8_t,2u>& untouched_stack_high_bytes) {
    map::Payload key{};
    key[0]=static_cast<std::uint8_t>(identity_dword);
    key[1]=static_cast<std::uint8_t>(identity_dword>>8);
    key[2]=static_cast<std::uint8_t>(identity_dword>>16);
    key[3]=static_cast<std::uint8_t>(identity_dword>>24);
    key[4]=static_cast<std::uint8_t>(notifier_word);
    key[5]=static_cast<std::uint8_t>(notifier_word>>8);
    key[6]=untouched_stack_high_bytes[0];
    key[7]=untouched_stack_high_bytes[1];
    return key;
}

// Models native adapter's parent map +0x1C candidate global write (if
// nonzero), PUSH 0 and delegated insertion. The underlying native map
// insert, placement, allocation, balancing are the previously recovered
// independent host reference/callbacks, not actual 32-bit game pointers.
inline std::uint16_t insert_with_native_adapter(
    map::Map& manager,
    std::uint32_t identity_dword,
    std::uint16_t notifier_word,
    const std::array<std::uint8_t,2u>& untouched_stack_high_bytes,
    std::uint32_t owner_map_comparator_candidate,
    GlobalComparatorPointer& global,
    map::InsertionBackend& insertion) {
    if(owner_map_comparator_candidate!=0u)
        (void)global.native_set_or_get(owner_map_comparator_candidate);
    return map::insert(manager,
        key_from_native_adapter(identity_dword,notifier_word,
                                untouched_stack_high_bytes),
        insertion);
}

} // namespace sin_compat::original_client_pvs_comparator_dispatch
