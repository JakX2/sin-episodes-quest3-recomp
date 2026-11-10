#include "sin_compat/original_client_hud_renderer_preflight.hpp"
#include "sin_compat/original_client_breath_timer_hud.hpp"
#include "sin_compat/original_client_poison_timer_hud.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace render = sin_compat::original_client_hud_render;
namespace breath = sin_compat::original_client_breath_hud;
namespace poison = sin_compat::original_client_poison_hud;

namespace {
void test_original_rect_getters_and_x86_signed_wrap() {
    render::ResourceRectangle rectangle{10,210,50,450};
    assert(render::read_original_resource_dimension_a(rectangle)==200);
    assert(render::read_original_resource_dimension_b(rectangle)==400);
    rectangle.value_at_a8=std::numeric_limits<std::int32_t>::max();
    rectangle.value_at_ac=std::numeric_limits<std::int32_t>::min();
    assert(render::read_original_resource_dimension_a(rectangle)==1);
    rectangle.value_at_a8=std::numeric_limits<std::int32_t>::min();
    rectangle.value_at_ac=std::numeric_limits<std::int32_t>::max();
    assert(render::read_original_resource_dimension_a(rectangle)==-1);
}
void test_null_primary_is_a_true_early_exit() {
    render::ResourceRectangle rect{0,200,0,400};
    const auto r=render::prepare_original_hud_bar(
        rect,false,true,std::numeric_limits<float>::quiet_NaN(),
        0,0.0f,3);
    assert(r.disposition==render::PaintDisposition::MissingPrimaryResource);
    assert(!r.secondary_resource_present);
    assert(r.scaled_dimension_a==0.0f && r.scaled_dimension_b==0.0f);
}
void test_fraction_bounds_nan_and_infinity() {
    assert(render::original_fraction_clamp(-1.0f)==0.0f);
    assert(render::original_fraction_clamp(0.5f)==0.5f);
    assert(render::original_fraction_clamp(2.0f)==1.0f);
    assert(render::original_fraction_clamp(
        std::numeric_limits<float>::infinity())==1.0f);
    assert(render::original_fraction_clamp(
        -std::numeric_limits<float>::infinity())==0.0f);
    assert(std::isnan(render::original_fraction_clamp(
        std::numeric_limits<float>::quiet_NaN())));
    assert(std::signbit(render::original_fraction_clamp(-0.0f)));
}
void test_original_fraction_and_two_extent_scaling() {
    const render::ResourceRectangle rect{10,210,50,450};
    auto r=render::prepare_original_hud_bar(
        rect,true,false,0.25f,600,1200.0f,3);
    assert(r.disposition==render::PaintDisposition::ValidMode);
    assert(r.original_mode==3);
    assert(!r.secondary_resource_present);
    assert(r.normalized_fraction==0.25f);
    assert(r.remaining_fraction==0.75f);
    assert(r.scaled_dimension_a==100.0f);
    assert(r.scaled_dimension_b==200.0f);
    r=render::prepare_original_hud_bar(rect,true,true,2.0f,1200,1200.0f,0);
    assert(r.normalized_fraction==1.0f && r.remaining_fraction==0.0f);
    assert(r.secondary_resource_present);
    assert(r.scaled_dimension_a==200.0f && r.scaled_dimension_b==400.0f);
    r=render::prepare_original_hud_bar(rect,true,false,-2.0f,1200,1200.0f,4);
    assert(r.normalized_fraction==0.0f && r.remaining_fraction==1.0f);
    assert(r.disposition==render::PaintDisposition::UnsupportedMode);
}
void test_original_unordered_fraction_and_zero_denominator() {
    render::ResourceRectangle rect{0,200,0,400};
    const auto nan=std::numeric_limits<float>::quiet_NaN();
    auto r=render::prepare_original_hud_bar(rect,true,false,nan,600,1200.0f,1);
    assert(std::isnan(r.normalized_fraction));
    assert(std::isnan(r.remaining_fraction));
    r=render::prepare_original_hud_bar(rect,true,false,0.5f,600,0.0f,1);
    assert(std::isinf(r.scaled_dimension_a) && std::isinf(r.scaled_dimension_b));
    r=render::prepare_original_hud_bar(rect,true,false,0.5f,0,0.0f,1);
    assert(std::isnan(r.scaled_dimension_a) && std::isnan(r.scaled_dimension_b));
}
void test_modes_and_existing_original_poison_breath_guards() {
    poison::State poison_state{};
    poison_state.paint_enabled=1;
    poison_state.display_fraction=0.5f;
    poison_state.positive_resource=1;
    breath::State breath_state{};
    breath_state.shared_status_byte=255;
    breath_state.first_paint_resource=1;
    breath_state.second_paint_resource=2;
    assert(poison::choose_paint_resource(poison_state).should_draw);
    assert(breath::should_paint_after_base(breath_state));
    const render::ResourceRectangle rect{0,200,0,400};
    for (std::uint32_t mode=0;mode<4;mode++) {
        const auto r=render::prepare_original_hud_bar(
            rect,true,true,0.75f,900,1200.0f,mode);
        assert(r.disposition==render::PaintDisposition::ValidMode);
        assert(r.scaled_dimension_a==150.0f);
        assert(r.scaled_dimension_b==300.0f);
    }
    auto r=render::prepare_original_hud_bar(rect,true,true,0.75f,900,1200.0f,4);
    assert(r.disposition==render::PaintDisposition::UnsupportedMode);
    poison_state.paint_enabled=0;
    breath_state.shared_status_byte=0;
    assert(!poison::choose_paint_resource(poison_state).should_draw);
    assert(!breath::should_paint_after_base(breath_state));
}
}
int main() {
    test_original_rect_getters_and_x86_signed_wrap();
    test_null_primary_is_a_true_early_exit();
    test_fraction_bounds_nan_and_infinity();
    test_original_fraction_and_two_extent_scaling();
    test_original_unordered_fraction_and_zero_denominator();
    test_modes_and_existing_original_poison_breath_guards();
}
