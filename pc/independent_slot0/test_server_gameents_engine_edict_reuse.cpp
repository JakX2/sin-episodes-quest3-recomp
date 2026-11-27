// Standalone original-free edict reuse transition and epoch semantics.
#include "server_gameents_engine_edict_reuse_core.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
using namespace sin_independent;
struct Edict {
    std::uint32_t flags=0,serial=0xffffffffu,networkable=0,
                  unknown=0x1234,reuse_time=0;
};
static_assert(sizeof(Edict)==0x14,"original 2006 engine edict stride");
static_assert(offsetof(Edict,reuse_time)==0x10,"original float reuse offset");
struct Fixture {
    std::array<Edict,4> edicts{};
    bool owned=true,deny_edict=false,deny_release=false,deny_epoch=false,
         revoke_on_release=false,mutate_serial=false,revoke_on_epoch=false,
         mutate_on_epoch=false,valid=true;
    unsigned frees=0,reservations=0,checks=0;
    std::uint64_t next_epoch=1000,latest_epoch=0;
    int last_index=-1;
    std::uint32_t last_prior=0,last_next=0,last_time_bits=0;
};
static bool Array(void* p,const void* ptr,std::size_t bytes) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    return f.owned&&ptr==f.edicts.data()&&bytes==sizeof(f.edicts);
}
static bool Record(void* p,const void* ptr,std::size_t bytes) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    ++f.checks;
    if(!f.owned||f.deny_edict||bytes!=sizeof(Edict))return false;
    for(auto& edict:f.edicts)if(ptr==&edict)return true;
    return false;
}
static bool Free(void* p,void* ptr) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    ++f.frees;
    const auto* edict=static_cast<const Edict*>(ptr);
    if(edict->flags&2u)f.valid=false;
    if(f.deny_release)return false;
    auto* writable=static_cast<Edict*>(ptr);
    writable->unknown=0;
    if(f.mutate_serial)writable->serial=7;
    if(f.revoke_on_release)f.owned=false;
    return true;
}
static bool Epoch(void* p,const EngineEdictReuseEvent& e,
                  std::uint64_t* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    ++f.reservations;
    if(!out||f.deny_epoch)return false;
    f.last_index=static_cast<int>(e.index);
    f.last_prior=e.prior_serial;
    f.last_next=e.next_serial;
    f.last_time_bits=e.time_bits;
    *out=++f.next_epoch;
    if(*out<=f.latest_epoch)f.valid=false;
    f.latest_epoch=*out;
    if(f.revoke_on_epoch)f.owned=false;
    if(f.mutate_on_epoch)f.edicts[e.index].serial^=4u;
    return true;
}
static EngineEdictReuseHooks Hooks(Fixture& f) noexcept {
    return {&f,&Array,&Record,&Free,&Epoch};
}
static EngineEdictReuseResult Run(Fixture& f,int index,
                                  std::size_t reserved=1,
                                  std::int32_t ticks=400,
                                  float scale=0.25f) noexcept {
    return ReuseEngineEdictAt(f.edicts.data(),sizeof(f.edicts),index,
                              reserved,ticks,scale,Hooks(f));
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL original engine reuse %d: %s\n",n,why);return n;
}
int main() {
    using S=EngineEdictReuseStatus;
    Fixture f{};
    f.edicts[2].flags=0x40;
    const auto first=Run(f,2);
    if(first.status!=S::Reused||first.owner_epoch!=1001||
       first.new_serial!=0||f.edicts[2].flags!=0x42||
       f.edicts[2].serial!=0||f.edicts[2].reuse_time!=0x42c80000||
       f.edicts[2].unknown!=0||f.last_prior!=0xffffffffu||
       f.last_next!=0||f.last_time_bits!=0x42c80000||
       f.last_index!=2||f.frees!=1||f.reservations!=1||!f.valid)
        return Fail(1,"callback order, serial wrap, timestamp, or epoch");
    const auto prior_calls=f.frees;
    if(Run(f,2).status!=S::AlreadyActive||f.frees!=prior_calls)
        return Fail(2,"already-active slot reentered server cleanup");
    if(Run(f,1).status!=S::ReservedIndex||f.frees!=prior_calls)
        return Fail(3,"reserved edict index was reused");
    if(Run(f,-1).status!=S::InvalidInput||Run(f,4).status!=S::InvalidInput||
       f.frees!=prior_calls)
        return Fail(4,"index bounds ignored");
    f.deny_edict=true;
    if(Run(f,3).status!=S::Unauthorized||f.frees!=prior_calls)
        return Fail(5,"foreign edict authorization bypassed");
    f.deny_edict=false;
    EngineEdictReuseHooks incomplete=Hooks(f);
    incomplete.authorized_array=nullptr;
    if(ReuseEngineEdictAt(f.edicts.data(),sizeof(f.edicts),3,1,1,1.f,
                          incomplete).status!=S::InvalidInput||
       f.frees!=prior_calls)
        return Fail(6,"missing owner-array callback accepted");
    Edict outsider{};
    if(ReuseEngineEdictAt(&outsider,sizeof(outsider),0,1,1,1.f,
                          Hooks(f)).status!=S::Unauthorized)
        return Fail(7,"foreign array pointer read");
    if(ReuseEngineEdictAt(f.edicts.data(),sizeof(f.edicts)-1,3,1,1,1.f,
                          Hooks(f)).status!=S::InvalidInput)
        return Fail(8,"partial array accepted");
    if(Run(f,3,1,10,-1.f).status!=S::InvalidInput||
       Run(f,3,1,10000,(std::numeric_limits<float>::max)()).status!=
           S::InvalidInput||f.frees!=prior_calls)
        return Fail(9,"invalid owner timing arithmetic entered callback");
    Fixture refused{};
    refused.deny_release=true;
    if(Run(refused,2).status!=S::CallbackRefused||
       refused.edicts[2].serial!=0xffffffffu||
       refused.edicts[2].flags!=0||refused.reservations)
        return Fail(10,"refused server cleanup finalized the engine slot");
    Fixture revoked{};
    revoked.revoke_on_release=true;
    if(Run(revoked,2).status!=S::PostCallbackUnauthorized||
       revoked.edicts[2].serial!=0xffffffffu||
       revoked.edicts[2].flags!=0||revoked.reservations)
        return Fail(11,"post-server revoked owner wrote stale edict");
    Fixture mutated{};
    mutated.mutate_serial=true;
    if(Run(mutated,2).status!=S::SerialChanged||
       mutated.edicts[2].serial!=7||mutated.reservations)
        return Fail(12,"post-server ABA serial changed without rejection");
    Fixture epoch_refused{};
    epoch_refused.deny_epoch=true;
    if(Run(epoch_refused,2).status!=S::EpochRefused||
       epoch_refused.edicts[2].serial!=0xffffffffu||
       epoch_refused.edicts[2].flags!=0)
        return Fail(13,"untrusted epoch rejection wrote fields");
    Fixture epoch_revoked{};
    epoch_revoked.revoke_on_epoch=true;
    if(Run(epoch_revoked,2).status!=S::PostCallbackUnauthorized||
       epoch_revoked.edicts[2].serial!=0xffffffffu||
       epoch_revoked.edicts[2].flags!=0)
        return Fail(14,"epoch callback revoked memory before writes");
    Fixture epoch_mutated{};
    epoch_mutated.mutate_on_epoch=true;
    if(Run(epoch_mutated,2).status!=S::SerialChanged||
       epoch_mutated.edicts[2].flags!=0)
        return Fail(15,"epoch callback ABA serial edit unnoticed");
    std::uint32_t bits=0;
    if(!EngineEdictReuseTimeBits(16777217,0.5f,&bits)||bits!=0x4b000000)
        return Fail(16,"x87 integer-to-float pre-rounding lost");
    f.edicts[3].serial=41;
    const auto next=Run(f,3,1,800,0.125f);
    if(next.status!=S::Reused||next.owner_epoch!=1002||
       next.new_serial!=42||f.edicts[3].serial!=42||
       f.edicts[3].reuse_time!=0x42c80000||f.frees!=2||f.reservations!=2)
        return Fail(17,"second owner epoch not monotonic across edicts");
    std::puts("PASS original-engine edict reuse: active/reserved gates, slot-3 release, float time, serial wrap, distinct owner epochs, denial/revocation/ABA");
    return 0;
}
