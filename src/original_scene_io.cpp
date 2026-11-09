#include "sin_compat/original_scene_io.hpp"

#include <array>

namespace sin_compat::scene_io {
namespace {

constexpr std::array<ChoreographedSceneOutput, 8> kChoreographedTriggers{{
    ChoreographedSceneOutput::OnTrigger1,
    ChoreographedSceneOutput::OnTrigger2,
    ChoreographedSceneOutput::OnTrigger3,
    ChoreographedSceneOutput::OnTrigger4,
    ChoreographedSceneOutput::OnTrigger5,
    ChoreographedSceneOutput::OnTrigger6,
    ChoreographedSceneOutput::OnTrigger7,
    ChoreographedSceneOutput::OnTrigger8
}};

constexpr std::array<ScriptedSequenceOutput, 8> kScriptedSequenceEvents{{
    ScriptedSequenceOutput::OnScriptEvent01,
    ScriptedSequenceOutput::OnScriptEvent02,
    ScriptedSequenceOutput::OnScriptEvent03,
    ScriptedSequenceOutput::OnScriptEvent04,
    ScriptedSequenceOutput::OnScriptEvent05,
    ScriptedSequenceOutput::OnScriptEvent06,
    ScriptedSequenceOutput::OnScriptEvent07,
    ScriptedSequenceOutput::OnScriptEvent08
}};

} // namespace

DispatchResult fire_choreographed_scene_output(
    SceneEntityToken scene,
    ChoreographedSceneOutput output,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept {
    if (!host.is_live(scene)) return DispatchResult::MissingEntity;
    return host.fire_choreographed_scene_output(scene, output, context)
        ? DispatchResult::Fired : DispatchResult::HostRejected;
}

DispatchResult dispatch_choreographed_scene_trigger(
    SceneEntityToken scene,
    std::int32_t trigger_number,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept {
    if (trigger_number < 1 ||
        trigger_number > static_cast<std::int32_t>(kChoreographedTriggers.size())) {
        return DispatchResult::InvalidEventNumber;
    }
    return fire_choreographed_scene_output(
        scene, kChoreographedTriggers[static_cast<std::size_t>(trigger_number - 1)],
        context, host);
}

DispatchResult fire_scripted_sequence_output(
    SceneEntityToken sequence,
    ScriptedSequenceOutput output,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept {
    if (!host.is_live(sequence)) return DispatchResult::MissingEntity;
    return host.fire_scripted_sequence_output(sequence, output, context)
        ? DispatchResult::Fired : DispatchResult::HostRejected;
}

DispatchResult dispatch_scripted_sequence_event(
    SceneEntityToken sequence,
    std::int32_t event_number,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept {
    if (event_number < 1 ||
        event_number > static_cast<std::int32_t>(kScriptedSequenceEvents.size())) {
        return DispatchResult::InvalidEventNumber;
    }
    return fire_scripted_sequence_output(
        sequence, kScriptedSequenceEvents[static_cast<std::size_t>(event_number - 1)],
        context, host);
}

DispatchResult dispatch_original_scripted_sequence_animation_event(
    std::uint32_t raw_event_id,
    std::optional<SceneEntityToken> linked_sequence,
    std::optional<std::int32_t> parsed_event_number,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept {
    if (raw_event_id != original_scripted_sequence_animation_event_id)
        return DispatchResult::NotHandled;
    if (!linked_sequence) return DispatchResult::MissingEntity;
    if (!parsed_event_number) return DispatchResult::InvalidEventNumber;
    return dispatch_scripted_sequence_event(
        *linked_sequence, *parsed_event_number, context, host);
}

} // namespace sin_compat::scene_io
