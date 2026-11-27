// Native MSVC x86, fully wired (but unregistered) ServerGameEnts001 vtable.
// Each typed virtual slot is called through the same complete concrete object.
// Source pointers/handles and networking are deliberately synthetic.
#include "server_gameents_bounded_dispatch.h"
#include "server_gameents_engine_edict_retire_core.h"
#include "server_gameents_owner_lifetime_registry.h"
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
    std::uint32_t reuse_time_bits=0x12345678;
};
class CBaseEntity {
public:
    unsigned char reserved[sin_independent::kOriginalEntityEdictOffset]{};
    edict_t* edict=nullptr;
    unsigned id=0;
};
struct CCheckTransmitInfo {unsigned char bytes[0x2010]{};};
static_assert(offsetof(edict_t,unknown)==0x0c,"retail edict unknown offset");
static_assert(offsetof(edict_t,reuse_time_bits)==0x10,"retail engine x87 reuse-time offset");
static_assert(sizeof(edict_t)==0x14,"retail engine 20-byte edict array stride");
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
    // Phase 1638: concrete owner-side generation and lease store.
    GameEntsOwnerLifetimeRegistry<4,4> owner_registry{};
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
         reentrancy_refused=false,foreign_touch=false,foreign_transmit=false,
         expire_pair_on_publish=false,crosswired_transmit=false;
    bool lease_active=false,deny_next_lease=false,
         reenter_lease_begin=false,reenter_lease_end=false;
    std::uint64_t owner_epoch=77,held_epoch=0;
    GameEntsDispatchSlot held_slot=GameEntsDispatchSlot::SetDebugEdictBase;
    unsigned lease_attempts=0,lease_granted=0,lease_released=0,
             lease_denied=0;
    unsigned pair_checks=0,pair_denials=0;
    unsigned unknown_pair_checks=0,unknown_pair_denials=0;
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
        // Owner-owned descriptors are created from these real (synthetic)
        // fixture allocations; no destination memory is read by registry.
        if(!owner_registry.Enroll(1,{&ea,sizeof(ea),&first,sizeof(first),
                                     &first_unknown,sizeof(first_unknown),
                                     &ta,sizeof(ta),1})||
           !owner_registry.Enroll(2,{&eb,sizeof(eb),&second,sizeof(second),
                                     &second_unknown,sizeof(second_unknown),
                                     &tb,sizeof(tb),2})||
           !owner_registry.BindCandidate(91,2,2))valid=false;
    }
    Fixture(const Fixture&)=delete;
    ~Fixture(){
        for(auto* root:{&ra,&rb}) {
            auto* p=root->next;unsigned visited=0;
            while(p&&p!=root&&visited++<8) {auto* next=p->next;delete p;p=next;}
        }
    }
};
static bool BeginOwnerAccess(void* p,GameEntsDispatchSlot slot,
                             std::uint64_t* token) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    ++f.lease_attempts;
    if(f.deny_next_lease){
        f.deny_next_lease=false;++f.lease_denied;
        return false;
    }
    if(f.lease_active||!token){f.valid=false;return false;}
    std::uint64_t owner_token=0;
    if(!f.owner_registry.Begin(static_cast<unsigned>(slot),&owner_token)) {
        f.valid=false;return false;
    }
    f.lease_active=true;
    f.held_epoch=owner_token;
    f.held_slot=slot;
    *token=f.held_epoch;
    ++f.lease_granted;
    if(f.reenter_lease_begin&&f.bridge){
        auto* v=static_cast<ServerGameEntsAbi*>(f.bridge);
        f.reentrancy_refused=v->BaseEntityToEdict(&f.first)==nullptr;
    }
    return true;
}
static void EndOwnerAccess(void* p,GameEntsDispatchSlot slot,
                           std::uint64_t token) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active||slot!=f.held_slot||token!=f.held_epoch)
        f.valid=false;
    if(f.reenter_lease_end&&f.bridge){
        auto* v=static_cast<ServerGameEntsAbi*>(f.bridge);
        f.reentrancy_refused=v->EdictToBaseEntity(&f.eb)==nullptr;
    }
    if(!f.owner_registry.End(static_cast<unsigned>(slot),token))
        f.valid=false;
    ++f.lease_released;
    f.held_epoch=0;f.lease_active=false;
}
static bool ValidEdict(void* p,const edict_t* edict) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active)f.valid=false;
    return f.owner_registry.ValidEdict(edict)&&
        (edict==&f.ea||edict==&f.eb);
}
static bool ValidEntity(void* p,const CBaseEntity* entity) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active)f.valid=false;
    return f.owner_registry.ValidEntity(entity)&&
        ((entity==&f.first&&f.first_alive)||entity==&f.second);
}
static bool ValidUnknown(void* p,const ServerUnknownAbi* unknown) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active)f.valid=false;
    return f.owner_registry.ValidUnknown(unknown)&&
        (unknown==&f.first_unknown||unknown==&f.second_unknown);
}
static bool ValidUnknownPair(void* p,const edict_t* edict,
                             const ServerUnknownAbi* unknown) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active)f.valid=false;
    // The registry supplies the owner generation association; the direct
    // fixture pair cross-check also guards against a synthetic serial edit.
    ++f.unknown_pair_checks;
    const bool authorized=f.owner_registry.ValidUnknownPair(edict,unknown)&&
        ((edict==&f.ea&&unknown==&f.first_unknown&&
          f.ea.serial==f.registry[1].serial)||
         (edict==&f.eb&&unknown==&f.second_unknown&&
          f.eb.serial==f.registry[2].serial));
    if(!authorized)++f.unknown_pair_denials;
    return authorized;
}
static bool ValidTouch(void* p,const TouchEntity* touch) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active)f.valid=false;
    return f.owner_registry.ValidTouch(touch,sizeof(TouchEntity))&&
        (touch==&f.ta||touch==&f.tb);
}
static bool ValidPair(void* p,const edict_t* edict,
                      const CBaseEntity* entity) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    ++f.pair_checks;
    if(!f.lease_active)f.valid=false;
    if(!f.owner_registry.ValidPair(edict,entity)) {
        ++f.pair_denials;return false;
    }
    // The fixture uses a generation-tagged registry instead of merely
    // comparing the numerical addresses of two independently live objects.
    const bool first=edict==&f.ea&&entity==&f.first&&f.first_alive&&
        f.ea.serial==f.registry[1].serial&&
        f.registry[1].entity==&f.first&&f.first.edict==&f.ea&&
        f.ea.unknown==&f.first_unknown;
    const bool second=edict==&f.eb&&entity==&f.second&&
        f.eb.serial==f.registry[2].serial&&
        f.registry[2].entity==&f.second&&f.second.edict==&f.eb&&
        f.eb.unknown==&f.second_unknown;
    if(!first&&!second)++f.pair_denials;
    return first||second;
}
static bool ValidCandidate(void* p,std::uint32_t table_token,
                           std::size_t index,const CBaseEntity* entity) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active)f.valid=false;
    // Authenticity proof over synthetic table slot 2, using its handle serial.
    return f.owner_registry.ValidCandidate(table_token,index,entity)&&
        table_token==91&&index==2&&ValidPair(p,&f.eb,entity);
}
static bool Extent(void* p,const CCheckTransmitInfo* info,
                   const unsigned short* indices,int,
                   std::size_t* out_info,std::size_t* out_indices) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.lease_active)f.valid=false;
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
    if(f.foreign_touch&&*out)
        *out=reinterpret_cast<TouchEntity*>(std::uintptr_t{1});
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
    // Stress a misbehaving host callback that recycles an entity generation
    // even while the bounded dispatch holds its access lease.
    if(f.expire_pair_on_publish)++f.registry[1].serial;
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
    // Source teardown is permitted to tombstone the actor while its
    // slot-3 owner access lease is still held; the edict prefix stays owned.
    if(!f.owner_registry.RetireEntity(1))f.valid=false;
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
    *out=f.foreign_transmit
        ?reinterpret_cast<CBaseEntity*>(std::uintptr_t{1})
        :f.crosswired_transmit?&f.first:&f.second;
    return true;
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
// Engine-side read authority is a short owner lease. It must end BEFORE
// the game-server FreeContainingEntity callback obtains its own slot-3 lease.
static bool EngineOwnedEdict(void* p,const void* edict,
                             std::size_t extent) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(extent!=kEngineOriginalEdictStride)return false;
    std::uint64_t token=0;
    if(!f.owner_registry.Begin(5,&token))return false;
    const bool authorized=f.owner_registry.ValidEdict(edict)&&
        (edict==&f.ea||edict==&f.eb);
    if(!f.owner_registry.End(5,token))f.valid=false;
    return authorized;
}
static bool EngineReleaseEntity(void* p,void* edict) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(!f.bridge||!edict)return false;
    // Force the actual seventh-slot-compatible MSVC x86 vtable entry.
    // Avoid a compiler devirtualizing this synthetic dispatch through
    // the statically known final class and silently bypassing the ABI test.
    auto** slots=*reinterpret_cast<void***>(f.bridge);
    if(!slots||!slots[3])return false;
    auto free_slot=reinterpret_cast<void (__thiscall *)(void*,edict_t*)>(
        slots[3]);
    free_slot(f.bridge,static_cast<edict_t*>(edict));
    return f.bridge->Report().status==GameEntsDispatchResult::Completed;
}
static GameEntsBoundedOwner Hooks(Fixture& f) noexcept {
    GameEntsBoundedOwner h{};
    h.context=&f;
    h.begin_access=&BeginOwnerAccess;h.end_access=&EndOwnerAccess;
    h.valid_edict=&ValidEdict;h.valid_entity=&ValidEntity;
    h.valid_unknown=&ValidUnknown;
    h.valid_unknown_pair=&ValidUnknownPair;h.valid_touch=&ValidTouch;
    h.valid_pair=&ValidPair;h.valid_candidate=&ValidCandidate;
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
    // Incomplete nested-pointer attestation must prevent readiness even
    // when the outer edict and buffer validators are present.
    GameEntsBoundedOwner no_lease_begin=Hooks(f);
    no_lease_begin.begin_access=nullptr;
    if(ServerGameEntsBoundedDispatch(no_lease_begin).Ready())
        return Fail(40,"missing owner access lease acquisition gate");
    GameEntsBoundedOwner no_lease_end=Hooks(f);
    no_lease_end.end_access=nullptr;
    if(ServerGameEntsBoundedDispatch(no_lease_end).Ready())
        return Fail(41,"missing owner access lease release gate");
    GameEntsBoundedOwner no_pair=Hooks(f);
    no_pair.valid_pair=nullptr;
    if(ServerGameEntsBoundedDispatch(no_pair).Ready())
        return Fail(49,"missing edict/entity generation association gate");
    GameEntsBoundedOwner no_candidate=Hooks(f);
    no_candidate.valid_candidate=nullptr;
    if(ServerGameEntsBoundedDispatch(no_candidate).Ready())
        return Fail(60,"missing transmit candidate generation gate");
    GameEntsBoundedOwner no_unknown=Hooks(f);
    no_unknown.valid_unknown=nullptr;
    if(ServerGameEntsBoundedDispatch(no_unknown).Ready())
        return Fail(31,"missing unknown-interface ownership gate");
    GameEntsBoundedOwner no_unknown_pair=Hooks(f);
    no_unknown_pair.valid_unknown_pair=nullptr;
    if(ServerGameEntsBoundedDispatch(no_unknown_pair).Ready())
        return Fail(66,"missing edict/unknown association callback was accepted");
    GameEntsBoundedOwner no_touch=Hooks(f);
    no_touch.valid_touch=nullptr;
    if(ServerGameEntsBoundedDispatch(no_touch).Ready())
        return Fail(32,"missing touch-storage ownership gate");
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
       game->Report().invocations[1]!=1||
       f.lease_active||f.lease_granted!=1||f.lease_released!=1)
        return Fail(7,"vtable slot 1 debug-edict owner state");
    // A denied lease must block every class of engine-facing operation
    // before dereferencing even an otherwise valid original pointer.
    f.deny_next_lease=true;f.calls.clear();
    v->SetDebugEdictBase(&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::LeaseDenied||
       game->DebugEdictBase()!=&f.ea||f.lease_active||
       f.lease_granted!=1||f.lease_released!=1||!f.calls.empty())
        return Fail(42,"denied lease cannot change debug edict base");
    f.deny_next_lease=true;
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::LeaseDenied||
       !f.calls.empty()||f.allocated||f.ta.dirtyOrigin!=true)
        return Fail(43,"denied lease cannot resolve or create contacts");
    f.deny_next_lease=true;
    v->FreeContainingEntity(&f.ea);
    if(game->Report().status!=GameEntsDispatchResult::LeaseDenied||
       f.ea.unknown!=&f.first_unknown||f.ea.flags!=0x65||
       !f.calls.empty())
        return Fail(44,"denied lease cannot detach original edict");
    f.deny_next_lease=true;
    v->CheckTransmit(&f.info,f.indices,2);
    if(game->Report().status!=GameEntsDispatchResult::LeaseDenied||
       f.selected[0]!=0||!f.calls.empty()||f.lease_denied!=4||
       f.lease_granted!=f.lease_released)
        return Fail(45,"denied lease cannot enter transmit callbacks");
    edict_t* fromBase=nullptr;
    CBaseEntity* fromEdict=nullptr;
    CHECK_X86_STACK(fromBase=baseSlot(game,&f.first),26);
    CHECK_X86_STACK(fromEdict=edictSlot(game,&f.eb),27);
    if(fromBase!=&f.ea||fromEdict!=&f.second||
       game->Report().status!=GameEntsDispatchResult::Completed)
        return Fail(8,"slots 4 and 5 original byte offsets and unknown vtable");
    // The host may invoke game interfaces while acquiring or releasing its
    // lease. In both cases the outer call owns the active reentrancy guard.
    f.reenter_lease_begin=true;f.reentrancy_refused=false;
    if(v->BaseEntityToEdict(&f.first)!=&f.ea||
       !f.reentrancy_refused||f.lease_active||!f.valid)
        return Fail(46,"reentry during owner lease acquire was not refused");
    f.reenter_lease_begin=false;
    f.reenter_lease_end=true;f.reentrancy_refused=false;
    if(v->EdictToBaseEntity(&f.eb)!=&f.second||
       !f.reentrancy_refused||f.lease_active||!f.valid)
        return Fail(47,"reentry during owner lease release was not refused");
    f.reenter_lease_end=false;

    // Owned-pointer status alone does not establish a correct edict/entity
    // pair. The owner checks reciprocal mapping and handle generation.
    f.calls.clear();
    const auto valid_pairs_before=f.pair_checks;
    f.first.edict=&f.eb;  // both pointers are individually approved
    if(v->BaseEntityToEdict(&f.first)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized||
       f.pair_checks==valid_pairs_before||f.lease_active)
        return Fail(50,"owned entity must not export another owned edict");
    f.first.edict=&f.ea;
    // A trusted *other* unknown can return a trusted *other* entity.
    // Neither identity may authorize release or touch for the wrong edict.
    f.ea.unknown=&f.second_unknown;
    const unsigned foreign_calls_before=f.second_unknown.calls;
    if(v->EdictToBaseEntity(&f.ea)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized||
       f.second_unknown.calls!=foreign_calls_before)
        return Fail(51,"owned but crosswired unknown must not be dispatched");
    v->FreeContainingEntity(&f.ea);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       f.ea.unknown!=&f.second_unknown||f.ea.flags!=0x65||
       f.second_unknown.calls!=foreign_calls_before||
       !f.calls.empty()||f.deleted)
        return Fail(52,"cross-wired unknown cannot free another owned entity");
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       f.second_unknown.calls!=foreign_calls_before||
       !f.calls.empty()||f.allocated||f.starts)
        return Fail(53,"cross-wired unknown must fail before touch resolution");
    f.ea.unknown=&f.first_unknown;
    f.calls.clear();
    // Simulate retirement and reuse of the same memory at a new generation
    // without changing either authorized address or its virtual target.
    const auto serial=f.registry[1].serial;
    f.registry[1].serial=serial+13;
    if(v->BaseEntityToEdict(&f.first)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized||
       v->EdictToBaseEntity(&f.ea)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized)
        return Fail(54,"stale-generation pair rejected by both conversions");
    v->FreeContainingEntity(&f.ea);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       f.ea.unknown!=&f.first_unknown||f.ea.flags!=0x65||
       !f.calls.empty()||f.deleted)
        return Fail(55,"stale-generation pair cannot free entity");
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       !f.calls.empty()||f.allocated)
        return Fail(56,"stale-generation unknown rejected before contact resolution");
    f.registry[1].serial=serial;
    f.calls.clear();
    // A perfectly valid TouchEntity object may embed a different, owned
    // CBaseEntity. The owner association must reject before reading origin.
    f.ta.entity=&f.second;
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::HostFailure||
       f.calls!="A"||f.allocated)
        return Fail(57,"owned touch data cannot refer to mismatched entity");
    f.ta.entity=&f.first;
    f.calls.clear();
    if(v->BaseEntityToEdict(&f.first)!=&f.ea||
       v->EdictToBaseEntity(&f.ea)!=&f.first||
       game->Report().status!=GameEntsDispatchResult::Completed)
        return Fail(58,"original generation restored for normal conversions");

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

    // The 0x0c unknown pointer is a second trust boundary. Reject it
    // before issuing any unknown virtual method, including Free.
    Unknown foreign_unknown{&f.first};
    f.ea.unknown=&foreign_unknown;
    f.calls.clear();
    if(v->EdictToBaseEntity(&f.ea)!=nullptr||
       game->Report().status!=GameEntsDispatchResult::Unauthorized||
       foreign_unknown.calls!=0)
        return Fail(33,"foreign unknown must not be virtually dispatched");
    v->FreeContainingEntity(&f.ea);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       foreign_unknown.calls||f.ea.unknown!=&foreign_unknown||
       f.ea.flags!=0x65||!f.calls.empty())
        return Fail(34,"foreign unknown cannot trigger release writes");
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       foreign_unknown.calls||!f.calls.empty()||f.allocated)
        return Fail(39,"foreign unknown cannot enter touch resolver");
    f.ea.unknown=&f.first_unknown;
    // A trusted unknown may itself return an unauthorized CBaseEntity.
    f.first_unknown.entity=&rogue_entity;
    v->FreeContainingEntity(&f.ea);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       f.ea.unknown!=&f.first_unknown||f.ea.flags!=0x65||
       !f.calls.empty())
        return Fail(35,"foreign resolved entity cannot trigger cleanup");
    f.first_unknown.entity=&f.first;
    // A touch callback can return a forged storage address. Never read its
    // fields before attesting the returned TouchEntity pointer.
    f.calls.clear();f.foreign_touch=true;
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::HostFailure||
       f.calls!="A"||f.allocated!=0)
        return Fail(36,"foreign touch storage must not be dereferenced");
    f.foreign_touch=false;f.ta.entity=&rogue_entity;
    f.calls.clear();v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::HostFailure||
       f.calls!="A"||f.allocated!=0)
        return Fail(37,"foreign nested touch entity must be rejected");
    f.ta.entity=&f.first;
    // The transmit policy must never receive foreign entity pointers.
    // Place the rejected full-check entity first: an earlier ALWAYS bit
    // would correctly be committed by retail-style sequential processing.
    f.calls.clear();f.foreign_transmit=true;f.selected[0]=0;
    f.indices[0]=2;
    v->CheckTransmit(&f.info,f.indices,1);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       !game->Report().transmit.unauthorized_entity||
       f.calls!="QRBE"||f.selected[0]!=0||f.should!=0)
        return Fail(38,"foreign transmit entity rejected before policy");
    f.foreign_transmit=false;
    // An individually authorized entity cannot be sent as another slot.
    f.crosswired_transmit=true;f.calls.clear();
    v->CheckTransmit(&f.info,f.indices,1);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       !game->Report().transmit.unauthorized_entity||
       f.calls!="QRBE"||f.selected[0]!=0||f.should!=0)
        return Fail(61,"owned but wrong transmit candidate must be refused");
    f.crosswired_transmit=false;
    // Same address, newer serial: reject before transmission policy.
    const auto second_generation=f.registry[2].serial;
    f.registry[2].serial=second_generation+21;
    f.calls.clear();v->CheckTransmit(&f.info,f.indices,1);
    if(game->Report().status!=GameEntsDispatchResult::Unauthorized||
       !game->Report().transmit.unauthorized_entity||
       f.calls!="QRBE"||f.selected[0]!=0||f.should!=0)
        return Fail(62,"stale-generation transmit candidate cannot enter policy");
    f.registry[2].serial=second_generation;
    // Negative fixtures performed multiple entity lookups; reset before
    // established successful-transmit probe.
    f.indices[0]=1;f.ent_reads=0;

    f.calls.clear();f.fail_publish=true;
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::HostFailure||
       f.calls!="ABI21P"||f.allocated||f.contacts.touchAllocated)
        return Fail(11,"original trace publish refusal before contact link allocation");
    f.fail_publish=false;f.expire_pair_on_publish=true;f.calls.clear();
    v->MarkEntitiesAsTouching(&f.ea,&f.eb);
    if(game->Report().status!=GameEntsDispatchResult::HostFailure||
       f.calls!="ABIP"||f.allocated||f.starts||f.contacts.touchAllocated)
        return Fail(59,"expired post-publication pair cannot mutate contact graph");
    f.expire_pair_on_publish=false;
    f.registry[1].serial=f.ea.serial;
    f.ta.dirtyOrigin=true;f.tb.dirtyOrigin=true;
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
    // An original engine-owned edict retirement call crosses the same
    // registered seven-slot vtable used for all preceding operations.
    // The server tombstones the entity under slot 3, then the engine
    // reattests the still-owned edict and poisons serial+0x04 and free
    // link+0x10. The full sequence occurs in one native x86 test.
    const EngineEdictRetireHooks engine_hooks{
        &f,&EngineOwnedEdict,nullptr,&EngineReleaseEntity};
    EngineEdictRetireResult retired=EngineEdictRetireResult::InvalidInput;
    CHECK_X86_STACK(retired=RetireEngineEdict(
        &f.ea,sizeof(f.ea),engine_hooks),30);
    if(retired!=EngineEdictRetireResult::Retired||
       game->Report().status!=GameEntsDispatchResult::Completed||
       game->Report().release!=EntityReleaseStatus::DetachedAndHooksInvoked||
       f.ea.serial!=kEngineOriginalEdictRetiredSerial||
       f.ea.reuse_time_bits!=0||f.calls!="LEFFDGMUX")
        return Fail(18,"engine-to-server slot 3 release, owner retirement, and engine epilogue");
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
    // Every successful virtual call releases precisely the owner token it
    // acquired, including failed authorization, no-entity and host failures.
    if(!f.valid||f.lease_active||f.owner_registry.HasLease()||
       f.lease_granted!=f.lease_released||
       f.pair_checks<10||f.pair_denials<3||
       f.unknown_pair_checks<10||f.unknown_pair_denials<6||
       f.lease_attempts!=f.lease_granted+f.lease_denied||
       game->Report().lease_attempts!=f.lease_attempts||
       game->Report().lease_acquisitions!=f.lease_granted||
       game->Report().lease_releases!=f.lease_released||
       game->Report().lease_denials!=f.lease_denied)
    {
        std::fprintf(stderr,
            "owner audit: valid=%d leases=%u/%u/%u/%u pairs=%u/%u unknown=%u/%u\n",
            static_cast<int>(f.valid),f.lease_attempts,f.lease_granted,
            f.lease_released,f.lease_denied,f.pair_checks,f.pair_denials,
            f.unknown_pair_checks,f.unknown_pair_denials);
        return Fail(48,"owner leases and association diagnostics must balance");
    }
    // The actual destruction stage already tombstoned the entity. The
    // owner must explicitly forget its edict after the release lease ends.
    std::uint64_t verify_token=0;
    if(!f.owner_registry.Begin(5,&verify_token)||
       f.owner_registry.ValidEntity(&f.first)||
       !f.owner_registry.ValidEdict(&f.ea)||
       !f.owner_registry.End(5,verify_token)||
       !f.owner_registry.ForgetEdict(1))
        return Fail(63,"actual slot-3 release failed owner tombstone contract");
    if(!f.owner_registry.Begin(5,&verify_token)||
       f.owner_registry.ValidEdict(&f.ea)||
       !f.owner_registry.End(5,verify_token))
        return Fail(64,"forgotten edict still passed owner allocation validator");
    // A second original engine call at the poisoned address cannot mutate
    // the already retired edict after the owner forgets the allocation.
    const std::uint32_t poisoned_serial=f.ea.serial;
    if(RetireEngineEdict(&f.ea,sizeof(f.ea),engine_hooks)!=
       EngineEdictRetireResult::Unauthorized||
       f.ea.serial!=poisoned_serial||f.deleted!=1)
        return Fail(65,"engine may not free a forgotten stale-generation edict");
    delete v;
    std::puts("PASS ServerGameEnts001 concrete seven-slot Win32 vtable: concrete owner registry, leases, generation-aware associations, conversions, edict touch, transmit, release and reentrant refusal");
    return 0;
}
