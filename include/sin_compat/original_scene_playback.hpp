#pragma once

// Engine-neutral reconstruction of the verified CSceneEntity playback inputs.
// Original x86 addresses/offsets are evidence labels only, never a native ABI.

#include "sin_compat/original_scene_io.hpp"

#include <optional>

namespace sin_compat::scene_io {

enum class ScenePlaybackInput : std::uint8_t {
    Start,
    Pause,
    Resume,
    Cancel
};

enum class ScenePlaybackInputResult : std::uint8_t {
    Started,
    IgnoredAlreadyPlaying,
    IgnoredWaitingForActor,
    Paused,
    Resumed,
    Canceled,
    MissingEntity,
    InvalidInput
};

struct ScenePlaybackInputDispatch {
    ScenePlaybackInputResult result = ScenePlaybackInputResult::InvalidInput;

    // Present only for Start. Source FireOutput is a void operation; this
    // reports whether the portable host accepted the output callback without
    // changing the original start path's continuation.
    std::optional<bool> on_start_output_accepted;
};

// The host owns the real engine entity, playback implementation and state.
// The callbacks correspond to original call boundaries, not guessed native
// SourceVR virtual methods. prepare_start excludes the OnStart output itself:
// this dispatcher emits that output between prestart and playback.
class IOriginalScenePlaybackHost : public IOriginalSceneIoHost {
public:
    virtual bool is_playing(SceneEntityToken scene) const noexcept = 0;
    virtual bool is_waiting_for_actor(SceneEntityToken scene) const noexcept = 0;

    // CSceneEntity prestart routine, excluding the OnStart FireOutput call.
    virtual void prepare_start(SceneEntityToken scene) noexcept = 0;

    // Original InputStart delegates to the playback virtual after prestart.
    virtual void start_playback(SceneEntityToken scene) noexcept = 0;

    // InputPause calls the pause virtual, then writes the pause flag true.
    virtual void pause_playback(SceneEntityToken scene) noexcept = 0;
    virtual void set_pause_flag(SceneEntityToken scene, bool paused) noexcept = 0;

    // Resume and Cancel delegate to their original virtual boundaries. Their
    // internal flag/output side effects remain the host's responsibility.
    virtual void resume_playback(SceneEntityToken scene) noexcept = 0;
    virtual void cancel_playback(SceneEntityToken scene) noexcept = 0;
};

ScenePlaybackInputDispatch dispatch_original_scene_playback_input(
    SceneEntityToken scene,
    ScenePlaybackInput input,
    OutputContextToken context,
    IOriginalScenePlaybackHost& host) noexcept;

} // namespace sin_compat::scene_io
