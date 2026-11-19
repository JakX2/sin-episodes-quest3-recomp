// Native PE32 host fixture of the original six shadow-manager actions.
#include "client_shadow_mgr_init.h"
#include "client_auto_gamesystem_init_core.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>
namespace shadow=sin_independent::client_shadow;
namespace gs=sin_independent::auto_gamesystem;
struct Capture {
    std::vector<shadow::Operation> actions;
};
static void Record(void* context,const shadow::Operation& operation) noexcept {
    static_cast<Capture*>(context)->actions.push_back(operation);
}
static bool Check(const std::vector<shadow::Operation>& events,bool refresh) {
    using shadow::Action;
    std::vector<Action> expected={
        Action::InitSimpleShadowTexture,Action::SetShadowDirection,
        Action::SetShadowDistance,Action::SetShadowBlobbyCutoffArea};
    if(refresh)expected.push_back(Action::InitRenderToTextureShadows);
    expected.push_back(Action::AddMaterialRestoreFunc);
    if(events.size()!=expected.size())return false;
    for(std::size_t i=0;i<events.size();++i)
        if(events[i].action!=expected[i])return false;
    if(std::strcmp(events[0].name,"decals/simpleshadow") ||
       std::strcmp(events[0].category,"Decal textures") ||
       events[0].callsite_va!=0x2411889au)return false;
    if(!events[1].vector_arg ||
       events[1].original_slot!=24 ||
       events[1].vector_arg[0]!=0.1f ||
       events[1].vector_arg[1]!=0.1f ||
       events[1].vector_arg[2]!=-1.0f)return false;
    if(events[2].original_slot!=27 || events[2].scalar_arg!=50.0f ||
       events[3].original_slot!=28 || events[3].scalar_arg!=0.005f)
        return false;
    const auto& last=events.back();
    return last.original_slot==116 &&
        last.original_callback_va==0x24115e70u &&
        last.callsite_va==0x24118905u;
}

// Assert observable state at every host callback boundary, not just after Init.
// This detects accidental reordering of the two original object writes.
struct Timing {
    unsigned char* owner;
    std::size_t seen=0;
    bool valid=true;
    bool refresh=false;
};
static void CheckTiming(void* context,const shadow::Operation& operation) noexcept {
    auto& t=*static_cast<Timing*>(context);
    const auto flag=t.owner[shadow::kOriginalFlagOffset];
    std::uint32_t mode=0;
    std::memcpy(&mode,t.owner+shadow::kOriginalModeOffset,sizeof(mode));
    if(flag!=0)t.valid=false;
    // The mode write happens after the fourth action and before refresh/restore.
    const bool modeWritten=t.seen>=4;
    if(mode!=(modeWritten?8u:0xa5a5a5a5u))t.valid=false;
    if(t.seen==0 && operation.action!=shadow::Action::InitSimpleShadowTexture)t.valid=false;
    if(t.seen==4 && operation.action!=(t.refresh?
        shadow::Action::InitRenderToTextureShadows:shadow::Action::AddMaterialRestoreFunc))
        t.valid=false;
    ++t.seen;
}
static bool TimingContract(bool refresh) {
    std::array<unsigned char,0xe0> storage{};
    storage.fill(0xa5);
    Timing timing{storage.data(),0,true,refresh};
    if(!shadow::Init(storage.data(),storage.size(),refresh,&timing,&CheckTiming))
        return false;
    if(!timing.valid || timing.seen!=(refresh?6u:5u))return false;
    // Exact minimum valid object extent must also be accepted.
    Timing exact{storage.data(),0,true,refresh};
    storage.fill(0xa5);
    if(!shadow::Init(storage.data(),0xdc,refresh,&exact,&CheckTiming) ||
       !exact.valid || exact.seen!=(refresh?6u:5u))return false;
    return storage[0xdc]==0xa5 && storage[0xdf]==0xa5;
}

// Full host-facing metadata contract for every recovered callsite. This is
// synthetic ABI evidence: it does not establish original helper execution.
static bool MetadataContract(bool refresh) {
    std::array<unsigned char,0xdc> owner{};
    Capture first;
    if(!shadow::Init(owner.data(),owner.size(),refresh,&first,&Record) ||
       !Check(first.actions,refresh))return false;
    const shadow::Operation expected[]={
        shadow::kPrecache,shadow::kVector,shadow::kRadius,shadow::kBias,
        shadow::kRefresh,shadow::kRegisterCallback
    };
    std::size_t next=0;
    for(const auto& actual:first.actions) {
        if(next==4 && !refresh)++next;
        if(next>=6)return false;
        const auto& reference=expected[next++];
        if(actual.action!=reference.action ||
           actual.callsite_va!=reference.callsite_va ||
           actual.original_slot!=reference.original_slot ||
           actual.original_callback_va!=reference.original_callback_va ||
           actual.scalar_arg!=reference.scalar_arg ||
           actual.name!=reference.name ||
           actual.category!=reference.category ||
           actual.vector_arg!=reference.vector_arg)return false;
    }
    if(next!=6)return false;
    // Re-entry must replay identical callbacks and must not accumulate
    // hidden registration state in this independent orchestration model.
    Capture again;
    if(!shadow::Init(owner.data(),owner.size(),refresh,&again,&Record) ||
       again.actions.size()!=first.actions.size())return false;
    for(std::size_t i=0;i<first.actions.size();++i) {
        const auto& a=first.actions[i];
        const auto& b=again.actions[i];
        if(a.action!=b.action || a.callsite_va!=b.callsite_va ||
           a.original_slot!=b.original_slot ||
           a.original_callback_va!=b.original_callback_va ||
           a.name!=b.name || a.category!=b.category ||
           a.vector_arg!=b.vector_arg || a.scalar_arg!=b.scalar_arg)
            return false;
    }
    return true;
}

