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
        Action::PrecacheDecalTexture,Action::ConfigureVector,
        Action::ConfigureRadius,Action::ConfigureBias};
    if(refresh)expected.push_back(Action::ConditionalRefresh);
    expected.push_back(Action::RegisterEngineCallback);
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
struct System:gs::System {
    std::array<unsigned char,0xdc> storage{};
    Capture capture;
    bool condition=false;
    bool Init() noexcept override {
        return shadow::Init(storage.data(),storage.size(),
                            condition,&capture,&Record);
    }
};
int main(){
    static_assert(sizeof(void*)==4,"original engine uses MSVC x86");
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
