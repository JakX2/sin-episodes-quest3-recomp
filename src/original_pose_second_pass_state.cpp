#include "sin_compat/original_pose_second_pass_state.hpp"
#include <cmath>
#include <cstring>

namespace sin_compat::jessica {
namespace {
constexpr std::size_t kMaxTracks=4096, kTrackStride=0xE4;
using Code=OriginalPoseSecondPassCode;
std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes,
                       std::size_t at) noexcept {
    return std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at+1])<<8) |
           (std::uint32_t(bytes[at+2])<<16) | (std::uint32_t(bytes[at+3])<<24);
}
}
std::optional<OriginalPoseSecondPassState> parse_original_pose_second_track_state(
    const std::vector<std::uint8_t>& bytes,std::size_t offset) noexcept {
    if(offset>bytes.size() || bytes.size()-offset<kTrackStride)
        return std::nullopt;
    OriginalPoseSecondPassState state{};
    state.full_cap_mode=bytes[offset+0xA0]!=0;
    state.history_active=bytes[offset+0xA1];
    state.history_accumulator_bits=read_u32(bytes,offset+0xA4);
    const auto raw=read_u32(bytes,offset+0xA8);
    static_assert(sizeof(state.last_sequence)==sizeof(raw),"32-bit int");
    std::memcpy(&state.last_sequence,&raw,sizeof(raw));
    return state;
}
OriginalPoseSecondPassResult apply_original_pose_second_pass_state(
    const std::vector<OriginalPoseDriverTrack>& tracks,std::int32_t sequence,
    const std::vector<OriginalPoseSecondPassState>& previous,
    IOriginalPoseSecondPassGeometry* geometry) noexcept {
    try {
        if(tracks.size()>kMaxTracks ||
           (!previous.empty() && previous.size()!=tracks.size()))
            return {Code::InvalidTrackCount,{}};
        OriginalPoseSecondPassResult result{};
        result.states=previous.empty()
            ?std::vector<OriginalPoseSecondPassState>(tracks.size()) :previous;
        for(std::size_t i=0;i<tracks.size();++i){
            const auto& track=tracks[i];
            // Native 0x22103D73 fcomp 0.0 / 0x22103D81 test ah,0x41:
            // zero, negative and unordered (NaN) influences all skip the track.
            if(std::isnan(track.influence)||track.influence<=0.f){
                ++result.skipped;
                continue;
            }
            // Positive nonfinite influence is rejected before crossing the
            // host geometry boundary. Cap is different: the original FUCOMPP
            // comparison treats unordered/nonfinite values as non-equal to 1.
            if(!std::isfinite(track.influence))
                return {Code::InvalidFloat,{}};
            if(geometry && !geometry->evaluate_active_track(i,track))
                return {Code::GeometryRejected,{}};
            auto& state=result.states[i];
            // 0x22103E15..0x22103E35 compares constant 1.0 to cap with
            // FUCOMPP, then parity of tested C2/C3. Only equality sets 1.
            // C++ equality also returns false for NaN and either infinity,
            // matching the original unordered/non-equal branch.
            state.full_cap_mode=(track.cap==1.f);
            // 0x22103EEA..0x22103F13: negative current index, or an index
            // farther away than one forward step, invalidates history.
            const bool contiguous = sequence>=0 &&
                state.last_sequence>=sequence-1 &&
                state.last_sequence<=sequence;
            if(!contiguous){
                state.history_active=0;
                state.history_accumulator_bits=0;
                ++result.histories_cleared;
            }
            state.last_sequence=sequence;
            ++result.evaluated;
        }
        result.code=Code::Applied;
        return result;
    }catch(...){return {Code::HostRejected,{}};}
}
} // namespace sin_compat::jessica
