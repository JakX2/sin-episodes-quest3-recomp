#include "sin_compat/original_client_breath_timer_hud.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

using namespace sin_compat::original_client_breath_hud;
namespace {
struct Recording {
    State& state;
    std::vector<std::string> sounds;
    float value_at_emission = 0.0f;
    void emit(std::string_view token) {
        value_at_emission = state.previous_breath_value;
        sounds.emplace_back(token);
    }
};
UpdateResult update(State& state, bool has_player, float value, Recording& record) {
    return update_from_original_player(
        state, has_player, value,
        [&](std::string_view event) { record.emit(event); });
}
void test_reset_is_single_field_only() {
    State s{};
    s.reset_value = 0.0f;
    s.previous_breath_value = 0.5f;
    s.shared_status_byte = 7U;
    s.first_paint_resource = 12U;
    s.second_paint_resource = 13U;
    s.resolved_resources = {{14U, 15U}};
    reset_slot(s);
    assert(s.reset_value == 1.0f);
    assert(s.previous_breath_value == 0.5f && s.shared_status_byte == 7U);
    assert(s.first_paint_resource == 12U &&
           s.second_paint_resource == 13U);
    assert((s.resolved_resources ==
            std::array<std::uintptr_t, 2>{{14U, 15U}}));
}
void test_original_resource_and_panel_paint_gate() {
    State s{};
    std::vector<std::string> visited;
    bind_two_resources(s, {{"breath_a", "breath_b"}},
        [&](std::string_view name) -> std::uintptr_t {
            visited.emplace_back(name);
            return 70U + visited.size();
        });
    assert((visited == std::vector<std::string>{"breath_a","breath_b"}));
    assert((s.resolved_resources ==
            std::array<std::uintptr_t, 2>{{71U,72U}}));
    s.first_paint_resource=1;
    s.second_paint_resource=2;
    assert(!should_paint_after_base(s));
    s.shared_status_byte=1;
    assert(should_paint_after_base(s));
    s.shared_status_byte=255;
    assert(should_paint_after_base(s));
    s.second_paint_resource=0;
    assert(!should_paint_after_base(s));
    s.second_paint_resource=2;
    s.first_paint_resource=0;
    assert(!should_paint_after_base(s));
}
void test_equal_and_no_player_early_exit() {
    State s{};
    s.previous_breath_value=0.5f;
    Recording rec{s};
    auto r=update(s,false,0.25f,rec);
    assert(!r.updated && r.event==BreathSoundCue::None &&
           rec.sounds.empty() && s.previous_breath_value==0.5f);
    r=update(s,true,0.5f,rec);
    assert(!r.updated && rec.sounds.empty());
    r=update(s,true,0.75f,rec);
    assert(r.updated && r.event==BreathSoundCue::FadeIn);
    assert(rec.sounds.back()=="BreathTimeFadeIn");
    assert(rec.value_at_emission==0.5f); // sound before field write
    assert(s.previous_breath_value==0.75f);
}
void test_all_four_original_event_branches() {
    // The "Empty" instruction branch exists, but its preceding
    // raw < 0.25 test consumes zero first for ordinary float inputs.
    for (float raw : {-std::numeric_limits<float>::infinity(),
                      -1.0f,0.0f,0.249f}) {
        State s{};
        s.previous_breath_value=2.0f;
        s.shared_status_byte=1U;
        Recording rec{s};
        const auto r=update(s,true,raw,rec);
        assert(r.updated && r.event==BreathSoundCue::Low);
        assert(rec.sounds.size()==1 &&
               rec.sounds.back()=="BreathTimeLow");
        assert(rec.value_at_emission==2.0f);
    }
    for (std::uint8_t status : {1U,2U,127U,255U}) {
        State s{};
        s.previous_breath_value=0.3f;
        s.shared_status_byte=status;
        Recording rec{s};
        const auto r=update(s,true,1.0f,rec);
        assert(r.updated && r.event==BreathSoundCue::FadeOut);
        assert(rec.sounds.size()==1 &&
               rec.sounds.back()=="BreathTimeFadeOut");
    }
    for (float raw : {0.0f,0.24f,0.25f,0.5f,0.999f}) {
        State s{};
        s.previous_breath_value=4.0f;
        s.shared_status_byte=0U;
        Recording rec{s};
        const auto r=update(s,true,raw,rec);
        assert(r.updated && r.event==BreathSoundCue::FadeIn);
        assert(rec.sounds.size()==1 &&
               rec.sounds.back()=="BreathTimeFadeIn");
    }
    // All ordered and NaN inputs tested below remain unable to reach
    // the original BreathTimeEmpty block due to the prior low test.
    assert(original_sound_token(BreathSoundCue::Empty)=="BreathTimeEmpty");
}
void test_thresholds_nan_infinity_and_no_event() {
    for (float raw : {0.25f,0.5f,0.999f,1.1f,
                      std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::quiet_NaN()}) {
        State s{};
        s.previous_breath_value=2.0f;
        s.shared_status_byte=255U;
        Recording rec{s};
        auto r=update(s,true,raw,rec);
        assert(r.updated && r.event==BreathSoundCue::None);
        assert(rec.sounds.empty());
        if (std::isnan(raw)) assert(std::isnan(s.previous_breath_value));
    }
    State a{};
    a.previous_breath_value=2.0f;
    a.shared_status_byte=0U;
    Recording rec{a};
    auto r=update(a,true,1.0f,rec);
    assert(r.updated && r.event==BreathSoundCue::None);
    assert(a.previous_breath_value==1.0f);
    a.previous_breath_value=std::numeric_limits<float>::quiet_NaN();
    r=update(a,true,std::numeric_limits<float>::quiet_NaN(),rec);
    assert(r.updated && r.event==BreathSoundCue::None);
}
}
int main() {
    test_reset_is_single_field_only();
    test_original_resource_and_panel_paint_gate();
    test_equal_and_no_player_early_exit();
    test_all_four_original_event_branches();
    test_thresholds_nan_infinity_and_no_event();
}
