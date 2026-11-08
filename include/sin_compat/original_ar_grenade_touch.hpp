#pragma once

// Reconstructed grenade_ar TOUCH branch gates from SHA-pinned 2006 server.dll.
// Original entry 0x223142F0, return 0x22314353, type CGrenadeAR.
// Exact original x86 evidence and test tool:
// research/tools/audit_sin_original_ar_touch_gate.py
//
// The source accesses contacted entity +0x11C, flags +0x120 bit 0x4,
// invokes a virtual function at offset +0xFC, compares that result against
// another helper's result, conditionally SETS grenade byte +0x795,
// then (only when the byte is nonzero) invokes Detonate slot 250 (+0x3E8).
// None of the unknown fields/pointers is assigned a guessed Source name.
//
// The project's ar_grenade::Runtime::armed() is a portable ONE-SHOT guard,
// NOT the original +0x795 field: the real spawn routine explicitly clears
// +0x795. Thus this adapter requires the original field as an independent
// host observation and does not confuse it with runtime arming.

#include "sin_compat/original_ar_grenade_runtime.hpp"

#include <cstdint>

namespace sin_compat::ar_grenade_touch {

struct OriginalTouchObservation {
    bool other_entity_present = false;
    std::uint32_t other_field_11c = 0;         // raw x86 field; meaning unverified
    std::uint8_t other_flags_120 = 0;         // bit 0x4 blocks touch handling
    bool other_virtual_fc_result_nonnull = false;
    bool other_virtual_fc_matches_local_helper = false;
    bool grenade_original_field_795 = false; // original BYTE, not Runtime::armed
};

enum class Gate : std::uint8_t {
    BlockedNoOtherEntity,
    BlockedOtherField11cZero,
    BlockedOtherFlag120Bit4,
    BlockedOriginalFlag795Zero,
    CallOriginalDetonateSlot250
};

struct Evaluation {
    Gate gate = Gate::BlockedNoOtherEntity;
    bool original_flag_795_set_requested = false;
    bool original_flag_795_after_contact = false;
    bool should_dispatch_detonation() const noexcept {
        return gate == Gate::CallOriginalDetonateSlot250;
    }
};

// Pure, total reconstruction of the original touch routine's observed
// conditional structure. Does NOT itself invoke unknown vtable +0xFC or
// original setter 0x222B2B40. The game host must obtain those observations
// from verified, generation-safe engine entities.
Evaluation evaluate(const OriginalTouchObservation& observation) noexcept;

struct DispatchResult {
    Evaluation evaluation{};
    ar_grenade::StepResult projectile_step = ar_grenade::StepResult::TriggerRejected;
};

// Connect original touch eligibility with PR #151's one-shot portable blast.
// The native game host must separately implement original +0x795 replicated
// field setter effects and trace-selected decal. If such effects are needed,
// call evaluate() first and update the host's field BEFORE invoking this
// adapter. No hidden calls to unverified SourceVR game interfaces occur here.
DispatchResult dispatch(const OriginalTouchObservation& observation,
                        ar_grenade::Runtime& projectile,
                        ar_grenade::Decal selected_decal,
                        ar_grenade::IHost& host);

} // namespace sin_compat::ar_grenade_touch
