#pragma once
// Phase201: host-neutral reference to the original 2006 SiN server.dll
// scene/model-sounds 24-byte node-vector grow and 16-byte key initialization.
// This models verified x86 branches and layout, not the original heap ABI.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace sin_compat::original_server_cache_array_growth {
inline constexpr std::uint32_t kSceneGrowVA = 0x221A3A10u;
inline constexpr std::uint32_t kModelSoundsGrowVA = 0x2213AAC0u;
inline constexpr std::uint32_t kSceneInitVA = 0x221A3A90u;
inline constexpr std::uint32_t kModelSoundsInitVA = 0x2213AB40u;
inline constexpr std::uint32_t kOriginalMallocVA = 0x2226E390u;
inline constexpr std::uint32_t kOriginalReallocVA = 0x2226E400u;
inline constexpr std::uint32_t kNodeStride = 24u;
inline constexpr std::uint32_t kDisabledPolicy = 0xffffffffu;

// Original grow method receives ECX = tree + 4, not the complete tree!
// So its ECX+0/4/8 are original tree+4/+8/+0c respectively.
struct NativeVectorHeader32 {
    std::uint32_t backing_00{};
    std::uint32_t capacity_04{};
    std::uint32_t growth_step_08{}; // -1 disables, 0 doubles, else adds step
};
static_assert(sizeof(NativeVectorHeader32)==12u);
static_assert(offsetof(NativeVectorHeader32, backing_00)==0u);
static_assert(offsetof(NativeVectorHeader32, capacity_04)==4u);
static_assert(offsetof(NativeVectorHeader32, growth_step_08)==8u);
static_assert(std::is_standard_layout<NativeVectorHeader32>::value);

struct NativeKeyInitializer32 {
    std::uint32_t word_00{}, word_04{}, word_08{}, word_0c{};
};
static_assert(sizeof(NativeKeyInitializer32)==16u);
// Original code reads the incoming pointer, and, if non-null, writes these
// exact four DWORD values. Both 36-byte initializer functions are identical.
inline void initialize_key(NativeKeyInitializer32* key) noexcept {
    if (key == nullptr) return;
    key->word_00=0u;
    key->word_04=0u;
    key->word_08=0xfffffffeu;
    key->word_0c=0xffffffffu;
}

enum class Allocation { skipped_disabled, allocated, reallocated };
struct Result { Allocation action; std::uint32_t bytes; std::uint32_t capacity; };
// Host: uint32_t allocate(uint32_t bytes); uint32_t resize(uint32_t backing,
// uint32_t bytes). Returned pointer is a synthetic 32-bit address/token.
// Native reads its *own* +8 growth policy and branches before touching the
// allocator. It tests old_capacity < old_capacity + additional as a SIGNED
// comparison, repeats +growth_step (if positive) or doubling (if zero),
// starting with capacity=2 if empty; then ALWAYS calls malloc/realloc even
// if capacity was already sufficient. Native does not validate allocation.
// Host-only overflow and iteration guards avoid corrupt/overflow scenarios.
template<class Host>
Result grow(NativeVectorHeader32& a, std::uint32_t additional, Host& host) {
    if (a.growth_step_08 == kDisabledPolicy)
        return {Allocation::skipped_disabled,0u,a.capacity_04};
    if (a.capacity_04 > static_cast<std::uint32_t>(INT32_MAX) ||
        additional > static_cast<std::uint32_t>(INT32_MAX) - a.capacity_04 ||
        (a.growth_step_08 != 0u && a.growth_step_08 > static_cast<std::uint32_t>(INT32_MAX)))
        throw std::overflow_error("host-only native array signed-capacity guard");
    const auto needed = a.capacity_04+additional;
    std::uint32_t cap = a.capacity_04;
    std::size_t fuel = 100000u;
    while (cap < needed) {
        if (fuel-- == 0u) throw std::length_error("host-only native growth loop guard");
        if (cap == 0u) cap=2u;
        else if (a.growth_step_08 == 0u) {
            if (cap > static_cast<std::uint32_t>(INT32_MAX)/2u)
                throw std::overflow_error("host-only doubling guard");
            cap *= 2u;
        } else {
            if (a.growth_step_08 > static_cast<std::uint32_t>(INT32_MAX) - cap)
                throw std::overflow_error("host-only growth-step guard");
            cap += a.growth_step_08;
        }
    }
    if (cap > static_cast<std::uint32_t>(INT32_MAX)/kNodeStride)
        throw std::overflow_error("host-only byte-count guard");
    const auto bytes = cap*kNodeStride;
    const bool old = a.backing_00 != 0u;
    // Native writes capacity BEFORE calling the allocation function.
    a.capacity_04 = cap;
    a.backing_00 = old ? host.resize(a.backing_00,bytes)
                        : host.allocate(bytes);
    return {old ? Allocation::reallocated : Allocation::allocated,bytes,cap};
}
} // namespace sin_compat::original_server_cache_array_growth
