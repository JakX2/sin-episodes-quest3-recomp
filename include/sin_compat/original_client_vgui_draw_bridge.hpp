#pragma once
// Phase 158: independent behavioral reconstruction of two original 2006
// SiN Episodes client.dll x86 functions: complete 0x241417C0 draw adapter,
// and verifiable decision/geometry paths of 0x24141DD0.
// This is not the original MSVC instance layout, VGUI API, or a renderer.
#include "sin_compat/original_client_hud_renderer_preflight.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_vgui_draw {
using sin_compat::original_client_hud_render::ResourceRectangle;
using sin_compat::original_client_hud_render::read_original_resource_dimension_a;
using sin_compat::original_client_hud_render::read_original_resource_dimension_b;
using sin_compat::original_client_hud_render::x86_wrapped_difference;

// The original 0x241417C0 uses seven raw 32-bit stack arguments, inserts
// two 32-bit resource rectangle extents at positions 3/4, and invokes
// 0x24141DD0 with NINE raw words. This wrapper is fully recovered.
// Words preserve original float/pointer bit patterns without interpreting
// potentially engine-owned pointers or inventing argument type names.
using OriginalSevenWords = std::array<std::uint32_t, 7>;
using OriginalNineWords = std::array<std::uint32_t, 9>;
inline OriginalNineWords expand_original_draw_arguments(
    const ResourceRectangle& rectangle,
    const OriginalSevenWords& original_words) noexcept {
    return {{
        original_words[0], original_words[1],
        static_cast<std::uint32_t>(
            read_original_resource_dimension_a(rectangle)),
        static_cast<std::uint32_t>(
            read_original_resource_dimension_b(rectangle)),
        original_words[2], original_words[3], original_words[4],
        original_words[5], original_words[6]
    }};
}

// The original rendering worker at 0x24141DD0 selects two very different
// original VGUI paths:
// +0x84 != 0: surface clip/vertex path, even if texture id == -1;
// +0x84 == 0 && texture id == -1: immediate return;
// +0x84 == 0 && texture id != -1: textured subrectangle path.
enum class OriginalDrawRoute : std::uint8_t {
    SurfaceClip,
    NoValidTexture,
    TexturedSubrectangle
};
inline OriginalDrawRoute choose_original_draw_route(
    std::uint8_t field_at_0x84,
    std::int32_t texture_id_at_0x94) noexcept {
    if (field_at_0x84 != 0U) return OriginalDrawRoute::SurfaceClip;
    if (texture_id_at_0x94 == -1) return OriginalDrawRoute::NoValidTexture;
    return OriginalDrawRoute::TexturedSubrectangle;
}

inline std::int32_t original_i32_word(std::uint32_t bits) noexcept {
    return bits <= 0x7fffffffU
        ? static_cast<std::int32_t>(bits)
        : static_cast<std::int32_t>(
              static_cast<std::int64_t>(bits) - 0x100000000LL);
}
inline std::int32_t x86_wrapped_add(
    std::int32_t first,std::int32_t second) noexcept {
    const auto bits = static_cast<std::uint32_t>(first) +
                      static_cast<std::uint32_t>(second);
    return original_i32_word(bits);
}

