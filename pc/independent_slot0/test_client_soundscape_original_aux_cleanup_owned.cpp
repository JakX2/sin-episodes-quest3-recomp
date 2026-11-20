// Phase 500: original 2006 SoundscapeSystem auxiliary cleanup.
// Fabricated owned pointer-presence flags only. Never call game DLL,
// original embedded virtual funcs, engine allocator or handle table.
#include "client_soundscape_original_aux_cleanup_owned.h"
#include <cstdio>

namespace s=sin_independent::soundscape_aux_cleanup;
static int Fail(int n){
    std::fprintf(stderr,"original soundscape aux cleanup fixture failed %d\n",n);
    return n;
}
static bool Is(const s::Plan& p,std::size_t n,s::EventKind k) {
    return n<p.count && p.actions[n].kind==k;
}
int main(){
    s::OwnedCleanup owner{};
    const auto empty=owner.Prepare();
    if(empty.count!=4 ||
       !Is(empty,0,s::EventKind::ClearCountAc) ||
       !Is(empty,1,s::EventKind::MirrorB0FromA0) ||
       !Is(empty,2,s::EventKind::ClearCountC0) ||
       !Is(empty,3,s::EventKind::MirrorC4FromB4) ||
       empty.handle_releases!=0 ||
       empty.buffer_releases!=0)return Fail(1);

    s::State source{};
    source.handle_9c=0xAA012345;
    source.handle_94=0xBB012346;
    source.handle_9c_generation_matches=true;
    source.handle_9c_target_present=true;
    source.vector_flag_a8=0;
    source.vector_flag_bc=0;
    source.buffer_a0_present=true;
    source.buffer_b4_present=true;
    source.mirror_b0_present=true;
    source.mirror_c4_present=true;
    source.count_a4=12;
    source.count_ac=13;
    source.count_b8=21;
    source.count_c0=22;
    owner.Set(source);
    const auto plan=owner.Prepare();
    if(plan.count!=12 ||plan.handle_releases!=2||
       plan.buffer_releases!=2 ||
       !Is(plan,0,s::EventKind::ReleaseHandle9cViaEmbeddedVirtual0) ||
       plan.actions[0].handle!=source.handle_9c ||
       !Is(plan,1,s::EventKind::ReleaseHandle94ViaEmbeddedVirtual0) ||
       plan.actions[1].handle!=source.handle_94 ||
       !Is(plan,2,s::EventKind::ClearCountAc) ||
       !Is(plan,3,s::EventKind::FreeBufferA0Via0x2416F940) ||
       !Is(plan,4,s::EventKind::ClearBufferA0) ||
       !Is(plan,5,s::EventKind::ClearCountA4) ||
       !Is(plan,6,s::EventKind::MirrorB0FromA0) ||
       !Is(plan,7,s::EventKind::ClearCountC0) ||
       !Is(plan,8,s::EventKind::FreeBufferB4Via0x2416F940) ||
       !Is(plan,9,s::EventKind::ClearBufferB4) ||
       !Is(plan,10,s::EventKind::ClearCountB8) ||
       !Is(plan,11,s::EventKind::MirrorC4FromB4))
        return Fail(2);
    if(owner.value().count_ac!=13 || !owner.value().buffer_a0_present)
        return Fail(3); // Prepare is read-only and never frees.

    s::Plan forged=plan;
    forged.actions[3].kind=s::EventKind::ClearBufferA0;
    if(owner.SimulateComplete(forged)!=s::Result::ModifiedPlan ||
       owner.value().buffer_a0_present!=true)
        return Fail(4);
    forged=plan;
    forged.actions[0].handle^=1;
    if(owner.SimulateComplete(forged)!=s::Result::ModifiedPlan)
        return Fail(5);
    forged=plan;
    forged.buffer_releases=0;
    if(owner.SimulateComplete(forged)!=s::Result::ModifiedPlan)
        return Fail(6);

    if(owner.SimulateComplete(plan)!=s::Result::Ready ||
       owner.value().handle_9c!=s::kInvalid ||
       owner.value().handle_94!=s::kInvalid ||
       owner.value().buffer_a0_present ||
       owner.value().buffer_b4_present ||
       owner.value().mirror_b0_present ||
       owner.value().mirror_c4_present ||
       owner.value().count_a4 ||owner.value().count_ac ||
       owner.value().count_b8 ||owner.value().count_c0 ||
       owner.value().vector_flag_a8!=0 ||
       owner.value().vector_flag_bc!=0 ||
       owner.SimulateComplete(plan)!=s::Result::StalePlan)
        return Fail(7);

    // Original handle 9C has a generation and table-target check;
    // on mismatch it is NOT invalidated by this method. Handle 94
    // has no equivalent visible check before its virtual cleanup.
    s::State stale=source;
    stale.handle_9c_generation_matches=false;
    stale.handle_9c_target_present=true;
    stale.vector_flag_a8=s::kInvalid;
    stale.vector_flag_bc=s::kInvalid;
    stale.buffer_a0_present=true;
    stale.buffer_b4_present=true;
    owner.Set(stale);
    const auto skip=owner.Prepare();
    if(skip.count!=5 ||skip.handle_releases!=1 ||
       skip.buffer_releases!=0 ||
       !Is(skip,0,s::EventKind::ReleaseHandle94ViaEmbeddedVirtual0) ||
       !Is(skip,1,s::EventKind::ClearCountAc) ||
       !Is(skip,2,s::EventKind::MirrorB0FromA0) ||
       !skip.actions[2].expected_buffer_present ||
       !Is(skip,3,s::EventKind::ClearCountC0) ||
       !Is(skip,4,s::EventKind::MirrorC4FromB4) ||
       !skip.actions[4].expected_buffer_present)return Fail(8);
    if(owner.SimulateComplete(skip)!=s::Result::Ready ||
       owner.value().handle_9c!=stale.handle_9c ||
       owner.value().handle_94!=s::kInvalid ||
       !owner.value().buffer_a0_present ||
       !owner.value().buffer_b4_present ||
       !owner.value().mirror_b0_present ||
       !owner.value().mirror_c4_present ||
       owner.value().count_ac!=0 ||owner.value().count_c0!=0 ||
       owner.value().count_a4!=12 ||owner.value().count_b8!=21)
        return Fail(9);

    // Valid vector flag + EMPTY pointer must still clear the
    // owned count, but must NOT emit any free-engine event.
    s::State noData=source;
    noData.handle_9c=s::kInvalid;
    noData.handle_94=s::kInvalid;
    noData.buffer_a0_present=false;
    noData.buffer_b4_present=false;
    owner.Set(noData);
    const auto noFree=owner.Prepare();
    if(noFree.count!=8 ||noFree.buffer_releases!=0 ||
       noFree.handle_releases!=0 ||
       !Is(noFree,0,s::EventKind::ClearCountAc) ||
       !Is(noFree,1,s::EventKind::ClearBufferA0) ||
       !Is(noFree,2,s::EventKind::ClearCountA4) ||
       !Is(noFree,3,s::EventKind::MirrorB0FromA0) ||
       !Is(noFree,4,s::EventKind::ClearCountC0) ||
       !Is(noFree,5,s::EventKind::ClearBufferB4) ||
       !Is(noFree,6,s::EventKind::ClearCountB8) ||
       !Is(noFree,7,s::EventKind::MirrorC4FromB4))
        return Fail(10);

    owner.Clear();
    if(owner.value().handle_9c!=s::kInvalid ||
       owner.value().vector_flag_a8!=s::kInvalid ||
       owner.Prepare().count!=4)return Fail(11);
    std::puts("PASS original source 223-byte SoundscapeSystem auxiliary cleanup: handle-generation guard, asymmetric 9C/94 release, two conditional buffer frees, always-cleared AC/C0, mirror ordering, sentinel branches, stale/forged plans");
    return 0;
}
