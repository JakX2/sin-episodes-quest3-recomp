#pragma once

// Independently authored state model for the original embedded
// CAI_BeserkBehavior in the 2006 SiN CNPC_GruntBase object.
// The activation trigger is not recovered; this state starts inactive.

#include "sin_compat/original_grunt_relationship.hpp"

#include <cstdint>
#include <optional>

namespace sin_compat::ai::grunt {

inline constexpr std::int32_t original_grunt_beserk_activation_seconds = 10;
inline constexpr std::uint32_t original_grunt_schedule_beserk = 0x171U;

class IOriginalGruntBeserkHost {
public:
    virtual ~IOriginalGruntBeserkHost() = default;

    // Confirmed first-activation call: original shared condition 0x22,
    // registered as COND_SCHEDULE_DONE.
    virtual void set_schedule_done_condition() noexcept = 0;

    // The original activation routine calls another NPC operation with arg 3.
    // Its method identity and semantic purpose are unresolved.
    virtual void invoke_unresolved_npc_operation(
        std::int32_t argument) noexcept = 0;

    // Original code may write 3 to the owning NPC field at +0x858. The
    // condition/guard is not yet reconstructed; the host owns that decision.
    virtual void apply_unresolved_npc_state_change_if_required() noexcept = 0;
};

class OriginalGruntBeserkBehavior {
public:
    // Mirrors the embedded behavior constructor: inactive, deadline=curtime.
    explicit OriginalGruntBeserkBehavior(float current_time) noexcept;

    void initialize(float current_time) noexcept;

    // Mirrors activation-like routine 0x22391D00. It sets deadline first,
    // runs first-activation callbacks only on inactive->active, then sets
    // the active byte. The real trigger/caller remains unverified.
    void activate(float current_time,
                  IOriginalGruntBeserkHost& host) noexcept;

    // Mirrors expiration routine 0x22391CB0: active clears only when
    // current_time is strictly greater than the saved deadline.
    void update_expiration(float current_time) noexcept;

    bool active() const noexcept;
    float deadline() const noexcept;

private:
    bool active_ = false;
    float deadline_ = 0.0F;
};

// Original behavior method at 0x22392700 reads a DISTINCT byte at behavior
// +0x18 and returns SCHED_BESERK (0x171) when set. It is not the active byte
// at +0x11. The host supplies this separate request flag.
std::optional<std::uint32_t> original_grunt_beserk_schedule(
    bool separate_schedule_request_flag) noexcept;

} // namespace sin_compat::ai::grunt
