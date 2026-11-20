// Source-free native 2006 SiN C_SoundscapeSystem three identical
// 83-byte array teardown helpers; no real engine allocator.
#include "client_soundscape_original_child_array_owned.h"
#include <cstdio>
namespace o=sin_independent::soundscape_original_child_arrays;
static int Fail(int n){
    std::fprintf(stderr,"original soundscape child array fixture=%d\n",n);
    return n;
}
static bool Is(const o::Plan& p,std::size_t i,o::Kind k,
               unsigned off,std::uint32_t target=0u){
    return i<p.count&&p.actions[i].kind==k&&
           p.actions[i].offset==off&&
           p.actions[i].original_target==target;
}
int main(){
    static_assert(o::kOriginalFunctionBytes==83u);
    static_assert(o::OriginalVA(o::Variant::Child08And1C)==0x240F3970u);
    static_assert(o::OriginalVA(o::Variant::ChildA0)==0x240F3A20u);
    static_assert(o::OriginalVA(o::Variant::ChildB4)==0x240F3AD0u);
    static_assert(o::OriginalVA(static_cast<o::Variant>(9))==0u);
    o::OwnedChildArray a{};
    // A sentinel vector must keep its data pointer/count4,
    // zero +C, and mirror the *existing* pointer to +10.
    o::OwnedState sentinel{};
    sentinel.flag_8=o::kSentinel;
    sentinel.pointer_0_present=true;
    sentinel.count_4=41;
    sentinel.count_c=47;
    a.Set(sentinel);
    const auto skip=a.Prepare(o::Variant::ChildB4);
    if(skip.count!=4 ||skip.original_allocator_free_count!=0 ||
       !Is(skip,0,o::Kind::CheckFlag8First,8) ||
       !Is(skip,1,o::Kind::ZeroCountC,12) ||
       !Is(skip,2,o::Kind::CheckFlag8Second,8) ||
       !Is(skip,3,o::Kind::MirrorPointer0To10,16))
        return Fail(1);
    if(a.SimulateComplete(skip)!=o::Result::Applied ||
       !a.value().pointer_0_present ||
       !a.value().mirror_10_present ||
       a.value().count_4!=41 ||a.value().count_c!=0 ||
       a.SimulateComplete(skip)!=o::Result::Stale)
        return Fail(2);

    // All three helper VAs have the same local structure.
    // For a valid vector with a pointer, original free helper is
    // reached only on first pass. Second source check sees zero.
    for(auto variant:{o::Variant::Child08And1C,
                      o::Variant::ChildA0,
                      o::Variant::ChildB4}){
        o::OwnedState loaded{};
        loaded.flag_8=0u;
        loaded.pointer_0_present=true;
        loaded.count_4=25;
        loaded.count_c=37;
        a.Set(loaded);
        const auto p=a.Prepare(variant);
        if(p.variant!=variant ||p.count!=10 ||
           p.original_allocator_free_count!=1 ||
           !Is(p,0,o::Kind::CheckFlag8First,8) ||
           !Is(p,1,o::Kind::ZeroCountC,12) ||
           !Is(p,2,o::Kind::CheckPointer0First,0) ||
           !Is(p,3,o::Kind::FreePointer0First,0,o::kOriginalFree) ||
           !Is(p,4,o::Kind::ZeroPointer0First,0) ||
           !Is(p,5,o::Kind::ZeroCount4First,4) ||
           !Is(p,6,o::Kind::CheckFlag8Second,8) ||
           !Is(p,7,o::Kind::MirrorPointer0To10,16) ||
           !Is(p,8,o::Kind::CheckPointer0Second,0) ||
           !Is(p,9,o::Kind::ZeroCount4Second,4))
            return Fail(3);
        auto corrupt=p;
        corrupt.actions[3].original_target^=4u;
        if(a.SimulateComplete(corrupt)!=o::Result::Invalid ||
           a.value().count_c!=37)return Fail(4);
        corrupt=p;
        corrupt.original_allocator_free_count=2u;
        if(a.SimulateComplete(corrupt)!=o::Result::Invalid)
            return Fail(5);
        corrupt=p;
        corrupt.count=9u;
        if(a.SimulateComplete(corrupt)!=o::Result::Invalid)
            return Fail(6);
        if(a.SimulateComplete(p)!=o::Result::Applied ||
           a.value().flag_8!=0u ||
           a.value().pointer_0_present ||
           a.value().mirror_10_present ||
           a.value().count_4||a.value().count_c)
            return Fail(7);
    }

    // Valid +8 flag with already-null pointer: no allocator
    // call and no explicit +0 pointer-zero store, but *both*
    // source writes of +4 and the +10 mirror still occur.
    o::OwnedState empty{};
    empty.flag_8=0;
    empty.pointer_0_present=false;
    empty.count_4=9;
    empty.count_c=10;
    a.Set(empty);
    const auto noFree=a.Prepare(o::Variant::ChildA0);
    if(noFree.count!=8 ||noFree.original_allocator_free_count!=0 ||
       !Is(noFree,0,o::Kind::CheckFlag8First,8) ||
       !Is(noFree,1,o::Kind::ZeroCountC,12) ||
       !Is(noFree,2,o::Kind::CheckPointer0First,0) ||
       !Is(noFree,3,o::Kind::ZeroCount4First,4) ||
       !Is(noFree,4,o::Kind::CheckFlag8Second,8) ||
       !Is(noFree,5,o::Kind::MirrorPointer0To10,16) ||
       !Is(noFree,6,o::Kind::CheckPointer0Second,0) ||
       !Is(noFree,7,o::Kind::ZeroCount4Second,4))
        return Fail(8);
    if(a.SimulateComplete(noFree)!=o::Result::Applied ||
       a.value().count_4 ||a.value().count_c ||
       a.value().pointer_0_present ||
       a.value().mirror_10_present)return Fail(9);

    // Event proof may not escape into a second owned object lifetime,
    // and may not forge a fourth undocumented cleanup-helper variant.
    const auto stale=a.Prepare(o::Variant::ChildB4);
    a.Reset();
    if(a.SimulateComplete(stale)!=o::Result::Stale ||
       a.value().revision<=stale.revision)return Fail(10);
    const auto baseline=a.Prepare(o::Variant::ChildA0);
    auto invalid=baseline;
    invalid.variant=static_cast<o::Variant>(99);
    if(a.SimulateComplete(invalid)!=o::Result::Invalid)
        return Fail(11);
    std::puts("PASS SHA-original three 83B Soundscape array destructors: first and second sentinel/pointer branches, zero/nonnull/sentinel buffers, at most one source free, mirror copy, exact event source ordering and stale/forged refusals");
    return 0;
}
