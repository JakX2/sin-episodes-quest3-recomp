// Original-free source-backed 2006 engine negative-index edict scan.
#include "server_gameents_engine_edict_select_core.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
using namespace sin_independent;
struct Edict {
    std::uint32_t flags=0,serial=42,networkable=0,
                  unknown=0x4567,reuse_time=0;
};
static_assert(sizeof(Edict)==0x14,"original 2006 record stride");
struct Owner {
    std::array<Edict,5> records{};
    unsigned frees=0,record_checks=0,epoch_reservations=0;
    bool owned=true,deny_edict=false,deny_callback=false,
         revoke_after_callback=false;
};
static bool Array(void* p,const void* ptr,std::size_t size) noexcept {
    auto& o=*static_cast<Owner*>(p);
    return o.owned&&ptr==o.records.data()&&size==sizeof(o.records);
}
static bool Record(void* p,const void* ptr,std::size_t size) noexcept {
    auto& o=*static_cast<Owner*>(p);
    ++o.record_checks;
    if(!o.owned||o.deny_edict||size!=sizeof(Edict))return false;
    for(const auto& record:o.records)if(ptr==&record)return true;
    return false;
}
static bool Release(void* p,void* ptr) noexcept {
    auto& o=*static_cast<Owner*>(p);
    ++o.frees;
    if(o.deny_callback)return false;
    static_cast<Edict*>(ptr)->unknown=0;
    if(o.revoke_after_callback)o.owned=false;
    return true;
}
static EngineEdictRetireHooks Hooks(Owner& o) noexcept {
    return {&o,&Record,&Array,&Release};
}
static bool ReserveEpoch(void* p,const EngineEdictReuseEvent& event,
                         std::uint64_t* out) noexcept {
    auto& o=*static_cast<Owner*>(p);
    if(!out||event.index!=3||event.prior_serial!=0xffffffffu||
       event.next_serial!=0||event.time_bits!=0x40800000)return false;
    *out=5000+ ++o.epoch_reservations;
    return true;
}
static void Time(Edict& e,float v) noexcept {
    std::memcpy(&e.reuse_time,&v,sizeof(v));
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL original edict selection %d: %s\n",n,why);return n;
}
int main() {
    using S=EngineEdictSelectStatus;
    Owner o{};
    // Records 0,1 protected; 2 inactive, 3 active stamped 0.5,
    // 4 active stamped 0.8. First eligible should be record 3.
    o.records[3].flags=2;
    Time(o.records[3],0.5f);
    o.records[4].flags=2;
    Time(o.records[4],0.8f);
    const auto select=SelectEngineEdictForReuse(o.records.data(),
        sizeof(o.records),5,1,16,0.25f,Hooks(o));
    if(select.status!=S::SelectedExisting||select.index!=3||
       select.new_count!=5||o.frees!=1||o.records[3].flags!=0||
       o.records[3].serial!=0xffffffffu||o.records[3].reuse_time!=0||
       o.records[3].unknown!=0||o.records[4].flags!=2)
        return Fail(1,"age scan chose wrong reusable edict or failed retirement");
    // Real synthetic lifecycle: selection -> ServerGameEnts release ->
    // per-record restamp with separate globally monotonic uint64 epoch.
    // The scanner must not immediately reclaim this freshly active record.
    const EngineEdictReuseHooks revive{
        &o,&Array,&Record,&Release,&ReserveEpoch};
    const auto restarted=ReuseEngineEdictAt(o.records.data(),
        sizeof(o.records),3,1,16,0.25f,revive);
    if(restarted.status!=EngineEdictReuseStatus::Reused||
       restarted.owner_epoch!=5001||restarted.new_serial!=0||
       o.records[3].flags!=2||o.records[3].serial!=0||
       o.records[3].reuse_time!=0x40800000||
       o.epoch_reservations!=1||o.frees!=2)
        return Fail(11,"selection-to-restamp handoff lost original serial/epoch");
    o.records[4].flags=2;Time(o.records[4],3.f);
    const auto other=SelectEngineEdictForReuse(o.records.data(),
        sizeof(o.records),5,1,16,0.25f,Hooks(o));
    if(other.status!=S::SelectedExisting||other.index!=4||o.frees!=3||
       o.records[3].flags!=2||o.records[3].serial!=0)
        return Fail(2,"restamped slot immediately reclaimed ahead of aged one");
    Owner append{};
    append.records[2].flags=0;
    auto added=SelectEngineEdictForReuse(append.records.data(),
        sizeof(append.records),2,1,100,0.01f,Hooks(append));
    if(added.status!=S::SelectedNew||added.index!=2||
       added.new_count!=3||append.frees!=1||
       append.records[2].serial!=0xffffffffu)
        return Fail(3,"append at current count without eligible reclaimed record");
    Owner exhausted{};
    const auto none=SelectEngineEdictForReuse(exhausted.records.data(),
        sizeof(exhausted.records),5,1,100,0.01f,Hooks(exhausted));
    if(none.status!=S::NoEligibleRecord||exhausted.frees)
        return Fail(4,"capacity exhaustion must not write memory");
    if(SelectEngineEdictForReuse(exhausted.records.data(),
        sizeof(exhausted.records),6,1,1,0.01f,Hooks(exhausted)).status!=
           S::InvalidInput||
       SelectEngineEdictForReuse(exhausted.records.data(),
        sizeof(exhausted.records),2,2,1,0.01f,Hooks(exhausted)).status!=
           S::InvalidInput)
        return Fail(5,"invalid capacity/count/protected range accepted");
    Owner foreign{};foreign.owned=false;
    if(SelectEngineEdictForReuse(foreign.records.data(),
        sizeof(foreign.records),2,1,1,0.01f,Hooks(foreign)).status!=
           S::Unauthorized||foreign.frees)
        return Fail(6,"untrusted edict backing read");
    Owner bad_record{};bad_record.deny_edict=true;
    if(SelectEngineEdictForReuse(bad_record.records.data(),
        sizeof(bad_record.records),2,1,1,0.01f,Hooks(bad_record)).status!=
           S::Unauthorized||bad_record.frees)
        return Fail(7,"unowned candidate record accepted");
    Owner callback{};callback.deny_callback=true;
    if(SelectEngineEdictForReuse(callback.records.data(),
        sizeof(callback.records),2,1,1,0.01f,Hooks(callback)).status!=
           S::CallbackRefused||callback.records[2].serial!=42||
       callback.records[2].reuse_time!=0)
        return Fail(8,"refused server cleanup finalized selected slot");
    Owner revoked{};revoked.revoke_after_callback=true;
    if(SelectEngineEdictForReuse(revoked.records.data(),
        sizeof(revoked.records),2,1,1,0.01f,Hooks(revoked)).status!=
           S::PostCallbackUnauthorized||revoked.records[2].serial!=42)
        return Fail(9,"post-callback revoked edict was stamped");
    // Timestamp exactly 2.0 is not the early-eligible case; at time 2.5
    // the elapsed duration 0.5 is too short to reclaim this record.
    Owner recent{};
    recent.records[2].flags=2;Time(recent.records[2],2.f);
    recent.records[3].flags=2;Time(recent.records[3],2.3f);
    recent.records[4].flags=0;
    const auto latest=SelectEngineEdictForReuse(recent.records.data(),
        sizeof(recent.records),4,1,5,0.5f,Hooks(recent));
    if(latest.status!=S::SelectedNew||latest.index!=4||recent.frees!=1)
        return Fail(10,"timestamp 2.0/age threshold misinterpreted");
    std::puts("PASS original-engine edict selector: source timing gates, selection-to-restamp epoch roundtrip, append/capacity, callback and owner failures");
    return 0;
}
