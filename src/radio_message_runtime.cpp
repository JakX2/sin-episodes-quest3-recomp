#include "sin_compat/radio_message_runtime.hpp"

#include <string>

namespace sin_compat {

RadioMessageResult OriginalRadioMessageRuntime::set_message(
    std::string_view value) {
    if (!host_.is_live(self_)) return RadioMessageResult::StaleEntity;
    const std::string next(value);
    if (state_.message == next) return RadioMessageResult::Unchanged;
    state_.message = next;
    return RadioMessageResult::Applied;
}

RadioMessageResult OriginalRadioMessageRuntime::set_speaker(
    std::string_view value) {
    if (!host_.is_live(self_)) return RadioMessageResult::StaleEntity;
    const std::string next(value);
    if (state_.speaker == next) return RadioMessageResult::Unchanged;
    state_.speaker = next;
    return RadioMessageResult::Applied;
}

RadioMessageResult OriginalRadioMessageRuntime::set_priority(
    std::int32_t value) {
    if (!host_.is_live(self_)) return RadioMessageResult::StaleEntity;
    if (state_.priority == value) return RadioMessageResult::Unchanged;
    state_.priority = value;
    return RadioMessageResult::Applied;
}

RadioMessageDispatchResult OriginalRadioMessageRuntime::dispatch() {
    if (!host_.is_live(self_))
        return {RadioMessageResult::StaleEntity, 0};

    const auto player = host_.resolve_local_player();
    if (!player || !host_.is_live(*player))
        return {RadioMessageResult::MissingLocalPlayer, 0};

    // The preserved x86 evidence establishes local-player resolution before
    // FireOutput. The portable adapter follows Source's conventional mapping:
    // local player as activator, this CRadioMessage as caller. The exact
    // original argument registers/stack slots still require a fresh raw-byte
    // disassembly and are intentionally isolated to this one call.
    const auto scheduled = entity_io_.fire(
        self_, std::string(kOnDispatchOutput), *player);
    return {RadioMessageResult::OutputInvoked, scheduled};
}

RadioMessageResult OriginalRadioMessageRuntime::remove() {
    if (!host_.is_live(self_)) return RadioMessageResult::StaleEntity;
    return host_.request_remove(self_)
        ? RadioMessageResult::RemoveRequested
        : RadioMessageResult::HostRejected;
}

} // namespace sin_compat
