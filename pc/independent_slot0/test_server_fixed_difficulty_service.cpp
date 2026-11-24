// Load the actual built module. No linking against its implementation.
#include "server_fixed_difficulty_service.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <cstdio>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if defined(_MSC_VER) && defined(_M_IX86)
#include "server_native_cvar_registration.h"
#endif
#else
#include <dlfcn.h>
#endif
namespace a=sin_port::fixed_service;
#if defined(_MSC_VER) && defined(_M_IX86)
namespace console=sin_independent::console_abi;
class SkillEngine final:public console::EngineCvar {
public:
    console::Variable root,alias;
    const a::Api* api=nullptr;
    a::ApplyEngineSkill apply=nullptr;
    mutable unsigned lookups=0;
    bool missing=false;
    SkillEngine(){
        root.CreateVariable("skill","2",0,"",false,0,false,0,nullptr);
        alias.CreateVariable("skill","1",0,"",false,0,false,0,nullptr);
        alias.parent=&root;
    }
    void __thiscall RegisterConCommandBase(console::CommandBase*) noexcept override {assert(false);}
    const char* __thiscall GetCommandLineValue(const char*) noexcept override {assert(false);return nullptr;}
    console::Variable* __thiscall FindMutableVar(const char*) noexcept override {assert(false);return nullptr;}
    const console::Variable* __thiscall FindConstVar(const char* name) const noexcept override {
        assert(name&&std::strcmp(name,"skill")==0);
        ++lookups;
        // A lookup cannot reenter the service and mutate its campaign.
        assert(!api->begin_campaign(3)&&!api->end_campaign()&&!api->set_skill(3));
        assert(!api->observe_trigger(0,0.f,257));
        assert(!apply(const_cast<SkillEngine*>(this),a::BeginFromEngine));
        return missing?nullptr:&alias;
    }
    console::CommandBase* __thiscall GetCommands() noexcept override {assert(false);return nullptr;}
};
static void test_engine_skill(HMODULE lib,const a::Api* api){
    auto apply=reinterpret_cast<a::ApplyEngineSkill>(GetProcAddress(lib,a::engine_skill_export_name));
    assert(apply);
    SkillEngine engine;
    engine.api=api;engine.apply=apply;
    a::State state{};
    assert(!apply(nullptr,a::BeginFromEngine));
    assert(!apply(reinterpret_cast<void*>(1),a::BeginFromEngine));
    assert(!apply(&engine,a::SyncFromEngine)&&!apply(&engine,99)&&engine.lookups==0);
    engine.missing=true;
    assert(!apply(&engine,a::BeginFromEngine)&&!api->get_state(&state));
    engine.missing=false;
    engine.root.SetInt(4);
    assert(!apply(&engine,a::BeginFromEngine)&&!api->get_state(&state));
    engine.root.SetInt(2);
    engine.alias.parent=nullptr;
    assert(!apply(&engine,a::BeginFromEngine));
    engine.alias.parent=&engine.root;
    engine.root.parent=&engine.alias;
    assert(!apply(&engine,a::BeginFromEngine));
    engine.root.parent=&engine.root;
    assert(apply(&engine,a::BeginFromEngine));
    assert(api->get_state(&state)&&state.skill==2&&state.initial_skill==2);
    const auto calls=engine.lookups;
    assert(!apply(&engine,a::BeginFromEngine)&&engine.lookups==calls);
    engine.root.SetInt(3);
    assert(apply(&engine,a::SyncFromEngine));
    assert(api->get_state(&state)&&state.skill==3&&state.revision==1&&state.initial_skill==2);
    float scaled=0;
    assert(api->scale_damage_taken(100.f,0,&scaled)&&scaled==140.f);
    assert(apply(&engine,a::SyncFromEngine));
    assert(api->get_state(&state)&&state.revision==1);
    engine.root.SetInt(0);
    assert(!apply(&engine,a::SyncFromEngine));
    engine.missing=true;
    assert(!apply(&engine,a::SyncFromEngine));
    assert(api->get_state(&state)&&state.skill==3&&state.revision==1);
    assert(engine.root.integer==0&&engine.alias.integer==1);
    assert(api->end_campaign());
    assert(!apply(&engine,a::SyncFromEngine));
    engine.missing=false;
    engine.root.SetInt(1);
    assert(apply(&engine,a::BeginFromEngine));
    assert(api->get_state(&state)&&state.initial_skill==1&&state.revision==0);
    assert(api->end_campaign());
}
#endif
int main(int argc,char** argv){
    assert(argc==2);
#ifdef _WIN32
    auto lib=LoadLibraryA(argv[1]);
    assert(lib);
    assert(GetProcAddress(lib,"CreateInterface"));
    auto get=reinterpret_cast<a::GetApi>(GetProcAddress(lib,a::export_name));
#else
    auto lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    assert(lib);
    auto get=reinterpret_cast<a::GetApi>(dlsym(lib,a::export_name));
#endif
    assert(get);
    assert(!get(0,sizeof(a::Api)));
    assert(!get(a::abi_version,0));
    const auto* api=get(a::abi_version,sizeof(a::Api));
    assert(api&&api->version==1&&api->size==sizeof(a::Api)&&api->metrics==14);
    a::State s{};
    float x=123.f;
    assert(!api->get_state(&s)&&!api->get_metric(0,&x)&&x==123.f);
    assert(!api->begin_campaign(0)&&!api->begin_campaign(4));
    assert(api->begin_campaign(2));
    assert(!api->begin_campaign(1));
    assert(api->get_state(&s)&&s.skill==2&&s.initial_skill==2&&!s.completed&&!s.adaptive_enabled);
    const float expected[]={1.075f,1.f,1.f,0.f,0.4f,0.25f,0.15f,0.15f,
        0.35000002f,0.6f,0.2f,0.f,0.75f,0.5f};
    for(std::uint32_t i=0;i<14;++i){
        assert(api->get_metric(i,&x));
        assert(std::fabs(x-expected[i])<0.00001f);
    }
    assert(!api->get_metric(14,&x)&&!api->get_metric(0,nullptr));
    float snapshot=-1.f;
    assert(api->capture_challenge(&snapshot)&&snapshot==0.5f);
    assert(api->set_skill(3));
    assert(!api->set_skill(-1));
    assert(api->get_state(&s)&&s.skill==3&&s.initial_skill==2&&s.revision==1);
    assert(api->scale_damage_taken(100.f,0,&x)&&x==140.f);
    assert(api->scale_damage_taken(100.f,1,&x)&&x==100.f);
    assert(api->scale_damage_done(25.f,&x)&&x==25.f);
    assert(api->npc_shoot_delay(0.25f,&x)&&std::fabs(x-0.15f)<0.00001f);
    assert(!api->scale_damage_taken(std::numeric_limits<float>::infinity(),0,&x));
    a::ChallengeOutput output{};
    assert(api->query_challenge(a::Initial,0,42,&output));
    assert(output.original_field_offset==0x394&&output.value==0.5f);
    assert(output.activator==42&&output.caller==42);
    assert(api->query_challenge(a::Current,0,42,&output)&&output.value==1.f);
    assert(output.original_field_offset==0x3ac);
    assert(api->query_challenge(a::Difference,snapshot,42,&output)&&output.value==50.f);
    assert(output.original_field_offset==0x3c4);
    assert(!api->query_challenge(a::Difference,0.2f,42,&output));
    assert(!api->query_challenge(9,0,42,&output));
    assert(api->observe_trigger(100,20.f,256));
    assert(api->get_state(&s)&&s.trigger_count==1&&!s.completed&&s.skill==3);
    assert(api->observe_trigger(-100,30.f,257));
    assert(api->get_state(&s)&&s.trigger_count==2&&s.completed&&s.skill==3);
    assert(api->set_skill(1));
    assert(api->scale_damage_taken(100.f,0,&x)&&x==75.f);
    assert(api->query_challenge(a::Difference,snapshot,42,&output)&&output.value==-50.f);
    assert(api->end_campaign()&&!api->end_campaign());
    assert(!api->get_state(&s)&&!api->observe_trigger(1,0.f,1));
    assert(api->begin_campaign(1));
    assert(api->get_state(&s)&&s.skill==1&&s.initial_skill==1&&!s.completed&&!s.trigger_count);
    assert(api->capture_challenge(&snapshot)&&snapshot==0.f);
    assert(api->query_challenge(a::Difference,snapshot,42,&output)&&output.value==0.f);
    assert(api->end_campaign());
#ifdef _WIN32
#if defined(_MSC_VER) && defined(_M_IX86)
    test_engine_skill(lib,api);
#endif
    assert(FreeLibrary(lib));
#else
    assert(dlclose(lib)==0);
#endif
    std::puts("PASS actual loaded module fixed difficulty campaign service");
}
