#include "sin_compat/original_client_hud_coordinate_scale.hpp"
#include "sin_compat/original_client_breath_timer_hud.hpp"
#include "sin_compat/original_client_poison_timer_hud.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

namespace scale = sin_compat::original_client_hud_scale;
namespace breath = sin_compat::original_client_breath_hud;
namespace poison = sin_compat::original_client_poison_hud;

namespace {
void test_exact_original_getter_globally_scoped_fields() {
    scale::OriginalViewportDimensions dimensions{720,1280};
    assert(scale::read_original_primary_dimension(dimensions)==720);
    assert(scale::read_original_secondary_dimension(dimensions)==1280);
    dimensions.global_at_0x2433fc14=800;
    assert(scale::read_original_primary_dimension(dimensions)==800);
    assert(scale::read_original_secondary_dimension(dimensions)==1280);
    assert(scale::original_reference_canvas_width==1600.0f);
    assert(scale::original_reference_canvas_height==1200.0f);
}
void test_single_ratio_both_float_outputs() {
    scale::OriginalViewportDimensions dimensions{600,800};
    float first=200.0f, second=400.0f;
    scale::scale_using_original_primary_dimension(
        first,second,dimensions,scale::original_reference_canvas_height);
    assert(first==100.0f);
    assert(second==200.0f);
    float first2=200.0f, second2=400.0f;
    // This helper doesn't read original arg3; changing it has no effect.
    scale::scale_original_hud_pair(first2,second2,600,1200.0f);
    assert(first2==first && second2==second);
    scale::scale_original_hud_pair(first2,second2,1200,1200.0f);
    assert(first2==first && second2==second);
}
void test_ordered_stores_and_aliased_pointers() {
    float value=2.0f;
    scale::scale_original_hud_pair(value,value,900,600.0f);
    // Native helper writes float* arg1, then reads float* arg2.
    // Aliasing must multiply the stored result a second time.
    assert(value==4.5f);
    float negative=-8.0f, zero=0.0f;
    scale::scale_original_hud_pair(negative,zero,600,1200.0f);
    assert(negative==-4.0f && zero==0.0f);
}
void test_original_zero_divisor_nan_and_infinity() {
    float a=2.0f,b=0.0f;
    scale::scale_original_hud_pair(a,b,600,0.0f);
    assert(std::isinf(a) && a>0.0f);
    assert(std::isnan(b)); // 0 * infinity
    a=3.0f;b=4.0f;
    scale::scale_original_hud_pair(a,b,0,0.0f);
    assert(std::isnan(a) && std::isnan(b));
    a=3.0f;b=4.0f;
    scale::scale_original_hud_pair(a,b,600,-1200.0f);
    assert(a==-1.5f && b==-2.0f);
}
void test_existing_poison_breath_draw_gate_integration() {
    breath::State air{};
    poison::State toxin{};
    air.shared_status_byte=255;
    air.first_paint_resource=123;
    air.second_paint_resource=456;
    toxin.paint_enabled=1;
    toxin.display_fraction=0.4f;
    toxin.positive_resource=789;
    assert(breath::should_paint_after_base(air));
    const auto selected=poison::choose_paint_resource(toxin);
    assert(selected.should_draw && selected.selected_resource==789);
    // Both original paint methods call 0x24154AB0 after these gates.
    scale::OriginalViewportDimensions d{900,1600};
    float airx=120.0f,airy=60.0f;
    float toxin_x=120.0f,toxin_y=60.0f;
    scale::scale_using_original_primary_dimension(
        airx,airy,d,scale::original_reference_canvas_height);
    scale::scale_using_original_primary_dimension(
        toxin_x,toxin_y,d,scale::original_reference_canvas_height);
    assert(airx==90.0f && airy==45.0f);
    assert(toxin_x==airx && toxin_y==airy);
    air.shared_status_byte=0;
    toxin.paint_enabled=0;
    assert(!breath::should_paint_after_base(air));
    assert(!poison::choose_paint_resource(toxin).should_draw);
}
}
int main(){
    test_exact_original_getter_globally_scoped_fields();
    test_single_ratio_both_float_outputs();
    test_ordered_stores_and_aliased_pointers();
    test_original_zero_divisor_nan_and_infinity();
    test_existing_poison_breath_draw_gate_integration();
}
