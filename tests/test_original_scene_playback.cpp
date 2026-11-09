#include "sin_compat/original_scene_playback.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace sin_compat::scene_io;

namespace {

class FakeScenePlaybackHost final : public IOriginalScenePlaybackHost {
public:
    std::set<SceneEntityToken> live{7U};
    mutable std::vector<std::string> calls;
    bool playing = false;
    bool waiting_for_actor = false;
    bool accept_start_output = true;
    bool pause_flag = false;
    int pause_flag_writes = 0;
    OutputContextToken last_context = 0U;

    bool is_live(SceneEntityToken scene) const noexcept override {
        return live.count(scene) != 0U;
    }

    bool fire_choreographed_scene_output(
        SceneEntityToken scene,
        ChoreographedSceneOutput output,
        OutputContextToken context) noexcept override {
        assert(scene == 7U);
        assert(output == ChoreographedSceneOutput::OnStart);
        calls.emplace_back("OnStart");
        last_context = context;
        return accept_start_output;
    }

    bool fire_scripted_sequence_output(
        SceneEntityToken,
        ScriptedSequenceOutput,
        OutputContextToken) noexcept override {
        calls.emplace_back("unexpected_scripted_output");
        return false;
    }

    bool is_playing(SceneEntityToken scene) const noexcept override {
        assert(scene == 7U);
        calls.emplace_back("is_playing");
        return playing;
    }

    bool is_waiting_for_actor(SceneEntityToken scene) const noexcept override {
        assert(scene == 7U);
        calls.emplace_back("is_waiting");
        return waiting_for_actor;
    }

    void prepare_start(SceneEntityToken scene) noexcept override {
        assert(scene == 7U);
        calls.emplace_back("prestart");
    }

    void start_playback(SceneEntityToken scene) noexcept override {
        assert(scene == 7U);
        calls.emplace_back("start_playback");
    }

    void pause_playback(SceneEntityToken scene) noexcept override {
        assert(scene == 7U);
        calls.emplace_back("pause_virtual");
    }

    void set_pause_flag(SceneEntityToken scene, bool paused) noexcept override {
        assert(scene == 7U);
        calls.emplace_back(paused ? "pause_flag_true" : "pause_flag_false");
        ++pause_flag_writes;
        pause_flag = paused;
    }

    void resume_playback(SceneEntityToken scene) noexcept override {
        assert(scene == 7U);
        calls.emplace_back("resume_virtual");
    }

    void cancel_playback(SceneEntityToken scene) noexcept override {
        assert(scene == 7U);
        calls.emplace_back("cancel_virtual");
    }
};

void test_start_preserves_guard_prestart_output_and_playback_order() {
    FakeScenePlaybackHost host;
    constexpr OutputContextToken context = 0xCAFEU;

    const auto result = dispatch_original_scene_playback_input(
        7U, ScenePlaybackInput::Start, context, host);

    assert(result.result == ScenePlaybackInputResult::Started);
    assert(result.on_start_output_accepted.has_value());
    assert(*result.on_start_output_accepted);
    assert((host.calls == std::vector<std::string>{
        "is_playing", "is_waiting", "prestart", "OnStart", "start_playback"}));
    assert(host.last_context == context);
}

void test_start_output_rejection_does_not_cancel_original_continuation() {
    FakeScenePlaybackHost host;
    host.accept_start_output = false;

    const auto result = dispatch_original_scene_playback_input(
        7U, ScenePlaybackInput::Start, 19U, host);

    assert(result.result == ScenePlaybackInputResult::Started);
    assert(result.on_start_output_accepted.has_value());
    assert(!*result.on_start_output_accepted);
    assert(host.calls.back() == "start_playback");
}

void test_start_active_playback_guard_short_circuits_first() {
    FakeScenePlaybackHost host;
    host.playing = true;

    const auto result = dispatch_original_scene_playback_input(
        7U, ScenePlaybackInput::Start, 0U, host);

    assert(result.result == ScenePlaybackInputResult::IgnoredAlreadyPlaying);
    assert(!result.on_start_output_accepted.has_value());
    assert((host.calls == std::vector<std::string>{"is_playing"}));
}

void test_start_actor_wait_guard_runs_after_active_guard() {
    FakeScenePlaybackHost host;
    host.waiting_for_actor = true;

    const auto result = dispatch_original_scene_playback_input(
        7U, ScenePlaybackInput::Start, 0U, host);

    assert(result.result == ScenePlaybackInputResult::IgnoredWaitingForActor);
    assert((host.calls == std::vector<std::string>{"is_playing", "is_waiting"}));
}

void test_pause_calls_virtual_before_setting_pause_flag() {
    FakeScenePlaybackHost host;

    const auto result = dispatch_original_scene_playback_input(
        7U, ScenePlaybackInput::Pause, 0U, host);

    assert(result.result == ScenePlaybackInputResult::Paused);
    assert((host.calls == std::vector<std::string>{
        "pause_virtual", "pause_flag_true"}));
    assert(host.pause_flag);
}

void test_resume_delegates_without_fabricating_a_pause_flag_write() {
    FakeScenePlaybackHost host;
    host.pause_flag = true;

    const auto result = dispatch_original_scene_playback_input(
        7U, ScenePlaybackInput::Resume, 0U, host);

    assert(result.result == ScenePlaybackInputResult::Resumed);
    assert((host.calls == std::vector<std::string>{"resume_virtual"}));
    assert(host.pause_flag_writes == 0);
}

void test_cancel_delegates_without_double_firing_on_canceled() {
    FakeScenePlaybackHost host;

    const auto result = dispatch_original_scene_playback_input(
        7U, ScenePlaybackInput::Cancel, 0U, host);

    assert(result.result == ScenePlaybackInputResult::Canceled);
    assert((host.calls == std::vector<std::string>{"cancel_virtual"}));
}

void test_missing_scene_handle_has_no_side_effects() {
    FakeScenePlaybackHost host;

    const auto result = dispatch_original_scene_playback_input(
        99U, ScenePlaybackInput::Start, 0U, host);

    assert(result.result == ScenePlaybackInputResult::MissingEntity);
    assert(host.calls.empty());
}

void test_invalid_input_is_rejected() {
    FakeScenePlaybackHost host;

    const auto result = dispatch_original_scene_playback_input(
        7U, static_cast<ScenePlaybackInput>(0xFFU), 0U, host);

    assert(result.result == ScenePlaybackInputResult::InvalidInput);
    assert(host.calls.empty());
}

} // namespace

int main() {
    test_start_preserves_guard_prestart_output_and_playback_order();
    test_start_output_rejection_does_not_cancel_original_continuation();
    test_start_active_playback_guard_short_circuits_first();
    test_start_actor_wait_guard_runs_after_active_guard();
    test_pause_calls_virtual_before_setting_pause_flag();
    test_resume_delegates_without_fabricating_a_pause_flag_write();
    test_cancel_delegates_without_double_firing_on_canceled();
    test_missing_scene_handle_has_no_side_effects();
    test_invalid_input_is_rejected();
    std::puts("PASS: original CSceneEntity Start/Pause/Resume/Cancel input order (mock host only)");
    return 0;
}
