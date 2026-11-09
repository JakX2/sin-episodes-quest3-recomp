#include "sin_compat/original_scene_playback.hpp"

namespace sin_compat::scene_io {

ScenePlaybackInputDispatch dispatch_original_scene_playback_input(
    SceneEntityToken scene,
    ScenePlaybackInput input,
    OutputContextToken context,
    IOriginalScenePlaybackHost& host) noexcept {
    if (!host.is_live(scene)) {
        return {ScenePlaybackInputResult::MissingEntity, std::nullopt};
    }

    switch (input) {
    case ScenePlaybackInput::Start: {
        // The original handler tests active playback before actor-wait state.
        if (host.is_playing(scene)) {
            return {ScenePlaybackInputResult::IgnoredAlreadyPlaying, std::nullopt};
        }
        if (host.is_waiting_for_actor(scene)) {
            return {ScenePlaybackInputResult::IgnoredWaitingForActor, std::nullopt};
        }

        host.prepare_start(scene);

        // The original choreography start path fires OnStart during prestart,
        // before InputStart invokes the playback virtual. FireOutput has no
        // control-flow return in the original path, so host rejection is
        // reported but must not suppress playback.
        const DispatchResult output_result = fire_choreographed_scene_output(
            scene, ChoreographedSceneOutput::OnStart, context, host);
        const bool output_accepted = output_result == DispatchResult::Fired;

        host.start_playback(scene);
        return {ScenePlaybackInputResult::Started, output_accepted};
    }

    case ScenePlaybackInput::Pause:
        // Preserve the observed call order: virtual pause, then pause flag.
        host.pause_playback(scene);
        host.set_pause_flag(scene, true);
        return {ScenePlaybackInputResult::Paused, std::nullopt};

    case ScenePlaybackInput::Resume:
        // The original handler delegates to its resume virtual. The handler
        // itself does not establish a separate pause-flag write.
        host.resume_playback(scene);
        return {ScenePlaybackInputResult::Resumed, std::nullopt};

    case ScenePlaybackInput::Cancel:
        // Cancellation side effects, including any OnCanceled output, belong
        // to the original cancel virtual. Do not fire the output a second time.
        host.cancel_playback(scene);
        return {ScenePlaybackInputResult::Canceled, std::nullopt};
    }

    return {ScenePlaybackInputResult::InvalidInput, std::nullopt};
}

} // namespace sin_compat::scene_io
