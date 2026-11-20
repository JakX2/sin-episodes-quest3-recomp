#pragma once
// Phase 489: original SiN 2006 C_SoundscapeSystem::Shutdown stop ordering
// and owned lifecycle cleanup, without invoking any engine function.
//
// Original SHA-pinned shutdown at VA 0x240F3EE0, 267 bytes:
// fd966f21a2da53fb0f4bf655c17e06fce64f4f2ed01c731ede98948dbe90d8f8
// The original iterates this+0xA0 entries of size 0x28 backward,
// selecting an engine virtual stop or a legacy direct stop using byte
// +0x24. The source Id is DWORD +0x0C. It then clears this+0xAC,
// +0xC0, +0x28, invalidates up to two EHANDLE values (+0x9C/+0x94),
// and removes *first* loaded KeyValues pointers from this+0x08,
// with count +0x14, deleting each with original helper 0x24166050.
//
// This implementation creates an immutable ordered EVENT PLAN on
// independent strings/integers. It never passes proprietary engine
// objects or original function pointers into this model.
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sin_independent::soundscape_shutdown {
inline constexpr std::uint32_t kOriginalVA=0x240F3EE0u;
inline constexpr std::size_t kOriginalSize=267u;
inline constexpr std::uint32_t kOriginalActiveEntriesOffset=0xA0u;
inline constexpr std::uint32_t kOriginalActiveCountOffset=0xACu;
inline constexpr std::uint32_t kOriginalSecondaryCountOffset=0xC0u;
inline constexpr std::uint32_t kOriginalLoadedPointerCountOffset=0x28u;
inline constexpr std::uint32_t kOriginalLoadedKeyValuesCountOffset=0x14u;
inline constexpr std::uint32_t kOriginalHandleAOffset=0x9Cu;
inline constexpr std::uint32_t kOriginalHandleBOffset=0x94u;
inline constexpr std::size_t kOriginalEntryStride=0x28u;
inline constexpr std::uint32_t kOriginalSoundIdOffset=0x0Cu;
inline constexpr std::uint32_t kOriginalRoutingFlagOffset=0x24u;
inline constexpr std::uint32_t kOriginalMixerVirtualByteOffset=0x2Cu;
inline constexpr std::uint32_t kOriginalLegacyStopVA=0x241BD680u;
inline constexpr std::uint32_t kOriginalKeyValuesDeleteVA=0x24166050u;
inline constexpr std::size_t kMaxOwnedActive=4096u;
inline constexpr std::size_t kMaxOwnedResources=256u;
struct CandidateOriginalEntry40 final {
    unsigned char unknown_before_id[0x0C]{};
    std::uint32_t sound_id=0;
    unsigned char unknown_between_id_and_flag[0x14]{};
    unsigned char use_engine_mixer_stop=0;
    unsigned char unknown_after_flag[3]{};
};
static_assert(sizeof(CandidateOriginalEntry40)==40,"original sound slot stride");
static_assert(offsetof(CandidateOriginalEntry40,sound_id)==0x0C,
              "original sound ID field offset");
static_assert(offsetof(CandidateOriginalEntry40,use_engine_mixer_stop)==0x24,
              "original sound stop path flag offset");
struct Sound final {
    std::uint32_t id=0;
    bool mixer_route=false;
};
struct State final {
    std::vector<Sound> active_sounds;
    std::vector<std::string> loaded_keyvalues_owners;
    std::uint32_t auxiliary_count_c0=0;
    std::int32_t handle_9c=-1;
    std::int32_t handle_94=-1;
};
enum class ActionKind {
    StopMixerVirtual11, // source uses sound interface vtable byte +0x2C
    StopLegacyHelper, // source direct CALL at 0x241BD680
    ClearActiveCount0xAC,
    ClearAuxiliaryCount0xC0,
    ClearLoadedPointerCount0x28,
    ReleaseHandle0x9C,
    ReleaseHandle0x94,
    DeleteKeyValuesFirst
};
struct Action final {
    ActionKind kind=ActionKind::ClearActiveCount0xAC;
    std::uint32_t sound_id=0;
    std::size_t source_index=0; // preserved original position
    std::int32_t candidate_handle=-1;
    std::string resource_name; // owned synthetic audit label only
    // Original mixer argument tuple after sound id is (0,0,4,0).
    // Original legacy helper argument tuple is (0,6,sound id).
    // No playback or pointer invocation occurs.
};
struct Plan final {
    std::uint64_t revision=0;
    std::vector<Action> actions;
    std::size_t sound_stop_count=0;
    std::size_t keyvalues_delete_count=0;
};
enum class Result {
    Ready, InvalidState, CapacityRefused, AllocationRefused,
    StalePlan, PlanRejected
};
inline const char* ResultName(Result result) noexcept {
    switch(result){
        case Result::Ready:return "owned-original-shutdown-plan-ready-no-engine-stop";
        case Result::InvalidState:return "owned-shutdown-state-refused";
        case Result::CapacityRefused:return "owned-shutdown-capacity-refused";
        case Result::AllocationRefused:return "owned-shutdown-allocation-refused";
        case Result::StalePlan:return "owned-shutdown-revision-stale";
        case Result::PlanRejected:return "owned-shutdown-plan-refused";
    }
    return "unexpected";
}
class OwnedLifecycle final {
public:
    const State& value() const noexcept{return state_;}
    std::uint64_t revision() const noexcept{return revision_;}
    void Clear() noexcept{
        state_={};
        ++revision_;
    }
    Result Install(const State& source) noexcept {
        if(source.active_sounds.size()>kMaxOwnedActive ||
           source.loaded_keyvalues_owners.size()>kMaxOwnedResources)
            return Result::CapacityRefused;
        for(const auto& name:source.loaded_keyvalues_owners)
            if(name.empty()||name.size()>256)return Result::InvalidState;
        try {
            State pending=source;
            state_=std::move(pending);
            ++revision_;
            return Result::Ready;
        }catch(...){return Result::AllocationRefused;}
    }
    // Preserve original *ordering* of shutdown side effects. This is
    // only a description of future calls, not active mixer dispatch.
    Result Prepare(Plan& destination) const noexcept {
        try {
            Plan pending{};
            pending.revision=revision_;
            pending.actions.reserve(state_.active_sounds.size()+
                                    state_.loaded_keyvalues_owners.size()+5u);
            for(std::size_t i=state_.active_sounds.size();i>0;--i){
                const auto& sound=state_.active_sounds[i-1];
                pending.actions.push_back({
                    sound.mixer_route?ActionKind::StopMixerVirtual11:
                                      ActionKind::StopLegacyHelper,
                    sound.id,i-1,-1,{}});
                ++pending.sound_stop_count;
            }
            pending.actions.push_back({ActionKind::ClearActiveCount0xAC});
            pending.actions.push_back({ActionKind::ClearAuxiliaryCount0xC0});
            pending.actions.push_back({ActionKind::ClearLoadedPointerCount0x28});
            if(state_.handle_9c!=-1)
                pending.actions.push_back({ActionKind::ReleaseHandle0x9C,
                                           0,0,state_.handle_9c,{}});
            if(state_.handle_94!=-1)
                pending.actions.push_back({ActionKind::ReleaseHandle0x94,
                                           0,0,state_.handle_94,{}});
            // The original repeatedly removes index zero and memmoves
            // later entries left, so deallocation is FIRST -> LAST.
            for(std::size_t i=0;i<state_.loaded_keyvalues_owners.size();++i){
                pending.actions.push_back({
                    ActionKind::DeleteKeyValuesFirst,0,i,-1,
                    state_.loaded_keyvalues_owners[i]});
                ++pending.keyvalues_delete_count;
            }
            destination=std::move(pending);
            return Result::Ready;
        }catch(...){return Result::AllocationRefused;}
    }
    // Synthetic-only final commit, after caller confirms the whole
    // event plan. Engine sound stop side effects are deliberately
    // *not* executed by this method.
    Result CommitCleared(const Plan& plan) noexcept {
        if(plan.revision!=revision_)return Result::StalePlan;
        if(plan.sound_stop_count!=state_.active_sounds.size() ||
           plan.keyvalues_delete_count!=state_.loaded_keyvalues_owners.size())
            return Result::PlanRejected;
        if(plan.actions.size()!=plan.sound_stop_count+
          plan.keyvalues_delete_count+3u+
          static_cast<std::size_t>(state_.handle_9c!=-1)+
          static_cast<std::size_t>(state_.handle_94!=-1))
            return Result::PlanRejected;
        // Never accept a caller-forged plan merely because its revision
        // and total event count happen to match. Verify exact source
        // order, stop route, sound IDs, handles and resource identities.
        std::size_t event=0;
        for(std::size_t i=state_.active_sounds.size();i>0;--i){
            const auto& source=state_.active_sounds[i-1];
            const auto& action=plan.actions[event++];
            if(action.kind!=(source.mixer_route?
                              ActionKind::StopMixerVirtual11:
                              ActionKind::StopLegacyHelper) ||
               action.source_index!=i-1 ||
               action.sound_id!=source.id)
                return Result::PlanRejected;
        }
        if(plan.actions[event++].kind!=ActionKind::ClearActiveCount0xAC ||
           plan.actions[event++].kind!=ActionKind::ClearAuxiliaryCount0xC0 ||
           plan.actions[event++].kind!=ActionKind::ClearLoadedPointerCount0x28)
            return Result::PlanRejected;
        if(state_.handle_9c!=-1){
            const auto& action=plan.actions[event++];
            if(action.kind!=ActionKind::ReleaseHandle0x9C ||
               action.candidate_handle!=state_.handle_9c)
                return Result::PlanRejected;
        }
        if(state_.handle_94!=-1){
            const auto& action=plan.actions[event++];
            if(action.kind!=ActionKind::ReleaseHandle0x94 ||
               action.candidate_handle!=state_.handle_94)
                return Result::PlanRejected;
        }
        for(std::size_t i=0;i<state_.loaded_keyvalues_owners.size();++i){
            const auto& action=plan.actions[event++];
            if(action.kind!=ActionKind::DeleteKeyValuesFirst ||
               action.source_index!=i ||
               action.resource_name!=state_.loaded_keyvalues_owners[i])
                return Result::PlanRejected;
        }
        if(event!=plan.actions.size())return Result::PlanRejected;
        // This is an independent state transition, not a claim that
        // the original game can stop sounds without a mixer callback.
        Clear();
        return Result::Ready;
    }
private:
    State state_{};
    std::uint64_t revision_=0;
};
} // namespace sin_independent::soundscape_shutdown
