#include "client_dynamic_difficulty_init.h"
#include "client_auto_gamesystem_init_core.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
namespace dd=sin_independent::dynamic_difficulty;
namespace game=sin_independent::auto_gamesystem;
struct Fixture {
    std::vector<dd::CallbackBinding> received;
    bool callbackAttempted=false;
};
static void Register(void* context,const dd::CallbackBinding& entry) noexcept {
    auto& fixture=*static_cast<Fixture*>(context);
    fixture.received.push_back(entry);
    fixture.callbackAttempted=true;
}
struct System:game::System {
    Fixture* registry=nullptr;
    bool Init() noexcept override {return dd::Init(registry,&Register);}
};
int main() {
    Fixture f;
    if(dd::Init(nullptr,&Register) || dd::Init(&f,nullptr) ||
       f.callbackAttempted)return 1;
    if(!dd::Init(&f,&Register) ||
       f.received.size()!=11)return 2;
    for(std::size_t i=0;i<dd::kInitBindings.size();++i) {
        const auto& a=dd::kInitBindings[i];
        const auto& b=f.received[i];
        if(std::strcmp(a.name,b.name) || a.original_callback_va!=b.original_callback_va ||
           a.original_name_va!=b.original_name_va ||
           a.original_callsite_va!=b.original_callsite_va ||
           (i && a.original_callsite_va!=
                    dd::kInitBindings[i-1].original_callsite_va+0x15))
            return 3;
    }
    if(std::strcmp(dd::kInitBindings.front().name,"GamePlayVarValue") ||
       std::strcmp(dd::kInitBindings.back().name,"Cheating"))
        return 4;
    // No once flag exists in this native Init function. A second call
    // must repeat the eleven exact registry callbacks in the same order.
    if(!dd::Init(&f,&Register) || f.received.size()!=22)return 5;
    // Integrate with a real reconstructed auto game-system Init pass.
    Fixture other;
    System system;system.registry=&other;
    game::Registry autoSystems;autoSystems.DirectRegister(&system);
    const auto result=autoSystems.Initialize();
    if(!result.succeeded() || result.initialized_count!=1 ||
       other.received.size()!=11)return 6;
    system.registry=nullptr;
    game::Registry missing;missing.DirectRegister(&system);
    if(missing.Initialize().outcome!=game::Outcome::InitRefused)return 7;
    std::puts("PASS original C_DynamicDifficulty::Init 11 named callbacks and first-failure gate");
    return 0;
}
