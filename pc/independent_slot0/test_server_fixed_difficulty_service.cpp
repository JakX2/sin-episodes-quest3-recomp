// Load the actual built module. No linking against its implementation.
#include "server_fixed_difficulty_service.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstring>
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
static void test_env_global(const a::GlobalStateApi* g,a::EnvGlobalDispatch env){
    assert(env&&g->clear()&&g->set_updates_disabled(0));
    char name[64]{},level[64]{};
    std::int32_t value=-1;
    assert(env(99,"x","map_a",1,0)==a::EnvRejected);
    assert(env(a::EnvSpawn,nullptr,"map_a",1,0)==a::EnvRemoveSelf);
    assert(env(a::EnvSpawn,"door","map_a",0,1)==a::EnvUnchanged&&g->count()==0);
    assert(env(a::EnvSpawn,"door","map_a",1,1)==a::EnvApplied);
    assert(env(a::EnvSpawn,"door","map_b",1,0)==a::EnvUnchanged);
    assert(g->count()==1&&g->read(0,name,64,level,64,&value)&&value==1);
    assert(std::strcmp(level,"map_a")==0);
    assert(env(a::EnvToggle,"door","map_b",0,0)==a::EnvApplied);
    assert(g->read(0,name,64,level,64,&value)&&value==0);
    assert(env(a::EnvTurnOn,"door","map_b",0,0)==a::EnvApplied);
    assert(g->read(0,name,64,level,64,&value)&&value==1);
    assert(env(a::EnvRemove,"door","map_b",0,0)==a::EnvApplied);
    assert(env(a::EnvToggle,"door","map_b",0,0)==a::EnvUnchanged);
    assert(g->count()==1&&g->read(0,name,64,level,64,&value)&&value==2);
    assert(std::strcmp(level,"map_a")==0);
    assert(env(a::EnvTurnOff,"door","map_b",0,0)==a::EnvApplied);
    assert(g->read(0,name,64,level,64,&value)&&value==0);
    assert(env(a::EnvToggle,"new","map_b",0,0)==a::EnvApplied);
    assert(g->read(1,name,64,level,64,&value)&&value==1);
    assert(std::strcmp(level,"map_b")==0);
    assert(g->set_updates_disabled(1));
    assert(env(a::EnvTurnOff,"new","map_b",0,0)==a::EnvBlocked);
    assert(g->read(1,name,64,level,64,&value)&&value==1);
    assert(env(a::EnvRemove,"missing","map_b",0,0)==a::EnvApplied);
    assert(g->read(2,name,64,level,64,&value)&&value==2);
    assert(env(a::EnvSpawn,"Door","map_c",1,7)==a::EnvApplied);
    assert(env(a::EnvToggle,"Door","map_c",0,0)==a::EnvUnchanged);
    assert(g->read(3,name,64,level,64,&value)&&value==7);
    assert(env(a::EnvTurnOff,nullptr,nullptr,0,0)==a::EnvApplied);
    assert(g->read(4,name,64,level,64,&value)&&!*name&&!*level&&value==0);
    assert(g->set_updates_disabled(0)&&g->clear());
}
static void test_global_state(const a::GlobalStateApi* g){
    assert(g&&g->version==1&&g->size==sizeof(*g));
    assert(g->clear()&&g->count()==0&&g->find("door")==-1);
    assert(g->add(nullptr,"map_a",1)==-1);
    assert(g->add("door","map_a",1)==0);
    assert(g->add("door","map_b",2)==0&&g->count()==1);
    assert(g->add("Door","map_b",2)==1&&g->count()==2);
    assert(g->find("door")==0&&g->find("Door")==1);
    char name[32]="unchanged",level[32]="unchanged";
    std::int32_t value=-99;
    assert(!g->read(0,name,2,level,32,&value));
    assert(std::strcmp(name,"unchanged")==0&&std::strcmp(level,"unchanged")==0&&value==-99);
    assert(g->read(0,name,32,level,32,&value)&&value==1);
    assert(std::strcmp(name,"door")==0&&std::strcmp(level,"map_a")==0);
    assert(g->set_updates_disabled(1)&&!g->set_state(0,3));
    assert(g->set_level(0,"map_c")&&g->read(0,name,32,level,32,&value)&&value==1);
    assert(std::strcmp(level,"map_c")==0);
    assert(g->set_level(0,nullptr)&&g->read(0,name,32,level,32,&value)&&!*level);
    assert(!g->set_updates_disabled(2)&&!g->set_state(0,3));
    assert(g->set_updates_disabled(0)&&g->set_state(0,3));
    assert(g->read(0,name,32,level,32,&value)&&value==3);
    assert(!g->set_state(-1,0)&&!g->set_level(2,"map"));
    assert(g->set_updates_disabled(1)&&g->clear()&&g->count()==0);
    assert(g->add("fresh","map_d",0)==0&&!g->set_state(0,1));
    assert(g->set_updates_disabled(0)&&g->clear()&&g->clear());
    assert(g->count()==0&&g->find("door")==-1);
}
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
    assert(api->get_state(&state)&&state.initial_skill==1&&state.revision==1&&!state.trigger_count&&!state.completed);
    assert(api->end_campaign());
}

