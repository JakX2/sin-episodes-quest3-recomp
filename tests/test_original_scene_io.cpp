#include "sin_compat/original_scene_io.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <set>
#include <tuple>
#include <vector>

using namespace sin_compat::scene_io;

namespace {

struct OutputCall {
    bool scripted = false;
    SceneEntityToken entity = 0;
    std::uint8_t output = 0;
    OutputContextToken context = 0;
};

class FakeSceneIoHost final : public IOriginalSceneIoHost {
public:
    std::set<SceneEntityToken> live_entities{11U, 22U};
    std::vector<OutputCall> calls;
    bool accept_outputs = true;

    bool is_live(SceneEntityToken entity) const noexcept override {
        return live_entities.count(entity) != 0U;
    }

    bool fire_choreographed_scene_output(
        SceneEntityToken scene,
        ChoreographedSceneOutput output,
        OutputContextToken context) noexcept override {
        calls.push_back({false, scene, static_cast<std::uint8_t>(output), context});
        return accept_outputs;
    }

    bool fire_scripted_sequence_output(
        SceneEntityToken sequence,
        ScriptedSequenceOutput output,
        OutputContextToken context) noexcept override {
        calls.push_back({true, sequence, static_cast<std::uint8_t>(output), context});
        return accept_outputs;
    }
};

void test_original_output_registration_order() {
    static_assert(static_cast<std::uint8_t>(ChoreographedSceneOutput::OnStart) == 0U);
    static_assert(static_cast<std::uint8_t>(ChoreographedSceneOutput::OnCompletion) == 1U);
    static_assert(static_cast<std::uint8_t>(ChoreographedSceneOutput::OnCanceled) == 2U);
    static_assert(static_cast<std::uint8_t>(ChoreographedSceneOutput::OnTrigger1) == 3U);
    static_assert(static_cast<std::uint8_t>(ChoreographedSceneOutput::OnTrigger8) == 10U);
    static_assert(static_cast<std::uint8_t>(ScriptedSequenceOutput::OnBeginSequence) == 0U);
    static_assert(static_cast<std::uint8_t>(ScriptedSequenceOutput::OnEndSequence) == 1U);
    static_assert(static_cast<std::uint8_t>(ScriptedSequenceOutput::OnScriptEvent01) == 2U);
    static_assert(static_cast<std::uint8_t>(ScriptedSequenceOutput::OnScriptEvent08) == 9U);
    assert(original_scripted_sequence_animation_event_id == 0x3EBU);
}

void test_all_eight_choreographed_scene_triggers() {
    FakeSceneIoHost host;
    constexpr std::array<ChoreographedSceneOutput, 8> expected{{
        ChoreographedSceneOutput::OnTrigger1,
        ChoreographedSceneOutput::OnTrigger2,
        ChoreographedSceneOutput::OnTrigger3,
        ChoreographedSceneOutput::OnTrigger4,
        ChoreographedSceneOutput::OnTrigger5,
        ChoreographedSceneOutput::OnTrigger6,
        ChoreographedSceneOutput::OnTrigger7,
        ChoreographedSceneOutput::OnTrigger8
    }};

    for (std::int32_t number = 1; number <= 8; ++number) {
        assert(dispatch_choreographed_scene_trigger(
            11U, number, 0xA100U + static_cast<OutputContextToken>(number), host)
            == DispatchResult::Fired);
        const auto& call = host.calls.back();
        assert(!call.scripted);
        assert(call.entity == 11U);
        assert(call.output == static_cast<std::uint8_t>(
            expected[static_cast<std::size_t>(number - 1)]));
        assert(call.context == 0xA100U + static_cast<OutputContextToken>(number));
    }
    assert(host.calls.size() == 8U);
}

void test_choreographed_scene_lifecycle_outputs() {
    FakeSceneIoHost host;
    constexpr std::array<ChoreographedSceneOutput, 3> outputs{{
        ChoreographedSceneOutput::OnStart,
        ChoreographedSceneOutput::OnCompletion,
        ChoreographedSceneOutput::OnCanceled
    }};
    for (const auto output : outputs) {
        assert(fire_choreographed_scene_output(11U, output, 0xCAFEU, host)
            == DispatchResult::Fired);
    }
    assert(host.calls.size() == 3U);
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        assert(host.calls[i].output == static_cast<std::uint8_t>(outputs[i]));
        assert(host.calls[i].context == 0xCAFEU);
    }
}

void test_invalid_choreographed_trigger_numbers_do_not_fire() {
    FakeSceneIoHost host;
    assert(dispatch_choreographed_scene_trigger(11U, 0, 1U, host)
        == DispatchResult::InvalidEventNumber);
    assert(dispatch_choreographed_scene_trigger(11U, 9, 2U, host)
        == DispatchResult::InvalidEventNumber);
    assert(host.calls.empty());
}

