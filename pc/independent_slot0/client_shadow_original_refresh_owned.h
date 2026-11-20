#pragma once
// Phase 491: source-authenticated CClientShadowMgr refresh helper
// VA 0x24115E80 (125 original bytes, SHA256
// 64b3d9cf23200b29329f83836b2507c243dfe9b49ac122aa86a2a5af2973dc66).
//
// Original helper sets this+0xCE=1, walks 16-bit linked indices
// starting at this+0x2C; 0xFFFF is sentinel. Each indexed entry
// is 0x78 (120) bytes; +0x08 bit 1 chooses a route, +0x04 holds
// a 16-bit external ID, +0x76 holds next index. It calls one of
// two internal helpers and optionally a third global interface call,
// then always calls helper 0x24114A90 on this+0xDC, sets +0xCE=0.
// This is an owned EVENT TRACE only, no original memory or engine ABI.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_independent::shadow_refresh {
inline constexpr std::uint32_t kOriginalRefreshVA=0x24115E80u;
inline constexpr std::size_t kOriginalRefreshBytes=125u;
inline constexpr std::uint32_t kOriginalOwnerHeadOffset=0x2Cu;
inline constexpr std::uint32_t kOriginalOwnerArrayOffset=0x20u;
inline constexpr std::uint32_t kOriginalOwnerGuardOffset=0xCEu;
inline constexpr std::uint32_t kOriginalOwnerFinalizeOffset=0xDCu;
inline constexpr std::size_t kOriginalRecordStride=0x78u;
inline constexpr std::size_t kOriginalEntryExternalIdOffset=0x04u;
inline constexpr std::size_t kOriginalEntryModeByteOffset=0x08u;
inline constexpr std::size_t kOriginalEntryNextIndexOffset=0x76u;
inline constexpr std::uint16_t kInvalidIndex=0xFFFFu;
inline constexpr std::uint8_t kOriginalFlagMask=0x01u;
inline constexpr std::uint32_t kOriginalExternalInterfaceGlobalVA=0x2433F6C0u;
inline constexpr std::uint32_t kOriginalExternalVirtualByteOffset=0x38u;
inline constexpr std::uint32_t kOriginalActiveHelperVA=0x24113EA0u;
inline constexpr std::uint32_t kOriginalInactiveHelperVA=0x24113F60u;
inline constexpr std::uint32_t kOriginalFinalizeHelperVA=0x24114A90u;
inline constexpr std::size_t kMaxOwnedShadowEntries=4096u;

struct CandidateEntry120 final {
    unsigned char original_opaque_prefix[0x04]{};
    std::uint16_t external_id=0;
    unsigned char original_opaque_preflag[0x02]{};
    std::uint8_t flags=0;
    unsigned char original_opaque_middle[0x6Du]{};
    std::uint16_t next=kInvalidIndex;
};
static_assert(sizeof(CandidateEntry120)==120,"source shadow record stride");
static_assert(offsetof(CandidateEntry120,external_id)==4,
              "source external 16-bit ID at entry+4");
static_assert(offsetof(CandidateEntry120,flags)==8,
              "source route flag at entry+8");
static_assert(offsetof(CandidateEntry120,next)==118,
              "source next index at entry+0x76");

