// Single original-order StatsSystem coordinator, native MSVC x86 fixture.
#include "client_stats_coordinator.h"
#include "client_auto_gamesystem_init_core.h"
#include "client_stats_definition_types.h"
#include <cstdio>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
namespace co=sin_independent::stats_coordinator;
namespace st=sin_independent::stats_system;
namespace auto_system=sin_independent::auto_gamesystem;
struct Fixture {
    co::State* state=nullptr;
    void* owner=nullptr;
    void* registry=nullptr;
    std::vector<std::string> order;
    std::vector<std::uint32_t> later_calls;
    std::size_t definition_calls=0;
    int missing_static=-1,missing_dynamic=-1,missing_catalog=-1;
    bool allow_resources=true;
    int reject_later=-1; // zero-based index into 4 remaining ops
    bool invalid_context=false;
};
static Fixture* active=nullptr;
static void* Define(void* ctx,std::size_t index) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    ++f.definition_calls;
    if(index==0)f.order.emplace_back("static");
    return index<16?
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(index+1)):
        nullptr;
}
static bool AllowData(void* ctx,bool dynamic,std::size_t index) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(index==0)f.order.emplace_back(dynamic?"dynamic":"static");
    return static_cast<int>(index)!=(dynamic?
        f.missing_dynamic:f.missing_static);
}
static bool AllowCatalog(void* ctx,std::size_t index) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(index==0)f.order.emplace_back("catalog");
    return static_cast<int>(index)!=f.missing_catalog;
}
static bool LoadResources(void* ctx,sin_independent::stats_catalog::Catalog& catalog) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    f.order.emplace_back("resources");
    if(!f.state || f.state->definitions.staticCount()!=55 ||
       f.state->definitions.dynamicCount()!=11 ||
       catalog.size()!=66)f.invalid_context=true;
    return f.allow_resources;
}
static bool Later(void* obj,const st::Step& step) noexcept {
    auto& f=*active;
    bool isRegistration=step.kind==st::StepKind::RegisterCallback;
    if(obj!=(isRegistration?f.registry:f.owner))
        f.invalid_context=true;
    f.order.emplace_back(step.argument?step.argument:"owner-step");
    f.later_calls.push_back(step.original_callsite);
    const int idx=static_cast<int>(f.later_calls.size())-1;
    return idx!=f.reject_later;
}
struct System final:auto_system::System {
    co::State owned;
    Fixture fixture;
    int owner=2, registry=3;
    co::Result latest;
    co::Hooks hooks() noexcept {
        fixture.owner=&owner;
        fixture.registry=&registry;
        fixture.state=&owned;
        active=&fixture;
        return {&fixture,&Define,&AllowData,&AllowCatalog,
            &LoadResources,&Later};
    }
    bool Init() noexcept override {
        latest=co::Initialize(owned,&owner,&registry,hooks());
        return latest.succeeded();
    }
};
static bool Equal(const std::vector<std::string>& actual,
                  const std::vector<std::string>& expected){
    return actual==expected;
}
int main(){
    static_assert(sizeof(void*)==4,"original client is native Windows x86");
    static_assert(sizeof(sin_independent::stats_storage::StaticRecord)==8,
                  "original static entry");
    static_assert(sizeof(sin_independent::stats_storage::DynamicRecord)==20,
                  "original dynamic entry");
    static_assert(sizeof(sin_independent::stats_catalog::Entry)==12,
                  "original catalog entry");
    static_assert(!std::is_copy_constructible<co::State>::value,
                  "published owner records must not be copied");
    System system;
    auto hooks=system.hooks();
    // Host refusal before mutation.
    if(co::Initialize(system.owned,nullptr,&system.registry,hooks).outcome!=
           co::Outcome::InvalidHost ||
       system.owned.definitions.staticCount() ||
       system.owned.catalog.size())return 1;
    // Missing enumerator is exposed at the real third callsite.
    auto missing=hooks;missing.load_catalog_resources=nullptr;
    auto r=co::Initialize(system.owned,&system.owner,&system.registry,missing);
    if(r.outcome!=co::Outcome::MissingResourceLoader || r.completed!=2 ||
       r.attempted!=3 || system.owned.definitions.staticCount()!=55 ||
       system.owned.definitions.dynamicCount()!=11 ||
       system.owned.catalog.size()!=0)return 2;
    // Restore a complete synthetic host and one denied allocation in
    // each of the three separately owned record vectors.
    auto& f=system.fixture;
    f.order.clear();f.later_calls.clear();f.definition_calls=0;
    f.missing_static=5;f.missing_dynamic=7;f.missing_catalog=17;
    r=co::Initialize(system.owned,&system.owner,&system.registry,hooks);
    if(!r.succeeded() || r.completed!=7 || r.attempted!=7 ||
       f.invalid_context || f.definition_calls!=54 ||
       system.owned.catalog.size()!=66 ||
       system.owned.definitions.staticAt(5) ||
       system.owned.definitions.dynamicAt(7) ||
       system.owned.catalog.at(17) || f.later_calls.size()!=4 ||
       !Equal(f.order,{"static","dynamic","catalog","resources",
         "scripts/stats/stats_display_layout.txt","StaticStat",
         "DynamicStat","ResetHUD"}))return 3;
    for(std::size_t i=0;i<4;++i)
        if(f.later_calls[i]!=st::kInitSteps[i+3].original_callsite)return 4;
    // Resource enumeration failure means steps 3-7 are not reported
    // complete; the 66 catalog pointers are still safely owned.
    f.order.clear();f.later_calls.clear();f.allow_resources=false;
    r=co::Initialize(system.owned,&system.owner,&system.registry,hooks);
    if(r.outcome!=co::Outcome::ResourceLoaderRefused ||
       r.completed!=2 || r.attempted!=3 ||
       f.later_calls.size() || system.owned.catalog.size()!=66 ||
       !Equal(f.order,{"static","dynamic","catalog","resources"}))return 5;
    // Deny the second registration and ensure third is never invoked.
    f.allow_resources=true;f.reject_later=2;
    f.order.clear();f.later_calls.clear();
    r=co::Initialize(system.owned,&system.owner,&system.registry,hooks);
    if(r.outcome!=co::Outcome::LaterStepRefused ||
       r.completed!=5 || r.attempted!=6 ||
       f.later_calls.size()!=3 ||
       !Equal(f.order,{"static","dynamic","catalog","resources",
         "scripts/stats/stats_display_layout.txt","StaticStat","DynamicStat"}))
        return 6;
    // Retry with no allocation failures: all 132 pointer slots are
    // present, and previous collection state is cleared.
    f.reject_later=-1;
    f.missing_static=f.missing_dynamic=f.missing_catalog=-1;
    f.order.clear();f.later_calls.clear();
    if(!system.Init() || system.latest.completed!=7 ||
       !system.owned.definitions.staticAt(5) ||
       !system.owned.definitions.dynamicAt(7) ||
       !system.owned.catalog.at(17) ||
       f.invalid_context)return 7;
    // Sound/other systems remain separate; only AutoGameSystem's
    // surrounding first-failure semantics are exercised here.
    f.reject_later=0;
    f.order.clear();f.later_calls.clear();
    auto_system::Registry registry;
    registry.DirectRegister(&system);
    if(registry.Initialize().outcome!=auto_system::Outcome::InitRefused ||
       system.latest.outcome!=co::Outcome::LaterStepRefused ||
       system.latest.completed!=3 || system.latest.attempted!=4)return 8;
    std::puts("PASS integrated original Stats 55+11+66 owned records and seven-stage fail-closed Init");
    return 0;
}
