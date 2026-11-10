#include "sin_compat/original_client_poison_timer_fade.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
using namespace sin_compat::original_client_poison_hud;
namespace {
struct Recorder {
    State* state;
    std::vector<std::string> tokens;
    float observed_before_store = 0.0f;
    void operator()(std::string_view token) {
        observed_before_store = state->display_fraction;
        tokens.emplace_back(token);
    }
};
void test_early_exit_and_order() {
    State s{};
    s.display_fraction=0.5f;
    Recorder recorder{&s,{},-1.0f};
    auto result=update_poison_timer_with_fades(s,false,0.1f,0U,
        [&](std::string_view token){ recorder(token); });
    assert(!result.updated && result.emitted==FadeCue::None);
    assert(s.display_fraction==0.5f && recorder.tokens.empty());
    result=update_poison_timer_with_fades(s,true,0.5f,0U,
        [&](std::string_view token){ recorder(token); });
    assert(!result.updated && s.display_fraction==0.5f);
    result=update_poison_timer_with_fades(s,true,0.25f,0U,
        [&](std::string_view token){ recorder(token); });
    assert(result.updated && result.emitted==FadeCue::PoisonTimeFadeIn);
    assert(recorder.tokens.size()==1 && recorder.tokens[0]=="PoisonTimeFadeIn");
    assert(recorder.observed_before_store==0.5f); // sound BEFORE HUD write
    assert(s.display_fraction==0.75f);
}
void test_fadeout_gate() {
    for (float input : {1.0f,1.25f,2.0f,
                        std::numeric_limits<float>::infinity()}) {
        State s{};
        s.display_fraction=0.34f;
        Recorder rec{&s, {}, 0.0f};
        auto r=update_poison_timer_with_fades(s,true,input,0xFFU,
            [&](std::string_view token){rec(token);});
        assert(r.updated && r.emitted==FadeCue::PoisonTimeFadeOut);
        assert(rec.tokens.size()==1 && rec.tokens[0]=="PoisonTimeFadeOut");
        assert(rec.observed_before_store==0.34f);
    }
    for (float input : {-2.0f, -0.0f, 0.0f,0.2f,0.25f,0.75f,
                       std::numeric_limits<float>::quiet_NaN()}) {
        State s{};
        s.display_fraction=0.34f;
        Recorder rec{&s, {}, 0.0f};
        auto r=update_poison_timer_with_fades(s,true,input,0xFFU,
            [&](std::string_view token){rec(token);});
        assert(r.updated && r.emitted==FadeCue::None);
        assert(rec.tokens.empty());
    }
}
void test_fadein_gate_and_nan() {
    for (float input : {-std::numeric_limits<float>::infinity(),-1.0f,
                        -0.0f,0.0f,0.25f,0.75f}) {
        State s{};
        s.display_fraction=42.0f;
        Recorder rec{&s, {}, 0.0f};
        auto r=update_poison_timer_with_fades(s,true,input,0U,
            [&](std::string_view token){rec(token);});
        assert(r.updated && r.emitted==FadeCue::PoisonTimeFadeIn);
        assert(rec.tokens.size()==1 && rec.tokens[0]=="PoisonTimeFadeIn");
    }
    for (float input : {1.0f,1.75f,
                        std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
        State s{};
        s.display_fraction=42.0f;
        Recorder rec{&s, {}, 0.0f};
        auto r=update_poison_timer_with_fades(s,true,input,0U,
            [&](std::string_view token){rec(token);});
        assert(r.updated && r.emitted==FadeCue::None && rec.tokens.empty());
        if (std::isnan(input)) assert(std::isnan(s.display_fraction));
    }
}
void test_untouched_status_values_do_not_emit() {
    for (std::uint8_t status : {1U,2U,127U,128U,254U}) {
        for (float input : {0.0f,0.4f,1.0f,2.0f}) {
            State s{};
            s.display_fraction=-1.0f;
            Recorder rec{&s, {}, 0.0f};
            auto r=update_poison_timer_with_fades(s,true,input,status,
                [&](std::string_view token){rec(token);});
            assert(r.updated && r.emitted==FadeCue::None && rec.tokens.empty());
        }
    }
}
void test_float_nan_equality_undefined_is_unordered() {
    State s{};
    s.display_fraction=std::numeric_limits<float>::quiet_NaN();
    Recorder rec{&s, {}, 0.0f};
    const auto r=update_poison_timer_with_fades(s,true,
         std::numeric_limits<float>::quiet_NaN(),0xFFU,
         [&](std::string_view token){rec(token);});
    assert(r.updated && r.emitted==FadeCue::None && rec.tokens.empty());
    assert(std::isnan(s.display_fraction));
}
void test_original_hud_selection_after_fade_state_update() {
    State s{};
    s.display_fraction=0.34f;
    s.paint_enabled=1;
    s.positive_resource=11;
    s.nonpositive_resource=22;
    Recorder rec{&s, {}, 0.0f};
    auto r=update_poison_timer_with_fades(s,true,0.75f,0U,
        [&](std::string_view token){rec(token);});
    assert(r.updated && choose_paint_resource(s).selected_resource==11);
    r=update_poison_timer_with_fades(s,true,1.25f,0xFFU,
        [&](std::string_view token){rec(token);});
    assert(r.updated && choose_paint_resource(s).selected_resource==22);
    assert((rec.tokens==std::vector<std::string>{"PoisonTimeFadeIn", "PoisonTimeFadeOut"}));
}
}
int main() {
    test_early_exit_and_order();
    test_fadeout_gate();
    test_fadein_gate_and_nan();
    test_untouched_status_values_do_not_emit();
    test_float_nan_equality_undefined_is_unordered();
    test_original_hud_selection_after_fade_state_update();
}
