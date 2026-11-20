// Native MSVC x86 tests for the SHA-pinned last part of original
// 2006 VClient011::Init. No original object, engine or game is invoked.
#include "client_original_init_tail_gates.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace tail=sin_independent::client_init_tail;
static int Fail(int id) {
    std::fprintf(stderr,"client Init tail known ordering fixture failed=%d\n",id);
    return id;
}
struct Fixture final {
    std::array<std::uint32_t,tail::kOriginalTailCalls.size()> observed{};
    std::size_t count=0;
    std::size_t fail_at=tail::kOriginalTailCalls.size();
    bool fail_unchecked=false;
    bool callbacks_are_ordered=true;
};
static bool Record(void* opaque,const tail::Step& step) noexcept {
    auto& f=*static_cast<Fixture*>(opaque);
    const std::size_t at=f.count++;
    if(at>=f.observed.size() ||
       step.callsite_va!=tail::kOriginalTailCalls[at].callsite_va){
        f.callbacks_are_ordered=false;
        return false;
    }
    f.observed[at]=step.callsite_va;
    if(at==f.fail_at)return false;
    return !f.fail_unchecked || step.gate!=tail::Gate::Unchecked;
}
int main() {
    static_assert(tail::ValidMetadata(),"native fixture requires exact list");
    if(tail::kOriginalTailCalls.size()!=26 ||
       tail::kOriginalTailCalls[4].direct_target_va!=0x24156860u ||
       tail::kOriginalTailCalls[18].direct_target_va!=0x24182c60u ||
       tail::kOriginalTailCalls[25].direct_target_va!=0x240fcdd0u)
        return Fail(1);
    Fixture all{};
    const auto result=tail::WalkKnownTail(&Record,&all);
    if(!result.accepted() || !all.callbacks_are_ordered ||
       result.calls!=26 || result.checked_gates_passed!=2 ||
       result.last_callsite_va!=0x240fe590u ||
       result.original_exit_va!=tail::kOriginalSuccessReturnVA)
        return Fail(2);
    for(std::size_t i=0;i<all.count;++i)
        if(all.observed[i]!=tail::kOriginalTailCalls[i].callsite_va)
            return Fail(3);

    // The original Init only branches on the two bool results, so
    // returning false from any other injected call is ignored at
    // this level. Its unknown real side effects are NOT modeled.
    Fixture unchecked{};
    unchecked.fail_unchecked=true;
    auto ignored=tail::WalkKnownTail(&Record,&unchecked);
    if(!ignored.accepted() || ignored.calls!=26 ||
       ignored.checked_gates_passed!=2 ||
       !unchecked.callbacks_are_ordered)return Fail(4);

    Fixture first{};
    first.fail_at=4;
    const auto refusedSystems=tail::WalkKnownTail(&Record,&first);
    if(refusedSystems.outcome!=tail::Outcome::AutoSystemGateRejected ||
       refusedSystems.calls!=5 || first.count!=5 ||
       refusedSystems.checked_gates_passed!=0 ||
       refusedSystems.original_exit_va!=tail::kSharedFailureVA ||
       refusedSystems.last_callsite_va!=tail::kAutoSystemsGateVA ||
       !first.callbacks_are_ordered)return Fail(5);

    Fixture second{};
    second.fail_at=18;
    const auto refusedPhysics=tail::WalkKnownTail(&Record,&second);
    if(refusedPhysics.outcome!=tail::Outcome::PhysicsGateRejected ||
       refusedPhysics.calls!=19 || second.count!=19 ||
       refusedPhysics.checked_gates_passed!=1 ||
       refusedPhysics.original_exit_va!=tail::kSharedFailureVA ||
       refusedPhysics.last_callsite_va!=tail::kPhysicsGateVA ||
       !second.callbacks_are_ordered)return Fail(6);

    const auto absent=tail::WalkKnownTail(nullptr,nullptr);
    if(absent.outcome!=tail::Outcome::MissingInjectedCallback ||
       absent.calls!=0 || absent.original_exit_va!=0)return Fail(7);

    // A false return from the post-physics interface calls is NOT
    // examined by this original function. Test all 7 post-gate
    // callbacks separately, so no accidental synthetic third
    // success/failure gate gets introduced into the oracle.
    for(std::size_t i=19;i<tail::kOriginalTailCalls.size();++i){
        Fixture later{};
        later.fail_at=i;
        const auto carried=tail::WalkKnownTail(&Record,&later);
        if(!carried.accepted() || carried.calls!=26 ||
           carried.checked_gates_passed!=2 ||
           !later.callbacks_are_ordered)return Fail(8);
    }
    // The original tail has exactly two inspected calls. Any other
    // individual callback failure does not branch in this function.
    for(std::size_t i=0;i<tail::kOriginalTailCalls.size();++i){
        if(i==4 || i==18)continue;
        Fixture other{};
        other.fail_at=i;
        const auto carried=tail::WalkKnownTail(&Record,&other);
        if(!carried.accepted() || carried.calls!=26 ||
           carried.original_exit_va!=tail::kOriginalSuccessReturnVA)
            return Fail(9);
    }
    std::puts("PASS 26 original VClient011 Init tail calls, 2 explicit early-exit gates, 24 unchecked calls and shared failure destination");
    return 0;
}
