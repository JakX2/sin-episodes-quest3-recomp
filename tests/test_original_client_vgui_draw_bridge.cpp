#include "sin_compat/original_client_vgui_draw_bridge.hpp"
#include "sin_compat/original_client_hud_renderer_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

namespace bridge = sin_compat::original_client_vgui_draw;
namespace hud = sin_compat::original_client_hud_render;
namespace {
bool near(float a,float b,float tolerance=0.00001f) {
    return std::fabs(a-b)<=tolerance;
}
void test_exact_original_seven_to_nine_arg_wrapper() {
    hud::ResourceRectangle rect{10,210,20,420};
    const bridge::OriginalSevenWords input{{
        11U,22U,33U,44U,55U,66U,0x7fc01234U
    }};
    const auto words=bridge::expand_original_draw_arguments(rect,input);
    assert((words==bridge::OriginalNineWords{{
        11U,22U,200U,400U,33U,44U,55U,66U,0x7fc01234U
    }}));
    // Original modulo-32-bit differences, including overflow in native SUB.
    rect.value_at_a8=std::numeric_limits<std::int32_t>::max();
    rect.value_at_ac=std::numeric_limits<std::int32_t>::min();
    const auto wrapped=bridge::expand_original_draw_arguments(rect,input);
    assert(wrapped[2]==1U);
    assert(wrapped[8]==input[6]);
}
void test_original_native_branch_order() {
    assert(bridge::choose_original_draw_route(1,-1)==
        bridge::OriginalDrawRoute::SurfaceClip);
    assert(bridge::choose_original_draw_route(255,-1)==
        bridge::OriginalDrawRoute::SurfaceClip);
    assert(bridge::choose_original_draw_route(0,-1)==
        bridge::OriginalDrawRoute::NoValidTexture);
    assert(bridge::choose_original_draw_route(0,0)==
        bridge::OriginalDrawRoute::TexturedSubrectangle);
    assert(bridge::choose_original_draw_route(0,-2)==
        bridge::OriginalDrawRoute::TexturedSubrectangle);
}
void test_original_clip_ratio_and_vertex_fallback_order() {
    const bridge::OriginalClipVertices original{0.0f,10.0f,100.0f,210.0f};
    assert(bridge::original_surface_clip_ratio(100,20)==0.8f);
    const auto no_clip=bridge::plan_original_surface_clip(
        original,100,20,1U,false);
    assert(!no_clip.clip_accepted && !no_clip.should_issue_native_surface_draw);
    assert(!no_clip.first_vertex_adjusted && !no_clip.second_vertex_adjusted);
    assert(no_clip.vertices.first_a==0.0f &&
           no_clip.vertices.second_b==210.0f);

    const auto first=bridge::plan_original_surface_clip(
        original,100,20,1U,true);
    assert(first.clip_accepted && first.should_issue_native_surface_draw);
    assert(first.first_vertex_adjusted && !first.second_vertex_adjusted);
    assert(near(first.vertices.first_a,80.0f));
    assert(near(first.vertices.first_b,170.0f));
    assert(first.vertices.second_a==100.0f);
    assert(first.vertices.second_b==210.0f);

    const auto second=bridge::plan_original_surface_clip(
        original,100,20,0U,true);
    assert(!second.first_vertex_adjusted && second.second_vertex_adjusted);
    assert(second.vertices.first_a==0.0f &&
           second.vertices.first_b==10.0f);
    assert(near(second.vertices.second_a,20.0f));
    assert(near(second.vertices.second_b,50.0f));

    const auto no_adjust=bridge::plan_original_surface_clip(
        original,100,100,0U,true);
    assert(no_adjust.should_issue_native_surface_draw);
    assert(!no_adjust.first_vertex_adjusted && !no_adjust.second_vertex_adjusted);
    assert(no_adjust.vertices.second_a==100.0f);
}
void test_original_clip_negative_nan_and_wrapping() {
    const bridge::OriginalClipVertices v{0.0f,0.0f,100.0f,200.0f};
    auto negative=bridge::plan_original_surface_clip(v,100,120,1U,true);
    assert(near(negative.original_ratio,-0.2f));
    assert(near(negative.vertices.first_a,-20.0f));
    assert(near(negative.vertices.first_b,-40.0f));

    const auto invalid=bridge::plan_original_surface_clip(v,0,0,1U,true);
    assert(std::isnan(invalid.original_ratio));
    assert(std::isnan(invalid.vertices.first_a));
    assert(std::isnan(invalid.vertices.first_b));
    // Original SUB operands are 32-bit and intentionally wrap.
    const auto wrapped=bridge::original_surface_clip_ratio(
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max());
    assert(wrapped<0.0f && std::isfinite(wrapped));
    assert(bridge::original_i32_word(0xffffffffU)==-1);
    assert(bridge::x86_wrapped_add(
        std::numeric_limits<std::int32_t>::max(),1)==
        std::numeric_limits<std::int32_t>::min());
}
void test_native_texture_uv_interpolation_and_chain_flag() {
    const hud::ResourceRectangle rectangle{10,210,20,420};
    const bridge::OriginalResourceUvBounds uv{0.1f,0.2f,0.9f,0.6f};
    const auto r=bridge::prepare_original_texture_subrectangle(
        rectangle,uv,50,100,50,100,true);
    assert(r.native_texture_draw_pending);
    assert(r.chained_resource_draw_pending);
    assert(near(r.first_u,0.3f) && near(r.first_v,0.3f));
    assert(near(r.second_u,0.5f) && near(r.second_v,0.4f));

    const auto no_chain=bridge::prepare_original_texture_subrectangle(
        rectangle,uv,0,0,200,400,false);
    assert(!no_chain.chained_resource_draw_pending);
    assert(near(no_chain.first_u,0.1f));
    assert(near(no_chain.first_v,0.2f));
    assert(near(no_chain.second_u,0.9f));
    assert(near(no_chain.second_v,0.6f));
}
void test_original_zero_extent_nan_without_invented_guard() {
    const hud::ResourceRectangle zero{0,0,0,0};
    const bridge::OriginalResourceUvBounds uv{0.0f,0.0f,1.0f,1.0f};
    const auto r=bridge::prepare_original_texture_subrectangle(
        zero,uv,0,0,20,20,false);
    assert(std::isnan(r.first_u) && std::isnan(r.first_v));
    assert(std::isinf(r.second_u) && std::isinf(r.second_v));
}
void test_phase157_hud_preflight_and_native_draw_bridge_together() {
    const hud::ResourceRectangle rect{0,200,0,400};
    const auto prep=hud::prepare_original_hud_bar(
        rect,true,true,0.75f,900,1200.0f,2);
    assert(prep.disposition==hud::PaintDisposition::ValidMode);
    assert(prep.scaled_dimension_a==150.0f);
    const bridge::OriginalSevenWords params{{100,200,50,100,75,25,0xabcdef01}};
    const auto expanded=bridge::expand_original_draw_arguments(rect,params);
    assert(expanded[2]==200 && expanded[3]==400);
    assert(expanded[4]==50 && expanded[5]==100);
    assert(expanded[8]==0xabcdef01);
    assert(bridge::choose_original_draw_route(0,42)==
           bridge::OriginalDrawRoute::TexturedSubrectangle);
    const auto subrect=bridge::prepare_original_texture_subrectangle(
        rect,{},50,100,75,25,false);
    assert(near(subrect.first_u,0.25f));
    assert(near(subrect.first_v,0.25f));
    assert(near(subrect.second_u,0.625f));
    assert(near(subrect.second_v,0.3125f));
}
}
int main() {
    test_exact_original_seven_to_nine_arg_wrapper();
    test_original_native_branch_order();
    test_original_clip_ratio_and_vertex_fallback_order();
    test_original_clip_negative_nan_and_wrapping();
    test_native_texture_uv_interpolation_and_chain_flag();
    test_original_zero_extent_nan_without_invented_guard();
    test_phase157_hud_preflight_and_native_draw_bridge_together();
}
