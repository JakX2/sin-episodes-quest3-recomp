// Standalone host-side generation registry test. No proprietary DLLs.
#include "server_gameents_owner_lifetime_registry.h"
#include <cstdint>
#include <cstdio>
#include <limits>
#include <thread>
using namespace sin_independent;
using Registry=GameEntsOwnerLifetimeRegistry<3,2>;
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL owner registry %d: %s\n",n,why);return n;
}
struct Storage {
    unsigned char edict[0x20]{},entity[0x180]{},
                  unknown[0x30]{},touch[0x90]{};
};
static GameEntsOwnerIdentity Identity(Storage& s,std::uint64_t gen) noexcept {
    return {s.edict,sizeof(s.edict),s.entity,sizeof(s.entity),
            s.unknown,sizeof(s.unknown),s.touch,sizeof(s.touch),gen};
}
int main() {
    Registry r;
    Storage a{},b{},c{};
    auto first=Identity(a,1);
    first.edict_bytes=0x14; // authenticated original edict array stride
    auto second=Identity(b,2);
    auto third=Identity(c,3);
    if(!r.Enroll(0,first)||!r.Enroll(1,second))
        return Fail(1,"initial enrollment denied");

    // Source-authenticated original CBaseEntity implements IServerUnknown at
    // the same base address: CBaseEntity +0x14 vtable entry returns this,
    // and CServerNetworkProperty writes that entity to edict +0x0c.
    // Test this exact alias independently from older disjoint fixtures.
    Registry original_alias_registry;
    Storage original_actor{},other_actor{};
    auto original_alias=Identity(original_actor,1);
    original_alias.unknown=original_alias.entity;
    original_alias.unknown_bytes=sizeof(void*);
    if(!original_alias_registry.Enroll(0,original_alias))
        return Fail(34,"original entity/unknown exact-base alias rejected");
    auto shifted=Identity(other_actor,2);
    shifted.unknown=static_cast<const unsigned char*>(shifted.entity)+4;
    shifted.unknown_bytes=sizeof(void*);
    if(original_alias_registry.Enroll(1,shifted))
        return Fail(35,"shifted unknown subobject treated as exact entity base");
    shifted=Identity(other_actor,2);
    shifted.unknown=shifted.entity;
    shifted.unknown_bytes=shifted.entity_bytes+1;
    if(original_alias_registry.Enroll(1,shifted))
        return Fail(36,"unknown alias extends beyond entity lifetime range");
    shifted=Identity(other_actor,2);
    shifted.unknown=shifted.edict;
    shifted.unknown_bytes=sizeof(void*);
    if(original_alias_registry.Enroll(1,shifted))
        return Fail(37,"edict overlapping unknown accepted");
    shifted=Identity(other_actor,2);
    shifted.touch=shifted.entity;
    if(original_alias_registry.Enroll(1,shifted))
        return Fail(38,"entity touching-list alias accepted");
    shifted=Identity(other_actor,2);
    shifted.unknown=original_alias.entity;
    shifted.unknown_bytes=sizeof(void*);
    if(original_alias_registry.Enroll(1,shifted))
        return Fail(39,"cross-entity unknown pointer accepted");
    if(!original_alias_registry.Enroll(1,Identity(other_actor,2)))
        return Fail(40,"negative alias cases poisoned legitimate new owner");
    std::uint64_t alias_token=0;
    if(!original_alias_registry.Begin(5,&alias_token)||
       !original_alias_registry.ValidUnknown(original_alias.entity)||
       !original_alias_registry.ValidUnknownPair(original_alias.edict,
                                                 original_alias.entity)||
       !original_alias_registry.ValidPair(original_alias.edict,
                                          original_alias.entity)||
       original_alias_registry.ValidUnknownPair(other_actor.edict,
                                                 original_alias.entity)||
       !original_alias_registry.End(5,alias_token))
        return Fail(41,"original unknown alias not associated with exact edict");
    if(!original_alias_registry.Begin(3,&alias_token)||
       !original_alias_registry.RetireEntity(0)||
       original_alias_registry.ValidUnknown(original_alias.entity)||
       original_alias_registry.ValidUnknownPair(original_alias.edict,
                                                original_alias.entity)||
       !original_alias_registry.ValidEdict(original_alias.edict)||
       !original_alias_registry.End(3,alias_token)||
       !original_alias_registry.ForgetEdict(0))
        return Fail(42,"original alias not tombstoned on entity release");
    original_alias.generation=3;
    if(!original_alias_registry.Enroll(0,original_alias))
        return Fail(43,"higher generation original alias re-enrollment refused");
    if(r.Enroll(2,first))return Fail(2,"duplicate live address enrolled");
    // All owner ranges are disjoint EXCEPT source-backed exact
    // CBaseEntity == IServerUnknown identity. Cross-owner, partial-offset,
    // oversized alias, or entity/touch alias must still be rejected.
    auto aliased=third;
    aliased.unknown=first.entity;
    if(r.Enroll(2,aliased))return Fail(27,"cross-field live address alias");
    aliased=third;
    aliased.edict=static_cast<const unsigned char*>(first.edict)+1;
    if(r.Enroll(2,aliased))return Fail(28,"partial live edict overlap");
    aliased=third;
    aliased.touch=third.entity;
    if(r.Enroll(2,aliased))return Fail(29,"overlapping fields in one slot");
    aliased=third;
    aliased.entity=reinterpret_cast<const void*>(
        (std::numeric_limits<std::uintptr_t>::max)()-4);
    if(r.Enroll(2,aliased))return Fail(30,"overflowed entity extent");
    auto short_edict=third;short_edict.edict_bytes=0x13;
    if(r.Enroll(2,short_edict))return Fail(3,"short edict prefix enrolled");
    auto short_entity=third;short_entity.entity_bytes=0x16f;
    if(r.Enroll(2,short_entity))return Fail(4,"short entity prefix enrolled");
    if(!r.BindCandidate(91,2,1)||!r.BindCandidate(92,0,0))
        return Fail(5,"candidate binding refused");
    if(r.BindCandidate(91,2,0))return Fail(6,"live candidate overwritten");
    if(r.BindCandidate(93,1,1))return Fail(7,"candidate capacity ignored");

    std::uint64_t token=99;
    if(!r.Begin(5,&token)||!token||!r.HasLease())
        return Fail(8,"cannot acquire original interface owner lease");
    if(!r.ValidEdict(first.edict)||!r.ValidEntity(first.entity)||
       !r.ValidUnknown(first.unknown)||!r.ValidTouch(first.touch,0x90)||
       !r.ValidPair(first.edict,first.entity)||
       !r.ValidUnknownPair(first.edict,first.unknown)||
       !r.ValidCandidate(91,2,second.entity))
        return Fail(9,"registered live bytes not authenticated under lease");
    if(r.ValidPair(first.edict,second.entity)||
       r.ValidUnknownPair(first.edict,second.unknown)||
       r.ValidCandidate(91,2,first.entity)||r.ValidTouch(first.touch,0x91))
        return Fail(10,"cross-wired entity or short touch extent accepted");
    if(r.Enroll(2,third)||r.BindCandidate(94,2,0)||
       r.RetireEntity(0)||r.ForgetEdict(1))
        return Fail(11,"mutation accepted under read-only lease");
    std::uint64_t nested=123;
    if(r.Begin(4,&nested)||nested!=0||r.End(4,token)||r.End(5,token+1))
        return Fail(12,"nested lease or wrong-token release accepted");
    bool foreign_valid=true,foreign_end=true,foreign_begin=true;
    std::thread outsider([&]() {
        foreign_valid=r.ValidEntity(first.entity);
        foreign_end=r.End(5,token);
        std::uint64_t next=0;
        foreign_begin=r.Begin(5,&next);
    });
    outsider.join();
    if(foreign_valid||foreign_end||foreign_begin||!r.HasLease())
        return Fail(13,"other thread accessed the owner's lease");
    if(!r.End(5,token)||r.HasLease()||r.ValidEdict(first.edict))
        return Fail(14,"lease close failed or unleased read succeeded");
    if(!r.Enroll(2,third))return Fail(15,"third independent slot refused");

    // Slot 3 can retire an actor during FreeContainingEntity cleanup while
    // retaining edict prefix authority until the owner forgets that storage.
    if(!r.Begin(3,&token)||!r.RetireEntity(1)||
       !r.ValidEdict(second.edict)||r.ValidEntity(second.entity)||
       r.ValidUnknown(second.unknown)||r.ValidPair(second.edict,second.entity)||
       r.ValidCandidate(91,2,second.entity))
        return Fail(16,"release tombstone did not revoke actor identity");
    if(r.ForgetEdict(1)||r.BindCandidate(91,2,0)||!r.End(3,token))
        return Fail(17,"release lease permitted premature reuse");
    if(!r.ForgetEdict(1))return Fail(18,"owner cannot forget released edict");
    if(r.Enroll(1,second))return Fail(19,"same generation resurrected");
    auto upgraded=Identity(b,4);
    if(!r.Enroll(1,upgraded))return Fail(20,"new owner generation refused");
    if(!r.Begin(6,&token)||r.ValidCandidate(91,2,second.entity)||
       !r.ValidPair(second.edict,second.entity)||!r.End(6,token))
        return Fail(21,"old candidate binding followed new address generation");
    if(!r.BindCandidate(91,2,1)||!r.Begin(6,&token)||
       !r.ValidCandidate(91,2,second.entity)||!r.End(6,token))
        return Fail(22,"explicit candidate rebind not accepted");

    if(!r.RetireEntity(1)||!r.ForgetEdict(1)||
       r.Enroll(1,Identity(b,4)))
        return Fail(23,"stale identity resurrected");
    if(!r.RetireEntity(2)||!r.ForgetEdict(2))
        return Fail(24,"owner cannot retire third slot");
    if(r.Enroll(2,Identity(b,3)))
        return Fail(25,"older generation migrated to a different slot");
    if(!r.Enroll(2,Identity(b,5))||
       !r.Begin(5,&token)||!r.ValidPair(second.edict,second.entity)||
       r.ValidCandidate(91,2,second.entity)||!r.End(5,token))
        return Fail(26,"new owner reused address without rebind");
    // Retired slots lose their previous record on reuse; their old
    // generation must nevertheless never be accepted under a fresh
    // address after the latest global enrollment has advanced.
    if(!r.RetireEntity(2)||!r.ForgetEdict(2))
        return Fail(31,"cannot retire reused slot for epoch regression");
    Storage fresh{};
    if(r.Enroll(2,Identity(fresh,5)))
        return Fail(32,"global generation rollback through fresh address");
    if(!r.Enroll(2,Identity(fresh,6)))
        return Fail(33,"new higher owner generation unexpectedly refused");

    // Phase 1644: the edict reservation must be visible while the engine
    // owner pin is still held; the entity is created and attested later.
    // These are independently owned synthetic allocations, not retail DLLs.
    Registry staged;
    Storage stage_a{},stage_b{},stage_c{},stage_spare{};
    if(staged.ReserveEdict(0,stage_a.edict,0x10,1000)||
       staged.ReserveEdict(0,stage_a.edict,0x14,0)||
       !staged.ReserveEdict(0,stage_a.edict,0x14,1000)||
       staged.ReserveEdict(0,stage_a.edict,0x14,1001)||
       staged.ReserveEdict(1,stage_a.edict,0x14,1001)||
       staged.ReserveEdict(1,stage_b.edict,0x14,1000)||
       !staged.ReserveEdict(1,stage_b.edict,0x14,1001))
        return Fail(44,"invalid, replayed or overlapping pinned reservation");
    std::uint64_t staged_lease=0;
    auto pending_a=Identity(stage_a,1000);
    pending_a.edict_bytes=0x14;
    pending_a.unknown=pending_a.entity;
    pending_a.unknown_bytes=sizeof(void*);
    auto pending_b=Identity(stage_b,1001);
    pending_b.edict_bytes=0x14;
    pending_b.unknown=pending_b.entity;
    pending_b.unknown_bytes=sizeof(void*);
    if(!staged.Begin(5,&staged_lease)||
       !staged.ValidEdict(pending_a.edict)||
       !staged.ValidEdict(pending_b.edict)||
       staged.ValidEntity(pending_a.entity)||
       staged.ValidUnknown(pending_b.unknown)||
       staged.ValidUnknownPair(pending_a.edict,pending_a.unknown)||
       staged.ActivateReservedEntity(0,pending_a)||
       staged.ReserveEdict(2,stage_spare.edict,0x14,1002)||
       !staged.End(5,staged_lease))
        return Fail(45,"pending edict exposed entity or bypassed lease");
    auto forged=pending_a;
    forged.generation=1001;
    if(staged.ActivateReservedEntity(0,forged))
        return Fail(46,"reserved slot accepted a different owner epoch");
    forged=pending_a;
    forged.edict=stage_spare.edict;
    if(staged.ActivateReservedEntity(0,forged))
        return Fail(47,"reserved slot accepted a different edict address");
    forged=pending_a;
    forged.unknown=static_cast<const unsigned char*>(forged.entity)+4;
    if(staged.ActivateReservedEntity(0,forged))
        return Fail(48,"reserved slot bypassed original unknown alias rule");
    // Attachment order need not match globally monotonic reservation order.
    if(!staged.ActivateReservedEntity(1,pending_b)||
       !staged.ActivateReservedEntity(0,pending_a)||
       staged.ActivateReservedEntity(0,pending_a)||
       !staged.Begin(5,&staged_lease)||
       !staged.ValidUnknownPair(pending_a.edict,pending_a.entity)||
       !staged.ValidUnknownPair(pending_b.edict,pending_b.entity)||
       staged.ValidUnknownPair(pending_a.edict,pending_b.entity)||
       !staged.End(5,staged_lease))
        return Fail(49,"out-of-order attested entity activation");
    if(staged.ReserveEdict(0,stage_a.edict,0x14,1002)||
       !staged.Begin(3,&staged_lease)||
       !staged.RetireEntity(0)||
       !staged.End(3,staged_lease)||
       !staged.ReserveEdict(0,stage_a.edict,0x14,1002))
        return Fail(50,"reused edict reservation before/after entity retirement");
    auto next_entity=Identity(stage_c,1002);
    next_entity.edict=stage_a.edict;
    next_entity.edict_bytes=0x14;
    next_entity.unknown=next_entity.entity;
    next_entity.unknown_bytes=sizeof(void*);
    if(!staged.ActivateReservedEntity(0,next_entity)||
       !staged.Begin(5,&staged_lease)||
       staged.ValidUnknownPair(pending_a.edict,pending_a.entity)||
       !staged.ValidUnknownPair(next_entity.edict,next_entity.entity)||
       !staged.End(5,staged_lease))
        return Fail(51,"retired entity alias survived new owner epoch");
    Registry abandoned;
    if(!abandoned.ReserveEdict(0,stage_spare.edict,0x14,1)||
       abandoned.ActivateReservedEntity(0,pending_a)||
       !abandoned.ForgetEdict(0)||
       !abandoned.ReserveEdict(0,stage_spare.edict,0x14,2))
        return Fail(52,"abandoned reservation not confined to its epoch");
    std::puts("PASS owner lifecycle registry: source-backed exact CBaseEntity/IServerUnknown alias, leases, thread isolation, overlap/overflow bounds, retirement, global generations and transmit rebinding");
    return 0;
}
