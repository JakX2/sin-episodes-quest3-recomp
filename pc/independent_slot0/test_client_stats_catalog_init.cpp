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
    std::puts("PASS Stats original 66 x 12 catalog reset/nullable allocation and unknown resource callback boundary");
    return 0;
}
