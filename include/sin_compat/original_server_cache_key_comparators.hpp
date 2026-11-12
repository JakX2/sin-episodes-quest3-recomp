#pragma once
// Phase192: independently recovered original 2006 server.dll comparator pair.
// These helpers reproduce only the verified unsigned first-DWORD predicate,
// not a Source-engine DLL, record serialization, or native pointer ownership.
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace sin_compat::original_server_cache_key_comparators {
inline constexpr std::uint32_t kSceneComparatorVA = 0x221A2F40u;
inline constexpr std::uint32_t kModelSoundsComparatorVA = 0x2213A3F0u;
inline constexpr std::uint32_t kSceneComparatorSlotVA = 0x22503EE4u;
inline constexpr std::uint32_t kModelSoundsComparatorSlotVA = 0x224F038Cu;
inline constexpr std::size_t kComparatorOriginalLength = 17u;

// Only +0 is inspected by each original 17-byte comparator. The remaining
// fields are intentionally opaque and MUST NOT be used to break equalities.
// A complete serializer for the native cache record key is not known.
struct NativeKeyPrefix32 {
    std::uint32_t first_word_00{};
    std::uint32_t opaque_04{};
    std::uint32_t opaque_08{};
    std::uint32_t opaque_0c{};
};
static_assert(sizeof(NativeKeyPrefix32) == 16u, "x86 native key descriptor width");
static_assert(offsetof(NativeKeyPrefix32, first_word_00) == 0u, "comparison word offset");
static_assert(std::is_standard_layout<NativeKeyPrefix32>::value, "native key layout");

// Exact x86 dataflow: read DWORDs from both passed pointers, CMP lhs,rhs,
// SBB EAX,EAX; NEG EAX; RET. This yields 1 for UNSIGNED lhs<rhs, else 0.
// Both original entrypoints contain byte-identical 17-byte implementations.
inline std::uint32_t less_first_dword_unsigned(const NativeKeyPrefix32& lhs,
                                               const NativeKeyPrefix32& rhs) noexcept {
    return lhs.first_word_00 < rhs.first_word_00 ? 1u : 0u;
}
} // namespace sin_compat::original_server_cache_key_comparators