// The original IVEngineServer command slots are 36 and 37. A native
// synthetic vtable with distinct preceding methods catches slot drift.
class ConfigEngine {
public:
#define SIN_CFG_UNUSED(n) virtual void __thiscall Unused##n(){assert(false);}
    SIN_CFG_UNUSED(0)
    SIN_CFG_UNUSED(1)
    SIN_CFG_UNUSED(2)
    SIN_CFG_UNUSED(3)
    SIN_CFG_UNUSED(4)
    SIN_CFG_UNUSED(5)
    SIN_CFG_UNUSED(6)
    SIN_CFG_UNUSED(7)
    SIN_CFG_UNUSED(8)
    SIN_CFG_UNUSED(9)
    SIN_CFG_UNUSED(10)
    SIN_CFG_UNUSED(11)
    SIN_CFG_UNUSED(12)
    SIN_CFG_UNUSED(13)
    SIN_CFG_UNUSED(14)
    SIN_CFG_UNUSED(15)
    SIN_CFG_UNUSED(16)
    SIN_CFG_UNUSED(17)
    SIN_CFG_UNUSED(18)
    SIN_CFG_UNUSED(19)
    SIN_CFG_UNUSED(20)
    SIN_CFG_UNUSED(21)
    SIN_CFG_UNUSED(22)
    SIN_CFG_UNUSED(23)
    SIN_CFG_UNUSED(24)
    SIN_CFG_UNUSED(25)
    SIN_CFG_UNUSED(26)
    SIN_CFG_UNUSED(27)
    SIN_CFG_UNUSED(28)
    SIN_CFG_UNUSED(29)
    SIN_CFG_UNUSED(30)
    SIN_CFG_UNUSED(31)
    SIN_CFG_UNUSED(32)
    SIN_CFG_UNUSED(33)
    SIN_CFG_UNUSED(34)
    SIN_CFG_UNUSED(35)
#undef SIN_CFG_UNUSED
    virtual void __thiscall Queue(const char* text){
        assert(text&&std::strcmp(text,"exec game.cfg\n")==0);
        assert(stage==0&&skill->lookups==before);
        assert(globals&&globals->count()==0&&!globals->clear());
        assert(!api->begin_campaign(1)&&!api->end_campaign());
        assert(start(this,skill)==a::ConfigRejected);
        stage=1;
        if(fail==1)throw 1;
    }
    virtual void __thiscall Execute(){
        assert(stage==1&&skill->lookups==before);
        assert(!api->begin_campaign(1)&&!api->set_skill(1));
        stage=2;
        skill->root.SetInt(configured);
        if(fail==2)throw 2;
    }
    const a::GlobalStateApi* globals=nullptr;
    SkillEngine* skill=nullptr;
    const a::Api* api=nullptr;
    a::StartConfiguredCampaign start=nullptr;
    unsigned stage=0,before=0,fail=0;
    int configured=3;
};
static void test_configured_campaign(HMODULE lib,const a::Api* api,const a::GlobalStateApi* globals){
    auto start=reinterpret_cast<a::StartConfiguredCampaign>(
        GetProcAddress(lib,a::configured_campaign_export_name));
    auto apply=reinterpret_cast<a::ApplyEngineSkill>(
        GetProcAddress(lib,a::engine_skill_export_name));
    assert(start&&apply);
    SkillEngine skill;
    skill.api=api;skill.apply=apply;
    ConfigEngine server;
    server.skill=&skill;server.api=api;server.start=start;server.globals=globals;
    assert(globals->add("old_global","old_map",1)==0);
    a::State state{};
    assert(start(nullptr,&skill)==a::ConfigRejected);
    assert(start(&server,nullptr)==a::ConfigRejected);
    assert(start(&server,reinterpret_cast<void*>(1))==a::ConfigRejected);
    assert(start(reinterpret_cast<void*>(1),&skill)==a::ConfigRejected);
    assert(!server.stage&&!skill.lookups&&globals->count()==1);
    assert(api->begin_campaign(1));
    assert(start(&server,&skill)==a::ConfigRejected&&!server.stage);
    assert(api->end_campaign());
    assert(globals->count()==1);
    server.fail=1;
    assert(start(&server,&skill)==a::ConfigQueueUncertain);
    assert(server.stage==1&&!skill.lookups&&!api->get_state(&state)&&globals->count()==0);
    server.stage=0;server.fail=2;
    assert(start(&server,&skill)==a::ConfigExecuteUncertain);
    assert(server.stage==2&&!skill.lookups&&!api->get_state(&state));
    server.stage=0;server.fail=0;server.configured=4;
    assert(start(&server,&skill)==a::ConfigRanSkillUnavailable);
    assert(server.stage==2&&skill.lookups==1&&!api->get_state(&state));
    server.stage=0;server.before=skill.lookups;server.configured=2;skill.missing=true;
    assert(start(&server,&skill)==a::ConfigRanSkillUnavailable);
    assert(server.stage==2&&skill.lookups==2&&!api->get_state(&state));
    server.stage=0;server.before=skill.lookups;server.configured=3;skill.missing=false;
    skill.root.SetInt(1);
    assert(start(&server,&skill)==a::ConfigStarted);
    assert(server.stage==2&&skill.lookups==3);
    assert(api->get_state(&state)&&state.skill==3&&state.initial_skill==3);
    assert(!state.trigger_count&&!state.completed&&!state.adaptive_enabled);
    float damage=0;
    assert(api->scale_damage_taken(100.f,0,&damage)&&damage==140.f);
    assert(start(&server,&skill)==a::ConfigRejected&&skill.lookups==3);
    skill.root.SetInt(2);
    assert(apply(&skill,a::SyncFromEngine));
    assert(api->get_state(&state)&&state.skill==2&&state.initial_skill==3);
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
#ifdef _WIN32
    auto getGlobals=reinterpret_cast<a::GetGlobalStateApi>(GetProcAddress(lib,a::global_state_export_name));
#else
    auto getGlobals=reinterpret_cast<a::GetGlobalStateApi>(dlsym(lib,a::global_state_export_name));
#endif
    assert(getGlobals&&!getGlobals(0,sizeof(a::GlobalStateApi))&&!getGlobals(1,0));
    const auto* globals=getGlobals(a::global_state_version,sizeof(a::GlobalStateApi));
    test_global_state(globals);
#ifdef _WIN32
    auto env=reinterpret_cast<a::EnvGlobalDispatch>(GetProcAddress(lib,a::env_global_export_name));
#else
    auto env=reinterpret_cast<a::EnvGlobalDispatch>(dlsym(lib,a::env_global_export_name));
#endif
    test_env_global(globals,env);
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
    test_configured_campaign(lib,api,globals);
#endif
    assert(FreeLibrary(lib));
#else
    assert(dlclose(lib)==0);
#endif
    std::puts("PASS actual loaded module fixed difficulty campaign service");
}
