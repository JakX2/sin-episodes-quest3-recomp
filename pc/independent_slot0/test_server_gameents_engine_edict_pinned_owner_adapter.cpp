// Original-free host-lifetime / owner-pinned engine allocation integration.
#include "server_gameents_engine_edict_pinned_owner_adapter.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <thread>
using namespace sin_independent;
struct Edict {
    std::uint32_t flags=0,serial=0xffffffffu,networkable=0,
                  unknown=0x4567,reuse_time=0;
};
static_assert(sizeof(Edict)==0x14,"original 2006 PE32 engine edict stride");
struct Owner {
    std::array<Edict,6> slots{};
    EnginePinnedEdictAllocatorAdapter* adapter=nullptr;
    std::size_t count=2, protected_last=1;
    std::uint64_t token=100,host_epoch=1000,last_reserved=0;
    unsigned begin_count=0,end_count=0,cleanup_calls=0,
             cleanup_actual=0,commit_calls=0,mutate_refusals=0,
             nested_busy=0;
    bool pin_held=false,storage_alive=true,valid=true,
         deny_acquire=false,deny_end=false,deny_cleanup=false,
         deny_commit=false,deny_epoch=false,malformed=false,
         revoke_in_cleanup=false,revoked=false,
         stale_serial_on_second_cleanup=false,
         attempt_mutation_in_cleanup=true,
         reenter_in_cleanup=false;
    bool concurrent_attempt=false;
    bool TryResizeOrFreeBacking() noexcept {
        if(pin_held) {++mutate_refusals;return false;}
        storage_alive=false;return true;
    }
};
static bool Acquire(void* p,EnginePinnedEdictSnapshot* out) noexcept {
    auto& o=*static_cast<Owner*>(p);
    if(!out||o.pin_held||o.deny_acquire||!o.storage_alive)return false;
    o.pin_held=true;
    ++o.begin_count;
    *out={o.slots.data(),sizeof(o.slots),
          o.malformed?o.slots.size()+1:o.count,
          o.protected_last,++o.token};
    return true;
}
static bool Validate(void* p,const EnginePinnedEdictSnapshot* snap) noexcept {
    const auto& o=*static_cast<Owner*>(p);
    return snap&&o.pin_held&&o.storage_alive&&o.valid&&!o.revoked&&
        snap->token==o.token&&snap->array==o.slots.data()&&
        snap->capacity_bytes==sizeof(o.slots);
}
static bool End(void* p,std::uint64_t token) noexcept {
    auto& o=*static_cast<Owner*>(p);
    if(!o.pin_held||token!=o.token)return false;
    ++o.end_count;
    o.pin_held=false;
    return !o.deny_end;
}
static EnginePinnedEdictHost Host(Owner& o) noexcept;
static bool Free(void* p,std::uint64_t token,std::size_t index,
                 void* edict) noexcept {
    auto& o=*static_cast<Owner*>(p);
    if(!o.pin_held||token!=o.token||index>=o.slots.size()||
       edict!=&o.slots[index])return false;
    ++o.cleanup_calls;
    if(o.attempt_mutation_in_cleanup&&o.TryResizeOrFreeBacking())return false;
    if(o.reenter_in_cleanup&&o.adapter) {
        const auto nested=o.adapter->Allocate(Host(o),400,0.01f);
        if(nested.status!=EnginePinnedEdictStatus::Busy)return false;
        ++o.nested_busy;
    }
    if(o.concurrent_attempt&&o.adapter) {
        EnginePinnedEdictStatus concurrent=EnginePinnedEdictStatus::InvalidHost;
        std::thread worker([&] {
            concurrent=o.adapter->Allocate(Host(o),400,0.01f).status;
        });
        worker.join();
        if(concurrent!=EnginePinnedEdictStatus::Busy)return false;
        ++o.nested_busy;
    }
    if(o.deny_cleanup)return false;
    if(o.slots[index].unknown) {
        o.slots[index].unknown=0;
        ++o.cleanup_actual;
    }
    if(o.stale_serial_on_second_cleanup&&o.cleanup_calls==2)
        o.slots[index].serial=777;
    if(o.revoke_in_cleanup)o.revoked=true;
    return true;
}
static bool Reserve(void* p,std::uint64_t token,
                    const EngineEdictReuseEvent& e,
                    std::uint64_t* out) noexcept {
    auto& o=*static_cast<Owner*>(p);
    if(!o.pin_held||token!=o.token||!out||o.deny_epoch||
       e.index>=o.slots.size()||e.next_serial!=e.prior_serial+1u)
        return false;
    o.last_reserved=++o.host_epoch;
    *out=o.last_reserved;
    return true;
}
static bool Commit(void* p,std::uint64_t token,
                   const EnginePinnedEdictCommit& c) noexcept {
    auto& o=*static_cast<Owner*>(p);
    ++o.commit_calls;
    if(!o.pin_held||token!=o.token||o.deny_commit||
       c.old_count!=o.count||c.new_count>o.slots.size()||
       c.new_count<c.old_count||c.new_count>c.old_count+1||
       c.index>=c.new_count||c.epoch!=o.last_reserved||
       c.index!=c.restamp.index||
       o.slots[c.index].serial!=c.restamp.next_serial||
       o.slots[c.index].reuse_time!=c.restamp.time_bits||
       (o.slots[c.index].flags&2u)==0)return false;
    o.count=c.new_count;
    return true;
}
static EnginePinnedEdictHost Host(Owner& o) noexcept {
    return {&o,&Acquire,&Validate,&End,&Free,&Reserve,&Commit};
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL owner-pinned edict allocation %d: %s\n",n,why);
    return n;
}
int main() {
    using S=EnginePinnedEdictStatus;
    Owner o{};
    EnginePinnedEdictAllocatorAdapter adapter(1000);
    o.adapter=&adapter;
    const auto first=adapter.Allocate(Host(o),400,0.01f);
    if(first.status!=S::Activated||first.index!=2||first.count!=3||
       first.owner_epoch!=1001||first.serial!=0||o.count!=3||
       o.slots[2].flags!=2||o.slots[2].serial!=0||
       o.slots[2].reuse_time!=0x40800000||
       o.cleanup_calls!=2||o.cleanup_actual!=1||
       o.begin_count!=1||o.end_count!=1||o.pin_held||
       o.mutate_refusals!=2||adapter.Quarantined()||
       adapter.LatestReservedEpoch()!=1001)
        return Fail(1,"pin, cleanup order, new count or reserved epoch");
    // The host can replace/reconfigure backing only after the pin ended.
    // Keep this fixture backing for another allocation, not a raw pointer.
    const auto second=adapter.Allocate(Host(o),400,0.01f);
    if(second.status!=S::Activated||second.index!=3||second.count!=4||
       second.owner_epoch!=1002||o.count!=4||o.cleanup_calls!=4||
       o.begin_count!=2||o.end_count!=2||o.mutate_refusals!=4)
        return Fail(2,"returned active edict immediately reselected");
    Owner parallel{};
    EnginePinnedEdictAllocatorAdapter parallel_adapter(1000);
    parallel.adapter=&parallel_adapter;
    parallel.reenter_in_cleanup=true;
    parallel.concurrent_attempt=true;
    if(parallel_adapter.Allocate(Host(parallel),400,0.01f).status!=
           S::Activated||parallel.nested_busy!=4||
       parallel.begin_count!=1||parallel.end_count!=1||
       parallel.cleanup_calls!=2||parallel.mutate_refusals!=2)
        return Fail(13,"nested or competing thread entered pinned callback");
    Owner stale_epoch{};
    EnginePinnedEdictAllocatorAdapter stale_epoch_adapter(2000);
    if(stale_epoch_adapter.Allocate(Host(stale_epoch),400,0.01f).status!=
           S::ReuseFailed||!stale_epoch_adapter.Quarantined()||
       stale_epoch_adapter.LatestReservedEpoch()!=2000||
       stale_epoch.count!=2||stale_epoch.commit_calls)
        return Fail(14,"stale host epoch accepted across allocator sessions");
    Owner pin_denied{};pin_denied.deny_acquire=true;
    EnginePinnedEdictAllocatorAdapter pin_adapter;
    if(pin_adapter.Allocate(Host(pin_denied),400,0.01f).status!=
           S::PinUnavailable||pin_denied.end_count||pin_adapter.Quarantined())
        return Fail(3,"rejected pin ran callbacks or quarantined clean state");
    Owner malformed{};malformed.malformed=true;
    EnginePinnedEdictAllocatorAdapter bad_adapter;
    if(bad_adapter.Allocate(Host(malformed),400,0.01f).status!=
           S::BadSnapshot||malformed.begin_count!=1||
       malformed.end_count!=1||malformed.cleanup_calls||
       bad_adapter.Quarantined())
        return Fail(4,"malformed capacity/count snapshot accepted");
    Owner refused{};refused.deny_cleanup=true;
    EnginePinnedEdictAllocatorAdapter refused_adapter;
    if(refused_adapter.Allocate(Host(refused),400,0.01f).status!=
           S::SelectionFailed||!refused_adapter.Quarantined()||
       refused.end_count!=1||refused.count!=2||
       refused_adapter.Allocate(Host(refused),400,0.01f).status!=S::Quarantined||
       refused.begin_count!=1)return Fail(5,"failed destructive callback retried");
    Owner revoked{};revoked.revoke_in_cleanup=true;
    EnginePinnedEdictAllocatorAdapter revoked_adapter;
    if(revoked_adapter.Allocate(Host(revoked),400,0.01f).status!=
           S::SelectionFailed||!revoked_adapter.Quarantined()||
       revoked.end_count!=1||revoked.commit_calls)
        return Fail(6,"postcallback owner revocation not quarantined");
    Owner serial_mutated{};serial_mutated.stale_serial_on_second_cleanup=true;
    EnginePinnedEdictAllocatorAdapter serial_adapter;
    const auto serial_result=serial_adapter.Allocate(
        Host(serial_mutated),400,0.01f);
    if(serial_result.status!=S::ReuseFailed||
       serial_result.reuse!=EngineEdictReuseStatus::SerialChanged||
       serial_mutated.slots[2].serial!=777||
       serial_mutated.commit_calls||!serial_adapter.Quarantined())
        return Fail(7,"retail serial ABA after callback not blocked");
    Owner epoch_refused{};epoch_refused.deny_epoch=true;
    EnginePinnedEdictAllocatorAdapter epoch_adapter;
    if(epoch_adapter.Allocate(Host(epoch_refused),400,0.01f).status!=
           S::ReuseFailed||!epoch_adapter.Quarantined()||
       epoch_adapter.LatestReservedEpoch()!=0||epoch_refused.commit_calls)
        return Fail(8,"epoch refusal not protected");
    Owner commit_refused{};commit_refused.deny_commit=true;
    EnginePinnedEdictAllocatorAdapter commit_adapter;
    if(commit_adapter.Allocate(Host(commit_refused),400,0.01f).status!=
           S::CommitFailed||!commit_adapter.Quarantined()||
       commit_adapter.LatestReservedEpoch()!=1001||
       commit_refused.count!=2||commit_refused.slots[2].serial!=0||
       commit_refused.end_count!=1)
        return Fail(9,"failed count/epoch commit returned live allocation");
    Owner end_refused{};end_refused.deny_end=true;
    EnginePinnedEdictAllocatorAdapter end_adapter;
    const auto release=end_adapter.Allocate(Host(end_refused),400,0.01f);
    if(release.status!=S::PinReleaseFailed||!end_adapter.Quarantined()||
       release.owner_epoch||end_refused.end_count!=1||
       end_refused.count!=3)
        return Fail(10,"failed pin release reported successful slot");
    Owner no_space{};
    no_space.count=no_space.slots.size();
    EnginePinnedEdictAllocatorAdapter full_adapter;
    const auto full=full_adapter.Allocate(Host(no_space),400,0.01f);
    if(full.status!=S::SelectionFailed||
       full.selection!=EngineEdictSelectStatus::NoEligibleRecord||
       full_adapter.Quarantined()||no_space.cleanup_calls||
       no_space.end_count!=1||no_space.count!=no_space.slots.size())
        return Fail(11,"no-capacity scan modified backing");
    EnginePinnedEdictHost incomplete=Host(o);
    incomplete.validate_pin=nullptr;
    if(adapter.Allocate(incomplete,400,0.01f).status!=S::InvalidHost)
        return Fail(12,"incomplete physical pin host accepted");
    std::puts("PASS owner-pinned edict allocator: exclusive physical lease, original select/free/restamp, atomic count/epoch commit, stale/revoked/capacity failures and quarantine");
    return 0;
}
