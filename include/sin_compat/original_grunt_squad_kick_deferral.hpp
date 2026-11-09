#pragma once

// Independently authored, evidence-bounded reconstruction of the original
// SiN CNPC_GruntBase TASK_GRUNT_DEFER_SQUAD_GRENADE_KICK start case.
// The original squad container and resolver ABI remain host-owned.

#include "sin_compat/original_grunt_help_up_task.hpp"
#include "sin_compat/original_grunt_relationship.hpp"

#include <vector>

namespace sin_compat::ai::grunt {

inline constexpr OriginalGruntTaskId task_grunt_defer_squad_grenade_kick{0x9BU};

class IOriginalGruntSquadKickDeferralHost : public IOriginalGruntTaskHost {
public:
    ~IOriginalGruntSquadKickDeferralHost() override = default;

    // The host resolves the original squad structure rooted at grunt +0x9C4
    // through its engine-specific member resolver. Return only resolved live
    // members, preserving the original traversal order. The original squad
    // container/layout is intentionally not represented here.
    virtual std::vector<OriginalGruntEntityToken>
    resolved_squad_members_in_original_order() const = 0;

    // Clears the member's original m_hCurrentGrenade (+0xCB8) to the host's
    // invalid-handle representation. This is not a raw x86 object offset.
    virtual void clear_member_current_grenade_handle(
        OriginalGruntEntityToken member) noexcept = 0;
};

// Handles only the verified start case for task 0x9B. Other tasks continue
// through the existing grunt task dispatcher. The original update path for
// 0x9B is inherited from CAI_BaseHumanoid.
GruntTaskDispatchDisposition start_original_grunt_squad_kick_deferral_task(
    OriginalGruntTaskId task,
    IOriginalGruntSquadKickDeferralHost& host);

} // namespace sin_compat::ai::grunt