// The native clipped surface path calls VGUI surface vtable +0xFC for an
// integer limit, computes FILD(int32(limit - caller_arg8))/FIDIV(limit),
// stores the ratio as float32, then calls virtual +0x18C to obtain a
// vertex pair and determine if drawing may continue.
// We model the post-clip x87 geometric changes, NOT the VGUI calls.
struct OriginalClipVertices {
    float first_a = 0.0f;   // original temporary vertex at stack +0x40
    float first_b = 0.0f;   // at +0x48
    float second_a = 0.0f;  // at +0x50
    float second_b = 0.0f;  // at +0x58
};
struct OriginalClipPlan {
    bool clip_accepted = false;
    bool should_issue_native_surface_draw = false;
    bool first_vertex_adjusted = false;
    bool second_vertex_adjusted = false;
    float original_ratio = 0.0f;
    OriginalClipVertices vertices{};
};
inline float original_surface_clip_ratio(
    std::int32_t virtual_surface_limit,
    std::int32_t original_caller_arg8) noexcept {
    const auto signed_numerator = x86_wrapped_difference(
        virtual_surface_limit, original_caller_arg8);
    // Original x87 FILD/FIDIV/FSTP stores this particular ratio to f32.
    return static_cast<float>(
        static_cast<long double>(signed_numerator) /
        static_cast<long double>(virtual_surface_limit));
}
inline float original_x87_lerp_store(
    float a,float b,long double ratio) noexcept {
    return static_cast<float>(
        static_cast<long double>(b - a) * ratio +
        static_cast<long double>(a));
}
inline OriginalClipPlan plan_original_surface_clip(
    OriginalClipVertices vertices,
    std::int32_t virtual_surface_limit,
    std::int32_t original_caller_arg8,
    std::uint32_t original_caller_arg6,
    bool native_clip_callback_accepted) noexcept {
    OriginalClipPlan result{};
    result.vertices = vertices;
    result.original_ratio = original_surface_clip_ratio(
        virtual_surface_limit,original_caller_arg8);
    result.clip_accepted = native_clip_callback_accepted;
    if (!native_clip_callback_accepted) return result;
    if (original_caller_arg6 != 0U) {
        // Native 0x24141E9D: update first pair toward second pair.
        result.vertices.first_a = original_x87_lerp_store(
            vertices.first_a,vertices.second_a,result.original_ratio);
        result.vertices.first_b = original_x87_lerp_store(
            vertices.first_b,vertices.second_b,result.original_ratio);
        result.first_vertex_adjusted = true;
    } else if (original_caller_arg8 != virtual_surface_limit) {
        // Native 0x24141ED1: update second pair with complement ratio.
        const long double remaining =
            1.0L - static_cast<long double>(result.original_ratio);
        result.vertices.second_a = original_x87_lerp_store(
            vertices.first_a,vertices.second_a,remaining);
        result.vertices.second_b = original_x87_lerp_store(
            vertices.first_b,vertices.second_b,remaining);
        result.second_vertex_adjusted = true;
    }
    // Original 0x24141F12 calls VGUI virtual +0x190 only after accepted
    // clipping. This flag is a *plan*, not proof of visible pixels.
    result.should_issue_native_surface_draw = true;
    return result;
}

// The non-clipped branch at 0x24141F22 requires texture id != -1.
// Native 0x24141F2F..0x2414201E uses the original rectangle's integer
// dimensions and the four float values at +0x98,+0x9C,+0xA0,+0xA4
// to calculate cropped UVs, then invokes original VGUI slot +0x178.
// This pure reference does NOT call it. No zero-extent guard exists in
// the native code, so unordered/Inf results are retained.
struct OriginalResourceUvBounds {
    float a98 = 0.0f; // initial U
    float a9c = 0.0f; // initial V
    float aa0 = 1.0f; // final U
    float aa4 = 1.0f; // final V
};
struct OriginalSubrectangleUvs {
    float first_u = 0.0f;
    float first_v = 0.0f;
    float second_u = 0.0f;
    float second_v = 0.0f;
    bool native_texture_draw_pending = false;
    bool chained_resource_draw_pending = false;
};
inline OriginalSubrectangleUvs prepare_original_texture_subrectangle(
    const ResourceRectangle& rectangle,
    const OriginalResourceUvBounds& uv,
    std::int32_t offset_a,
    std::int32_t offset_b,
    std::int32_t span_a,
    std::int32_t span_b,
    bool has_chained_resource) noexcept {
    const auto resource_width = read_original_resource_dimension_a(rectangle);
    const auto resource_height = read_original_resource_dimension_b(rectangle);

    // Original FSTP m32 rounds the first reciprocal and both UV spans;
    // the second reciprocal stays in x87 extended precision.
    const float inv_width = static_cast<float>(
        1.0L / static_cast<long double>(resource_width));
    const long double inv_height =
        1.0L / static_cast<long double>(resource_height);
    const float u_span = static_cast<float>(
        static_cast<long double>(uv.aa0) - static_cast<long double>(uv.a98));
    const float v_span = static_cast<float>(
        static_cast<long double>(uv.aa4) - static_cast<long double>(uv.a9c));
    auto u_at = [&](std::int32_t x) noexcept {
        return static_cast<float>(
            static_cast<long double>(x) * inv_width * u_span +
            static_cast<long double>(uv.a98));
    };
    auto v_at = [&](std::int32_t y) noexcept {
        return static_cast<float>(
            static_cast<long double>(y) * inv_height * v_span +
            static_cast<long double>(uv.a9c));
    };
    return {
        u_at(offset_a), v_at(offset_b),
        u_at(x86_wrapped_add(offset_a,span_a)),
        v_at(x86_wrapped_add(offset_b,span_b)),
        true,has_chained_resource
    };
}
} // namespace sin_compat::original_client_vgui_draw
