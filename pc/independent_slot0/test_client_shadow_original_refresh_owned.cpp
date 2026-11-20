// Phase 491 synthetic native Win32 test. No original renderer calls.
// Source-proven linked 0x78-byte entries, 16-bit next and two branches.
#include "client_shadow_original_refresh_owned.h"
#include <cstddef>
#include <cstdio>

namespace s=sin_independent::shadow_refresh;
static int Fail(int n){
    std::fprintf(stderr,"source shadow refresh owned fixture failed %d\n",n);
    return n;
}
static bool Is(const s::Action& a,s::ActionKind kind,
               std::uint16_t at=s::kInvalidIndex,
               std::uint16_t id=0) {
    return a.kind==kind && a.record_index==at && a.external_id==id;
}
int main(){
    static_assert(sizeof(s::CandidateEntry120)==0x78,"source stride");
    static_assert(offsetof(s::CandidateEntry120,flags)==0x8,"source flag");
    static_assert(offsetof(s::CandidateEntry120,next)==0x76,"source next");
    s::OwnedRefresh owned{};
    s::Plan plan{};
    if(owned.Prepare(plan)!=s::Result::Ready ||
       plan.ordered.size()!=3 ||
       !Is(plan.ordered[0],s::ActionKind::EnterGuardSetCe) ||
       !Is(plan.ordered[1],s::ActionKind::CallFinalizeHelper0x24114A90) ||
       !Is(plan.ordered[2],s::ActionKind::LeaveGuardClearCe))
        return Fail(1);

    // Source order: index 0 -> index 2 -> index 1.
    // Index 3 exists but is unreachable and must never be processed.
    s::State source{};
    source.head=0;
    source.entries={
        {2,0x1234,false},
        {s::kInvalidIndex,0x3456,false},
        {1,0x2345,true},
        {s::kInvalidIndex,0x4567,true}
    };
    if(owned.Install(source)!=s::Result::Ready ||
       owned.Prepare(plan)!=s::Result::Ready ||
       plan.ordered.size()!=7 ||
       plan.traversed_entries!=3 ||
       plan.active_helper_calls!=1 ||
       plan.inactive_helper_calls!=2 ||
       plan.external_calls!=1)
        return Fail(2);
    const auto& events=plan.ordered;
    if(!Is(events[0],s::ActionKind::EnterGuardSetCe) ||
       !Is(events[1],s::ActionKind::CallInactiveHelper0x24113F60,0,0x1234) ||
       !Is(events[2],s::ActionKind::CallExternalVirtual14,2,0x2345) ||
       !Is(events[3],s::ActionKind::CallActiveHelper0x24113EA0,2,0x2345) ||
       !Is(events[4],s::ActionKind::CallInactiveHelper0x24113F60,1,0x3456) ||
       !Is(events[5],s::ActionKind::CallFinalizeHelper0x24114A90) ||
       !Is(events[6],s::ActionKind::LeaveGuardClearCe))
        return Fail(3);
    if(owned.SimulateComplete(plan)!=s::Result::Ready ||
       owned.modeled_guard_ce()!=0 ||
       owned.completed_simulated_refreshes()!=1)
        return Fail(4);

    // A correct event count but changed original route or external
    // handle must not be accepted.
    s::Plan forged=plan;
    forged.ordered[2].kind=s::ActionKind::CallInactiveHelper0x24113F60;
    if(owned.SimulateComplete(forged)!=s::Result::PlanRejected ||
       owned.completed_simulated_refreshes()!=1)return Fail(5);
    forged=plan;
    forged.ordered[3].external_id^=1;
    if(owned.SimulateComplete(forged)!=s::Result::PlanRejected)
        return Fail(6);
    forged=plan;
    forged.ordered.pop_back();
    if(owned.SimulateComplete(forged)!=s::Result::PlanRejected)
        return Fail(7);

    // An entirely different owner revision invalidates an old plan.
    s::State modified=source;
    modified.entries[2].active_branch=false;
    if(owned.Install(modified)!=s::Result::Ready ||
       owned.SimulateComplete(plan)!=s::Result::StalePlan)return Fail(8);

    // A cycle in a modified script must be refused transactionally.
    const auto oldPlan=plan.ordered.size();
    modified=source;
    modified.entries[1].next=0;
    if(owned.Install(modified)!=s::Result::Ready ||
       owned.Prepare(plan)!=s::Result::LinkedCycle ||
       plan.ordered.size()!=oldPlan)return Fail(9);

    // Source field is a 16-bit next-index. Invalid non-sentinel
    // values must never index outside the owned array.
    modified=source;
    modified.entries[2].next=0x4000;
    if(owned.Install(modified)!=s::Result::Ready ||
       owned.Prepare(plan)!=s::Result::BadNextIndex ||
       plan.ordered.size()!=oldPlan)return Fail(10);

    s::State invalid=source;
    invalid.head=0x4000;
    if(owned.Install(invalid)!=s::Result::BadHeadIndex ||
       owned.value().head!=source.head)return Fail(11);
    invalid={};
    invalid.entries.assign(s::kMaxOwnedShadowEntries+1,{});
    if(owned.Install(invalid)!=s::Result::OverCapacity ||
       owned.value().entries.size()!=source.entries.size())
        return Fail(12);

    // Empty list still performs the original +0xCE guard, auxiliary
    // finalization at +0xDC, and guard clear.
    s::State empty{};
    if(owned.Install(empty)!=s::Result::Ready ||
       owned.Prepare(plan)!=s::Result::Ready ||
       plan.ordered.size()!=3 ||
       plan.traversed_entries!=0 ||
       owned.SimulateComplete(plan)!=s::Result::Ready)return Fail(13);

    owned.Clear();
    if(owned.modeled_guard_ce()!=0 ||
       owned.completed_simulated_refreshes()!=0 ||
       owned.value().head!=s::kInvalidIndex ||
       !owned.value().entries.empty())return Fail(14);
    std::puts("PASS original ShadowMgr 125B helper: 120B linked entries, 16-bit source handles, active/inactive update branches, guard/finalize, cyclic/bad index/forged plan refusal");
    return 0;
}
