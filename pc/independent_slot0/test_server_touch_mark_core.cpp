#include "server_touch_mark_core.h"
#include <array>
#include <cstdio>
#include <new>
#include <initializer_list>

class CBaseEntity { public: unsigned id; };
using namespace sin_independent;
struct Fixture {
    CBaseEntity a{1},b{2};
    std::array<EntityHandleEntry,3> entries{};
    ContactNode ra{},rb{};
    TouchEntity first{},second{};
    ContactCleanupState state{};
    TouchTraceState trace{};
    unsigned refresh=0,start=0,touch=0,freed=0,allocations=0;
    unsigned ended=0,destroyed=0;
    std::array<unsigned,2> refreshOrder{};
    bool failSecond=false,valid=true;
    Fixture() {
        entries[1].entity=&a; entries[1].serial=1;
        entries[2].entity=&b; entries[2].serial=2;
        ra.next=ra.previous=&ra; rb.next=rb.previous=&rb;
        first.entity=&a; first.handle=0x1001; first.touchStamp=7;
        second.entity=&b; second.handle=0x2002; second.touchStamp=9;
        first.contacts={&ra,true}; second.contacts={&rb,true};
        first.dirtyOrigin=second.dirtyOrigin=true;
    }
    ~Fixture() {
        for(auto* root:{&ra,&rb}) {
            auto* n=root->next;
            while(n!=root) { auto* next=n->next; delete n; n=next; }
        }
    }
    EntityHandleView View() const {return EntityHandleView(entries.data(),entries.size());}
};
static bool Refresh(TouchEntity& e,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(f.refresh>=2) return false;
    f.refreshOrder[f.refresh++]=e.entity->id;
    e.origin=e.entity->id==1?ContactPoint{2,4,6}:ContactPoint{8,10,12};
    e.dirtyOrigin=false;
    return true;
}
static ContactNode* Allocate(void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    ++f.allocations;
    return f.failSecond && f.allocations==2?nullptr:new(std::nothrow) ContactNode;
}
static void Free(ContactNode* n,void* ctx) noexcept {
    ++static_cast<Fixture*>(ctx)->freed; delete n;
}
static void Start(CBaseEntity* e,CBaseEntity* other,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(!e || !other || e==other || f.trace.end.x!=5 || f.trace.fraction!=1)
        f.valid=false;
    ++f.start;
}
static void Touch(CBaseEntity* e,CBaseEntity* other,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(!e || !other || e==other) f.valid=false;
    ++f.touch;
}
static TouchMarkResult Run(Fixture& f) {
    TouchMarkHooks h{&Refresh,&Allocate,&Free,&Start,&Touch,&f};
    return MarkTouchPair(f.first,f.second,f.state,f.View(),f.trace,h);
}
static void End(CBaseEntity* owner,CBaseEntity* other,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(owner!=&f.b || other!=&f.a || f.state.cleanupDataObjects) f.valid=false;
    ++f.ended;
}
static void CleanupNode(ContactNode* n,ContactKind kind,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(kind!=ContactKind::Touch)f.valid=false;
    ++f.freed; delete n;
}
static void DestroyRoot(CBaseEntity* owner,ContactNode* root,ContactKind kind,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(owner!=&f.a || root!=&f.ra || kind!=ContactKind::Touch ||
       !f.state.cleanupDataObjects) f.valid=false;
    root->next=root->previous=root;
    ++f.destroyed;
}
static void Notify(CBaseEntity* owner,CBaseEntity* partner,ContactKind kind,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(owner!=&f.a || partner!=&f.b || kind!=ContactKind::Touch) {f.valid=false;return;}
    ReciprocalHooks hooks{&End,nullptr,&CleanupNode,&DestroyRoot,&f};
    if(RemoveReciprocalContact(owner,partner,kind,f.second.contacts,f.state,f.View(),hooks)
       !=ReciprocalResult::Removed)f.valid=false;
}

