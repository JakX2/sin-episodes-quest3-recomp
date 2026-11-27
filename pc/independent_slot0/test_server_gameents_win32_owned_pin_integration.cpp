// Phase 1647: native MSVC x86 VirtualAlloc-backed, physically pinned owner.
// The original 2006 PE32 engine/game DLLs are NEVER loaded or called.
#include "server_gameents_win32_owned_pin_pages.h"
#include "server_gameents_engine_edict_pinned_owner_adapter.h"
#include "server_gameents_pinned_original_link_attacher.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <thread>
using namespace sin_independent;
using Pages=Win32OwnedGameEntsPinPages;
using Registry=GameEntsOwnerLifetimeRegistry<1,1>;
using Attacher=GameEntsPinnedOriginalLinkAttacher<1,1>;
using Allocator=EnginePinnedEdictAllocatorAdapter;

static_assert(sizeof(void*)==4,"owner physical pin integrates exact Win32 PE32");
static_assert(Pages::kArrayBytes==6*0x14&&Pages::kEntityBytes>=0x170,
              "original engine and game link footprints");

static std::uint32_t Ptr32(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
static void Put32(unsigned char* ptr,std::uint32_t value) noexcept {
    ptr[0]=static_cast<unsigned char>(value);
    ptr[1]=static_cast<unsigned char>(value>>8);
    ptr[2]=static_cast<unsigned char>(value>>16);
    ptr[3]=static_cast<unsigned char>(value>>24);
}
static std::uint32_t Get32(const unsigned char* ptr) noexcept {
    return std::uint32_t(ptr[0])|(std::uint32_t(ptr[1])<<8)|
           (std::uint32_t(ptr[2])<<16)|(std::uint32_t(ptr[3])<<24);
}

struct Owner {
    Pages pages{};
    Registry registry{};
    Pages::Snapshot pinned{};
    std::size_t count=2;
    std::uint64_t host_epoch=1000,last_reserved=0,committed_epoch=0;
    unsigned acquires=0,ends=0,cleanups=0,checks=0,link_checks=0;
    bool committed=false;
    bool attached=false;
    bool deny_commit=false;
    bool deny_link_readback=false;
    bool simultaneous_probe_succeeded=false;

    bool Held(std::uint64_t token) noexcept {
        return token&&pinned.token==token&&pages.Validate(&pinned);
    }
    bool TryClose() noexcept {
        // Host MUST reconcile the logical owner before releasing backing.
        if(committed)return false;
        return pages.TryRelease();
    }
    bool ReconcileAndClose() noexcept {
        if(committed) {
            std::uint64_t lease=0;
            if(attached) {
                if(!registry.Begin(3,&lease))return false;
                const bool retired=registry.RetireEntity(0);
                const bool ended=registry.End(3,lease);
                if(!retired||!ended)return false;
            }
            if(!registry.ForgetEdict(0))return false;
            attached=false;
            committed=false;
        }
        return pages.TryRelease();
    }
    bool PrepareLink(GameEntsOwnerIdentity* out) noexcept {
        if(!out||!committed)return false;
        Pages::Snapshot s{};
        if(!pages.Begin(&s))return false;
        // These are actual 32-bit Windows pointer values in VirtualAlloc
        // backing, never fabricated x64 PE tokens or original game pointers.
        auto* edict=s.array+2*Pages::kEdictStride;
        Put32(edict+0x0c,Ptr32(s.entity));
        Put32(s.entity+0x168,Ptr32(s.entity));
        Put32(s.entity+0x16c,Ptr32(edict));
        *out={edict,Pages::kEdictStride,s.entity,s.entity_bytes,
              s.entity,sizeof(void*),s.touch,s.touch_bytes,committed_epoch};
        return pages.End(s.token);
    }
    static bool AcquireEngine(void* p,EnginePinnedEdictSnapshot* out) noexcept {
        auto& self=*static_cast<Owner*>(p);
        if(!out||!self.pages.Begin(&self.pinned))return false;
        ++self.acquires;
        *out={self.pinned.array,self.pinned.array_bytes,
              self.count,1,self.pinned.token};
        return true;
    }
    static bool ValidateEngine(void* p,const EnginePinnedEdictSnapshot* s)
        noexcept {
        auto& self=*static_cast<Owner*>(p);
        return s&&self.Held(s->token)&&
               s->array==self.pinned.array&&
               s->capacity_bytes==self.pinned.array_bytes&&
               self.pages.IsCommittedPrivatePage(self.pinned.array)&&
               self.pages.IsCommittedPrivatePage(self.pinned.entity)&&
               self.pages.IsCommittedPrivatePage(self.pinned.touch);
    }
    static bool EndEngine(void* p,std::uint64_t token) noexcept {
        auto& self=*static_cast<Owner*>(p);
        if(!self.pages.End(token))return false;
        ++self.ends;
        self.pinned={};
        return true;
    }
    static bool Cleanup(void* p,std::uint64_t token,std::size_t index,
                        void* edict) noexcept {
        auto& self=*static_cast<Owner*>(p);
        if(!self.Held(token)||index!=2||
           edict!=self.pinned.array+index*Pages::kEdictStride)return false;
        ++self.cleanups;
        // Real VirtualFree-backed host, not a boolean-only fake: neither
        // nested nor concurrent operations may release or modify pages.
        if(self.pages.TryRelease()||
           self.pages.TryWrite(self.pinned.array,0x0c,0xBAD00000))
            return false;
        std::atomic<bool> other_freed{false},other_wrote{false},wrong_end{false};
        std::thread competing([&]{
            other_freed=self.pages.TryRelease();
            other_wrote=self.pages.TryWrite(self.pinned.array,0,123u);
            wrong_end=self.pages.End(token);
        });
        competing.join();
        if(other_freed||other_wrote||wrong_end||!self.Held(token))
            return false;
        // An invalid token must also refuse to unlock on the SAME thread.
        if(self.pages.End(token+1)||!self.Held(token))return false;
        auto* rec=static_cast<unsigned char*>(edict);
        Put32(rec+0x0c,0); // idempotent original +0x0C cleanup
        return true;
    }
    static bool Reserve(void* p,std::uint64_t token,
                        const EngineEdictReuseEvent& event,
                        std::uint64_t* out) noexcept {
        auto& self=*static_cast<Owner*>(p);
        if(!out||!self.Held(token)||event.index!=2||
           event.next_serial!=event.prior_serial+1u||
           self.host_epoch==(std::numeric_limits<std::uint64_t>::max)())
            return false;
        *out=self.last_reserved=++self.host_epoch;
        return true;
    }
    static bool Commit(void* p,std::uint64_t token,
                       const EnginePinnedEdictCommit& c) noexcept {
        auto& self=*static_cast<Owner*>(p);
        if(!self.Held(token)||self.deny_commit||self.committed||
           c.index!=2||c.old_count!=self.count||
           c.new_count!=self.count+1||c.epoch!=self.last_reserved||
           c.restamp.index!=c.index||
           c.restamp.next_serial!=Get32(self.pinned.array+2*0x14+4)||
           c.restamp.time_bits!=Get32(self.pinned.array+2*0x14+0x10)||
           (Get32(self.pinned.array+2*0x14)&2u)==0)return false;
        // Single owner transaction under the ACTUAL Win32 memory pin.
        if(!self.registry.ReserveEdict(
            0,self.pinned.array+2*0x14,0x14,c.epoch))return false;
        self.count=c.new_count;
        self.committed_epoch=c.epoch;
        self.committed=true;
        return true;
    }
    EnginePinnedEdictHost EngineCallbacks() noexcept {
        return {this,&AcquireEngine,&ValidateEngine,&EndEngine,
                &Cleanup,&Reserve,&Commit};
    }
    static bool AcquireLink(void* p,GameEntsOriginalLinkSnapshot* out)
        noexcept {
        auto& self=*static_cast<Owner*>(p);
        if(!out||!self.committed||
           !self.pages.Begin(&self.pinned))return false;
        ++self.acquires;
        const auto* edict=self.pinned.array+2*Pages::kEdictStride;
        *out={edict,Pages::kEdictStride,self.pinned.entity,
              self.pinned.entity_bytes,self.pinned.touch,
              self.pinned.touch_bytes,Ptr32(edict),Ptr32(self.pinned.entity),
              self.committed_epoch,self.pinned.token};
        return true;
    }
    static bool ValidateLink(void* p,const GameEntsOriginalLinkSnapshot* s)
        noexcept {
        auto& self=*static_cast<Owner*>(p);
        ++self.link_checks;
        if(!s||!self.Held(s->pin_token)||self.deny_link_readback||
           s->edict!=self.pinned.array+2*Pages::kEdictStride||
           s->entity!=self.pinned.entity||s->touch!=self.pinned.touch||
           s->epoch!=self.committed_epoch||s->edict_pe32!=Ptr32(s->edict)||
           s->entity_pe32!=Ptr32(s->entity))return false;
        // Mutation and physical free must still be refused by the host while
        // the attachment callback and registry activation are underway.
        if(self.pages.TryRelease()||
           self.pages.TryWrite(self.pinned.entity,0x168,0))
            return false;
        self.simultaneous_probe_succeeded=true;
        return true;
    }
    static bool EndLink(void* p,std::uint64_t token) noexcept {
        return EndEngine(p,token);
    }
    GameEntsOriginalLinkHost LinkCallbacks() noexcept {
        return {this,&AcquireLink,&ValidateLink,&EndLink};
    }
};

static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL Win32 owned physical pin %d: %s\n",n,why);
    return n;
}
int main() {
    Owner owner{};
    if(!owner.pages.Allocate())return Fail(1,"VirtualAlloc 3 distinct pages");
    Pages::Snapshot first{};
    if(!owner.pages.Begin(&first)||
       !Pages::IsCommittedPrivatePage(first.array)||
       !Pages::IsCommittedPrivatePage(first.entity)||
       !Pages::IsCommittedPrivatePage(first.touch)||
       first.array==first.entity||first.array==first.touch||
       owner.pages.TryRelease()||
       owner.pages.TryWrite(first.entity,0x168,111)||
       !owner.pages.Validate(&first)||owner.pages.End(first.token+1)||
       !owner.pages.End(first.token)||owner.pages.Validate(&first))
        return Fail(2,"real page commit, exclusive pin and exact token");
    // All non-pinned writes must be bounded and physically committed.
    if(!owner.pages.TryWrite(first.entity,0x168,0)||
       owner.pages.TryWrite(first.entity,Pages::kEntityBytes-3,0)||
       owner.pages.TryWrite(first.array+1,0,0))
        return Fail(3,"host write bounds after unpin");

    Allocator allocator(1000);
    const auto allocated=allocator.Allocate(owner.EngineCallbacks(),400,0.01f);
    if(allocated.status!=EnginePinnedEdictStatus::Activated||
       allocated.index!=2||allocated.count!=3||
       allocated.owner_epoch!=1001||allocated.serial!=0||
       owner.count!=3||owner.committed_epoch!=1001||
       owner.cleanups!=2||owner.ends!=2||owner.acquires!=2||
       owner.pages.RefusedOperations()<10||owner.TryClose()||
       allocator.Quarantined())
        return Fail(4,"owner-backed edict selection, dual free and epoch commit");
    GameEntsOwnerIdentity identity{};
    if(!owner.PrepareLink(&identity)||
       identity.generation!=allocated.owner_epoch||
       identity.unknown!=identity.entity||
       identity.edict_bytes!=0x14||
       identity.edict!=static_cast<const unsigned char*>(first.array)+2*0x14)
        return Fail(5,"owner-bound CServerNetworkProperty reciprocal links");
    Attacher attacher;
    if(attacher.Attach(owner.registry,0,identity,owner.LinkCallbacks())!=
           GameEntsOriginalLinkStatus::Activated||
       !owner.simultaneous_probe_succeeded||
       owner.link_checks!=3||attacher.Quarantined())
        return Fail(6,"simultaneous Win32 pin through entity activation");
    owner.attached=true;
    std::uint64_t lease=0;
    if(!owner.registry.Begin(5,&lease)||
       !owner.registry.ValidEdictAtEpoch(identity.edict,1001)||
       !owner.registry.ValidUnknownPairAtEpoch(
           identity.edict,identity.unknown,1001)||
       owner.registry.ValidUnknownPairAtEpoch(
           identity.edict,identity.unknown,1000)||
       !owner.registry.ValidPairAtEpoch(identity.edict,identity.entity,1001)||
       !owner.registry.ValidTouchAtEpoch(identity.touch,0x90,1001)||
       !owner.registry.End(5,lease))
        return Fail(7,"epoch-scoped live entity identity from Windows backing");
    if(owner.TryClose())return Fail(8,"logical live owner released backing");
    if(!owner.ReconcileAndClose()||!owner.TryClose()||
       Pages::IsCommittedPrivatePage(first.array))
        return Fail(9,"real VirtualFree after slot-3 retire and ForgetEdict");

    // A separate physical owner can fail after selecting an edict and
    // must quarantine rather than expose an uncommitted live slot.
    Owner refused{};
    if(!refused.pages.Allocate())return Fail(10,"second VirtualAlloc");
    refused.deny_commit=true;
    Allocator refused_allocator(1000);
    const auto rejected=refused_allocator.Allocate(
        refused.EngineCallbacks(),400,0.01f);
    if(rejected.status!=EnginePinnedEdictStatus::CommitFailed||
       !refused_allocator.Quarantined()||refused.committed||
       refused.count!=2||refused.cleanups!=2||
       !refused.TryClose())
        return Fail(11,"native backing commit refusal and physical cleanup");
    std::puts("PASS Win32 VirtualAlloc owner pin: physically stable pages, mutex-held engine callbacks, denied VirtualFree and cross-thread writes, atomic edict epoch commit, reciprocal CBaseEntity links, protected reaping and quarantine");
    return 0;
}
