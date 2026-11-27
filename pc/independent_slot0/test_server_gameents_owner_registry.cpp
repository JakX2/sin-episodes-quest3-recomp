// Standalone host-side generation registry test. No proprietary DLLs.
#include "server_gameents_owner_lifetime_registry.h"
#include <cstdint>
#include <cstdio>
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
    if(r.Enroll(2,first))return Fail(2,"duplicate live address enrolled");
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
       !r.ValidCandidate(91,2,second.entity))
        return Fail(9,"registered live bytes not authenticated under lease");
    if(r.ValidPair(first.edict,second.entity)||
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
    std::puts("PASS owner lifecycle registry: leases, thread isolation, extents, retire/reuse, generation and transmit rebinding");
    return 0;
}
