#include "sin_compat/original_shutdown_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::shutdown;
namespace {
std::vector<std::string> trace;
void client_shutdown(void* p){trace.push_back(*static_cast<std::string*>(p));}
}
int main(){
    assert(scene_cache_name=="scene.cache");
    assert(model_sounds_cache_name=="modelsounds.cache");
    assert(scene_cache_expected_version==3 && model_sounds_cache_expected_version==2);
    assert(server_game_cfg_command=="exec game.cfg\n");
    std::string a="A",b="B",c="C";
    std::vector<ClientSystem> systems{{&a,client_shutdown},{&b,client_shutdown},{&c,client_shutdown}};
    std::size_t active=3;
    close_client_systems_reverse(systems,active);
    assert((trace==std::vector<std::string>{"C","B","A"}) && active==0);
    trace.clear();active=2;
    close_client_systems_reverse(systems,active);
    assert((trace==std::vector<std::string>{"B","A"}) && active==0);
    trace.clear();close_client_systems_reverse(systems,active);
    assert(trace.empty());
    for(int scenario=0;scenario<4;++scenario){
        CacheState state{scenario!=0,scenario==1,scenario==2};trace.clear();
        const auto path=close_cache(state,
            [](){trace.push_back("dirty");},
            [](){trace.push_back("check");return false;},
            [](){trace.push_back("fallback");},
            [](){trace.push_back("finish");});
        assert(path==(scenario==0?CacheClosePath::NotStarted:scenario==1?CacheClosePath::Dirty:
                     scenario==2?CacheClosePath::ConditionalCheck:CacheClosePath::SimpleCleanup));
        assert(!state.started);
        if(scenario==0)assert(trace.empty());
        if(scenario==1)assert((trace==std::vector<std::string>{"dirty","finish"}));
        if(scenario==2)assert((trace==std::vector<std::string>{"check","fallback","finish"}));
        if(scenario==3)assert((trace==std::vector<std::string>{"finish"}));
    }
    CacheState state{true,false,true};trace.clear();
    close_cache(state,[](){trace.push_back("dirty");},
      [](){trace.push_back("check");return true;},
      [](){trace.push_back("unwanted");},[](){trace.push_back("finish");});
    assert((trace==std::vector<std::string>{"check","finish"}));
    int instance=1;void* owned=&instance;trace.clear();
    close_server_level(
       [](){trace.push_back("modelsounds");},
       [](){trace.push_back("scene");},
       [](std::size_t i){trace.push_back("item-"+std::to_string(i));},
       [](){trace.push_back("reset");},owned,
       [&](void* p,int flag){assert(p==&instance && flag==1);trace.push_back("release");});
    assert(!owned);
    assert((trace==std::vector<std::string>{"modelsounds","scene","item-0","item-1",
           "item-2","item-3","item-4","reset","release"}));
    trace.clear();
    close_server_level(
       [](){trace.push_back("modelsounds");},[](){trace.push_back("scene");},
       [](std::size_t i){trace.push_back("item-"+std::to_string(i));},
       [](){trace.push_back("reset");},owned,
       [](void*,int){assert(false);});
    assert(trace.size()==8 && !owned);
}