static int fail(int code,const char* msg) {
    std::fprintf(stderr,"FAIL touch marking %d: %s\n",code,msg);
    return code;
}
int main() {
    Fixture f;
    if(Run(f)!=TouchMarkResult::Recorded || !f.valid || f.refresh!=2 ||
       f.refreshOrder[0]!=2 || f.refreshOrder[1]!=1 ||
       f.trace.end.x!=5 || f.trace.end.y!=7 || f.trace.end.z!=9 ||
       f.allocations!=2 || f.state.touchAllocated!=2 || f.start!=2 || f.touch ||
       f.ra.next->handle!=f.second.handle || f.rb.next->handle!=f.first.handle ||
       f.ra.next->stamp!=7 || f.rb.next->stamp!=9 ||
       f.ra.next->flags!=kTouchStarted || f.rb.next->flags!=kTouchStarted ||
       !ValidateReciprocalChain(&f.ra,2) || !ValidateReciprocalChain(&f.rb,2))
        return fail(1,"new reciprocal touch links and trace midpoint");
    f.first.touchStamp=11; f.second.touchStamp=13;
    if(Run(f)!=TouchMarkResult::Recorded || f.allocations!=2 || f.start!=2 ||
       f.touch!=2 || f.ra.next->stamp!=11 || f.rb.next->stamp!=13 ||
       f.state.touchAllocated!=2)
        return fail(2,"refresh existing links without allocating duplicates");
    // Exercise actual creation -> repeated contact -> bulk unlink and
    // reciprocal removal -> recreate. No manually preseeded contact nodes.
    ContactCleanupHooks cleanup{&Notify,&CleanupNode,&DestroyRoot,&f};
    if(RemoveContactList(&f.a,ContactKind::Touch,f.first.contacts,f.state,
                         f.first.touchStamp,f.View(),cleanup)!=ContactCleanupResult::Removed ||
       !f.valid || f.freed!=2 || f.ended!=1 || f.destroyed!=1 ||
       f.state.touchAllocated || f.first.contacts.present || f.first.contacts.root ||
       !f.second.contacts.present || f.rb.next!=&f.rb || f.rb.previous!=&f.rb ||
       f.first.touchStamp || !f.state.cleanupDataObjects)
        return fail(10,"created contacts clean up in both directions");
    f.first.contacts={&f.ra,true}; f.first.touchStamp=15;
    if(Run(f)!=TouchMarkResult::Recorded || !f.valid || f.allocations!=4 ||
       f.state.touchAllocated!=2 || f.start!=4 || f.ra.next->stamp!=15)
        return fail(11,"contact recreation after cleanup");
    Fixture blocked;
    blocked.first.parent=blocked.second.handle;
    if(Run(blocked)!=TouchMarkResult::Ignored || blocked.allocations ||
       blocked.start || blocked.state.touchAllocated)
        return fail(3,"parent hierarchy refusal");
    blocked.first.parent=kInvalidEntityHandle;
    blocked.second.dontTouch=true;
    if(Run(blocked)!=TouchMarkResult::Ignored || blocked.allocations)
        return fail(4,"no-touch flag");
    blocked.second.dontTouch=false;
    blocked.first.trigger=blocked.second.trigger=true;
    blocked.first.solid=blocked.second.solid=false;
    if(Run(blocked)!=TouchMarkResult::Ignored || blocked.allocations)
        return fail(5,"two nonsolid triggers");
    blocked.first.trigger=blocked.second.trigger=false;
    blocked.second.markedForDeletion=true;
    if(Run(blocked)!=TouchMarkResult::Ignored || blocked.allocations)
        return fail(6,"pending deletion");
    Fixture alloc;
    alloc.failSecond=true;
    if(Run(alloc)!=TouchMarkResult::AllocationFailed || alloc.freed!=1 ||
       alloc.state.touchAllocated || alloc.ra.next!=&alloc.ra ||
       alloc.rb.next!=&alloc.rb || alloc.start)
        return fail(7,"independent transactional allocation refusal");
    Fixture trigger;
    trigger.first.trigger=true;
    trigger.first.solid=false;
    if(Run(trigger)!=TouchMarkResult::Recorded || trigger.start!=1 ||
       trigger.ra.next->flags!=kTouchStarted || trigger.rb.next->flags ||
       trigger.state.touchAllocated!=2)
        return fail(8,"trigger receives touch but other direction does not start");
    Fixture stale;
    stale.entries[2].serial=3;
    if(Run(stale)!=TouchMarkResult::InvalidInput || stale.allocations)
        return fail(9,"invalid entity generation");
    std::puts("PASS independent touch marking: midpoint, eligibility, reciprocal allocation, repeat refresh, trigger flags");
    return 0;
}
