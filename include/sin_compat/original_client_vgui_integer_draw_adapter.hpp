#pragma once
// Phase159: original 2006 SiN Episodes client.dll 0x241420D0 x87 draw adapter.
// Independent behavior only: an original nine-word native draw CALL is
// represented as raw words; there is no VGUI engine, ABI or actual drawing.
#include "sin_compat/original_client_vgui_draw_bridge.hpp"
#include <cmath>
#include <cstdint>
#include <limits>

namespace sin_compat::original_client_vgui_integer_draw {
using sin_compat::original_client_vgui_draw::OriginalNineWords;
using sin_compat::original_client_vgui_draw::ResourceRectangle;
using sin_compat::original_client_vgui_draw::read_original_resource_dimension_a;
using sin_compat::original_client_vgui_draw::read_original_resource_dimension_b;

// FISTP with the original OR AH,0x0C control-word override: truncate
// toward zero to signed 32 bits. For masked x87 invalid conversions,
// actual 2006 hardware writes integer-indefinite 0x80000000. C++ casts
// outside representable range are undefined, so this is explicit.
inline std::int32_t original_x87_fistp_i32(long double number) noexcept {
    constexpr auto lower = -2147483648.0L;
    constexpr auto upper_exclusive = 2147483648.0L;
    if (!std::isfinite(number) || number < lower ||
        number >= upper_exclusive)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(std::trunc(number));
}

struct OriginalFiveDrawArgs {
    float first_coordinate = 0.0f;       // original arg1, converted to int
    float second_coordinate = 0.0f;      // original arg2, converted to int
    std::uint32_t unused_third_word = 0; // original arg3, never read
    float reference_denominator = 1.0f;  // original arg4, used by FDIV
    std::uint32_t opaque_fifth_word = 0; // original arg5, passed unchanged
};

// Native function 0x241420D0 performs these exact steps:
// 1. signed rectangle extents from +A8/+AC, +B0/+B4;
// 2. FILD each extent followed by FSTP float32 (important above 2^24);
// 3. CALL secondary dimension getter 0x240FFAE0 (result discarded), then
//    CALL primary getter 0x240FFAD0 (int32 @0x2433FC14);
// 4. x87 ratio = FILD(primary_int) / original float32 arg4;
// 5. FISTP truncates two extents and two scaled extents, then arg2/arg1;
// 6. CALL 0x24141DD0 with nine stack words, RET 0x14.
// Getter callbacks preserve call order even though secondary value unused.
// The returned word sequence is the CALL input; no engine method is invoked.
template<typename ReadSecondary,typename ReadPrimary>
OriginalNineWords prepare_original_integer_draw_words(
    const ResourceRectangle& resource,
    const OriginalFiveDrawArgs& input,
    ReadSecondary&& read_secondary,
    ReadPrimary&& read_primary) {
    const float extent_a = static_cast<float>(
        read_original_resource_dimension_a(resource));
    const float extent_b = static_cast<float>(
        read_original_resource_dimension_b(resource));
    (void) read_secondary();
    const std::int32_t primary = static_cast<std::int32_t>(read_primary());
    const long double ratio = static_cast<long double>(primary) /
                              static_cast<long double>(input.reference_denominator);
    // Each of these FISTP operations has temporary x87 round-toward-zero.
    const auto extent_a_i32 = original_x87_fistp_i32(extent_a);
    const auto extent_b_i32 = original_x87_fistp_i32(extent_b);
    const auto scaled_a_i32 = original_x87_fistp_i32(
        static_cast<long double>(extent_a) * ratio);
    const auto scaled_b_i32 = original_x87_fistp_i32(
        static_cast<long double>(extent_b) * ratio);
    const auto coord1 = original_x87_fistp_i32(input.first_coordinate);
    const auto coord2 = original_x87_fistp_i32(input.second_coordinate);
    return {{
        static_cast<std::uint32_t>(coord1),
        static_cast<std::uint32_t>(coord2),
        static_cast<std::uint32_t>(scaled_a_i32),
        static_cast<std::uint32_t>(scaled_b_i32),
        0U, 0U,
        static_cast<std::uint32_t>(extent_a_i32),
        static_cast<std::uint32_t>(extent_b_i32),
        input.opaque_fifth_word,
    }};
}
} // namespace sin_compat::original_client_vgui_integer_draw
