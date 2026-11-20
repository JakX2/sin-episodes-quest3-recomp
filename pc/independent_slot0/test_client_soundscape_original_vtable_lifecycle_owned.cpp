// Original 2006 C_SoundscapeSystem virtual slot2 and slot11
// owned event-order tests. No commercial DLL or game allocator called.
#include "client_soundscape_original_vtable_lifecycle_owned.h"
#include <cstdio>

namespace v=sin_independent::soundscape_original_vtable_lifecycle;
static int Fail(int n){
    std::fprintf(stderr,"original soundscape slot2/11 event fixture=%d\n",n);
    return n;
}
static bool Is(const v::Plan& p,std::size_t index,v::Kind kind,
               std::uint32_t offset,std::uint32_t target) {
    return index<p.count&&p.actions[index].kind==kind&&
           p.actions[index].offset==offset&&
           p.actions[index].original_target==target;
}
int main(){
    static_assert(v::kOriginalVtableVA==0x24293C34u);
    static_assert(v::kSlot2TailVA==0x240F37D0u);
    static_assert(v::kOriginalOperatorDeleteVA==0x2416FD60u);
    v::OwnedLifecycle owner{};
    auto slot2=owner.PrepareSlot2();
    if(slot2.count!=3 ||slot2.revision!=0 ||
       !Is(slot2,0,v::Kind::Slot2VirtualShutdown1,4,0) ||
       !Is(slot2,1,v::Kind::Slot2VirtualInit0,0,0) ||
       !Is(slot2,2,v::Kind::Slot2TailCall0x240F37D0,0,
           v::kSlot2TailVA))return Fail(1);

    // No execution in Prepare. The source order must be immutable.
    auto wrong=slot2;
    wrong.actions[0]=slot2.actions[1];
    if(owner.SimulateComplete(wrong)!=v::Result::Invalid ||
       owner.value().revision!=0 ||
       owner.value().slot2_completed!=0)
        return Fail(2);
    wrong=slot2;
    wrong.actions[2].original_target^=4;
    if(owner.SimulateComplete(wrong)!=v::Result::Invalid)
        return Fail(3);
    if(owner.SimulateComplete(slot2)!=v::Result::Applied ||
       owner.value().revision!=1 ||
       owner.value().slot2_completed!=1 ||
       owner.value().destroyed)
        return Fail(4);
    if(owner.SimulateComplete(slot2)!=v::Result::Stale)
        return Fail(5);
    slot2=owner.PrepareSlot2();
    if(owner.SimulateComplete(slot2)!=v::Result::Applied ||
       owner.value().slot2_completed!=2 ||
       owner.value().revision!=2)return Fail(6);

    // Source deleting-destructor flag bit0 false: restore vptr,
    // destroy subobjects in reverse ownership order, base cleanup,
    // skip deallocation, then return this pointer.
    auto destructor=owner.PrepareSlot11(0u);
    if(destructor.count!=7 ||
       destructor.source_delete_flag_bit0 ||
       destructor.revision!=2 ||
       !Is(destructor,0,v::Kind::Slot11RestoreVtable,0,v::kOriginalVtableVA) ||
       !Is(destructor,1,v::Kind::Slot11DestroyChildB4,0xB4u,v::kOriginalChildB4VA) ||
       !Is(destructor,2,v::Kind::Slot11DestroyChildA0,0xA0u,v::kOriginalChildA0VA) ||
       !Is(destructor,3,v::Kind::Slot11DestroyChild1C,0x1Cu,v::kOriginalChildArrayVA) ||
       !Is(destructor,4,v::Kind::Slot11DestroyChild08,0x08u,v::kOriginalChildArrayVA) ||
       !Is(destructor,5,v::Kind::Slot11DestroyBase,0,v::kOriginalBaseDestroyVA) ||
       !Is(destructor,6,v::Kind::Slot11ReturnThis,0,0))
        return Fail(7);
    wrong=destructor;
    wrong.count=8;
    if(owner.SimulateComplete(wrong)!=v::Result::Invalid ||
       owner.value().revision!=2)return Fail(8);
    wrong=destructor;
    wrong.actions[1].offset=0xA0u;
    if(owner.SimulateComplete(wrong)!=v::Result::Invalid)
        return Fail(9);
    if(owner.SimulateComplete(destructor)!=v::Result::Applied ||
       owner.value().revision!=3 ||
       !owner.value().destroyed ||
       !owner.value().child_b4_destroyed ||
       !owner.value().child_a0_destroyed ||
       !owner.value().child_1c_destroyed ||
       !owner.value().child_08_destroyed ||
       !owner.value().base_destroyed ||
       owner.value().storage_deallocated)
        return Fail(10);
    if(owner.SimulateComplete(owner.PrepareSlot2())!=
       v::Result::AlreadyDestroyed ||
       owner.SimulateComplete(owner.PrepareSlot11(1))!=
       v::Result::AlreadyDestroyed)
        return Fail(11);

    // Delete flag bit0 controls the only source branch where the
    // separately verified helper 0x2416FD60 is entered. Other flag
    // bits do not permit a deallocation.
    owner.Reset();
    destructor=owner.PrepareSlot11(0x100u);
    if(destructor.source_delete_flag_bit0 ||
       destructor.count!=7)return Fail(12);
    destructor=owner.PrepareSlot11(0x2u);
    if(destructor.count!=7)return Fail(13);
    destructor=owner.PrepareSlot11(0x101u);
    if(destructor.count!=8 ||
       !destructor.source_delete_flag_bit0 ||
       !Is(destructor,6,v::Kind::Slot11OptionalDelete,0,
           v::kOriginalOperatorDeleteVA) ||
       !Is(destructor,7,v::Kind::Slot11ReturnThis,0,0))
        return Fail(14);
    wrong=destructor;
    wrong.actions[6].original_target=0x2416F940u;
    if(owner.SimulateComplete(wrong)!=v::Result::Invalid)return Fail(15);
    if(owner.SimulateComplete(destructor)!=v::Result::Applied ||
       !owner.value().storage_deallocated ||
       owner.value().revision!=1 ||
       !owner.value().destroyed ||
       owner.value().slot2_completed!=0)return Fail(16);
    std::puts("PASS original 2006 SoundscapeSystem virtual slot2 ordered shutdown-init-tail and slot11 reverse subobject+base deleting dtor with optional bit0 deallocation, stale/forged/duplicate refusing owned model");
    return 0;
}
