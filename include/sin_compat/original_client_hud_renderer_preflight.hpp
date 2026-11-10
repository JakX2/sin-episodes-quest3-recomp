#pragma once
// Phase157. Independent C++17 model of the verified entry-stage logic in
// original SiN Episodes: Emergence (2006) client.dll HUD bar renderer
// at 0x24154D50, plus two complete resource-bound getters at
// 0x241417A0 / 0x241417B0.
// Not the native VGUI draw API, original MSVC layout, or complete renderer.
#include <cstdint>
#include <limits>

namespace sin_compat::original_client_hud_render {

// Values read from an engine-owned original x86 render-resource object.
// The original getter at 0x241417A0 returns [+0xAC]-[+0xA8],
// and 0x241417B0 returns [+0xB4]-[+0xB0].
// Preserve signed 32-bit x86 subtraction even for extreme coordinates.
struct ResourceRectangle {
    std::int32_t value_at_a8 = 0;
    std::int32_t value_at_ac = 0;
    std::int32_t value_at_b0 = 0;
    std::int32_t value_at_b4 = 0;
};
inline std::int32_t x86_wrapped_difference(
        std::int32_t right, std::int32_t left) noexcept {
    const auto bits = static_cast<std::uint32_t>(right) -
                      static_cast<std::uint32_t>(left);
    const std::int64_t signed_value = bits < 0x80000000u
        ? static_cast<std::int64_t>(bits)
        : static_cast<std::int64_t>(bits) - 0x100000000LL;
    return static_cast<std::int32_t>(signed_value);
}
inline std::int32_t read_original_resource_dimension_a(
        const ResourceRectangle& rect) noexcept {
    return x86_wrapped_difference(rect.value_at_ac, rect.value_at_a8);
}
inline std::int32_t read_original_resource_dimension_b(
        const ResourceRectangle& rect) noexcept {
    return x86_wrapped_difference(rect.value_at_b4, rect.value_at_b0);
}

// Original entry to the full 0x24154D50 renderer accepts a primary
// resource pointer (arg6), a scalar fraction (arg7), an optional
// secondary resource pointer (arg5), a float denominator (arg4) and
// dispatch mode (arg10). The remaining draw arguments and the engine
// VGUI method at 0x24141DD0 are intentionally not modeled.
//
// Original floating point handling at 0x24154D60..0x24154D94:
// ordered values above 1 clamp to 1; below 0 clamp to 0.
// Unordered NaN remains NaN, rather than being silently clamped.
inline float original_fraction_clamp(float fraction) noexcept {
    if (fraction > 1.0f) return 1.0f;
    if (fraction < 0.0f) return 0.0f;
    return fraction;
}

enum class PaintDisposition : std::uint8_t {
    MissingPrimaryResource, ValidMode, UnsupportedMode
};

struct PreflightResult {
    PaintDisposition disposition = PaintDisposition::MissingPrimaryResource;
    bool secondary_resource_present = false;
    float normalized_fraction = 0.0f;
    float remaining_fraction = 0.0f;
    float scaled_dimension_a = 0.0f;
    float scaled_dimension_b = 0.0f;
    std::uint32_t original_mode = 0;
};

inline PreflightResult prepare_original_hud_bar(
    const ResourceRectangle& rect,
    bool has_primary_resource,
    bool has_secondary_resource,
    float input_fraction,
    std::int32_t original_dimension_global,
    float original_denominator,
    std::uint32_t original_mode) noexcept {
    PreflightResult result{};
    result.original_mode = original_mode;
    if (!has_primary_resource) return result;
    result.normalized_fraction = original_fraction_clamp(input_fraction);
    // Original 0x24154D94: 1.0 - fraction is rounded to a float store.
    result.remaining_fraction = static_cast<float>(
        1.0L - static_cast<long double>(result.normalized_fraction));
    const long double ratio =
        static_cast<long double>(original_dimension_global) /
        static_cast<long double>(original_denominator);
    // The original FILD of both getter results and two FSTP m32 stores
    // preserve separate float32 roundings. Exact x87 last-bit parity on
    // all non-x86 hosts has NOT been proven.
    result.scaled_dimension_a = static_cast<float>(
        static_cast<long double>(read_original_resource_dimension_a(rect)) * ratio);
    result.scaled_dimension_b = static_cast<float>(
        static_cast<long double>(read_original_resource_dimension_b(rect)) * ratio);
    result.secondary_resource_present = has_secondary_resource;
    // The original checks this only after optional secondary drawing
    // at 0x24154EA5. This is a dispatch classification, NOT an execution.
    result.disposition = original_mode <= 3U
        ? PaintDisposition::ValidMode
        : PaintDisposition::UnsupportedMode;
    return result;
}

} // namespace sin_compat::original_client_hud_render
