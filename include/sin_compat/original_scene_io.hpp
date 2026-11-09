#pragma once

// Independently authored, engine-neutral routing for original SiN scene and
// scripted-sequence map outputs. No Source object layout or vtable is reused.

#include <cstdint>
#include <optional>

namespace sin_compat::scene_io {

using SceneEntityToken = std::uint64_t;
using OutputContextToken = std::uint64_t;

// Portable symbolic IDs. These are not original object offsets or ABI slots.
enum class ChoreographedSceneOutput : std::uint8_t {
    OnStart = 0,
    OnCompletion = 1,
    OnCanceled = 2,
    OnTrigger1 = 3,
    OnTrigger2 = 4,
    OnTrigger3 = 5,
    OnTrigger4 = 6,
    OnTrigger5 = 7,
    OnTrigger6 = 8,
    OnTrigger7 = 9,
    OnTrigger8 = 10
};

enum class ScriptedSequenceOutput : std::uint8_t {
    OnBeginSequence = 0,
    OnEndSequence = 1,
    OnScriptEvent01 = 2,
    OnScriptEvent02 = 3,
    OnScriptEvent03 = 4,
    OnScriptEvent04 = 5,
    OnScriptEvent05 = 6,
    OnScriptEvent06 = 7,
    OnScriptEvent07 = 8,
    OnScriptEvent08 = 9
};

enum class DispatchResult : std::uint8_t {
    Fired,
    NotHandled,
    InvalidEventNumber,
    MissingEntity,
    HostRejected
};

// The host owns Source handles, output objects, activator/caller/value data,
// and the engine's FireOutput implementation. context is an opaque host token;
// this library never interprets it or treats it as a Source ABI structure.
class IOriginalSceneIoHost {
public:
    virtual ~IOriginalSceneIoHost() = default;

    virtual bool is_live(SceneEntityToken entity) const noexcept = 0;

    virtual bool fire_choreographed_scene_output(
        SceneEntityToken scene,
        ChoreographedSceneOutput output,
        OutputContextToken context) noexcept = 0;

    virtual bool fire_scripted_sequence_output(
        SceneEntityToken sequence,
        ScriptedSequenceOutput output,
        OutputContextToken context) noexcept = 0;
};

inline constexpr std::uint32_t original_scripted_sequence_animation_event_id = 0x3EBU;

DispatchResult fire_choreographed_scene_output(
    SceneEntityToken scene,
    ChoreographedSceneOutput output,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept;

// Original CSceneEntity numbered trigger dispatcher accepts 1..8 only.
DispatchResult dispatch_choreographed_scene_trigger(
    SceneEntityToken scene,
    std::int32_t trigger_number,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept;

DispatchResult fire_scripted_sequence_output(
    SceneEntityToken sequence,
    ScriptedSequenceOutput output,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept;

// Original CAI_ScriptedSequence numbered output dispatcher accepts 1..8 only.
DispatchResult dispatch_scripted_sequence_event(
    SceneEntityToken sequence,
    std::int32_t event_number,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept;

// The fixed raw animation-event ID is preserved without assigning it an
// unverified symbolic name. The host resolves m_hCine and parses the event
// parameter before calling this portable dispatcher.
DispatchResult dispatch_original_scripted_sequence_animation_event(
    std::uint32_t raw_event_id,
    std::optional<SceneEntityToken> linked_sequence,
    std::optional<std::int32_t> parsed_event_number,
    OutputContextToken context,
    IOriginalSceneIoHost& host) noexcept;

} // namespace sin_compat::scene_io
