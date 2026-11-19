// Original-guided, owner-free Win32 x86 stats storage and callback contract.
#include "client_stats_storage_init.h"
#include "client_auto_gamesystem_init_core.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <type_traits>
namespace storage=sin_independent::stats_storage;
namespace stats=sin_independent::stats_system;
namespace gs=sin_independent::auto_gamesystem;
struct Fixture {
    int factoryCalls=0;
    std::array<int,storage::kStaticSlots> attempted{};
    int staticDenied=-1;
    int dynamicDenied=-1;
    bool rejectLast=false;
    void* owner=nullptr;
    void* registry=nullptr;
    std::vector<stats::Step> remaining;
    bool badContext=false;
    storage::Storage* instance=nullptr;
};
static Fixture* now=nullptr;
static void* CreateDefinition(void* context,std::size_t index) noexcept {
    auto& f=*static_cast<Fixture*>(context);
    if(index>=storage::kStaticSlots)return nullptr;
    ++f.factoryCalls;
    ++f.attempted[index];
    // Original 0x24205BC0 type dispatch only has specialized 0..15
    // cases. These invented nonnull tags are NOT original types.
    return index<storage::kStaticDefinitionCases?
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(index+1)):nullptr;
}
static bool Allowed(void* context,bool dynamic,std::size_t index) noexcept {
    auto& f=*static_cast<Fixture*>(context);
    return static_cast<int>(index)!=(dynamic?f.dynamicDenied:f.staticDenied);
}
static bool Observe(void* where,const stats::Step& step) noexcept {
    auto& f=*now;
    if((step.kind==stats::StepKind::RegisterCallback?f.registry:f.owner)!=where)
        f.badContext=true;
    if(!f.instance || f.instance->staticCount()!=storage::kStaticSlots ||
       f.instance->dynamicCount()!=storage::kDynamicSlots)f.badContext=true;
    f.remaining.push_back(step);
    return !(f.rejectLast && f.remaining.size()==5);
}
struct System:gs::System {
    storage::Storage store;
    Fixture fixture;
    int owner=11,registry=17;
    bool Init() noexcept override {
        fixture.owner=&owner;fixture.registry=&registry;
        fixture.instance=&store;now=&fixture;
        return storage::ExecuteKnownStages(store,&owner,&registry,
            &fixture,&CreateDefinition,&Allowed,&Observe);
    }
};
int main(){
    static_assert(sizeof(void*)==4,"original Windows x86 client");
    static_assert(sizeof(storage::StaticRecord)==8,"native record size");
    static_assert(sizeof(storage::DynamicRecord)==20,"native record size");
    static_assert(!std::is_copy_constructible<storage::Storage>::value,
                  "published record pointers cannot be copied");
    System system;
    if(storage::ExecuteKnownStages(system.store,nullptr,&system.registry,
            &system.fixture,&CreateDefinition,&Allowed,&Observe)){ std::fprintf(stderr,"Stats storage fixture failure at assertion 1\n");return 1; }
    auto& f=system.fixture;
    f.staticDenied=5;f.dynamicDenied=7;
    if(!system.Init() || f.badContext || f.remaining.size()!=5 ||
       system.store.staticCount()!=55 || system.store.dynamicCount()!=11 ||
       f.factoryCalls!=54){ std::fprintf(stderr,"Stats storage fixture failure at assertion 2\n");return 2; }
    for(std::size_t i=0;i<55;++i){
        auto* record=system.store.staticAt(i);
        if(i==5){
            if(record || f.attempted[i]){ std::fprintf(stderr,"Stats storage fixture failure at assertion 3\n");return 3; }
        } else {
            if(!record || f.attempted[i]!=1 || record->runtime){ std::fprintf(stderr,"Stats storage fixture failure at assertion 4\n");return 4; }
            const auto expected=i<16?
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(i+1)):nullptr;
            if(record->definition!=expected){ std::fprintf(stderr,"Stats storage fixture failure at assertion 5\n");return 5; }
        }
    }
    for(std::size_t i=0;i<11;++i){
        auto* record=system.store.dynamicAt(i);
        if(i==7){if(record){ std::fprintf(stderr,"Stats storage fixture failure at assertion 6\n");return 6; }}
        else {
            if(!record){ std::fprintf(stderr,"Stats storage fixture failure at assertion 7\n");return 7; }
            for(auto field:record->fields)if(field){ std::fprintf(stderr,"Stats storage fixture failure at assertion 8\n");return 8; }
        }
    }
    if(system.store.staticAt(55) || system.store.dynamicAt(11)){ std::fprintf(stderr,"Stats storage fixture failure at assertion 9\n");return 9; }
    // Reinitializing destroys previous records and resets all payloads,
    // then appends 55 + 11 pointer entries in the original order.
    f.staticDenied=f.dynamicDenied=-1;f.remaining.clear();
    if(!system.Init() || f.remaining.size()!=5 || f.factoryCalls!=109 ||
       !system.store.staticAt(5) || !system.store.dynamicAt(7)){ std::fprintf(stderr,"Stats storage fixture failure at assertion 10\n");return 10; }
    for(std::size_t i=0;i<55;++i)
        if(f.attempted[i]!=(i==5?1:2)){ std::fprintf(stderr,"Stats storage fixture failure at assertion 11\n");return 11; }
    for(std::size_t i=0;i<11;++i)
        for(auto field:system.store.dynamicAt(i)->fields)if(field){ std::fprintf(stderr,"Stats storage fixture failure at assertion 12\n");return 12; }
    // The next original Stats callsite after both now-owned helpers
    // is the owner-memory helper at VA 0x241FC5A0.
    if(f.remaining[0].original_callee!=0x241FC5A0u ||
       f.remaining[1].original_callee!=0x24204F30u ||
       f.remaining[2].original_callback_va!=0x241FB7A0u ||
       f.remaining[3].original_callback_va!=0x241FB7E0u ||
       f.remaining[4].original_callback_va!=0x241FCA60u){ std::fprintf(stderr,"Stats storage fixture failure at assertion 13\n");return 13; }
    // A missing external helper refuses this independent adapter.
    f.rejectLast=true;f.remaining.clear();
    if(system.Init() || f.remaining.size()!=5){ std::fprintf(stderr,"Stats storage fixture failure at assertion 14\n");return 14; }
    // The enclosing game-system loop now receives that refusal.
    f.remaining.clear(); // reset per-Init callback count before outer gate
    gs::Registry registry;registry.DirectRegister(&system);
    if(registry.Initialize().outcome!=gs::Outcome::InitRefused){ std::fprintf(stderr,"Stats storage fixture failure at assertion 15\n");return 15; }
    std::puts("PASS owned 55 static + 11 dynamic original Stats records, null slots, reset and 7-call Init ordering");
    return 0;
}
