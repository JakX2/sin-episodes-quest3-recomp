// Standalone C++17 regression for original GameEnts touch entry composed
// with the existing independently owned reciprocal-contact graph.
// No retail DLL loading, physics, models or entity byte-layout assumptions.
#include "server_gameents_touch_contact_bridge.h"
#include <array>
#include <cstdio>
#include <new>
#include <string>

class CBaseEntity { public: unsigned id=0; };
using namespace sin_independent;
struct Edict { unsigned id; };
struct Fixture {
    CBaseEntity a{1},b{2};
    Edict ea{1},eb{2};
    std::array<EntityHandleEntry,3> registry{};
    ContactNode ra{},rb{};
    TouchEntity first{},second{};
    ContactCleanupState state{};
    std::string calls;
    unsigned allocations=0,freed=0,starts=0,touches=0,refreshes=0;
    bool missing_first=false,missing_second=false,fail_resolve=false,
         fail_init=false,fail_second_refresh=false,fail_first_refresh=false,
         leave_dirty=false,fail_second_alloc=false,valid=true;
    Fixture() {
        registry[1].entity=&a;registry[1].serial=1;
        registry[2].entity=&b;registry[2].serial=2;
        ra.next=ra.previous=&ra;
        rb.next=rb.previous=&rb;
        first.entity=&a;first.handle=0x1001;first.touchStamp=7;
        first.origin={0.f,0.f,0.f};
        first.contacts={&ra,true};first.dirtyOrigin=true;
        second.entity=&b;second.handle=0x2002;second.touchStamp=9;
        second.origin={0.f,0.f,0.f};
        second.contacts={&rb,true};second.dirtyOrigin=true;
    }
    Fixture(const Fixture&)=delete;
    Fixture& operator=(const Fixture&)=delete;
    ~Fixture() {
        for(auto* root:{&ra,&rb}) {
            auto* p=root->next;
            unsigned steps=0;
            while(p&&p!=root&&steps++<5){
                auto* n=p->next;delete p;p=n;
            }
        }
    }
};
static bool Resolve(void* ctx,const void* edict,TouchEntity** out) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(edict==&f.ea){f.calls+='A';*out=f.missing_first?nullptr:&f.first;}
    else if(edict==&f.eb){f.calls+='B';*out=f.missing_second?nullptr:&f.second;}
    else {f.valid=false;return false;}
    return !f.fail_resolve;
}
static bool Init(void* ctx,GameEntsTouchTrace* trace) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);f.calls+='I';
    if(f.fail_init)return false;
    trace->WriteFloat(0x2c,1.f);trace->WriteFloat(0x3c,42.f);
    return true;
}
static bool Refresh(TouchEntity& t,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    const bool is_first=t.entity==&f.a;
    f.calls+=is_first?'1':'2';++f.refreshes;
    if((is_first&&f.fail_first_refresh)||(!is_first&&f.fail_second_refresh))
        return false;
    t.origin=is_first?ContactPoint{2.f,4.f,6.f}:ContactPoint{8.f,10.f,12.f};
    if(!f.leave_dirty)t.dirtyOrigin=false;
    return true;
}
static ContactNode* Allocate(void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);f.calls+='N';
    ++f.allocations;
    if(f.fail_second_alloc&&f.allocations==2)return nullptr;
    return new(std::nothrow) ContactNode;
}
static void Free(ContactNode* n,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    f.calls+='F';++f.freed;delete n;
}
static void Start(CBaseEntity* owner,CBaseEntity* partner,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);f.calls+='S';++f.starts;
    if(!owner||!partner||owner==partner)f.valid=false;
}
static void Touch(CBaseEntity* owner,CBaseEntity* partner,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);f.calls+='T';++f.touches;
    if(!owner||!partner||owner==partner)f.valid=false;
}
static GameEntsContactBridgeHooks Hooks(Fixture& f) noexcept {
    return {&f,&Resolve,&Init,&f.state,f.registry.data(),f.registry.size(),
            {&Refresh,&Allocate,&Free,&Start,&Touch,&f}};
}
static GameEntsTouchEntryStatus Run(Fixture& f,GameEntsContactBridgeResult& out) {
    f.calls.clear();
    return MarkEntitiesAsTouchingWithContactGraph(&f.ea,&f.eb,Hooks(f),out);
}
static int Fail(int i,const char* why) {
    std::fprintf(stderr,"FAIL GameEnts touch/contact composition %d: %s\n",i,why);
    return i;
}
int main() {
    GameEntsContactBridgeResult out{};
    Fixture f;
    if(Run(f,out)!=GameEntsTouchEntryStatus::Completed ||
       !f.valid||f.calls!="ABI21NNSS"||
       out.entry.resolved!=2||out.entry.trace_initialized!=1||
       out.entry.second_refreshes!=1||out.entry.first_refreshes!=1||
       out.resolved_calls!=2||out.refresh_calls!=2||out.contact_calls!=1||
       out.contact_status!=TouchMarkResult::Recorded||
       out.entry.trace.ReadFloat(0x0c)!=5.f||
       out.entry.trace.ReadFloat(0x10)!=7.f||
       out.entry.trace.ReadFloat(0x14)!=9.f||
       out.entry.trace.ReadFloat(0x3c)!=42.f||
       out.contact_trace.end.x!=5.f||out.contact_trace.end.y!=7.f||
       out.contact_trace.end.z!=9.f||out.contact_trace.fraction!=1.f||
       f.state.touchAllocated!=2||f.starts!=2||f.allocations!=2||
       f.ra.next->handle!=f.second.handle||f.rb.next->handle!=f.first.handle||
       f.ra.next->stamp!=7||f.rb.next->stamp!=9||
       !ValidateReciprocalChain(&f.ra,2)||!ValidateReciprocalChain(&f.rb,2))
        return Fail(1,"original edict trace/dirty refresh -> directed reciprocal links");
    f.first.touchStamp=11;f.second.touchStamp=13;
    if(Run(f,out)!=GameEntsTouchEntryStatus::Completed ||
       f.calls!="ABITT"||out.refresh_calls!=0||
       out.contact_status!=TouchMarkResult::Recorded||
       f.allocations!=2||f.touches!=2||f.starts!=2||
       f.ra.next->stamp!=11||f.rb.next->stamp!=13||
       f.state.touchAllocated!=2)
        return Fail(2,"repeated pair reuses links with no second origin refresh");
    Fixture same;
    same.calls.clear();
    if(MarkEntitiesAsTouchingWithContactGraph(&same.ea,&same.ea,Hooks(same),out)
       !=GameEntsTouchEntryStatus::Completed ||
       same.calls!="AAI1"&&false)
        return Fail(3,"same entity should be ignored");
    // Same edict resolves twice to the same TouchEntity, and its dirty
    // origin is refreshed exactly once during the second argument path.
    if(out.contact_status!=TouchMarkResult::Ignored||out.contact_calls!=1||
       same.state.touchAllocated||same.allocations||!same.valid)
        return Fail(4,"self-touch is ignored before reciprocal allocations");
    Fixture absent;absent.missing_second=true;
    if(Run(absent,out)!=GameEntsTouchEntryStatus::NoEntities||
       absent.calls!="AB"||out.contact_calls||out.entry.trace_initialized||
       absent.allocations)
        return Fail(5,"absent edict entity stops before trace/pair work");
    Fixture init;init.fail_init=true;
    if(Run(init,out)!=GameEntsTouchEntryStatus::HostFailure||
       init.calls!="ABI"||out.contact_calls||init.allocations)
        return Fail(6,"trace initialization failure does not register contacts");
    Fixture fail2;fail2.fail_second_refresh=true;
    if(Run(fail2,out)!=GameEntsTouchEntryStatus::HostFailure||
       fail2.calls!="ABI2"||out.refresh_calls!=1||
       out.entry.first_refreshes||out.contact_calls||fail2.allocations)
        return Fail(7,"second dirty refresh failure prevents first refresh");
    Fixture fail1;fail1.fail_first_refresh=true;
    if(Run(fail1,out)!=GameEntsTouchEntryStatus::HostFailure||
       fail1.calls!="ABI21"||out.refresh_calls!=2||
       out.contact_calls||fail1.allocations)
        return Fail(8,"first refresh failure prevents contact call");
    Fixture dirty;dirty.leave_dirty=true;
    if(Run(dirty,out)!=GameEntsTouchEntryStatus::HostFailure||
       dirty.calls!="ABI2"||out.refresh_calls!=1||
       out.contact_calls||dirty.allocations)
        return Fail(9,"owner must clear dirty state to prevent double-refresh");
    Fixture parent;parent.first.parent=parent.second.handle;
    if(Run(parent,out)!=GameEntsTouchEntryStatus::Completed||
       parent.calls!="ABI21"||out.contact_status!=TouchMarkResult::Ignored||
       parent.allocations||parent.state.touchAllocated)
        return Fail(10,"parent touch ignores pair without error or mutation");
    Fixture failed_nodes;failed_nodes.fail_second_alloc=true;
    if(Run(failed_nodes,out)!=GameEntsTouchEntryStatus::HostFailure||
       failed_nodes.calls!="ABI21NNF"||
       out.contact_status!=TouchMarkResult::AllocationFailed||
       failed_nodes.state.touchAllocated||failed_nodes.freed!=1||
       failed_nodes.ra.next!=&failed_nodes.ra||
       failed_nodes.rb.next!=&failed_nodes.rb)
        return Fail(11,"second node failure rolls back first unlinked node");
    Fixture invalid;invalid.registry[2].serial=3;
    if(Run(invalid,out)!=GameEntsTouchEntryStatus::HostFailure||
       invalid.calls!="ABI21"||out.contact_status!=TouchMarkResult::InvalidInput||
       invalid.allocations||invalid.state.touchAllocated)
        return Fail(12,"stale generation prevents contact list mutation");
    Fixture resolve;resolve.fail_resolve=true;
    if(Run(resolve,out)!=GameEntsTouchEntryStatus::HostFailure||
       resolve.calls!="A"||out.contact_calls)
        return Fail(13,"host edict resolution failure");
    Fixture missing;auto h=Hooks(missing);h.touch.refreshOrigin=nullptr;
    if(MarkEntitiesAsTouchingWithContactGraph(&missing.ea,&missing.eb,h,out)
       !=GameEntsTouchEntryStatus::InvalidInput||
       !missing.calls.empty())
        return Fail(14,"missing owned origin callback fails closed");
    std::puts("PASS GameEnts original touch entry + reciprocal contact graph: 0x54 trace, refresh ordering, allocation, repeat, refusal");
    return 0;
}
