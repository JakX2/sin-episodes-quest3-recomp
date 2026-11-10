#pragma once
// Phase156: verified original SiN Episodes 2006 client.dll display-coordinate
// helper 0x24154AB0 and global integer getters 0x240FFAD0/0x240FFAE0.
// Portable behavioral reconstruction; NOT a 2006 x86 binary-compatible HUD
// class, native VGUI implementation, or gameplay-rendering engine adapter.
#include <cstdint>

namespace sin_compat::original_client_hud_scale {

// The two native getters simply read distinct 32-bit global integers.
// Naming these original globals "width"/"height" is plausible from callers
// but not required for this functional contract.
struct OriginalViewportDimensions {
    std::int32_t global_at_0x2433fc14 = 0; // getter 0x240FFAD0
    std::int32_t global_at_0x2433fc10 = 0; // getter 0x240FFAE0
};
inline std::int32_t read_original_primary_dimension(
    const OriginalViewportDimensions& dimensions) noexcept {
    return dimensions.global_at_0x2433fc14;
}
inline std::int32_t read_original_secondary_dimension(
    const OriginalViewportDimensions& dimensions) noexcept {
    return dimensions.global_at_0x2433fc10;
}

// The exact original 0x24154AB0 function takes two float* arguments,
// a third argument that the original code does not read, and a float
// denominator. It calls original 0x240FFAD0 (int global), then computes
// the common x87 ratio FILD(global)/denominator, multiplying and
// overwriting each float pointed to in order. The second pointer sees
// the new value if both pointers alias. The caller is responsible for
// live pointers, just as in the original engine.
inline void scale_original_hud_pair(
    float& first, float& second,
    std::int32_t original_global_dimension,
    float original_denominator) noexcept {
    // Preserve extended-precision intermediate in a way that is close to
    // original x87 (platform long double precision still varies). The
    // two independent FSTP m32 stores round the results to float.
    const long double ratio =
        static_cast<long double>(original_global_dimension) /
        static_cast<long double>(original_denominator);
    first = static_cast<float>(static_cast<long double>(first) * ratio);
    second = static_cast<float>(static_cast<long double>(second) * ratio);
}
inline void scale_using_original_primary_dimension(
    float& first, float& second,
    const OriginalViewportDimensions& dimensions,
    float original_denominator) noexcept {
    scale_original_hud_pair(
        first, second, read_original_primary_dimension(dimensions),
        original_denominator);
}

// Exact native client.dll RDATA values at 0x2429B69C/0x2429B6A0.
// At both CHudBreathTimer and CHudPoisonTimer paint callsites, the
// fourth argument is original_ref_1200; the third argument 1600
// is passed but is NOT consumed by the helper's machine code.
inline constexpr float original_reference_canvas_width = 1600.0f;
inline constexpr float original_reference_canvas_height = 1200.0f;

} // namespace sin_compat::original_client_hud_scale
