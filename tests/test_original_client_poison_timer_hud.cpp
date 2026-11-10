#include "sin_compat/original_client_poison_timer_hud.hpp"
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

using namespace sin_compat::original_client_poison_hud;
namespace {
void test_original_reset_only_one_member() {
    State state{};
    state.reset_word = 42;
    state.paint_enabled = 1;
    state.display_fraction = 0.75f;
    state.positive_resource = 17;
    state.nonpositive_resource = 23;
    state.resolved_resources = {{1, 2, 3}};
    reset_slot(state);
    assert(state.reset_word == 0);
    assert(state.paint_enabled == 1);
    assert(state.display_fraction == 0.75f);
    assert(state.positive_resource == 17);
    assert(state.nonpositive_resource == 23);
    assert((state.resolved_resources == std::array<std::uintptr_t, 3>{{1, 2, 3}}));
}

void test_resource_resolution_matches_original_call_order() {
    State state{};
    std::vector<std::string> visited;
    const std::array<std::string_view, 3> names = {{
        "test_positive", "test_negative", "test_warning"
    }};
    bind_three_resources(state, names, [&](std::string_view name) {
        visited.emplace_back(name);
        return static_cast<std::uintptr_t>(visited.size() * 11U);
    });
    assert((visited == std::vector<std::string>{
        "test_positive", "test_negative", "test_warning"
    }));
    assert((state.resolved_resources ==
            std::array<std::uintptr_t, 3>{{11, 22, 33}}));
}

void test_original_render_threshold_and_gates() {
    State state{};
    state.paint_enabled = 7;
    state.positive_resource = 0xABCDU;
    state.nonpositive_resource = 0x1234U;

    state.display_fraction = 0.75f;
    auto choice = choose_paint_resource(state);
    assert(choice.should_draw && choice.selected_resource == 0xABCDU);
    state.display_fraction = std::numeric_limits<float>::infinity();
    assert(choose_paint_resource(state).selected_resource == 0xABCDU);

    for (float value : {0.0f, -0.0f, -0.1f,
                        -std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
        state.display_fraction = value;
        choice = choose_paint_resource(state);
        assert(choice.should_draw && choice.selected_resource == 0x1234U);
    }
    state.display_fraction = 0.4f;
    state.paint_enabled = 0;
    choice = choose_paint_resource(state);
    assert(!choice.should_draw && choice.selected_resource == 0xABCDU);

    state.paint_enabled = 1;
    state.positive_resource = 0;
    choice = choose_paint_resource(state);
    assert(!choice.should_draw && choice.selected_resource == 0U);
    state.display_fraction = -1.0f;
    choice = choose_paint_resource(state);
    assert(choice.should_draw && choice.selected_resource == 0x1234U);
}

void test_player_value_to_hud_projection() {
    State state{};
    state.display_fraction = 8.0f;
    assert(!update_display_from_player(state, false, 0.8f));
    assert(state.display_fraction == 8.0f);
    assert(update_display_from_player(state, true, 0.0f));
    assert(state.display_fraction == 1.0f);
    // Original 0x24236320 compares incoming raw value against the
    // previous HUD display_fraction first; equal means no update.
    assert(!update_display_from_player(state, true, 1.0f));
    assert(state.display_fraction == 1.0f);
    state.display_fraction = 0.5f;
    assert(update_display_from_player(state, true, 1.0f));
    assert(state.display_fraction == 0.0f);
    assert(update_display_from_player(state, true, 0.25f));
    assert(state.display_fraction == 0.75f);
    assert(update_display_from_player(state, true, 1.5f));
    assert(state.display_fraction == -0.5f);
    assert(update_display_from_player(state, true,
            std::numeric_limits<float>::quiet_NaN()));
    assert(std::isnan(state.display_fraction));
    assert(update_display_from_player(state, true,
            std::numeric_limits<float>::infinity()));
    assert(std::isinf(state.display_fraction) && state.display_fraction < 0.0f);
}

void test_player_projection_selects_expected_paint_resource() {
    State state{};
    state.paint_enabled = 1;
    state.positive_resource = 55;
    state.nonpositive_resource = 66;
    update_display_from_player(state, true, 0.25f);
    assert(choose_paint_resource(state).selected_resource == 55);
    update_display_from_player(state, true, 1.25f);
    assert(choose_paint_resource(state).selected_resource == 66);
    update_display_from_player(state, true, 1.0f);
    assert(choose_paint_resource(state).selected_resource == 66);
}
}
int main() {
    test_original_reset_only_one_member();
    test_resource_resolution_matches_original_call_order();
    test_original_render_threshold_and_gates();
    test_player_value_to_hud_projection();
    test_player_projection_selects_expected_paint_resource();
}
