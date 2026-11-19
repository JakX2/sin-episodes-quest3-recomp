// Native x86 independently owned original 66 x 12 byte Stats catalog.
#include "client_stats_catalog_init.h"
#include "client_auto_gamesystem_init_core.h"
#include <cstdio>
#include <type_traits>
namespace stats=sin_independent::stats_catalog;
namespace gs=sin_independent::auto_gamesystem;
struct Host {
    int deniedIndex=-1;
    int allocations=0;
    int continuations=0;
    bool shouldContinue=true;
    bool badState=false;
};
static bool Allow(void* ctx,std::size_t index) noexcept {
    ++static_cast<Host*>(ctx)->allocations;
    return static_cast<int>(index)!=static_cast<Host*>(ctx)->deniedIndex;
}
static bool Continue(void* ctx,stats::Catalog& catalog) noexcept {
    auto& h=*static_cast<Host*>(ctx);
    ++h.continuations;
    if(catalog.size()!=66)h.badState=true;
    for(std::size_t i=0;i<catalog.size();++i){
        auto* x=catalog.at(i);
        if(static_cast<int>(i)==h.deniedIndex){if(x)h.badState=true;}
        else if(!x || x->first || x->second || x->identifier!=-1)
            h.badState=true;
    }
    return h.shouldContinue;
}

struct NestedHost {
    stats::Catalog* nested=nullptr;
    int calls=0;
    bool nestedOk=false;
};
static bool NestedContinue(void* ctx,stats::Catalog& outer) noexcept {
    auto& h=*static_cast<NestedHost*>(ctx);
    ++h.calls;
    if(!h.nested || &outer==h.nested || outer.size()!=66)return false;
    Host inner{};
    inner.deniedIndex=31;
    h.nestedOk=stats::BeginOwnerSetup(*h.nested,&inner,&Continue,&Allow) &&
        !inner.badState && inner.allocations==66 && inner.continuations==1 &&
        h.nested->at(31)==nullptr && h.nested->at(30)!=nullptr &&
        outer.at(31)!=nullptr;
    return h.nestedOk;
}

struct GameSystem:gs::System {
    stats::Catalog catalog;
    Host host;
    bool Init() noexcept override {
        return stats::BeginOwnerSetup(catalog,&host,&Continue,&Allow);
    }
};
int main(){
    static_assert(sizeof(void*)==4,"original PE32 owner record layout");
    static_assert(sizeof(stats::Entry)==12,"original catalog node size");
    static_assert(!std::is_copy_constructible<stats::Catalog>::value,
                  "catalog published record storage must stay owned");
    GameSystem system;
    if(system.catalog.size()!=0 ||
       stats::BeginOwnerSetup(system.catalog,nullptr,&Continue,&Allow) ||
       stats::BeginOwnerSetup(system.catalog,&system.host,nullptr,&Allow) ||
       system.host.continuations || system.catalog.size())return 1;
    system.host.deniedIndex=17;
    if(!system.Init() || system.host.badState ||
       system.host.allocations!=66 || system.host.continuations!=1 ||
       system.catalog.at(17) || system.catalog.at(66))return 2;
    // Independent entries have individually stable ownership, and
    // resetting rebuilds their values from scratch.
    system.host.deniedIndex=-1;
    system.catalog.mutableAt(0)->first=42;
    system.catalog.mutableAt(1)->identifier=123;
    if(!system.Init() || system.host.badState ||
       system.host.allocations!=132 || system.host.continuations!=2 ||
       !system.catalog.at(17) ||
       system.catalog.at(0)->first || system.catalog.at(1)->identifier!=-1)
        return 3;
    // The external resource loading continuation can fail safely;
    // native original function has no bool return, so this is a host
    // safety boundary, not original failure propagation.
    system.host.shouldContinue=false;
    gs::Registry outer;outer.DirectRegister(&system);
    if(outer.Initialize().outcome!=gs::Outcome::InitRefused ||
       system.host.continuations!=3)return 4;
    // Exhaustive synthetic fault injection: every possible single failed
    // allocation must retain all 66 logical slots and reach the continuation.
    // This is host behavior, not a claim about the proprietary resource ABI.
    for(int denied=0;denied<66;++denied){
        GameSystem trial;
        trial.host.deniedIndex=denied;
        if(!trial.Init() || trial.host.badState ||
           trial.host.allocations!=66 || trial.host.continuations!=1 ||
           trial.catalog.size()!=66 || trial.catalog.at(denied))return 5;
        for(int i=0;i<66;++i){
            auto* entry=trial.catalog.at(i);
            if(i!=denied && (!entry || entry->identifier!=-1 ||
                             entry->first!=0 || entry->second!=0))return 6;
        }
        // A second pass must replace the missing slot and reset all values.
        trial.host.deniedIndex=-1;
        if(!trial.Init() || trial.host.badState ||
           trial.host.allocations!=132 || trial.host.continuations!=2 ||
           !trial.catalog.at(denied))return 7;
    }
    // A refused resource continuation must never be mistaken for a
    // successful AutoGameSystem initialization. Reinitialization can recover.
    GameSystem recovering;
    recovering.host.shouldContinue=false;
    gs::Registry registry;
    registry.DirectRegister(&recovering);
    if(registry.Initialize().outcome!=gs::Outcome::InitRefused ||
       recovering.host.continuations!=1 ||
       recovering.catalog.size()!=66)return 8;
    recovering.host.shouldContinue=true;
    gs::Registry retry;
    retry.DirectRegister(&recovering);
    if(retry.Initialize().outcome!=gs::Outcome::Initialized ||
       recovering.host.continuations!=2 ||
       recovering.catalog.size()!=66 ||
       recovering.host.badState)return 9;

    // Independent client-side catalog owners must not share allocation
    // state or record pointers, including nested resource continuations.
    stats::Catalog outerCatalog;
    stats::Catalog nestedCatalog;
    NestedHost nestedHost{&nestedCatalog};
    if(!stats::BeginOwnerSetup(outerCatalog,&nestedHost,&NestedContinue) ||
       !nestedHost.nestedOk || nestedHost.calls!=1 ||
       outerCatalog.size()!=66 || nestedCatalog.size()!=66)return 10;
    for(std::size_t i=0;i<66;++i){
        if(i==31)continue;
        if(!outerCatalog.at(i) || !nestedCatalog.at(i) ||
           outerCatalog.at(i)==nestedCatalog.at(i))return 11;
    }
    outerCatalog.mutableAt(31)->identifier=1234;
    if(nestedCatalog.at(31) || outerCatalog.at(30)->identifier!=-1)return 12;
    // Rebuilding one owner must not invalidate or modify another owner.
    auto* saved=nestedCatalog.at(30);
    outerCatalog.Rebuild();
    if(nestedCatalog.at(30)!=saved || nestedCatalog.at(31) ||
       !outerCatalog.at(31) || outerCatalog.at(31)->identifier!=-1)return 13;
    std::puts("PASS Stats original 66 x 12 catalog reset/nullable allocation and unknown resource callback boundary");
    return 0;
}
