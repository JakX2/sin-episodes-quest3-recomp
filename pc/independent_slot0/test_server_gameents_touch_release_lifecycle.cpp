// Original ServerGameEnts001 +0x08 -> +0x0c lifecycle composition.
// Connect existing touch-entry, reciprocal graph, edict-detach and delete.
// This is a synthetic PE32 test, not a replacement retail game interface.
#include "server_gameents_touch_contact_bridge.h"
#include "server_entity_release_core.h"
#include "server_entity_immediate_removal_core.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <new>
#include <string>

class CBaseHandle { public: unsigned value=0; };
class CBaseEntity { public: unsigned id=0; };
struct edict_t {
    std::uint32_t state=0x65, serial=0;
    IServerNetworkable* networkable=nullptr;
    sin_independent::ServerUnknownAbi* unknown=nullptr;
};
static_assert(offsetof(edict_t,unknown)==0x0c,"original x86 edict unknown");
using namespace sin_independent;
class Unknown final : public ServerUnknownAbi {
public:
    explicit Unknown(CBaseEntity* e):entity(e){}
    void __thiscall SetRefEHandle(const CBaseHandle& h) noexcept override {handle=h;}
    const CBaseHandle& __thiscall GetRefEHandle() const noexcept override {return handle;}
    ICollideable* __thiscall GetCollideable() noexcept override {return nullptr;}
    IServerNetworkable* __thiscall GetNetworkable() noexcept override {return nullptr;}
    CBaseEntity* __thiscall GetBaseEntity() noexcept override {return entity;}
    CBaseEntity* entity=nullptr;
    CBaseHandle handle{};
};
struct Fixture {
    CBaseEntity* a=new CBaseEntity{1};
    CBaseEntity b{2};
    Unknown ua{a},ub{&b};
    edict_t ea{0x65,1,nullptr,&ua},eb{0x66,2,nullptr,&ub};
    ServerGameEntsConversionCore conversion{};
    std::array<EntityHandleEntry,3> registry{};
    ContactNode ra{},rb{};
    TouchEntity ta{},tb{};
    ContactCleanupState contacts{};
    ContactList ground{};
    EntityRemovalRecord record{};
    EntityRemovalEnvironment removal{};
    std::string calls{};
    int stage=0;
    unsigned allocated=0,freed=0,starts=0,touches=0,ended=0,
             roots=0,updated=0,deleted=0;
    bool valid=true;
    Fixture() {
        registry[1].entity=a;registry[1].serial=1;
        registry[2].entity=&b;registry[2].serial=2;
        ra.next=ra.previous=&ra;rb.next=rb.previous=&rb;
        ta.entity=a;ta.handle=0x1001;ta.origin={0,0,0};
        ta.dirtyOrigin=true;ta.touchStamp=7;ta.contacts={&ra,true};
        tb.entity=&b;tb.handle=0x2002;tb.origin={0,0,0};
        tb.dirtyOrigin=true;tb.touchStamp=9;tb.contacts={&rb,true};
    }
    Fixture(const Fixture&)=delete;
    ~Fixture(){
        for(auto* root:{&ra,&rb}) {
            auto* p=root->next;unsigned n=0;
            while(p&&p!=root&&n++<8){auto* next=p->next;delete p;p=next;}
        }
        if(a)delete a;
    }
};
static bool Resolve(void* ctx,const void* edict,TouchEntity** out) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);
    if(edict!=&w.ea&&edict!=&w.eb)return false;
    w.calls+=edict==&w.ea?'A':'B';
    auto* entity=w.conversion.EdictToBaseEntity(
        const_cast<edict_t*>(static_cast<const edict_t*>(edict)));
    if(!entity){*out=nullptr;return true;}
    *out=entity==w.a?&w.ta:entity==&w.b?&w.tb:nullptr;
    return *out!=nullptr;
}
static bool Init(void* ctx,GameEntsTouchTrace* trace) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='I';
    if(w.stage!=0)w.valid=false;
    trace->WriteFloat(0x2c,1.f);trace->WriteFloat(0x3c,123.f);
    return true;
}
static bool Refresh(TouchEntity& ent,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);
    if(ent.entity==w.a){w.calls+='1';ent.origin={2,4,6};}
    else if(ent.entity==&w.b){w.calls+='2';ent.origin={8,10,12};}
    else return false;
    ent.dirtyOrigin=false;return true;
}
static bool Publish(void* ctx,const GameEntsTouchTrace* trace) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='P';
    // The original global trace publisher runs on BOTH first contact and
    // repeated contact; existing reciprocal nodes must be allowed here.
    if(w.stage!=0||
       trace->ReadFloat(0x0c)!=5.f||trace->ReadFloat(0x10)!=7.f||
       trace->ReadFloat(0x14)!=9.f||trace->ReadFloat(0x2c)!=1.f)
        w.valid=false;
    return true;
}
static ContactNode* Allocate(void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='N';++w.allocated;
    return new(std::nothrow) ContactNode;
}
static void FreeUnlinked(ContactNode* n,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);++w.freed;delete n;
}
static void Start(CBaseEntity* a,CBaseEntity* b,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='S';++w.starts;
    if(!a||!b||a==b||w.stage!=0)w.valid=false;
}
static void Touch(CBaseEntity* a,CBaseEntity* b,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='T';++w.touches;
    if(!a||!b||a==b||w.stage!=0)w.valid=false;
}
static GameEntsContactBridgeHooks ContactHooks(Fixture& w) noexcept {
    return {&w,&Resolve,&Init,&Publish,&w.contacts,
            w.registry.data(),w.registry.size(),
            {&Refresh,&Allocate,&FreeUnlinked,&Start,&Touch,&w}};
}
static void End(CBaseEntity* receiver,CBaseEntity* departing,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='E';++w.ended;
    if(receiver!=&w.b||departing!=w.a||w.stage!=1||
       w.contacts.cleanupDataObjects||w.ea.unknown||w.ea.state)
        w.valid=false;
}
static void FreeNode(ContactNode* n,ContactKind kind,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='F';++w.freed;
    if(kind!=ContactKind::Touch||w.stage!=1)w.valid=false;
    delete n;
}
static void Root(CBaseEntity* owner,ContactNode* root,
                 ContactKind kind,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='D';++w.roots;
    if(owner!=w.a||root!=&w.ra||kind!=ContactKind::Touch||
       w.stage!=1||!w.contacts.cleanupDataObjects)w.valid=false;
    root->next=root->previous=root;
}
static void Notify(CBaseEntity* owner,CBaseEntity* partner,
                   ContactKind kind,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);
    if(owner!=w.a||partner!=&w.b||kind!=ContactKind::Touch){
        w.valid=false;return;
    }
    EntityHandleView view(w.registry.data(),w.registry.size());
    ReciprocalHooks hooks{&End,nullptr,&FreeNode,&Root,&w};
    if(RemoveReciprocalContact(owner,partner,kind,w.tb.contacts,
        w.contacts,view,hooks)!=ReciprocalResult::Removed)w.valid=false;
}
static void __cdecl Touched(CBaseEntity* entity,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='L';
    if(entity!=w.a||w.stage!=0||w.ea.unknown||w.ea.state)w.valid=false;
    w.stage=1;
    EntityHandleView view(w.registry.data(),w.registry.size());
    ContactCleanupHooks hooks{&Notify,&FreeNode,&Root,&w};
    if(RemoveContactList(entity,ContactKind::Touch,w.ta.contacts,
        w.contacts,w.ta.touchStamp,view,hooks)!=ContactCleanupResult::Removed)
        w.valid=false;
}
static void __cdecl Ground(CBaseEntity* entity,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='G';
    if(entity!=w.a||w.stage!=1||w.contacts.touchAllocated||
       w.ta.contacts.present||w.contacts.groundAllocated)w.valid=false;
    w.stage=2;
    EntityHandleView view(w.registry.data(),w.registry.size());
    ContactCleanupHooks hooks{&Notify,&FreeNode,&Root,&w};
    int stamp=0;
    if(RemoveContactList(entity,ContactKind::Ground,w.ground,w.contacts,
        stamp,view,hooks)!=ContactCleanupResult::NoList)w.valid=false;
}
static void Update(CBaseEntity* entity,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='U';++w.updated;
    if(entity!=w.a||w.stage!=3||w.contacts.touchAllocated||
       w.ended!=1||w.freed!=2||!(w.record.extendedFlags&kEntityKillMe)||
       w.removal.handleAccessDisabled)w.valid=false;
}
static void Destroy(CBaseEntity* entity,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='X';++w.deleted;
    EntityHandleView view(w.registry.data(),w.registry.size());
    if(entity!=w.a||w.stage!=3||w.updated!=1||
       !w.removal.handleAccessDisabled||
       ResolveLiveEntity(view,w.ta.handle,w.removal))
        w.valid=false;
    w.registry[1].entity=nullptr;w.registry[1].serial=2;
    w.ua.entity=nullptr;delete entity;w.a=nullptr;w.stage=4;
}
static void __cdecl Immediate(CBaseEntity* entity,void* ctx) noexcept {
    auto& w=*static_cast<Fixture*>(ctx);w.calls+='I';
    if(entity!=w.a||w.stage!=2)w.valid=false;
    w.stage=3;
    EntityRemovalHooks hooks{&Update,&Destroy,nullptr,&w};
    if(TryRemoveImmediate(entity,w.record,w.removal,hooks)
        !=ImmediateRemovalResult::Destroyed)w.valid=false;
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL GameEnts touch->free %d: %s\n",n,why);return n;
}
int main() {
    Fixture w;
    GameEntsContactBridgeResult result{};
    if(MarkEntitiesAsTouchingWithContactGraph(&w.ea,&w.eb,ContactHooks(w),result)
        !=GameEntsTouchEntryStatus::Completed ||
       !w.valid||w.calls!="ABI21PNNSS"||w.allocated!=2||
       w.contacts.touchAllocated!=2||w.starts!=2||
       w.ra.next->handle!=w.tb.handle||w.rb.next->handle!=w.ta.handle)
        return Fail(1,"original edicts create reciprocal contact links");
    w.calls.clear();w.ta.touchStamp=23;w.tb.touchStamp=29;
    if(MarkEntitiesAsTouchingWithContactGraph(&w.ea,&w.eb,ContactHooks(w),result)
        !=GameEntsTouchEntryStatus::Completed||!w.valid||
       w.calls!="ABIPTT"||w.allocated!=2||w.contacts.touchAllocated!=2||
       w.touches!=2||w.ra.next->stamp!=23||w.rb.next->stamp!=29)
        return Fail(2,"repeated contact reuses link nodes");
    w.calls.clear();
    EntityReleaseHooks missing{&Touched,nullptr,&Immediate,&w};
    if(TryReleaseContainingEntity(w.conversion,&w.ea,missing)
        !=EntityReleaseStatus::MissingCleanupHooks||
       !w.ea.unknown||w.ea.state!=0x65||w.contacts.touchAllocated!=2||
       !w.calls.empty())return Fail(3,"incomplete release hooks do not detach");
    EntityReleaseHooks full{&Touched,&Ground,&Immediate,&w};
    w.calls.clear();
    if(TryReleaseContainingEntity(w.conversion,&w.ea,full)
        !=EntityReleaseStatus::DetachedAndHooksInvoked||
       !w.valid||w.calls!="LEFFDGIUX"||w.stage!=4||
       w.a||w.ea.unknown||w.ea.state||w.ea.serial!=1||
       w.contacts.touchAllocated||w.contacts.groundAllocated||
       !w.contacts.cleanupDataObjects||w.ta.contacts.present||
       w.ta.contacts.root||w.ta.touchStamp||
       !w.tb.contacts.present||w.tb.contacts.root!=&w.rb||
       w.rb.next!=&w.rb||w.rb.previous!=&w.rb||
       w.ended!=1||w.freed!=2||w.roots!=1||
       w.updated!=1||w.deleted!=1||
       w.registry[1].entity||w.registry[1].serial!=2||
       w.removal.handleAccessDisabled)
        return Fail(4,"edict detach -> touch -> ground -> immediate delete");
    w.calls.clear();
    if(TryReleaseContainingEntity(w.conversion,&w.ea,full)
       !=EntityReleaseStatus::NoAssociatedEntity||
       !w.calls.empty()||w.deleted!=1)
        return Fail(5,"repeated release is idempotent");
    if(MarkEntitiesAsTouchingWithContactGraph(&w.ea,&w.eb,ContactHooks(w),result)
       !=GameEntsTouchEntryStatus::NoEntities||
       result.entry.trace_initialized||result.publish_calls||
       w.contacts.touchAllocated||w.deleted!=1)
        return Fail(6,"retired edict cannot recreate deleted actor contacts");
    if(w.conversion.EdictToBaseEntity(&w.ea))
        return Fail(7,"retired edict conversion does not resolve freed entity");
    std::puts("PASS GameEnts lifecycle: original edict touch -> repeat -> detach -> reciprocal touch cleanup -> ground stage -> immediate delete -> no-resurrection");
    return 0;
}
