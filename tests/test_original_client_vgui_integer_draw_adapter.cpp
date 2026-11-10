#include "sin_compat/original_client_vgui_integer_draw_adapter.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace draw = sin_compat::original_client_vgui_integer_draw;
namespace old = sin_compat::original_client_vgui_draw;
namespace {
void test_complete_native_stack_words_and_getter_order() {
    const old::ResourceRectangle rect{10,210,-10,390};
    draw::OriginalFiveDrawArgs args{12.9f,-3.9f,0xdecafbadU,1200.0f,0x7fc01234U};
    std::vector<int> calls;
    auto out = draw::prepare_original_integer_draw_words(
        rect,args,[&]{ calls.push_back(2);return 1800; },
                  [&]{ calls.push_back(1);return 900; });
    assert((calls==std::vector<int>{2,1}));
    const old::OriginalNineWords expected{{12U,static_cast<std::uint32_t>(-3),
        150U,300U,0U,0U,200U,400U,0x7fc01234U}};
    assert(out==expected);
    args.unused_third_word=0;
    const auto again=draw::prepare_original_integer_draw_words(
        rect,args,[]{return 987654;},[]{return 900;});
    assert(again==expected); // original third word is dead
}
void test_x87_truncation_negative_positive_extremes() {
    using draw::original_x87_fistp_i32;
    assert(original_x87_fistp_i32(9.999L)==9);
    assert(original_x87_fistp_i32(-9.999L)==-9);
    assert(original_x87_fistp_i32(-0.999L)==0);
    assert(original_x87_fistp_i32(2147483647.0L)==2147483647);
    assert(original_x87_fistp_i32(-2147483648.0L)==
           std::numeric_limits<std::int32_t>::min());
    assert(original_x87_fistp_i32(2147483648.0L)==
           std::numeric_limits<std::int32_t>::min());
    assert(original_x87_fistp_i32(-2147483649.0L)==
           std::numeric_limits<std::int32_t>::min());
    assert(original_x87_fistp_i32(std::numeric_limits<long double>::infinity())==
           std::numeric_limits<std::int32_t>::min());
    assert(original_x87_fistp_i32(std::numeric_limits<long double>::quiet_NaN())==
           std::numeric_limits<std::int32_t>::min());
}
void test_original_float32_extent_rounding_before_fistp() {
    // 2^24+1 is exactly representable in an original signed integer but
    // not in float32, so FSTP m32 followed by FLD m32 rounds downward.
    const old::ResourceRectangle rect{0,16777217,0,16777217};
    auto out=draw::prepare_original_integer_draw_words(
        rect,{},[]{return 0;},[]{return 2;});
    assert(out[6]==16777216U && out[7]==16777216U);
    assert(out[2]==33554432U && out[3]==33554432U);

    const old::ResourceRectangle max_rect{0,
        std::numeric_limits<std::int32_t>::max(),0,1};
    out=draw::prepare_original_integer_draw_words(
        max_rect,{},[]{return 0;},[]{return 1;});
    // Original FILD(+INT32_MAX), FSTP float32 => 2^31, then FISTP =>
    // integer-indefinite 0x80000000 with invalid exception masked.
    assert(out[6]==0x80000000U && out[2]==0x80000000U);
}
void test_zero_denominator_and_nan_masked_invalid() {
    const old::ResourceRectangle rect{0,100,0,200};
    draw::OriginalFiveDrawArgs args{};
    args.reference_denominator=0.0f;
    args.opaque_fifth_word=0xffffffffU;
    auto out=draw::prepare_original_integer_draw_words(
        rect,args,[]{return 600;},[]{return 600;});
    assert(out[2]==0x80000000U && out[3]==0x80000000U);
    assert(out[6]==100U && out[7]==200U && out[8]==0xffffffffU);
    args.reference_denominator=std::numeric_limits<float>::quiet_NaN();
    out=draw::prepare_original_integer_draw_words(
        rect,args,[]{return 0;},[]{return 600;});
    assert(out[2]==0x80000000U && out[3]==0x80000000U);
    args.first_coordinate=std::numeric_limits<float>::infinity();
    args.second_coordinate=std::numeric_limits<float>::quiet_NaN();
    out=draw::prepare_original_integer_draw_words(
        rect,args,[]{return 0;},[]{return 600;});
    assert(out[0]==0x80000000U && out[1]==0x80000000U);
}
void test_original_integer_draw_and_previous_vgui_draw_bridge() {
    const old::ResourceRectangle rect{0,200,0,400};
    draw::OriginalFiveDrawArgs args{15.6f,25.9f,0,1200.0f,0x55aa55aaU};
    const auto nine=draw::prepare_original_integer_draw_words(
        rect,args,[]{return 800;},[]{return 600;});
    assert(nine[0]==15 && nine[1]==25);
    assert(nine[2]==100 && nine[3]==200);
    assert(nine[4]==0 && nine[5]==0);
    assert(nine[6]==200 && nine[7]==400);
    assert(nine[8]==0x55aa55aaU);
    assert(old::choose_original_draw_route(0,5)==
           old::OriginalDrawRoute::TexturedSubrectangle);
}
}
int main(){
    test_complete_native_stack_words_and_getter_order();
    test_x87_truncation_negative_positive_extremes();
    test_original_float32_extent_rounding_before_fistp();
    test_zero_denominator_and_nan_masked_invalid();
    test_original_integer_draw_and_previous_vgui_draw_bridge();
}