struct Entry final {
    std::uint16_t next=kInvalidIndex;
    std::uint16_t external_id=0;
    bool active_branch=false;
};
struct State final {
    std::uint16_t head=kInvalidIndex;
    std::vector<Entry> entries;
};
enum class ActionKind {
    EnterGuardSetCe,
    CallExternalVirtual14, // original global+vtable byte+0x38
    CallActiveHelper0x24113EA0,
    CallInactiveHelper0x24113F60,
    CallFinalizeHelper0x24114A90,
    LeaveGuardClearCe
};
struct Action final {
    ActionKind kind=ActionKind::EnterGuardSetCe;
    std::uint16_t record_index=kInvalidIndex;
    std::uint16_t external_id=0;
};
struct Plan final {
    std::uint64_t state_revision=0;
    std::vector<Action> ordered;
    std::size_t traversed_entries=0;
    std::size_t external_calls=0;
    std::size_t active_helper_calls=0;
    std::size_t inactive_helper_calls=0;
};
enum class Result {
    Ready, OverCapacity, BadHeadIndex, BadNextIndex,
    LinkedCycle, AllocationRefused, StalePlan, PlanRejected
};
inline std::string_view ResultName(Result result) noexcept {
    switch(result){
        case Result::Ready:return "owned-shadow-refresh-plan-ready-no-renderer-calls";
        case Result::OverCapacity:return "owned-shadow-refresh-capacity-refused";
        case Result::BadHeadIndex:return "owned-shadow-refresh-head-refused";
        case Result::BadNextIndex:return "owned-shadow-refresh-next-index-refused";
        case Result::LinkedCycle:return "owned-shadow-refresh-cycle-refused";
        case Result::AllocationRefused:return "owned-shadow-refresh-allocation-refused";
        case Result::StalePlan:return "owned-shadow-refresh-plan-stale";
        case Result::PlanRejected:return "owned-shadow-refresh-plan-modified";
    }
    return "unknown";
}
class OwnedRefresh final {
public:
    const State& value() const noexcept {return state_;}
    std::uint64_t revision() const noexcept{return revision_;}
    std::uint64_t completed_simulated_refreshes() const noexcept {
        return completed_;
    }
    std::uint8_t modeled_guard_ce() const noexcept{return guard_ce_;}
    void Clear() noexcept {state_={};++revision_;guard_ce_=0;completed_=0;}
    Result Install(const State& source) noexcept {
        if(source.entries.size()>kMaxOwnedShadowEntries)
            return Result::OverCapacity;
        if(source.head!=kInvalidIndex &&
           source.head>=source.entries.size())
            return Result::BadHeadIndex;
        try{
            State next=source;
            state_=std::move(next);
            ++revision_;
            return Result::Ready;
        }catch(...){return Result::AllocationRefused;}
    }
    Result Prepare(Plan& output) const noexcept {
        try {
            Plan pending{};
            pending.state_revision=revision_;
            pending.ordered.reserve(state_.entries.size()*2u+3u);
            std::vector<unsigned char> visited(state_.entries.size(),0);
            pending.ordered.push_back({ActionKind::EnterGuardSetCe});
            std::uint16_t at=state_.head;
            while(at!=kInvalidIndex){
                if(at>=state_.entries.size())
                    return Result::BadNextIndex;
                if(visited[at])return Result::LinkedCycle;
                visited[at]=1;
                const auto& entry=state_.entries[at];
                if(entry.active_branch){
                    pending.ordered.push_back({
                        ActionKind::CallExternalVirtual14,at,entry.external_id});
                    ++pending.external_calls;
                    pending.ordered.push_back({
                        ActionKind::CallActiveHelper0x24113EA0,at,
                        entry.external_id});
                    ++pending.active_helper_calls;
                }else{
                    pending.ordered.push_back({
                        ActionKind::CallInactiveHelper0x24113F60,at,
                        entry.external_id});
                    ++pending.inactive_helper_calls;
                }
                ++pending.traversed_entries;
                at=entry.next;
            }
            pending.ordered.push_back({ActionKind::CallFinalizeHelper0x24114A90});
            pending.ordered.push_back({ActionKind::LeaveGuardClearCe});
            output=std::move(pending);
            return Result::Ready;
        }catch(...){return Result::AllocationRefused;}
    }
    // This deliberately applies only the local guard/counter state,
    // after exact trace validation. It DOES NOT perform external or
    // helper calls, so no actual retail renderer refresh is claimed.
    Result SimulateComplete(const Plan& plan) noexcept {
        if(plan.state_revision!=revision_)return Result::StalePlan;
        Plan expected{};
        if(Prepare(expected)!=Result::Ready)return Result::PlanRejected;
        if(plan.ordered.size()!=expected.ordered.size() ||
           plan.traversed_entries!=expected.traversed_entries ||
           plan.external_calls!=expected.external_calls ||
           plan.active_helper_calls!=expected.active_helper_calls ||
           plan.inactive_helper_calls!=expected.inactive_helper_calls)
            return Result::PlanRejected;
        for(std::size_t i=0;i<expected.ordered.size();++i){
            const auto& a=expected.ordered[i];
            const auto& b=plan.ordered[i];
            if(a.kind!=b.kind||a.record_index!=b.record_index||
               a.external_id!=b.external_id)
                return Result::PlanRejected;
        }
        guard_ce_=0; // original clear at exit
        ++completed_;
        return Result::Ready;
    }
private:
    State state_{};
    std::uint64_t revision_=0;
    std::uint64_t completed_=0;
    std::uint8_t guard_ce_=0;
};
} // namespace sin_independent::shadow_refresh
