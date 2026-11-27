// Native MSVC x86, fully wired (but unregistered) ServerGameEnts001 vtable.
// Each typed virtual slot is called through the same complete concrete object.
// Source pointers/handles and networking are deliberately synthetic.
#include "server_gameents_bounded_dispatch.h"
#include "server_entity_immediate_removal_core.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <new>
#include <string>

class CBaseHandle { public: unsigned value=0; };
struct edict_t {
    std::uint32_t flags=0x65, serial=0;
    IServerNetworkable* networkable=nullptr;
    sin_independent::ServerUnknownAbi* unknown=nullptr;
};
class CBaseEntity {
public:
    unsigned char reserved[sin_independent::kOriginalEntityEdictOffset]{};
    edict_t* edict=nullptr;
    unsigned id=0;
};
struct CCheckTransmitInfo {unsigned char bytes[0x2010]{};};
static_assert(offsetof(edict_t,unknown)==0x0c,"retail edict unknown offset");
static_assert(offsetof(CBaseEntity,edict)==0x16c,"retail entity edict offset");
static_assert(sizeof(void*)==4,"original PE32 only");
using namespace sin_independent;

static void Write32(unsigned char* ptr,std::uint32_t val) noexcept {
    for(unsigned i=0;i<4;++i)ptr[i]=static_cast<unsigned char>(val>>(i*8));
}
class Unknown final : public ServerUnknownAbi {
public:
    explicit Unknown(CBaseEntity* entity):entity(entity){}
    void __thiscall SetRefEHandle(const CBaseHandle& h) noexcept override {handle=h;}
    const CBaseHandle& __thiscall GetRefEHandle() const noexcept override {return handle;}
    ICollideable* __thiscall GetCollideable() noexcept override {return nullptr;}
    IServerNetworkable* __thiscall GetNetworkable() noexcept override {return nullptr;}
    CBaseEntity* __thiscall GetBaseEntity() noexcept override {
        ++calls;return entity;
    }
    CBaseEntity* entity=nullptr;
    CBaseHandle handle{};
    unsigned calls=0;
};
struct Fixture {
    CBaseEntity first{},second{};
    Unknown first_unknown{&first},second_unknown{&second};
    edict_t ea{},eb{};
    std::array<EntityHandleEntry,3> registry{};
    ContactNode ra{},rb{};
    TouchEntity ta{},tb{};
    ContactCleanupState contacts{};
    ContactList ground{};
    EntityRemovalRecord removal_record{};
    EntityRemovalEnvironment removal_env{};
    CCheckTransmitInfo info{};
    unsigned short indices[2]{1,2};
    unsigned char table[3*20]{};
    std::uint32_t selected[1]{};
    std::string calls{};
    ServerGameEntsBoundedDispatch* bridge=nullptr;
    unsigned starts=0,touches=0,ends=0,allocated=0,freed=0,
             roots=0,updated=0,deleted=0,should=0,set=0,ent_reads=0;
    std::size_t index_extent=2;
    bool first_alive=true,valid=true,fail_publish=false,reenter=false,
         reentrancy_refused=false;
    Fixture() {
        ea.serial=1;eb.serial=2;
        ea.unknown=&first_unknown;eb.unknown=&second_unknown;
        first.edict=&ea;first.id=1;second.edict=&eb;second.id=2;
        ra.next=ra.previous=&ra;rb.next=rb.previous=&rb;
        registry[1].entity=&first;registry[1].serial=1;
        registry[2].entity=&second;registry[2].serial=2;
        ta.entity=&first;ta.handle=0x1001;ta.origin={0,0,0};
        ta.dirtyOrigin=true;ta.touchStamp=7;ta.contacts={&ra,true};
        tb.entity=&second;tb.handle=0x2002;tb.origin={0,0,0};
        tb.dirtyOrigin=true;tb.touchStamp=9;tb.contacts={&rb,true};
        Write32(info.bytes,91);
        Write32(info.bytes+0x2008,201);
        Write32(table+20*1,kTransmitAlways);
        Write32(table+20*2,kTransmitFullCheck);
        Write32(table+20*2+0x0c,1002);
    }
    Fixture(const Fixture&)=delete;
    ~Fixture(){
        for(auto* root:{&ra,&rb}) {
            auto* p=root->next;unsigned visited=0;
            while(p&&p!=root&&visited++<8) {auto* next=p->next;delete p;p=next;}
        }
    }
};
static bool ValidEdict(void* p,const edict_t* edict) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    return edict==&f.ea||edict==&f.eb;
}
static bool ValidEntity(void* p,const CBaseEntity* entity) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    return (entity==&f.first&&f.first_alive)||entity==&f.second;
}
static bool Extent(void* p,const CCheckTransmitInfo* info,
                   const unsigned short* indices,int,
                   std::size_t* out_info,std::size_t* out_indices) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(info!=&f.info||indices!=f.indices||!out_info||!out_indices)
        return false;
    *out_info=sizeof(f.info);*out_indices=f.index_extent;
    return true;
}
static bool ResolveTouch(void* p,const void* edict,TouchEntity** out) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    f.calls+=edict==&f.ea?'A':'B';
    if(edict!=&f.ea&&edict!=&f.eb)return false;
    auto* e=ServerGameEntsConversionCore{}.EdictToBaseEntity(
        const_cast<edict_t*>(static_cast<const edict_t*>(edict)));
    if(!e){*out=nullptr;return true;}
    *out=e==&f.first&&f.first_alive?&f.ta:
         e==&f.second?&f.tb:nullptr;
    return *out!=nullptr;
}
static bool Init(void* p,GameEntsTouchTrace* trace) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='I';
    trace->WriteFloat(0x2c,1.f);
    trace->WriteFloat(0x3c,123.f);
    return true;
}
static bool Refresh(TouchEntity& ent,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(ent.entity==&f.first){f.calls+='1';ent.origin={2,4,6};}
    else if(ent.entity==&f.second){f.calls+='2';ent.origin={8,10,12};}
    else return false;
    ent.dirtyOrigin=false;
    return true;
}
static bool Publish(void* p,const GameEntsTouchTrace* trace) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='P';
    if(trace->ReadFloat(0x0c)!=5.f||trace->ReadFloat(0x10)!=7.f||
       trace->ReadFloat(0x14)!=9.f||trace->ReadFloat(0x2c)!=1.f||
       trace->ReadFloat(0x3c)!=123.f)f.valid=false;
    return !f.fail_publish;
}
static ContactNode* Allocate(void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='N';++f.allocated;
    return new(std::nothrow) ContactNode;
}
static void FreeUnlinked(ContactNode* n,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);++f.freed;delete n;
}
static void Start(CBaseEntity* a,CBaseEntity* b,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='S';++f.starts;
    if(!a||!b||a==b)f.valid=false;
}
static void Touch(CBaseEntity* a,CBaseEntity* b,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='T';++f.touches;
    if(!a||!b||a==b)f.valid=false;
}
static void End(CBaseEntity* receiver,CBaseEntity* departing,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='E';++f.ends;
    if(receiver!=&f.second||departing!=&f.first||
       f.contacts.cleanupDataObjects||f.ea.unknown||f.ea.flags)f.valid=false;
}
static void Free(ContactNode* node,ContactKind kind,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='F';++f.freed;
    if(kind!=ContactKind::Touch)f.valid=false;
    delete node;
}
static void DestroyRoot(CBaseEntity* owner,ContactNode* root,
                        ContactKind kind,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='D';++f.roots;
    if(owner!=&f.first||root!=&f.ra||kind!=ContactKind::Touch)
        f.valid=false;
    root->next=root->previous=root;
}
static void Notify(CBaseEntity* owner,CBaseEntity* partner,
                   ContactKind kind,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(owner!=&f.first||partner!=&f.second||kind!=ContactKind::Touch){
        f.valid=false;return;
    }
    EntityHandleView handles(f.registry.data(),f.registry.size());
    ReciprocalHooks hooks{&End,nullptr,&Free,&DestroyRoot,&f};
    if(RemoveReciprocalContact(owner,partner,kind,f.tb.contacts,
        f.contacts,handles,hooks)!=ReciprocalResult::Removed)f.valid=false;
}
static void __cdecl Touched(CBaseEntity* entity,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='L';
    if(entity!=&f.first||f.ea.unknown||f.ea.flags)f.valid=false;
    EntityHandleView handles(f.registry.data(),f.registry.size());
    ContactCleanupHooks hooks{&Notify,&Free,&DestroyRoot,&f};
    if(RemoveContactList(entity,ContactKind::Touch,f.ta.contacts,
        f.contacts,f.ta.touchStamp,handles,hooks)!=ContactCleanupResult::Removed)
        f.valid=false;
}
static void __cdecl Ground(CBaseEntity* entity,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='G';
    if(entity!=&f.first||f.contacts.touchAllocated||
       f.ta.contacts.present||f.contacts.groundAllocated)f.valid=false;
}
static void Update(CBaseEntity* entity,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='U';++f.updated;
    if(entity!=&f.first||f.contacts.touchAllocated||
       !(f.removal_record.extendedFlags&kEntityKillMe))f.valid=false;
}
static void Destroy(CBaseEntity* entity,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='X';++f.deleted;
    if(entity!=&f.first||!f.removal_env.handleAccessDisabled)f.valid=false;
    f.first_alive=false;
    f.registry[1].entity=nullptr;f.registry[1].serial=2;
    f.first_unknown.entity=nullptr;
}
static void __cdecl Immediate(CBaseEntity* entity,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='M';
    EntityRemovalHooks h{&Update,&Destroy,nullptr,&f};
    if(TryRemoveImmediate(entity,f.removal_record,f.removal_env,h)
       !=ImmediateRemovalResult::Destroyed)f.valid=false;
}
static bool Table(void* p,int arg,GameEntsTransmitTable* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='Q';
    if(arg!=0)f.valid=false;
    *out={f.table,sizeof(f.table),91};return true;
}
static bool Recipient(void* p,std::uint32_t token,TransmitRecipient* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='R';
    if(token!=91)f.valid=false;
    *out={true,false,17};return true;
}
static bool Bits(void* p,std::uint32_t token,TransmitBits* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='B';
    if(token!=201) return false;
    *out={f.selected,1};return true;
}
static bool TransmitEntity(void* p,std::uint32_t token,
                           CBaseEntity** out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='E';++f.ent_reads;
    if(token!=1002)return false;
    *out=&f.second;return true;
}
static std::uint32_t Should(CBaseEntity* e,TransmitContext&,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='W';++f.should;
    if(e!=&f.second)f.valid=false;
    if(f.reenter&&f.bridge) {
        auto* interface_ptr=static_cast<ServerGameEntsAbi*>(f.bridge);
        f.reentrancy_refused=interface_ptr->BaseEntityToEdict(&f.first)==nullptr;
    }
    return kTransmitAlways;
}
static int Area(CBaseEntity*,void*) noexcept {return 17;}
static bool InPvs(CBaseEntity*,TransmitContext&,void*) noexcept {return true;}
static void Set(CBaseEntity* e,std::size_t index,TransmitContext& ctx,
                bool forced,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='S';++f.set;
    if(e!=&f.second||index!=2||!forced)f.valid=false;
    ctx.selected.Set(index);
}
static GameEntsBoundedOwner Hooks(Fixture& f) noexcept {
    GameEntsBoundedOwner h{};
    h.context=&f;
    h.valid_edict=&ValidEdict;h.valid_entity=&ValidEntity;
    h.transmit_extent=&Extent;
    h.touch={&f,&ResolveTouch,&Init,&Publish,&f.contacts,
             f.registry.data(),f.registry.size(),
             {&Refresh,&Allocate,&FreeUnlinked,&Start,&Touch,&f}};
    h.release={&Touched,&Ground,&Immediate,&f};
    h.transmit={&f,&Table,&Recipient,&Bits,&TransmitEntity,
                {&Should,&Area,&InPvs,&Set,&f}};
    return h;
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL GameEnts complete virtual dispatch %d: %s\n",n,why);
    return n;
}
// Real MSVC x86 vtable entry calls must not shift ESP. C++ source-level
// virtual calls can be devirtualized, so exercise actual function pointers.
#define CHECK_X86_STACK(call, code) do { \
    std::uintptr_t before=0,after=0; \
    __asm { mov before, esp } \
    call; \
    __asm { mov after, esp } \
    if(before!=after)return Fail(code,"original x86 vtable stack imbalance"); \
} while(false)
int main() {
    Fixture f;
    // A partially injected bridge must NEVER be passed to any engine.
    GameEntsBoundedOwner missing=Hooks(f);
    missing.release.removeGroundList=nullptr;
    ServerGameEntsBoundedDispatch unbound(missing);
    if(unbound.Ready())return Fail(1,"incomplete host bindings accepted");
    ServerGameEntsAbi* invalid=&unbound;
    invalid->SetDebugEdictBase(&f.ea);
    if(unbound.Report().status!=GameEntsDispatchResult::Unbound||
       unbound.DebugEdictBase()!=nullptr)return Fail(2,"unbound debug setter");
    if(invalid->EdictToBaseEntity(&f.ea)!=nullptr||
       unbound.Report().status!=GameEntsDispatchResult::Unbound)
        return Fail(3,"unbound vtable never reads entity");
    const unsigned before=f.allocated;
    invalid->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(f.allocated!=before||unbound.Report().status!=GameEntsDispatchResult::Unbound)
        return Fail(4,"unbound touch cannot allocate");

    auto* game=new(std::nothrow) ServerGameEntsBoundedDispatch(Hooks(f));
    if(!game||!game->Ready())return Fail(5,"fully bound GameEnts object");
    f.bridge=game;
    ServerGameEntsAbi* v=game;
    void** slots=*reinterpret_cast<void***>(game);
    for(int i=0;i<7;++i)if(!slots[i])return Fail(6,"missing one of seven x86 slots");
    const auto debugSlot=reinterpret_cast<void (__thiscall *)(void*,edict_t*)>(slots[1]);
    const auto touchSlot=reinterpret_cast<void (__thiscall *)(void*,edict_t*,edict_t*)>(slots[2]);
    const auto freeSlot=reinterpret_cast<void (__thiscall *)(void*,edict_t*)>(slots[3]);
    const auto baseSlot=reinterpret_cast<edict_t* (__thiscall *)(void*,CBaseEntity*)>(slots[4]);
    const auto edictSlot=reinterpret_cast<CBaseEntity* (__thiscall *)(void*,edict_t*)>(slots[5]);
    const auto transmitSlot=reinterpret_cast<void (__thiscall *)(
        void*,CCheckTransmitInfo*,const unsigned short*,int)>(slots[6]);
    CHECK_X86_STACK(debugSlot(game,&f.ea),25);
    if(game->DebugEdictBase()!=&f.ea||
       game->Report().status!=GameEntsDispatchResult::Completed||
       game->Report().invocations[1]!=1)
        return Fail(7,"vtable slot 1 debug-edict owner state");
    edict_t* fromBase=nullptr;
    CBaseEntity* fromEdict=nullptr;
    CHECK_X86_STACK(fromBase=baseSlot(game,&f.first),26);
    CHECK_X86_STACK(fromEdict=edictSlot(game,&f.eb),27);
    if(fromBase!=&f.ea||fromEdict!=&f.second||
       game->Report().status!=GameEntsDispatchResult::Completed)
        return Fail(8,"slots 4 and 5 original byte offsets and unknown vtable");

    // Unauthorized tokens are tested without ever dereferencing memory.
    edict_t rogue{};
    CBaseEntity rogue_entity{};
    f.calls.clear();
    if(v->BaseEntityToEdict(&rogue_entity)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized||
       v->EdictToBaseEntity(&rogue)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized)
        return Fail(9,"reject unauthorized raw original pointers");
    v->MarkEntitiesAsTouching(&rogue,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       !f.calls.empty()||f.allocated)
        return Fail(10,"reject unauthorized edict before touch effects");

    f.calls.clear();f.fail_publish=true;
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::HostFailure||
       f.calls!="ABI21P"||f.allocated||f.contacts.touchAllocated)
        return Fail(11,"original trace publish refusal before contact link allocation");
    f.fail_publish=false;f.ta.dirtyOrigin=true;f.tb.dirtyOrigin=true;
    f.ta.origin={0,0,0};f.tb.origin={0,0,0};f.calls.clear();
    CHECK_X86_STACK(touchSlot(game,&f.ea,&f.eb),28);
    if(game->Report().status!=GameEntsDispatchResult::Completed||
       f.calls!="ABI21PNNSS"||!f.valid||f.allocated!=2||
       f.contacts.touchAllocated!=2||f.starts!=2||
       game->Report().touch.entry.midpoint[0]!=5.f)
        return Fail(12,"slot 2 original trace and two reciprocal touch links");
    f.calls.clear();f.ta.touchStamp=11;f.tb.touchStamp=13;
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::Completed||
       f.calls!="ABIPTT"||f.touches!=2||f.allocated!=2)
        return Fail(13,"slot 2 repeat contact reuses original nodes");

    // Original CheckTransmit slot 6 calls the existing source-owned
    // policy using the complete host-bounded table, recipient and bitsets.
    f.calls.clear();
    CHECK_X86_STACK(transmitSlot(game,&f.info,f.indices,2),29);
    if(game->Report().status!=GameEntsDispatchResult::Completed||
       game->Report().transmit_status!=TransmitResult::Completed||
       f.calls!="QRBEWS"||f.selected[0]!=6u||f.should!=1||
       f.set!=1||f.ent_reads!=1)
        return Fail(14,"slot 6 original bounded 20-byte records and policy");
    // All candidate bounds are checked before any per-entity mutation.
    f.calls.clear();f.selected[0]=0;f.index_extent=1;
    v->CheckTransmit(&f.info,f.indices,2);
    if(game->Report().status!=GameEntsDispatchResult::InvalidInput||
       f.calls!="QR"||f.selected[0])
        return Fail(15,"short candidate array rejected without bitset side effects");
    f.index_extent=2;
    // A virtual callback cannot recursively enter a half-mutated object.
    f.calls.clear();f.selected[0]=0;f.reenter=true;
    v->CheckTransmit(&f.info,f.indices,2);
    if(game->Report().status!=GameEntsDispatchResult::Completed||
       !f.reentrancy_refused||f.selected[0]!=6u)
        return Fail(16,"reentrant virtual dispatch rejected without aborting outer call");

    f.calls.clear();v->FreeContainingEntity(&rogue);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       !f.calls.empty()||f.ea.unknown==nullptr)
        return Fail(17,"slot 3 refuses foreign edict without touching memory");
    f.calls.clear();
    CHECK_X86_STACK(freeSlot(game,&f.ea),30);
    if(game->Report().status!=GameEntsDispatchResult::Completed||
       game->Report().release!=EntityReleaseStatus::DetachedAndHooksInvoked||
       f.calls!="LEFFDGMUX")
        return Fail(18,"slot 3 complete detachment and ordered owned cleanup");
    // The test checks mutation and cleanup separately so no original
    // entity pointer is dereferenced after synthetic immediate removal.
    if(!f.valid||f.first_alive||f.ea.unknown||f.ea.flags||
       f.contacts.touchAllocated||!f.contacts.cleanupDataObjects||
       f.ta.contacts.present||f.tb.contacts.root!=&f.rb||
       f.rb.next!=&f.rb||f.ends!=1||f.freed!=2||
       f.updated!=1||f.deleted!=1||f.registry[1].entity)
        return Fail(19,"all owner lifetime and contact invariants after release");
    f.calls.clear();v->FreeContainingEntity(&f.ea);
    if(game->Report().status!=GameEntsDispatchResult::NoEntity||
       !f.calls.empty()||f.deleted!=1)
        return Fail(20,"retired original edict may not be freed twice");
    if(v->EdictToBaseEntity(&f.ea)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::NoEntity||
       v->BaseEntityToEdict(&f.first)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized)
        return Fail(21,"retired entity not exported through PE32 conversion slots");
    f.calls.clear();v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::NoEntity||
       f.calls!="AB"||f.contacts.touchAllocated)
        return Fail(22,"retired edict touch no-op");
    v->SetDebugEdictBase(nullptr);
    if(game->DebugEdictBase()!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Completed)
        return Fail(23,"original debug edict pointer can be cleared");
    if(game->Report().invocations[1]<2||
       game->Report().invocations[2]<3||
       game->Report().invocations[3]<3||
       game->Report().invocations[4]<3||
       game->Report().invocations[5]<3||
       game->Report().invocations[6]<3)
        return Fail(24,"all six non-destructor virtual slots invoked");
    delete v;
    std::puts("PASS ServerGameEnts001 concrete seven-slot Win32 vtable: bounded conversions, edict touch, transmit, release, invalid extents and reentrant refusal");
    return 0;
}
