#include "sin_compat/original_grunt_map_inputs.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using namespace sin_compat::ai::grunt;

namespace {

class FakeGruntMapInputHost final : public IOriginalGruntMapInputHost {
public:
    std::set<OriginalGruntEntityToken> live{42U};
    mutable std::vector<std::string> calls;
    OriginalGruntEntityToken last_entity = 0U;
    bool should_patrol = false;
    std::uint32_t look_mask = 0U;
    bool look_enabled = false;
    std::string assault_payload;
    std::int32_t assault_argument = 0;
    float grenade_height = -1.0F;
    bool grenade_happy = false;

    bool is_live(OriginalGruntEntityToken entity) const noexcept override {
        calls.emplace_back("is_live");
        return live.count(entity) != 0U;
    }

    void set_should_patrol(
        OriginalGruntEntityToken entity, bool enabled) noexcept override {
        calls.emplace_back("set_should_patrol");
        last_entity = entity;
        should_patrol = enabled;
    }

    void update_look_flag(
        OriginalGruntEntityToken entity,
        std::uint32_t mask,
        bool enabled) noexcept override {
        calls.emplace_back("update_look_flag");
        last_entity = entity;
        look_mask = mask;
        look_enabled = enabled;
    }

    void invoke_assault_helper(
        OriginalGruntEntityToken entity,
        std::string_view input,
        std::int32_t argument) noexcept override {
        calls.emplace_back("invoke_assault_helper");
        last_entity = entity;
        assault_payload.assign(input);
        assault_argument = argument;
    }

    void set_grenade_height(
        OriginalGruntEntityToken entity, float height) noexcept override {
        calls.emplace_back("set_grenade_height");
        last_entity = entity;
        grenade_height = height;
    }

    void set_grenade_happy(
        OriginalGruntEntityToken entity, bool enabled) noexcept override {
        calls.emplace_back("set_grenade_happy");
        last_entity = entity;
        grenade_happy = enabled;
    }
};

void test_start_and_stop_patrolling_write_the_shared_ai_field() {
    FakeGruntMapInputHost host;
    OriginalGruntMapInputValue value{};

    assert(dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::StartPatrolling, value, host) ==
        OriginalGruntMapInputResult::Applied);
    assert(host.should_patrol);
    assert(host.last_entity == 42U);

    host.calls.clear();
    assert(dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::StopPatrolling, value, host) ==
        OriginalGruntMapInputResult::Applied);
    assert(!host.should_patrol);
    assert((host.calls == std::vector<std::string>{"is_live", "set_should_patrol"}));
}

void test_look_off_sets_only_the_verified_mask() {
    FakeGruntMapInputHost host;
    const auto result = dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::LookOff, {}, host);
    assert(result == OriginalGruntMapInputResult::Applied);
    assert(host.look_mask == 0x00010000U);
    assert(host.look_enabled);
    assert((host.calls == std::vector<std::string>{"is_live", "update_look_flag"}));
}

void test_look_on_clears_the_same_mask() {
    FakeGruntMapInputHost host;
    const auto result = dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::LookOn, {}, host);
    assert(result == OriginalGruntMapInputResult::Applied);
    assert(host.look_mask == 0x00010000U);
    assert(!host.look_enabled);
}

void test_assault_preserves_string_and_opaque_helper_argument() {
    FakeGruntMapInputHost host;
    OriginalGruntMapInputValue value{};
    value.string_value = "squad_alpha";
    const auto result = dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::Assault, value, host);
    assert(result == OriginalGruntMapInputResult::Applied);
    assert(host.assault_payload == "squad_alpha");
    assert(host.assault_argument == 3);
    assert((host.calls == std::vector<std::string>{
        "is_live", "invoke_assault_helper"}));
}

void test_empty_assault_string_is_not_missing() {
    FakeGruntMapInputHost host;
    OriginalGruntMapInputValue value{};
    value.string_value = std::string_view{};
    const auto result = dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::Assault, value, host);
    assert(result == OriginalGruntMapInputResult::Applied);
    assert(host.assault_payload.empty());
    assert(host.assault_argument == 3);
}

void test_throw_grenades_resets_height_before_writing_mode() {
    FakeGruntMapInputHost host;
    OriginalGruntMapInputValue value{};
    value.boolean_value = true;
    const auto result = dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::ThrowGrenades, value, host);
    assert(result == OriginalGruntMapInputResult::Applied);
    assert(host.grenade_height == 0.0F);
    assert(host.grenade_happy);
    assert((host.calls == std::vector<std::string>{
        "is_live", "set_grenade_height", "set_grenade_happy"}));
}

void test_throw_grenades_false_is_a_valid_typed_input() {
    FakeGruntMapInputHost host;
    OriginalGruntMapInputValue value{};
    value.boolean_value = false;
    const auto result = dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::ThrowGrenades, value, host);
    assert(result == OriginalGruntMapInputResult::Applied);
    assert(host.grenade_height == 0.0F);
    assert(!host.grenade_happy);
}

void test_required_typed_payloads_are_not_fabricated() {
    FakeGruntMapInputHost host;
    assert(dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::Assault, {}, host) ==
        OriginalGruntMapInputResult::MissingInputValue);
    assert(dispatch_original_grunt_map_input(
        42U, OriginalGruntMapInput::ThrowGrenades, {}, host) ==
        OriginalGruntMapInputResult::MissingInputValue);
    assert(host.calls.size() == 2U);
}

void test_missing_entity_short_circuits_without_mutation() {
    FakeGruntMapInputHost host;
    OriginalGruntMapInputValue value{};
    value.boolean_value = true;
    const auto result = dispatch_original_grunt_map_input(
        99U, OriginalGruntMapInput::ThrowGrenades, value, host);
    assert(result == OriginalGruntMapInputResult::MissingEntity);
    assert((host.calls == std::vector<std::string>{"is_live"}));
    assert(host.grenade_height == -1.0F);
    assert(!host.grenade_happy);
}

void test_unknown_input_is_rejected() {
    FakeGruntMapInputHost host;
    const auto result = dispatch_original_grunt_map_input(
        42U, static_cast<OriginalGruntMapInput>(0xFFU), {}, host);
    assert(result == OriginalGruntMapInputResult::InvalidInput);
    assert((host.calls == std::vector<std::string>{"is_live"}));
}

} // namespace

int main() {
    test_start_and_stop_patrolling_write_the_shared_ai_field();
    test_look_off_sets_only_the_verified_mask();
    test_look_on_clears_the_same_mask();
    test_assault_preserves_string_and_opaque_helper_argument();
    test_empty_assault_string_is_not_missing();
    test_throw_grenades_resets_height_before_writing_mode();
    test_throw_grenades_false_is_a_valid_typed_input();
    test_required_typed_payloads_are_not_fabricated();
    test_missing_entity_short_circuits_without_mutation();
    test_unknown_input_is_rejected();
    std::puts("PASS: original SiN grunt map inputs update patrol, look flag, assault dispatch and grenade mode (mock host only)");
    return 0;
}
