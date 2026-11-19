// Independent exact sequence test, no original game DLL loaded.
#include "client_stats_system_init.h"
#include "client_auto_gamesystem_init_core.h"
#include <cstdio>
#include <cstring>
#include <vector>
namespace stats=sin_independent::stats_system;
namespace gs=sin_independent::auto_gamesystem;
struct Calls {
    std::vector<stats::Step> records;
    int* owner=nullptr;
    int* registry=nullptr;
    bool wrong_context=false;
};
static Calls* current=nullptr;
static void Collect(void* ctx,const stats::Step& step) noexcept {
    if((step.kind==stats::StepKind::RegisterCallback && ctx!=current->registry) ||
       (step.kind!=stats::StepKind::RegisterCallback && ctx!=current->owner))
        current->wrong_context=true;
    current->records.push_back(step);
}
struct System : gs::System {
    int own=1,reg=2;
    bool fail=false;
    bool Init() noexcept override {
        return !fail && stats::Init(&own,&reg,&Collect);
    }
};
int main(){
    int owner=1,registry=2;
    Calls calls;current=&calls;calls.owner=&owner;calls.registry=&registry;
    if(stats::Init(nullptr,&registry,&Collect) ||
       stats::Init(&owner,nullptr,&Collect) ||
       stats::Init(&owner,&registry,nullptr) ||
       !calls.records.empty())return 1;
    if(!stats::Init(&owner,&registry,&Collect) ||
       calls.records.size()!=7 || calls.wrong_context)return 2;
    for(std::size_t i=0;i<7;++i) {
        const auto& expected=stats::kInitSteps[i];
        const auto& actual=calls.records[i];
        if(expected.kind!=actual.kind ||
           expected.original_callsite!=actual.original_callsite ||
           expected.original_callee!=actual.original_callee ||
           expected.original_argument_va!=actual.original_argument_va ||
           expected.original_callback_va!=actual.original_callback_va ||
           expected.original_this_offset!=actual.original_this_offset ||
           ((expected.argument==nullptr)!=(actual.argument==nullptr)) ||
           (expected.argument && std::strcmp(expected.argument,actual.argument)))return 3;
        if(i && stats::kInitSteps[i-1].original_callsite>=expected.original_callsite)
            return 4;
    }
    if(std::strcmp(calls.records[3].argument,
                   "scripts/stats/stats_display_layout.txt") ||
       std::strcmp(calls.records[4].argument,"StaticStat") ||
       std::strcmp(calls.records[5].argument,"DynamicStat") ||
       std::strcmp(calls.records[6].argument,"ResetHUD"))return 5;
    // Native function has no guard: a subsequent Init repeats sequence.
    calls.records.clear();
    if(!stats::Init(&owner,&registry,&Collect) ||
       calls.records.size()!=7)return 6;
    // Its outer game-system first-failure gate can still be exercised.
    System system;current=&calls;calls.owner=&system.own;calls.registry=&system.reg;
    calls.records.clear();
    gs::Registry registryOwner;registryOwner.DirectRegister(&system);
    if(!registryOwner.Initialize().succeeded() ||
       calls.records.size()!=7 || calls.wrong_context)return 7;
    // Rejecting the StatsSystem must prevent the next system from running.
    struct LaterSystem : gs::System {
        int calls=0;
        bool Init() noexcept override { ++calls; return true; }
    } later;
    system.fail=true;
    gs::Registry reject;reject.DirectRegister(&system);
    if(reject.Initialize().outcome!=gs::Outcome::InitRefused)return 8;
    std::puts("PASS C_StatsSystem seven original Init callsites and AutoGameSystem gate");
    return 0;
}