void test_all_eight_scripted_sequence_events() {
    FakeSceneIoHost host;
    constexpr std::array<ScriptedSequenceOutput, 8> expected{{
        ScriptedSequenceOutput::OnScriptEvent01,
        ScriptedSequenceOutput::OnScriptEvent02,
        ScriptedSequenceOutput::OnScriptEvent03,
        ScriptedSequenceOutput::OnScriptEvent04,
        ScriptedSequenceOutput::OnScriptEvent05,
        ScriptedSequenceOutput::OnScriptEvent06,
        ScriptedSequenceOutput::OnScriptEvent07,
        ScriptedSequenceOutput::OnScriptEvent08
    }};

    for (std::int32_t number = 1; number <= 8; ++number) {
        assert(dispatch_scripted_sequence_event(
            22U, number, 0xB200U + static_cast<OutputContextToken>(number), host)
            == DispatchResult::Fired);
        const auto& call = host.calls.back();
        assert(call.scripted);
        assert(call.entity == 22U);
        assert(call.output == static_cast<std::uint8_t>(
            expected[static_cast<std::size_t>(number - 1)]));
        assert(call.context == 0xB200U + static_cast<OutputContextToken>(number));
    }
    assert(host.calls.size() == 8U);
}

void test_scripted_sequence_lifecycle_outputs() {
    FakeSceneIoHost host;
    assert(fire_scripted_sequence_output(
        22U, ScriptedSequenceOutput::OnBeginSequence, 7U, host)
        == DispatchResult::Fired);
    assert(fire_scripted_sequence_output(
        22U, ScriptedSequenceOutput::OnEndSequence, 8U, host)
        == DispatchResult::Fired);
    assert(host.calls.size() == 2U);
    assert(host.calls[0].output ==
        static_cast<std::uint8_t>(ScriptedSequenceOutput::OnBeginSequence));
    assert(host.calls[1].output ==
        static_cast<std::uint8_t>(ScriptedSequenceOutput::OnEndSequence));
}

void test_invalid_scripted_sequence_event_numbers_do_not_fire() {
    FakeSceneIoHost host;
    assert(dispatch_scripted_sequence_event(22U, 0, 1U, host)
        == DispatchResult::InvalidEventNumber);
    assert(dispatch_scripted_sequence_event(22U, 9, 2U, host)
        == DispatchResult::InvalidEventNumber);
    assert(host.calls.empty());
}

void test_original_animation_event_routes_to_numbered_output() {
    FakeSceneIoHost host;
    assert(dispatch_original_scripted_sequence_animation_event(
        original_scripted_sequence_animation_event_id, 22U, 3, 0xFACEU, host)
        == DispatchResult::Fired);
    assert(host.calls.size() == 1U);
    assert(host.calls[0].scripted);
    assert(host.calls[0].output ==
        static_cast<std::uint8_t>(ScriptedSequenceOutput::OnScriptEvent03));
    assert(host.calls[0].context == 0xFACEU);
}

void test_unrelated_animation_event_is_not_handled() {
    FakeSceneIoHost host;
    assert(dispatch_original_scripted_sequence_animation_event(
        0x3ECU, 22U, 3, 0U, host) == DispatchResult::NotHandled);
    assert(host.calls.empty());
}

void test_missing_sequence_handle_and_unparsed_parameter() {
    FakeSceneIoHost host;
    assert(dispatch_original_scripted_sequence_animation_event(
        original_scripted_sequence_animation_event_id, std::nullopt, 3, 0U, host)
        == DispatchResult::MissingEntity);
    assert(dispatch_original_scripted_sequence_animation_event(
        original_scripted_sequence_animation_event_id, 22U, std::nullopt, 0U, host)
        == DispatchResult::InvalidEventNumber);
    assert(host.calls.empty());
}

void test_stale_entity_and_host_rejection_are_reported() {
    FakeSceneIoHost host;
    assert(dispatch_choreographed_scene_trigger(99U, 1, 0U, host)
        == DispatchResult::MissingEntity);
    assert(dispatch_scripted_sequence_event(99U, 1, 0U, host)
        == DispatchResult::MissingEntity);
    assert(host.calls.empty());

    host.accept_outputs = false;
    assert(fire_choreographed_scene_output(
        11U, ChoreographedSceneOutput::OnStart, 0U, host)
        == DispatchResult::HostRejected);
    assert(fire_scripted_sequence_output(
        22U, ScriptedSequenceOutput::OnBeginSequence, 0U, host)
        == DispatchResult::HostRejected);
    assert(host.calls.size() == 2U);
}

} // namespace

int main() {
    test_original_output_registration_order();
    test_all_eight_choreographed_scene_triggers();
    test_choreographed_scene_lifecycle_outputs();
    test_invalid_choreographed_trigger_numbers_do_not_fire();
    test_all_eight_scripted_sequence_events();
    test_scripted_sequence_lifecycle_outputs();
    test_invalid_scripted_sequence_event_numbers_do_not_fire();
    test_original_animation_event_routes_to_numbered_output();
    test_unrelated_animation_event_is_not_handled();
    test_missing_sequence_handle_and_unparsed_parameter();
    test_stale_entity_and_host_rejection_are_reported();
    std::puts("PASS: original SiN choreographed-scene trigger and scripted-sequence output routing (mock host only)");
    return 0;
}
