// Original 2006 soundscape array field alias integration test.
// Aux method 0x240F3D10 and original 83-byte destructor child
// variants +A0/+B4 operate on the SAME parent fields. No real
// engine memory is accessed or freed.
#include "client_soundscape_original_array_overlap_owned.h"
#include <cstdio>
namespace o=sin_independent::soundscape_original_array_overlap;
namespace a=sin_independent::soundscape_aux_cleanup;
namespace c=sin_independent::soundscape_original_child_arrays;
namespace d=sin_independent::soundscape_original_object;
static int Fail(int n) {
    std::fprintf(stderr,"2006 original soundscape aux/child alias model failure %d\n",n);
    return n;
}
static a::State Loaded() {
    a::State s{};
    s.vector_flag_a8=0u;
    s.vector_flag_bc=0u;
    s.buffer_a0_present=true;
    s.buffer_b4_present=true;
    s.mirror_b0_present=true;
    s.mirror_c4_present=true;
    s.count_a4=11;
    s.count_ac=13;
    s.count_b8=17;
    s.count_c0=19;
    return s;
}
int main(){
    static_assert(o::Offsets(o::Region::ArrayA0).helper==
                  c::Variant::ChildA0);
    static_assert(o::Offsets(o::Region::ArrayB4).helper==
                  c::Variant::ChildB4);
    static_assert(o::Offsets(o::Region::ArrayA0).base+0x10u==0xB0u);
    static_assert(o::Offsets(o::Region::ArrayB4).base+0x10u==0xC4u);
    static_assert(o::Offsets(o::Region::ArrayA0).countC==
                  o::Offsets(o::Region::ArrayA0).base+0x0Cu);
    static_assert(o::Offsets(o::Region::ArrayB4).flag8==
                  o::Offsets(o::Region::ArrayB4).base+0x08u);

    d::KnownDwordModel constructed{};
    d::ApplyOriginalConstructor(constructed);
    if(!o::ConstructorInitializesBothArrayFlagsToZero(constructed) ||
       constructed.At(0xA0u)!=0u ||
       constructed.At(0xA4u)!=0u ||
       constructed.At(0xB4u)!=0u ||
       constructed.At(0xB8u)!=0u ||
       constructed.At(0xC0u)!=0u ||
       constructed.At(0xC4u)!=0u)
        return Fail(1);

    // Auxiliary then child: two source frees happen in the
    // auxiliary method, and both child destructors must see
    // the already zeroed original +A0/+B4 pointers.
    a::OwnedCleanup aux{};
    aux.Set(Loaded());
    auto plan=aux.Prepare();
    if(plan.buffer_releases!=2 ||
       aux.SimulateComplete(plan)!=a::Result::Ready)
        return Fail(2);
    c::OwnedChildArray childA{},childB{};
    childA.Set(o::Project(aux.value(),o::Region::ArrayA0));
    childB.Set(o::Project(aux.value(),o::Region::ArrayB4));
    auto ca=childA.Prepare(c::Variant::ChildA0);
    auto cb=childB.Prepare(c::Variant::ChildB4);
    if(ca.original_allocator_free_count!=0 ||
       cb.original_allocator_free_count!=0 ||
       ca.count!=8 ||cb.count!=8 ||
       childA.SimulateComplete(ca)!=c::Result::Applied ||
       childB.SimulateComplete(cb)!=c::Result::Applied)
        return Fail(3);
    auto copied=o::ProjectBack(aux.value(),childA.value(),o::Region::ArrayA0);
    copied=o::ProjectBack(copied,childB.value(),o::Region::ArrayB4);
    if(copied.buffer_a0_present ||
       copied.buffer_b4_present ||
       copied.mirror_b0_present ||
       copied.mirror_c4_present ||
       copied.count_a4 ||copied.count_ac ||
       copied.count_b8 ||copied.count_c0 ||
       copied.vector_flag_a8!=0u ||
       copied.vector_flag_bc!=0u)
        return Fail(4);
    const unsigned totalAuxThenChild=plan.buffer_releases+
        ca.original_allocator_free_count+
        cb.original_allocator_free_count;
    if(totalAuxThenChild!=2u)return Fail(5);

    // Child then auxiliary: child implementations free each
    // non-null pointer only once; the auxiliary thereafter
    // must not emit a second free of the overlapping fields.
    a::State earlier=Loaded();
    c::OwnedChildArray firstA{},firstB{};
    firstA.Set(o::Project(earlier,o::Region::ArrayA0));
    firstB.Set(o::Project(earlier,o::Region::ArrayB4));
    ca=firstA.Prepare(c::Variant::ChildA0);
    cb=firstB.Prepare(c::Variant::ChildB4);
    if(ca.original_allocator_free_count!=1 ||
       cb.original_allocator_free_count!=1 ||
       firstA.SimulateComplete(ca)!=c::Result::Applied ||
       firstB.SimulateComplete(cb)!=c::Result::Applied)
        return Fail(6);
    earlier=o::ProjectBack(earlier,firstA.value(),o::Region::ArrayA0);
    earlier=o::ProjectBack(earlier,firstB.value(),o::Region::ArrayB4);
    aux.Set(earlier);
    plan=aux.Prepare();
    if(plan.buffer_releases!=0 ||
       aux.SimulateComplete(plan)!=a::Result::Ready ||
       aux.value().buffer_a0_present ||
       aux.value().buffer_b4_present ||
       aux.value().count_a4 ||aux.value().count_ac ||
       aux.value().count_b8 ||aux.value().count_c0)
        return Fail(7);
    const unsigned totalChildThenAux=ca.original_allocator_free_count+
        cb.original_allocator_free_count+plan.buffer_releases;
    if(totalChildThenAux!=2u)return Fail(8);

    // Original sentinel -1: neither code can free these pointers.
    // Both preserve source pointer/+4 count while clearing +C
    // and mirroring pointer+0 to +10. No ownership guesses.
    a::State absent=Loaded();
    absent.vector_flag_a8=a::kInvalid;
    absent.vector_flag_bc=a::kInvalid;
    aux.Set(absent);
    plan=aux.Prepare();
    if(plan.buffer_releases!=0 ||
       aux.SimulateComplete(plan)!=a::Result::Ready)
        return Fail(9);
    childA.Set(o::Project(aux.value(),o::Region::ArrayA0));
    childB.Set(o::Project(aux.value(),o::Region::ArrayB4));
    ca=childA.Prepare(c::Variant::ChildA0);
    cb=childB.Prepare(c::Variant::ChildB4);
    if(ca.count!=4 ||cb.count!=4 ||
       ca.original_allocator_free_count!=0 ||
       cb.original_allocator_free_count!=0 ||
       childA.SimulateComplete(ca)!=c::Result::Applied ||
       childB.SimulateComplete(cb)!=c::Result::Applied)
        return Fail(10);
    copied=o::ProjectBack(aux.value(),childA.value(),o::Region::ArrayA0);
    copied=o::ProjectBack(copied,childB.value(),o::Region::ArrayB4);
    if(!copied.buffer_a0_present ||!copied.buffer_b4_present ||
       !copied.mirror_b0_present ||!copied.mirror_c4_present ||
       copied.count_a4!=11 ||copied.count_b8!=17 ||
       copied.count_ac!=0 ||copied.count_c0!=0)
        return Fail(11);
    std::puts("PASS original 2006 soundscape A0/B4 source field aliases: constructor zero flags, aux-before-child/child-before-aux each at most 2 owned free events, sentinel preservation and source mirrors");
    return 0;
}
