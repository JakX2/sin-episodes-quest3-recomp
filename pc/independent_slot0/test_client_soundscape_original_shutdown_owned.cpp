// Phase 489: exact source-derived shutdown stop ordering on a fake
// owned state. Original client SHA audit stays private; no game DLL
// or real sound interface is invoked by these tests.
#include "client_soundscape_original_shutdown_owned.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace s=sin_independent::soundscape_shutdown;
static int Fail(int n) {
    std::fprintf(stderr,"owned 2006 soundscape Shutdown test failed %d\n",n);
    return n;
}
static bool Is(const s::Action& a,s::ActionKind kind,
               std::uint32_t id=0) noexcept {
    return a.kind==kind && a.sound_id==id;
}
int main(){
    static_assert(sizeof(s::CandidateOriginalEntry40)==40,"original 40B");
    static_assert(offsetof(s::CandidateOriginalEntry40,sound_id)==12,"id +0x0C");
    static_assert(offsetof(s::CandidateOriginalEntry40,use_engine_interface_stop)==36,"flag +0x24");
    s::OwnedLifecycle owned{};
    s::Plan plan{};
    if(owned.Prepare(plan)!=s::Result::Ready ||
       plan.actions.size()!=3 ||
       !Is(plan.actions[0],s::ActionKind::ClearActiveCount0xAC) ||
       !Is(plan.actions[1],s::ActionKind::ClearAuxiliaryCount0xC0) ||
       !Is(plan.actions[2],s::ActionKind::ClearSoundscapeTreeCount0x28))
        return Fail(1);

    s::State prepared{};
    prepared.active_sounds={
        {7,false},{8,true},{9,false},{10,true}
    };
    prepared.loaded_keyvalues_owners={
        "scripts/soundscapes.txt",
        "scripts/soundscapes_intro.txt",
        "scripts/soundscapes_docks.txt"
    };
    prepared.auxiliary_count_c0=0xFAu;
    prepared.handle_9c=0xABCD1234;
    prepared.handle_94=0x55443322;
    if(owned.Install(prepared)!=s::Result::Ready ||
       owned.value().active_sounds.size()!=4 ||
       owned.value().loaded_keyvalues_owners.size()!=3)
        return Fail(2);
    const auto rev=owned.revision();
    if(owned.Prepare(plan)!=s::Result::Ready ||
       plan.revision!=rev ||plan.sound_stop_count!=4 ||
       plan.keyvalues_delete_count!=3 ||plan.actions.size()!=12)
        return Fail(3);
    // Original destroys active sounds LAST-IN FIRST-OUT in 40-byte
    // strides. Alternate flag decides between virtual slot11 and
    // legacy helper 0x241BD680, without changing source id.
    if(!Is(plan.actions[0],s::ActionKind::StopViaEngineInterfaceSlot11,10) ||
       plan.actions[0].source_index!=3 ||
       !Is(plan.actions[1],s::ActionKind::StopViaOriginalHelper,9) ||
       plan.actions[1].source_index!=2 ||
       !Is(plan.actions[2],s::ActionKind::StopViaEngineInterfaceSlot11,8) ||
       plan.actions[2].source_index!=1 ||
       !Is(plan.actions[3],s::ActionKind::StopViaOriginalHelper,7) ||
       plan.actions[3].source_index!=0)
        return Fail(4);
    // The +0xAC/+0xC0/+0x28 resets precede handle+resource frees.
    if(!Is(plan.actions[4],s::ActionKind::ClearActiveCount0xAC) ||
       !Is(plan.actions[5],s::ActionKind::ClearAuxiliaryCount0xC0) ||
       !Is(plan.actions[6],s::ActionKind::ClearSoundscapeTreeCount0x28) ||
       plan.actions[7].kind!=s::ActionKind::ReleaseHandle0x9C ||
       plan.actions[7].candidate_handle!=prepared.handle_9c ||
       plan.actions[8].kind!=s::ActionKind::ReleaseHandle0x94 ||
       plan.actions[8].candidate_handle!=prepared.handle_94)
        return Fail(5);
    // In the retail code the KeyValues array is cleared by deleting
    // index zero, memmoving and repeating: FIRST-IN FIRST-OUT.
    for(std::size_t i=0;i<3;++i){
        const auto& action=plan.actions[9+i];
        if(action.kind!=s::ActionKind::DeleteKeyValuesFirst ||
           action.source_index!=i ||
           action.resource_name!=prepared.loaded_keyvalues_owners[i])
            return Fail(6);
    }
    if(owned.value().active_sounds.size()!=4 ||
       owned.value().loaded_keyvalues_owners.size()!=3)
        return Fail(7); // Prepare NEVER performs side effects.

    // State transitions refuse stale plans after any replacement.
    s::State newer=prepared;
    newer.active_sounds.pop_back();
    if(owned.Install(newer)!=s::Result::Ready ||
       owned.CommitCleared(plan)!=s::Result::StalePlan ||
       owned.value().active_sounds.size()!=3)
        return Fail(8);
    if(owned.Install(prepared)!=s::Result::Ready ||
       owned.Prepare(plan)!=s::Result::Ready)
        return Fail(9);
    const auto oldRevision=owned.revision();
    s::Plan corrupt=plan;
    corrupt.keyvalues_delete_count=0;
    if(owned.CommitCleared(corrupt)!=s::Result::PlanRejected ||
       owned.revision()!=oldRevision ||
       owned.value().active_sounds.size()!=4)
        return Fail(10);
    corrupt=plan;
    corrupt.actions.pop_back();
    if(owned.CommitCleared(corrupt)!=s::Result::PlanRejected ||
       owned.value().loaded_keyvalues_owners.size()!=3)
        return Fail(11);
    // A forged plan with the RIGHT count/revision but swapped stop
    // routes or corrupted resource identity must also be refused.
    corrupt=plan;
    corrupt.actions[0].kind=s::ActionKind::StopViaOriginalHelper;
    if(owned.CommitCleared(corrupt)!=s::Result::PlanRejected ||
       owned.value().active_sounds.size()!=4)return Fail(18);
    corrupt=plan;
    corrupt.actions.back().resource_name="scripts/forged.txt";
    if(owned.CommitCleared(corrupt)!=s::Result::PlanRejected ||
       owned.value().loaded_keyvalues_owners.size()!=3)return Fail(19);
    if(owned.CommitCleared(plan)!=s::Result::Ready ||
       !owned.value().active_sounds.empty() ||
       !owned.value().loaded_keyvalues_owners.empty() ||
       owned.value().auxiliary_count_c0!=0 ||
       owned.value().handle_94!=-1 ||
       owned.value().handle_9c!=-1 ||
       owned.CommitCleared(plan)!=s::Result::StalePlan)
        return Fail(12);

    // -1 EHANDLEs are not candidates for cleanup. Sound id zero is
    // not rejected because the original instruction does not check it.
    s::State noHandles{};
    noHandles.active_sounds={{0,false}};
    noHandles.loaded_keyvalues_owners={"scripts/fixture.txt"};
    if(owned.Install(noHandles)!=s::Result::Ready ||
       owned.Prepare(plan)!=s::Result::Ready ||
       plan.actions.size()!=5 ||
       plan.actions[0].sound_id!=0 ||
       plan.actions[4].resource_name!="scripts/fixture.txt")
        return Fail(13);

    // Original sound counts are not a known hard cap. Our owner
    // refuses pathological reinitialization atomically.
    s::State oversized{};
    oversized.active_sounds.assign(s::kMaxOwnedActive+1,{1,true});
    if(owned.Install(oversized)!=s::Result::CapacityRefused ||
       owned.value().active_sounds.size()!=1)return Fail(14);
    oversized={};
    oversized.loaded_keyvalues_owners.assign(
        s::kMaxOwnedResources+1,"scripts/resource.txt");
    if(owned.Install(oversized)!=s::Result::CapacityRefused ||
       owned.value().loaded_keyvalues_owners.size()!=1)return Fail(15);
    oversized={};
    oversized.loaded_keyvalues_owners={""};
    if(owned.Install(oversized)!=s::Result::InvalidState ||
       owned.value().loaded_keyvalues_owners.size()!=1)return Fail(16);

    owned.Clear();
    if(!owned.value().active_sounds.empty() ||
       !owned.value().loaded_keyvalues_owners.empty() ||
       owned.Prepare(plan)!=s::Result::Ready ||
       plan.actions.size()!=3)return Fail(17);
    std::puts("PASS source-derived Shutdown: 40B entry, reverse mixer/legacy stop routing, 3 resets, 2 handle candidates, FIFO KeyValues releases, stale-plan/capacity rollback, no actual audio calls");
    return 0;
}