struct System:gs::System {
    std::array<unsigned char,0xdc> storage{};
    Capture capture;
    bool condition=false;
    bool Init() noexcept override {
        return shadow::Init(storage.data(),storage.size(),
                            condition,&capture,&Record);
    }
};

// Exercise the original registry's live-count semantics with a shadow
// subsystem that registers another system during Init. This remains a
// synthetic Windows ABI fixture, not execution of retail game methods.
struct OrderedSystem:gs::System {
    std::vector<int>* order;
    int id;
    bool accept;
    gs::Registry* registry=nullptr;
    gs::System* late=nullptr;
    bool Init() noexcept override {
        order->push_back(id);
        if(registry && late)registry->DirectRegister(late);
        return accept;
    }
};
struct ShadowWithLateRegistration:gs::System {
    std::array<unsigned char,0xdc> storage{};
    Capture capture;
    std::vector<int>* order;
    gs::Registry* registry;
    gs::System* late;
    bool Init() noexcept override {
        order->push_back(2);
        const bool ok=shadow::Init(storage.data(),storage.size(),
                                   true,&capture,&Record);
        if(ok)registry->DirectRegister(late);
        return ok;
    }
};
static bool RegistryShadowIntegration(bool rejectFirst) {
    std::vector<int> order;
    gs::Registry registry;
    OrderedSystem before{ };
    before.order=&order;before.id=1;before.accept=!rejectFirst;
    OrderedSystem late{ };
    late.order=&order;late.id=3;late.accept=true;
    ShadowWithLateRegistration shadowSystem{ };
    shadowSystem.order=&order;
    shadowSystem.registry=&registry;
    shadowSystem.late=&late;
    registry.DirectRegister(&before);
    registry.DirectRegister(&shadowSystem);
    const auto result=registry.Initialize();
    if(rejectFirst)
        return result.outcome==gs::Outcome::InitRefused &&
               result.initialized_count==0 &&
               order==std::vector<int>{1} &&
               shadowSystem.capture.actions.empty() &&
               registry.registered_size()==2;
    return result.succeeded() && result.initialized_count==3 &&
           order==std::vector<int>({1,2,3}) &&
           registry.registered_size()==3 &&
           Check(shadowSystem.capture.actions,true);
}

int main(){
    static_assert(sizeof(void*)==4,"original engine uses MSVC x86");
    if(!TimingContract(false) || !TimingContract(true))return 7;
    if(!MetadataContract(false) || !MetadataContract(true))return 8;
    if(!RegistryShadowIntegration(false) || !RegistryShadowIntegration(true))return 9;
    System system;
    system.storage.fill(0xa5);
    if(shadow::Init(nullptr,system.storage.size(),false,&system.capture,&Record) ||
       shadow::Init(system.storage.data(),0xdb,false,&system.capture,&Record) ||
       shadow::Init(system.storage.data(),system.storage.size(),false,nullptr,&Record) ||
       shadow::Init(system.storage.data(),system.storage.size(),false,&system.capture,nullptr))
        return 1;
    for(auto b:system.storage)if(b!=0xa5)return 2;
    if(!system.Init() || !Check(system.capture.actions,false))return 3;
    for(std::size_t i=0;i<system.storage.size();++i) {
        const unsigned char expected=(i==0xcd)?0:
            (i==0xd8)?8:(i>0xd8 && i<0xdc)?0:0xa5;
        if(system.storage[i]!=expected)return 4;
    }
    Capture second;
    auto clone=system.storage;
    if(!shadow::Init(clone.data(),clone.size(),true,&second,&Record) ||
       !Check(second.actions,true) || second.actions.size()!=6)
        return 5;
    // The original function replays setup, with no once bit.
    gs::Registry registry;
    registry.DirectRegister(&system);
    auto result=registry.Initialize();
    if(!result.succeeded() || result.initialized_count!=1 ||
       system.capture.actions.size()!=10)return 6;
    // The whole fixture is not a complete original helper implementation.
    std::puts("PASS original shadow manager texture, vector, scalars, flag, conditional refresh and engine callback order");
    return 0;
}
