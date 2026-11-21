#pragma once
// Phase546: original 2006 client.dll string-token comparator decisions.
// THIS IS A PORTABLE, OWNER-FREE REFERENCE, NOT THE ORIGINAL DLL ABI.
// The real key parameter is a two-word token/page descriptor; the native
// resolver maps that to a C string. We model only already resolved strings.
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_client_token_compare {

enum class ComparatorKind : std::uint8_t {
    BytewiseCaseSensitive, // native callback 0x241D3BF0
    CaseFolded             // native callback 0x241D3C90
};

// Direct 2006 static initialization at original 0x24262410 initializes
// the token subobject at global converter registry +0x24 with flag=1.
// Base constructor 0x241D3D90 clears its callback first; the resulting
// default pointer is the original folded comparator, unless replaced later.
inline constexpr std::uint32_t kHudConverterTokenTableVA=0x2432289C;
inline constexpr std::uint32_t kHudConverterTokenTableInitializerVA=0x24262410;
inline constexpr ComparatorKind kOriginalHudConverterInitialComparator=ComparatorKind::CaseFolded;

// Exact original selection at constructor 0x241D4640. When an earlier
// initializer already set callback this+0x0, do not overwrite it.
constexpr bool constructor_installs_callback(bool previously_set) noexcept {
    return !previously_set;
}
constexpr ComparatorKind constructor_selection(bool argument_flag_nonzero) noexcept {
    return argument_flag_nonzero ? ComparatorKind::CaseFolded
                                 : ComparatorKind::BytewiseCaseSensitive;
}

// The original compares *unsigned bytes* using a two-at-a-time loop, and
// stops when it reaches the first C NUL. std::string_view is the owner-free
// test transport; an embedded NUL still ends the comparison.
inline bool less_unsigned_c_string(std::string_view left,
                                   std::string_view right) noexcept {
    std::size_t i=0;
    for (;;) {
        const std::uint8_t a = i < left.size() ?
            static_cast<std::uint8_t>(left[i]) : std::uint8_t{0};
        const std::uint8_t b = i < right.size() ?
            static_cast<std::uint8_t>(right[i]) : std::uint8_t{0};
        if (a != b) return a < b;
        if (a == 0) return false;
        ++i;
    }
}

// The original folded comparator calls 0x2425F81B, whose *locale-not-ready*
// fallback at 0x24261C60 folds only ASCII 'A'..'Z'.  When the original
// locale-ready flag is nonzero it calls a distinct CRT-style casefold helper
// 0x2424C373 for each byte. Its locale-dependent results are outside this
// helper's scope and must be provided by a host with verified locale state.
constexpr std::uint8_t ascii_fold(std::uint8_t b) noexcept {
    return b >= 'A' && b <= 'Z' ? static_cast<std::uint8_t>(b+0x20) : b;
}
inline bool less_ascii_folded_c_string(std::string_view left,
                                       std::string_view right) noexcept {
    std::size_t i=0;
    for (;;) {
        const std::uint8_t a = ascii_fold(i<left.size() ?
            static_cast<std::uint8_t>(left[i]) : std::uint8_t{0});
        const std::uint8_t b = ascii_fold(i<right.size() ?
            static_cast<std::uint8_t>(right[i]) : std::uint8_t{0});
        if (a != b) return a < b;
        if (a == 0) return false;
        ++i;
    }
}

// The native two-word handle is not a pointer and has its own page mapping.
// Capture widths/sentinel without claiming the runtime page array layout.
struct OriginalKeyDescriptor {
    std::uint16_t page_or_sentinel;
    std::uint16_t byte_offset_or_sentinel;
};
static_assert(sizeof(OriginalKeyDescriptor)==4, "native token key is two uint16 values");
constexpr bool uses_inline_string(OriginalKeyDescriptor key) noexcept {
    return key.page_or_sentinel == 0xFFFF &&
           key.byte_offset_or_sentinel == 0xFFFF;
}

} // namespace sin_compat::original_client_token_compare