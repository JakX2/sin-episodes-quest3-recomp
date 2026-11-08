#pragma once
// Independent, bounded model of SiN Episodes: Emergence (2006) server.dll
// second pose processing pass, specifically 0x22103D70..0x22103D8A and
// 0x22103E15..0x22103F1C. Does NOT reconstruct its animation-effect loop.
#include "sin_compat/original_pose_driver_first_pass.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sin_compat::jessica {

// Original 0xE4-byte native track layout has the following second-pass
// metadata. Separate values here instead of relying on MSVC x86 packing.
struct OriginalPoseSecondPassState {
    bool full_cap_mode = false;                // native track +0xA0
    std::uint8_t history_active = 0;           // +0xA1 (preserved/cleared)
    std::uint32_t history_accumulator_bits = 0;// +0xA4 (opaque DWORD)
    std::int32_t last_sequence = -1;           // +0xA8
};

// Original second loop calls recursive bone evaluation and several matrix
// helpers before updating track flags. Native model/studio/pose indexing is
// left behind this callback until its ABI is independently established.
class IOriginalPoseSecondPassGeometry {
public:
    virtual ~IOriginalPoseSecondPassGeometry()=default;
    virtual bool evaluate_active_track(std::size_t index,
                                      const OriginalPoseDriverTrack& track)=0;
};

enum class OriginalPoseSecondPassCode : std::uint8_t {
    Applied, InvalidTrackCount, InvalidFloat, GeometryRejected, HostRejected
};
struct OriginalPoseSecondPassResult {
    OriginalPoseSecondPassCode code=OriginalPoseSecondPassCode::HostRejected;
    std::vector<OriginalPoseSecondPassState> states;
    std::size_t skipped=0, evaluated=0, histories_cleared=0;
    bool successful()const noexcept {return code==OriginalPoseSecondPassCode::Applied;}
};

// For tracks whose first-pass influence is >0, original evaluates geometry,
// tests cap == 1.0 for +0xA0, and retains history only if the sequence index
// stays at its previous value or advances by exactly one. Non-positive
// influence skips the entire body and preserves the prior metadata.
// This function has value-owned output; failure rolls back output state,
// though host callbacks may have performed external work.
OriginalPoseSecondPassResult apply_original_pose_second_pass_state(
    const std::vector<OriginalPoseDriverTrack>& tracks,
    std::int32_t current_sequence,
    const std::vector<OriginalPoseSecondPassState>& previous_states={},
    IOriginalPoseSecondPassGeometry* geometry=nullptr) noexcept;

// Copies metadata from an owner-supplied 0xE4-byte track snapshot; never
// reads live original pointers. Rejected on malformed bounds.
std::optional<OriginalPoseSecondPassState> parse_original_pose_second_track_state(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset=0) noexcept;

} // namespace sin_compat::jessica
