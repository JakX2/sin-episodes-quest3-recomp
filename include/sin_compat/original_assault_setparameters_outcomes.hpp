#pragma once

// Original x86 CAI_AssaultBehavior::SetParameters cue-state effects verified
// at 0x220CFC00 / reset helper 0x220CE960 in owner's 2006 server.dll.
// This portable adapter does not implement rallypoint lookup, locking,
// NPC fallback, SourceVR engine interfaces or original class memory layouts.

#include "sin_compat/original_assault_cue_wait.hpp"
#include <cstdint>
#include <string_view>
#include <utility>

namespace sin_compat {

enum class OriginalAssaultParameterOutcome : std::uint8_t {
    NoRallyCandidate,
    UnresolvedRally,
    ResolvedRally,
};

// Only models two verified cue fields. Successful original reset additionally
// clears active assault handles and flags that the real host must handle.
inline bool apply_original_setparameters_cue_effects(
    OriginalAssaultCueGate& cue,
    std::int32_t requested_mode,
    OriginalAssaultParameterOutcome outcome) noexcept {
    switch (outcome) {
        case OriginalAssaultParameterOutcome::NoRallyCandidate:
            return false; // no mode/received-cue writes on this original exit
        case OriginalAssaultParameterOutcome::UnresolvedRally:
            cue.configured_mode = 0; // no received-cue reset here
            return false;
        case OriginalAssaultParameterOutcome::ResolvedRally:
            cue.configured_mode = requested_mode;
            cue.received_cue = 0;
            return true;
    }
    return false; // unknown host outcome must fail closed
}

// Explicit NEW host callback: the outcome classification is not an original
// C++ return type. The original routine returns with 'ret 8'. The old bool-
// only goal setup reference stays unchanged for its existing callers.
template <typename HostSelectRally>
OriginalAssaultParameterOutcome configure_original_assault_parameters(
    OriginalAssaultCueGate& cue,
    std::uint64_t actor_token,
    std::string_view rally_name,
    std::int32_t requested_mode,
    HostSelectRally&& select_rally) {
    const auto result = std::forward<HostSelectRally>(select_rally)(
        actor_token, rally_name, requested_mode);
    apply_original_setparameters_cue_effects(cue, requested_mode, result);
    return result;
}

} // namespace sin_compat
