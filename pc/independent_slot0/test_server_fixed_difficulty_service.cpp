// Load the actual built module. No linking against its implementation.
#include "server_fixed_difficulty_service.h"
#include "original_ammo_definitions.h"
#include "server_native_map_host.h"
#include <vector>
#include <utility>
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <iterator>
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

struct MapOutputFixture {
    const a::MapEntityApi* map;
    const a::Api* api;
    const a::GlobalStateApi* globals;
    a::MapEntityRef expected;
    const char* name;
    std::uint32_t offset;
    float value;
    int calls=0,mode=0;
    static int SIN_FIXED_CALL emit(void* context,const a::MapEntityRef* ref,
                                  const char* name,std::uint32_t offset,float value){
        auto& f=*static_cast<MapOutputFixture*>(context);
        assert(ref&&ref->epoch==f.expected.epoch&&ref->index==f.expected.index&&ref->generation==f.expected.generation);
        assert(std::strcmp(name,f.name)==0&&offset==f.offset&&value==f.value);
        ++f.calls;
        assert(!f.map->remove(*ref)&&!f.map->end_level(ref->epoch));
        assert(!f.api->end_campaign()&&!f.api->set_skill(1)&&!f.globals->clear());
        if(f.mode==2)throw 7;
        return f.mode==0?1:0;
    }
};
static void test_map_entities(const a::MapEntityApi* m,const a::Api* api,const a::GlobalStateApi* g){
    assert(m&&m->version==1&&m->size==sizeof(*m));
    assert(!m->begin_level("map_a")&&g->clear());
    assert(api->begin_campaign(2));
    assert(!m->begin_level("")&&!m->begin_level(nullptr));
    const auto e=m->begin_level("map_a");
    assert(e&&!m->begin_level("map_b")&&!api->end_campaign()&&m->count(e)==0);
    a::MapEntityRef door{e,10,1}, challenge{e,11,1}, bad{e,12,1};
    assert(m->create(door,"env_global")==a::MapApplied);
    assert(m->create({e,10,2},"env_global")==a::MapRejected);
    assert(m->create(bad,"npc_unknown")==a::MapUnknownClass);
    assert(m->input(door,"TurnOn",nullptr,nullptr)==a::MapRejected);
    assert(m->keyvalue(door,"targetname","door_controller")==a::MapUnhandledKey);
    assert(m->keyvalue(door,"globalstate","door")==a::MapApplied);
    assert(m->keyvalue(door,"initialstate","2")==a::MapApplied);
    assert(m->keyvalue(door,"initialstate","1")==a::MapApplied);
    assert(m->keyvalue(door,"initialstate","9x")==a::MapRejected);
    assert(m->keyvalue(door,"spawnflags","4294967296")==a::MapRejected);
    assert(m->keyvalue(door,"spawnflags","1")==a::MapApplied);
    assert(m->spawn(door)==a::MapApplied&&m->spawn(door)==a::MapRejected);
    assert(m->keyvalue(door,"globalstate","other")==a::MapRejected);
    char name[32]{},map[32]{};
    std::int32_t value=-1;
    assert(g->read(0,name,32,map,32,&value)&&value==1&&std::strcmp(map,"map_a")==0);
    assert(m->input(door,"Toggle",nullptr,nullptr)==a::MapApplied);
    assert(g->read(0,name,32,map,32,&value)&&value==0);
    assert(m->input(door,"GetCurrentChallenge",nullptr,nullptr)==a::MapUnhandledInput);
    assert(m->create(bad,"env_global")==a::MapApplied);
    assert(m->spawn(bad)==a::MapRemoveSelf&&m->spawn(bad)==a::MapRejected);
    assert(m->input(bad,"TurnOn",nullptr,nullptr)==a::MapRejected);
    assert(m->remove(bad)&&!m->remove(bad));
    assert(m->create(bad,"env_global")==a::MapRejected);
    assert(m->create({e,12,2},"env_global")==a::MapApplied);
    assert(m->create(challenge,"info_challenge")==a::MapApplied);
    assert(m->keyvalue(challenge,"OnCurrentChallenge","host output")==a::MapUnhandledKey);
    assert(m->spawn(challenge)==a::MapApplied&&m->count(e)==3);
    assert(api->set_skill(3));
    MapOutputFixture f{m,api,g,challenge,"OnChallengeDifference",0x3c4,50.f};
    assert(m->input(challenge,"GetChallengeDifference",nullptr,nullptr)==a::MapRejected);
    assert(m->input(challenge,"GetChallengeDifference",&MapOutputFixture::emit,&f)==a::MapApplied&&f.calls==1);
    f.mode=1;
    assert(m->input(challenge,"GetChallengeDifference",&MapOutputFixture::emit,&f)==a::MapOutputFailed);
    f.mode=2;
    assert(m->input(challenge,"GetChallengeDifference",&MapOutputFixture::emit,&f)==a::MapOutputFailed);
    f.mode=0;f.name="OnInitialChallenge";f.offset=0x394;f.value=0.5f;
    assert(m->input(challenge,"GetInitialChallenge",&MapOutputFixture::emit,&f)==a::MapApplied&&f.calls==4);
    assert(m->end_level(e)&&m->count(e)==-1&&g->count()==1);
    const auto e2=m->begin_level("map_b");
    assert(e2&&e2!=e&&!m->end_level(e));
    assert(m->input(door,"TurnOn",nullptr,nullptr)==a::MapRejected);
    a::MapEntityRef next{e2,10,1};
    assert(m->create(next,"env_global")==a::MapApplied);
    assert(m->keyvalue(next,"globalstate","door")==a::MapApplied);
    assert(m->keyvalue(next,"spawnflags","1")==a::MapApplied);
    assert(m->spawn(next)==a::MapApplied&&g->count()==1);
    assert(g->read(0,name,32,map,32,&value)&&value==0&&std::strcmp(map,"map_a")==0);
    assert(m->input(next,"TurnOn",nullptr,nullptr)==a::MapApplied);
    assert(m->end_level(e2)&&api->end_campaign()&&g->clear());
}


namespace sm=sin_compat::map;
using sin_compat::EntityRef;
class BspShellHost final:public sm::ISourceMapEntityHost {
public:
    std::map<std::uint32_t,EntityRef> live;
    std::vector<std::string> events;
    std::vector<std::pair<std::string,std::string>> base_keys;
    std::uint32_t next=1;
    bool fail_remove=false,allow_triggers=false;
    bool is_live(EntityRef e) const override {
        const auto it=live.find(e.index);
        return it!=live.end()&&it->second==e;
    }
    bool create_entity(std::string_view classname,EntityRef& e) override {
        if(classname!="worldspawn"&&classname!="env_global"&&classname!="info_challenge"&&
           classname!="weapon_magnum"&&classname!="weapon_assault_rifle"&&classname!="weapon_scattergun"&&
           !(allow_triggers&&(classname=="trigger_dynamic_difficulty"||classname=="trigger_secret")))return false;
        e={next++,1};
        live.emplace(e.index,e);
        events.push_back("create:"+std::string(classname));
        return true;
    }
    bool key_value(EntityRef e,std::string_view k,std::string_view v) override {
        if(!is_live(e))return false;
        base_keys.emplace_back(k,v);
        return true;
    }
    bool dispatch_spawn(EntityRef e) override {
        events.push_back("spawn:"+std::to_string(e.index));return is_live(e);
    }
    bool activate(EntityRef e) override {
        events.push_back("activate:"+std::to_string(e.index));return is_live(e);
    }
    bool remove_entity(EntityRef e) override {
        if(!is_live(e)||fail_remove)return false;
        events.push_back("remove:"+std::to_string(e.index));
        live.erase(e.index);return true;
    }
};
static std::vector<std::uint8_t> make_native_bsp(std::string_view entity_text){
    constexpr std::size_t header=1036;
    std::vector<std::uint8_t> bytes(header+entity_text.size(),0);
    std::memcpy(bytes.data(),"VBSP",4);
    const auto put=[&](std::size_t offset,std::uint32_t value){
        for(unsigned i=0;i<4;++i)bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
    };
    put(4,19);put(8,header);put(12,static_cast<std::uint32_t>(entity_text.size()));
    std::memcpy(bytes.data()+header,entity_text.data(),entity_text.size());
    return bytes;
}
static void test_native_bsp_host(const a::MapEntityApi* m,const a::Api* api,const a::GlobalStateApi* g){
    assert(api->begin_campaign(2)&&g->clear());
    const auto epoch=m->begin_level("native_bsp");
    assert(epoch);
    BspShellHost shell;
    sin_port::map_host::NativeBehaviorHost host(shell,*m,epoch);
    sm::SourceMapRuntime runtime(host);
    const auto bsp=make_native_bsp(R"KV(
{"classname" "worldspawn" "message" "fixture"}
{"classname" "env_global" "targetname" "door_controller" "globalstate" "door"
 "initialstate" "0" "initialstate" "1" "spawnflags" "1"}
{"classname" "info_challenge" "targetname" "report"
 "OnCurrentChallenge" "door_controller,TurnOff,,0,-1"}
)KV");
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok()&&loaded.entities_created==3&&loaded.entities_spawned==3&&loaded.entities_activated==3);
    assert(loaded.key_values_applied==8&&host.attached_count()==2&&m->count(epoch)==2);
    assert(shell.base_keys.size()==4);
    assert(shell.base_keys[0].first=="message"&&shell.base_keys[1].second=="door_controller");
    assert(shell.base_keys[2].second=="report"&&shell.base_keys[3].first=="OnCurrentChallenge");
    assert(shell.base_keys[3].second=="door_controller,TurnOff,,0,-1");
    char name[32]{},map[32]{};
    std::int32_t state=-1;
    assert(g->read(0,name,32,map,32,&state)&&state==1&&std::strcmp(map,"native_bsp")==0);
    const auto world=runtime.entities()[0],door=runtime.entities()[1],challenge=runtime.entities()[2];
    assert(host.input(world,"TurnOn")==a::MapUnhandledInput);
    assert(api->set_skill(3));
    MapOutputFixture f{m,api,g,{epoch,challenge.index,challenge.generation},"OnCurrentChallenge",0x3ac,1.f};
    assert(host.input(challenge,"GetCurrentChallenge",&MapOutputFixture::emit,&f)==a::MapApplied&&f.calls==1);
    // Model the engine delivering a queued connection after callback return.
    // This is an explicit fixture dispatch, not a reconstructed Source queue.
    assert(host.input(door,"TurnOff")==a::MapApplied);
    assert(g->read(0,name,32,map,32,&state)&&state==0);
    shell.fail_remove=true;
    assert(runtime.unload().code==sm::SourceMapLoadCode::PartialUnload&&host.attached_count()==2);
    shell.fail_remove=false;
    assert(runtime.unload().code==sm::SourceMapLoadCode::Unloaded&&host.attached_count()==0);
    assert(m->count(epoch)==0&&host.input(door,"TurnOn")==a::MapRejected&&g->count()==1);
    assert(m->end_level(epoch));
    const auto epoch2=m->begin_level("invalid_bsp");
    assert(epoch2);
    sin_port::map_host::NativeBehaviorHost host2(shell,*m,epoch2);
    sm::SourceMapRuntime runtime2(host2);
    const auto bad=make_native_bsp(R"KV({"classname" "env_global" "globalstate" "bad" "initialstate" "junk"})KV");
    assert(runtime2.load_bsp(bad.data(),bad.size()).code==sm::SourceMapLoadCode::HostKeyValueFailed);
    assert(shell.live.empty()&&host2.attached_count()==0&&m->count(epoch2)==0&&g->count()==1);
    const auto noName=make_native_bsp(R"KV({"classname" "env_global"})KV");
    assert(runtime2.load_bsp(noName.data(),noName.size()).code==sm::SourceMapLoadCode::HostSpawnFailed);
    assert(shell.live.empty()&&host2.attached_count()==0&&m->count(epoch2)==0);
    const auto unknown=make_native_bsp(R"KV({"classname" "npc_unimplemented"})KV");
    assert(runtime2.load_bsp(unknown.data(),unknown.size()).code==sm::SourceMapLoadCode::HostCreateFailed);
    assert(shell.live.empty()&&m->count(epoch2)==0&&m->end_level(epoch2));
    assert(api->end_campaign()&&g->clear());
}


struct NativeTriggerFixture {
    const a::MapEntityApi* map;
    const a::Api* api;
    const a::CampaignTriggerApi* triggers;
    a::MapEntityRef current,player,grunt;
    int bases=0,warnings=0,specials=0,events=0,removals=0;
    bool event_ok=true,remove_ok=true,special_ok=true;
    std::uint32_t expected_kind=a::TriggerDynamicEvent;
    int expected_descriptor=0,expected_decision=900,expected_owner=5;
    float expected_value=88.5f;
    const char* expected_name="gate";
    a::PublishCampaignEvent native_publish=nullptr;
    void* native_manager=nullptr;
    static NativeTriggerFixture& self(void* c){return *static_cast<NativeTriggerFixture*>(c);}
    static bool same(a::MapEntityRef x,a::MapEntityRef y){
        return x.epoch==y.epoch&&x.index==y.index&&x.generation==y.generation;
    }
    static int SIN_FIXED_CALL live(void* c,a::MapEntityRef e){
        auto& f=self(c);return same(e,f.current)||same(e,f.player)||same(e,f.grunt);
    }
    static int SIN_FIXED_CALL is_player(void* c,a::MapEntityRef e){return same(e,self(c).player);}
    static int SIN_FIXED_CALL base(void* c,a::MapEntityRef e){
        auto& f=self(c);assert(same(e,f.current));++f.bases;return 1;
    }
    static void SIN_FIXED_CALL warn(void* c,const char* text){
        auto& f=self(c);assert(std::strcmp(text,"DD Trigger with don't destroy set\n")==0);++f.warnings;
    }
    static int SIN_FIXED_CALL owner(void*){return 5;}
    static int SIN_FIXED_CALL special(void* c,a::MapEntityRef e){
        auto& f=self(c);assert(same(e,f.current));++f.specials;return f.special_ok;
    }
    static int SIN_FIXED_CALL publish(void* c,a::MapEntityRef e,const a::NativeTriggerEvent* event){
        auto& f=self(c);assert(same(e,f.current)&&event);
        ++f.events;
        assert(event->kind==f.expected_kind&&event->descriptor==f.expected_descriptor&&event->owner==f.expected_owner);
        assert(std::strcmp(event->name,f.expected_name)==0);
        if(event->kind==a::TriggerDynamicEvent){
            assert(event->advisor==-1&&event->decision==f.expected_decision&&event->value==f.expected_value);
            assert(event->x==1.f&&event->y==2.f&&event->z==3.f);
        }
        assert(!f.map->remove(e)&&!f.api->end_campaign()&&!f.api->set_skill(1));
        if(f.native_publish)return f.native_publish(f.native_manager,event)==a::EventFired;
        return f.event_ok;
    }
    static int SIN_FIXED_CALL queue(void* c,a::MapEntityRef e){
        auto& f=self(c);assert(same(e,f.current));++f.removals;return f.remove_ok;
    }
};
static void test_native_triggers(const a::MapEntityApi* m,const a::Api* api,const a::CampaignTriggerApi* tr){
    assert(tr&&tr->version==1&&tr->size==sizeof(*tr)&&api->begin_campaign(3));
    const auto epoch=m->begin_level("trigger_map");
    assert(epoch);
    a::MapEntityRef dynamic{epoch,20,1},secret{epoch,21,1},end{epoch,22,1};
    NativeTriggerFixture f{m,api,tr,dynamic,{epoch,5,2},{epoch,7,2}};
    a::NativeTriggerHost host{1,sizeof(host),&NativeTriggerFixture::live,&NativeTriggerFixture::is_player,
        &NativeTriggerFixture::base,&NativeTriggerFixture::warn,&NativeTriggerFixture::owner,
        &NativeTriggerFixture::special,&NativeTriggerFixture::publish,&NativeTriggerFixture::queue};
    a::NativeTriggerSpawn spawn{0,0,0x400};
    a::NativeTriggerTouch touch{1.f,2.f,3.f,0x800};
    a::NativeTriggerReport report{};
    assert(m->create(dynamic,"trigger_dynamic_difficulty")==a::MapApplied);
    assert(m->keyvalue(dynamic,"EstimatedTime","88.5")==a::MapApplied);
    assert(m->keyvalue(dynamic,"EstimatedDifficulty","900")==a::MapApplied);
    assert(m->keyvalue(dynamic,"EndOfGame","256")==a::MapApplied);
    assert(m->keyvalue(dynamic,"targetname","gate")==a::MapApplied);
    assert(m->keyvalue(dynamic,"EstimatedTime","nan")==a::MapRejected);
    assert(m->keyvalue(dynamic,"spawnflags","1")==a::MapUnhandledKey);
    assert(m->spawn(dynamic)==a::MapRejected);
    assert(!tr->touch(dynamic,f.player,&touch,&host,&f,&report));
    assert(tr->spawn(dynamic,&spawn,&host,&f,&report)&&report.code==a::TriggerHandled);
    assert(report.base_initialized&&report.warning_emitted&&f.bases==1&&f.warnings==1);
    assert(tr->spawn(dynamic,&spawn,&host,&f,&report)&&report.code==a::TriggerAlreadyConsumed);
    assert(tr->touch(dynamic,f.grunt,&touch,&host,&f,&report)&&report.code==a::TriggerNonPlayer);
    assert(!f.events&&!f.specials&&!f.removals);
    f.special_ok=false;
    assert(tr->touch(dynamic,f.player,&touch,&host,&f,&report)&&report.code==a::TriggerHostRefused);
    assert(f.specials==1&&!f.events);
    f.special_ok=true;f.event_ok=false;f.remove_ok=false;
    assert(tr->touch(dynamic,f.player,&touch,&host,&f,&report)&&report.code==a::TriggerRemovalPending);
    assert(f.events==1&&f.removals==1&&!report.event_published&&!report.manager_signaled&&!report.removal_queued);
    a::State state{};
    assert(api->get_state(&state)&&state.trigger_count==1&&!state.completed&&state.skill==3);
    f.remove_ok=true;
    assert(tr->retry_remove(dynamic,&host,&f,&report)&&report.code==a::TriggerHandled&&report.removal_queued);
    assert(f.events==1&&f.removals==2);
    assert(tr->touch(dynamic,f.player,&touch,&host,&f,&report)&&report.code==a::TriggerAlreadyConsumed);
    assert(m->remove(dynamic));
    f.current=secret;f.event_ok=true;
    f.expected_kind=a::TriggerGameEvent;f.expected_descriptor=11;f.expected_owner=-1;f.expected_name="trigger_secret";
    assert(m->create(secret,"trigger_secret")==a::MapApplied);
    spawn={0,8,0};
    assert(tr->spawn(secret,&spawn,&host,&f,&report)&&report.code==a::TriggerHandled&&report.event_published);
    f.expected_descriptor=12;
    assert(tr->touch(secret,f.player,&touch,&host,&f,&report)&&report.code==a::TriggerHandled&&report.removal_queued);
    assert(api->get_state(&state)&&state.trigger_count==1&&!state.completed);
    assert(m->remove(secret));
    f.current=end;f.expected_kind=a::TriggerDynamicEvent;f.expected_descriptor=0;f.expected_owner=5;
    f.expected_decision=0;f.expected_value=0.f;f.expected_name="trigger_dynamic_difficulty";
    assert(m->create(end,"trigger_dynamic_difficulty")==a::MapApplied);
    assert(m->keyvalue(end,"EndOfGame","257")==a::MapApplied);
    spawn={0,0,0};touch.flags_b8=0;
    assert(tr->spawn(end,&spawn,&host,&f,&report)&&report.code==a::TriggerHandled);
    assert(tr->touch(end,f.player,&touch,&host,&f,&report)&&report.code==a::TriggerHandled);
    assert(report.manager_signaled&&report.event_published&&report.removal_queued);
    assert(api->get_state(&state)&&state.trigger_count==2&&state.completed&&state.skill==3);
    assert(m->end_level(epoch)&&api->end_campaign());
    assert(!tr->touch(end,f.player,&touch,&host,&f,&report));
}


struct TriggerBspFixture final:sin_port::map_host::ITriggerShellState {
    BspShellHost& shell;
    std::uint32_t epoch;
    const a::Api* api;
    const a::MapEntityApi* map;
    EntityRef player{99,3},grunt{100,4};
    bool snapshots=true,remove_ok=false,suppress=false;
    int bases=0,warnings=0,specials=0;
    std::vector<int> descriptors;
    std::vector<std::string> names;
    std::vector<EntityRef> pending;
    TriggerBspFixture(BspShellHost& s,std::uint32_t ep,const a::Api* a_,const a::MapEntityApi* m)
        :shell(s),epoch(ep),api(a_),map(m){}
    bool observe_spawn(EntityRef e,a::NativeTriggerSpawn& out) const override {
        if(!snapshots||!shell.is_live(e))return false;
        // Deliberately independent of the map's textual spawnflags key.
        out={suppress?1u:0u,e.index==3?8u:0u,e.index==2?0x400u:0u};
        return true;
    }
    bool observe_touch(EntityRef e,a::NativeTriggerTouch& out) const override {
        if(!snapshots||!shell.is_live(e))return false;
        out={4.f,5.f,6.f,e.index==2?0x800u:0u};return true;
    }
    static TriggerBspFixture& self(void* c){return *static_cast<TriggerBspFixture*>(c);}
    static EntityRef entity(a::MapEntityRef e){return {e.index,e.generation};}
    static int SIN_FIXED_CALL live(void* c,a::MapEntityRef e){
        auto& f=self(c);const auto id=entity(e);
        return e.epoch==f.epoch&&(f.shell.is_live(id)||id==f.player||id==f.grunt);
    }
    static int SIN_FIXED_CALL is_player(void* c,a::MapEntityRef e){return live(c,e)&&entity(e)==self(c).player;}
    static int SIN_FIXED_CALL base(void* c,a::MapEntityRef e){
        auto& f=self(c);assert(f.shell.is_live(entity(e)));++f.bases;
        f.shell.events.push_back("trigger_base:"+std::to_string(e.index));return 1;
    }
    static void SIN_FIXED_CALL warn(void* c,const char* text){
        assert(std::strcmp(text,"DD Trigger with don't destroy set\n")==0);++self(c).warnings;
    }
    static int SIN_FIXED_CALL owner(void*){return 99;}
    static int SIN_FIXED_CALL special(void* c,a::MapEntityRef e){
        assert(e.index==2);++self(c).specials;return 1;
    }
    static int SIN_FIXED_CALL publish(void* c,a::MapEntityRef e,const a::NativeTriggerEvent* event){
        auto& f=self(c);assert(event&&f.shell.is_live(entity(e)));
        f.descriptors.push_back(event->descriptor);f.names.emplace_back(event->name);
        if(e.index==2){
            assert(event->kind==a::TriggerDynamicEvent&&event->owner==99&&event->advisor==-1&&
                event->decision==17&&event->value==12.5f&&event->x==4.f&&event->y==5.f&&event->z==6.f);
        }else assert(event->kind==a::TriggerGameEvent&&event->owner==-1);
        assert(!f.map->remove(e)&&!f.api->set_skill(1));
        return 1;
    }
    static int SIN_FIXED_CALL queue(void* c,a::MapEntityRef e){
        auto& f=self(c);assert(f.shell.is_live(entity(e)));
        if(!f.remove_ok)return 0;
        f.pending.push_back(entity(e));return 1;
    }
    a::NativeTriggerHost callbacks() const {
        return {1,sizeof(a::NativeTriggerHost),&live,&is_player,&base,&warn,&owner,&special,&publish,&queue};
    }
};
static void test_trigger_bsp_host(const a::MapEntityApi* m,const a::Api* api,const a::CampaignTriggerApi* tr){
    assert(api->begin_campaign(3));
    const auto epoch=m->begin_level("trigger_bsp");
    assert(epoch);
    BspShellHost shell;shell.allow_triggers=true;
    TriggerBspFixture fixture(shell,epoch,api,m);
    const auto callbacks=fixture.callbacks();
    sin_port::map_host::NativeBehaviorHost host(shell,*m,epoch,*tr,fixture,callbacks,&fixture);
    sm::SourceMapRuntime runtime(host);
    const auto bsp=make_native_bsp(R"KV(
{"classname" "worldspawn"}
{"classname" "trigger_dynamic_difficulty" "targetname" "final_gate" "EstimatedTime" "12.5"
 "EstimatedDifficulty" "17" "EndOfGame" "257" "spawnflags" "1"}
{"classname" "trigger_secret" "targetname" "secret_room" "spawnflags" "0"}
)KV");
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok()&&loaded.entities_created==3&&loaded.entities_spawned==3&&loaded.entities_activated==3);
    assert(loaded.key_values_applied==7&&shell.base_keys.size()==7);
    assert(shell.base_keys[0].first=="targetname"&&shell.base_keys[4].first=="spawnflags");
    assert(fixture.bases==2&&fixture.warnings==1&&host.attached_count()==2&&m->count(epoch)==2);
    assert(std::count(shell.events.begin(),shell.events.end(),"spawn:1")==1);
    assert(std::count(shell.events.begin(),shell.events.end(),"spawn:2")==0);
    assert(std::count(shell.events.begin(),shell.events.end(),"spawn:3")==0);
    assert(fixture.descriptors==std::vector<int>{11}&&fixture.names[0]=="secret_room");
    const auto dynamic=runtime.entities()[1],secret=runtime.entities()[2];
    a::NativeTriggerReport report{};
    fixture.snapshots=false;
    assert(!host.touch(dynamic,fixture.player,report));
    fixture.snapshots=true;
    assert(host.touch(dynamic,fixture.grunt,report)&&report.code==a::TriggerNonPlayer);
    assert(host.touch(dynamic,fixture.player,report)&&report.code==a::TriggerRemovalPending);
    assert(report.manager_signaled&&report.event_published&&!report.removal_queued&&fixture.specials==1);
    a::State state{};
    assert(api->get_state(&state)&&state.trigger_count==1&&state.completed&&state.skill==3);
    assert(!host.notify_deleted(dynamic));
    fixture.remove_ok=true;
    assert(host.retry_remove(dynamic,report)&&report.code==a::TriggerHandled&&report.removal_queued);
    assert(host.touch(dynamic,fixture.player,report)&&report.code==a::TriggerAlreadyConsumed);
    assert(host.touch(secret,fixture.player,report)&&report.code==a::TriggerHandled&&report.removal_queued);
    assert((fixture.descriptors==std::vector<int>{11,0,12}));
    assert(fixture.names[1]=="final_gate"&&fixture.names[2]=="secret_room");
    assert(fixture.pending.size()==2);
    for(auto e:fixture.pending){assert(shell.remove_entity(e));assert(host.notify_deleted(e));}
    assert(host.notify_deleted(dynamic)&&!host.touch(dynamic,fixture.player,report));
    assert(host.attached_count()==0&&m->count(epoch)==0);
    assert(runtime.unload().code==sm::SourceMapLoadCode::Unloaded&&shell.live.empty());
    assert(m->end_level(epoch)&&api->end_campaign());
    // Absence of trigger services is not silently interpreted as a working
    // SiN entity factory. A failed snapshot must roll back native attachment.
    assert(api->begin_campaign(2));
    const auto ep2=m->begin_level("trigger_bsp_failure");
    assert(ep2);
    BspShellHost shell2;shell2.allow_triggers=true;
    const auto one=make_native_bsp(R"KV({"classname" "trigger_secret"})KV");
    sin_port::map_host::NativeBehaviorHost noServices(shell2,*m,ep2);
    sm::SourceMapRuntime rejected(noServices);
    assert(rejected.load_bsp(one.data(),one.size()).code==sm::SourceMapLoadCode::HostCreateFailed);
    assert(shell2.live.empty()&&m->count(ep2)==0);
    TriggerBspFixture bad(shell2,ep2,api,m);bad.snapshots=false;
    const auto cb2=bad.callbacks();
    sin_port::map_host::NativeBehaviorHost badHost(shell2,*m,ep2,*tr,bad,cb2,&bad);
    sm::SourceMapRuntime badRuntime(badHost);
    assert(badRuntime.load_bsp(one.data(),one.size()).code==sm::SourceMapLoadCode::HostSpawnFailed);
    assert(shell2.live.empty()&&badHost.attached_count()==0&&m->count(ep2)==0&&bad.bases==0);
    bad.snapshots=true;bad.suppress=true;bad.remove_ok=true;
    assert(badRuntime.load_bsp(one.data(),one.size()).code==sm::SourceMapLoadCode::HostSpawnFailed);
    assert(shell2.live.empty()&&badHost.attached_count()==0&&m->count(ep2)==0&&bad.bases==0);
    assert(m->end_level(ep2)&&api->end_campaign());
}


static void test_combat_damage(const a::CombatDamageApi* damage,const a::Api* api,
        const a::GlobalStateApi* g,const a::MapEntityApi* map){
    assert(damage&&damage->version==1&&damage->size==sizeof(*damage));
    assert(damage->player_gate(0)==a::PlayerDamageRejected);
    assert(api->begin_campaign(3)&&g->clear()&&g->set_updates_disabled(0));
    assert(damage->player_gate(0)==a::PlayerDamageReady);
    assert(damage->player_gate(0x100)==a::PlayerDamageNotAlive);
    a::PlayerDamageRecord source{};
    for(unsigned i=0;i<9;++i)source.vector_bits[i]=0x13579bdfu+i;
    source.inflictor_handle=0x12345678;
    source.attacker_handle=0x87654321;
    source.amount=100.f;source.opaque_30=0xfedcba98;source.base_damage=-1.f;
    source.tail[0]=0x11223344;source.tail[1]=0x55667788;source.tail[2]=0x99aabbcc;
    a::PlayerDamageObservation observed{};
    a::PlayerDamageRecord output{};
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageReady);
    auto expected=source;expected.amount=140.f;expected.base_damage=100.f;
    assert(std::memcmp(&output,&expected,sizeof(output))==0&&source.amount==100.f&&source.base_damage==-1.f);
    const std::uint32_t bypass[]={0x1,0x20,0x4000,0x20000,0x80000000,0x80024021};
    for(auto mask:bypass){
        source.damage_type=mask;expected=source;expected.base_damage=source.amount;
        assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageReady);
        assert(std::memcmp(&output,&expected,sizeof(output))==0);
    }
    source.damage_type=0;
    observed.attacker_is_self=1;
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageReady);
    assert(std::memcmp(&source,&output,sizeof(source))==0);
    observed.attacker_is_self=0;observed.inflictor_is_self=1;
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageReady);
    assert(std::memcmp(&source,&output,sizeof(source))==0);
    observed.inflictor_is_self=0;
    assert(api->set_skill(1));
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageReady&&output.amount==75.f);
    const auto epoch=map->begin_level("damage_test");
    assert(epoch);
    const a::MapEntityRef gate{epoch,1,1};
    assert(map->create(gate,"env_global")==a::MapApplied);
    assert(map->keyvalue(gate,"globalstate","gordon_invulnerable")==a::MapApplied);
    assert(map->keyvalue(gate,"spawnflags","1")==a::MapApplied);
    assert(map->spawn(gate)==a::MapApplied);
    assert(map->input(gate,"TurnOn",nullptr,nullptr)==a::MapApplied);
    assert(damage->player_gate(0)==a::PlayerDamageInvulnerable&&damage->player_gate(1)==a::PlayerDamageInvulnerable);
    output=source;
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageInvulnerable);
    assert(std::memcmp(&source,&output,sizeof(source))==0);
    assert(map->input(gate,"Remove",nullptr,nullptr)==a::MapApplied);
    assert(damage->player_gate(0)==a::PlayerDamageReady);
    assert(map->input(gate,"TurnOff",nullptr,nullptr)==a::MapApplied);
    assert(damage->player_gate(0)==a::PlayerDamageReady);
    observed.life_state=1;
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageNotAlive);
    observed.life_state=0;observed.attacker_is_self=2;
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageRejected);
    observed.attacker_is_self=0;
    source.amount=std::numeric_limits<float>::infinity();
    assert(damage->prepare_player_copy(&source,&observed,&output)==a::PlayerDamageRejected);
    // Common final damage retains fractional health carry, independent
    // of the earlier player rules (never scale this amount a second time).
    a::CommonDamageInput in{100,2,0.f,0.25f};
    a::CommonDamageOutput out{};
    for(int i=1;i<=4;++i){
        const int code=damage->common_health_stage(&in,&out);
        assert(code==(i==4?a::CommonDamageReduced:a::CommonDamageFractionOnly));
        assert(out.health==100-(i==4?1:0)&&out.fraction==(i==4?0.f:0.25f*i));
        assert(out.fraction_write==1&&out.health_changed==(i==4?1u:0u));
        in.health=out.health;in.fraction=out.fraction;
    }
    in={100,2,0.75f,3.5f};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageReduced);
    assert(out.health==96&&out.integer_damage==4&&out.fraction==0.25f);
    in={2,2,0.5f,2.75f};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageReduced&&out.health==-1);
    in={100,1,0.75f,3.5f};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageEventsOnly);
    assert(out.health==100&&out.integer_damage==0&&out.fraction==0.75f&&!out.fraction_write&&!out.health_changed);
    in={100,2,1.f,0.f};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageReduced&&out.health==99&&out.fraction==0.f);
    in={100,2,0.f,-1.f};
    out={123,456,0.5f,7,8};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageRejected&&out.health==123&&out.health_changed==8);
    in={100,2,0.f,std::numeric_limits<float>::infinity()};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageRejected);
    in={(std::numeric_limits<std::int32_t>::min)(),2,0.f,1.f};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageRejected);
    in={100,2,0.f,2147483648.f};
    assert(damage->common_health_stage(&in,&out)==a::CommonDamageRejected);
    assert(map->end_level(epoch)&&api->end_campaign()&&g->clear());
}


struct NpcRandomFixture {
    const a::Api* api;
    const a::NpcRulesApi* rules;
    float sample=0.f;
    unsigned calls=0;
    int mode=0;
    static int SIN_FIXED_CALL draw(void* context,float lo,float hi,float* out){
        auto& self=*static_cast<NpcRandomFixture*>(context);
        assert(lo==0.f&&hi==1.f&&out);++self.calls;
        assert(!self.api->set_skill(2)&&!self.api->end_campaign());
        a::NpcDecisionOutput nested{};
        assert(!self.rules->decide(a::NpcMobile,0.f,nullptr,nullptr,&nested));
        if(self.mode==2)throw 42;
        if(self.mode==1)return 0;
        *out=self.sample;return 1;
    }
};
static void test_npc_rules(const a::NpcRulesApi* npc,const a::Api* api){
    assert(npc&&npc->version==1&&npc->size==sizeof(*npc));
    a::NpcDecisionOutput out{123.f,456.f,7,8};
    assert(!npc->decide(a::NpcSecondaryFire,1.f,nullptr,nullptr,&out)&&out.decision==8);
    assert(api->begin_campaign(1));
    NpcRandomFixture rng{api,npc};
    assert(npc->decide(a::NpcSecondaryFire,1.f,nullptr,nullptr,&out));
    assert(!out.drew_random&&!out.decision&&out.threshold==0.f);
    assert(npc->decide(a::NpcSecondaryFire,1.f,&NpcRandomFixture::draw,&rng,&out)&&rng.calls==0);
    assert(npc->decide(a::NpcMobile,0.f,nullptr,nullptr,&out)&&!out.drew_random);
    assert(!npc->decide(a::NpcWearsHelmet,1.f,nullptr,nullptr,&out));
    rng.sample=0.1f;
    assert(npc->decide(a::NpcWearsHelmet,1.f,&NpcRandomFixture::draw,&rng,&out));
    assert(out.drew_random&&out.decision==1&&out.threshold==0.1f&&rng.calls==1);
    rng.sample=std::nextafter(0.1f,1.f);
    assert(npc->decide(a::NpcWearsHelmet,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==0);
    a::NpcSpread spread{};
    assert(npc->uncached_spread(0.2f,&spread)&&spread.x==0.2f&&spread.y==spread.x&&spread.z==spread.x);
    assert(api->set_skill(2));
    assert(npc->uncached_spread(0.2f,&spread)&&std::fabs(spread.x-0.15f)<0.00001f);
    assert(api->set_skill(3));
    assert(npc->uncached_spread(0.2f,&spread)&&spread.x==0.1f&&spread.y==spread.x&&spread.z==spread.x);
    rng.sample=0.5f;
    assert(npc->decide(a::NpcSecondaryFire,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==1);
    rng.sample=0.6f;
    assert(npc->decide(a::NpcHelmetShotOff,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==1);
    rng.sample=0.2f;
    assert(npc->decide(a::NpcTakesCover,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==1);
    rng.sample=0.3f;
    assert(npc->decide(a::NpcHealthKitDrop,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==1);
    rng.sample=0.15f;
    assert(npc->decide(a::NpcSpawnCountDelta,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==1);
    rng.sample=1.f;
    const unsigned before=rng.calls;
    assert(npc->decide(a::NpcMobile,20.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==1&&out.threshold>1.f);
    assert(rng.calls==before+1);
    rng.sample=0.f;
    assert(npc->decide(a::NpcMobile,-4.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==1&&out.threshold>0.f);
    out={123.f,456.f,7,8};
    assert(!npc->decide(999,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==8);
    assert(!npc->decide(a::NpcMobile,std::numeric_limits<float>::infinity(),&NpcRandomFixture::draw,&rng,&out));
    rng.mode=1;assert(!npc->decide(a::NpcMobile,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==8);
    rng.mode=2;assert(!npc->decide(a::NpcMobile,1.f,&NpcRandomFixture::draw,&rng,&out)&&out.decision==8);
    rng.mode=0;rng.sample=1.01f;assert(!npc->decide(a::NpcMobile,1.f,&NpcRandomFixture::draw,&rng,&out));
    rng.sample=std::numeric_limits<float>::quiet_NaN();
    assert(!npc->decide(a::NpcMobile,1.f,&NpcRandomFixture::draw,&rng,&out));
    assert(!npc->uncached_spread(-1.f,&spread)&&spread.x==0.1f);
    assert(api->end_campaign());
}

struct WeaponEffectFixture {
    const a::WeaponBehaviorApi* weapons;
    const a::MapEntityApi* map;
    const a::Api* api;
    a::MapEntityRef expected,owner;
    std::vector<int> events;
    bool fail_activity=false,throw_fov=false;
    static bool same(a::MapEntityRef x,a::MapEntityRef y){
        return x.epoch==y.epoch&&x.index==y.index&&x.generation==y.generation;
    }
    static int SIN_FIXED_CALL fov(void* context,a::MapEntityRef ref,a::MapEntityRef owner,std::int32_t value,float duration){
        auto& f=*static_cast<WeaponEffectFixture*>(context);
        assert(same(ref,f.expected)&&same(owner,f.owner));
        assert(duration==(value?0.25f:0.15f));
        f.events.push_back(value);
        assert(!f.weapons->reconcile(ref,0,0)&&!f.map->remove(ref)&&!f.api->set_skill(1));
        if(f.throw_fov)throw 1;
        return 1;
    }
    static int SIN_FIXED_CALL activity(void* context,a::MapEntityRef ref,std::int32_t value){
        auto& f=*static_cast<WeaponEffectFixture*>(context);
        assert(same(ref,f.expected));
        f.events.push_back(value);
        assert(!f.weapons->reconcile(ref,0,0)&&!f.map->remove(ref)&&!f.api->set_skill(1));
        return f.fail_activity?0:1;
    }
};
static void test_native_weapon_behavior(const a::WeaponBehaviorApi* weapons,const a::MapEntityApi* map,const a::Api* api){
    assert(weapons&&weapons->version==1&&weapons->size==sizeof(*weapons)&&api->begin_campaign(2));
    const auto epoch=map->begin_level("weapon_bsp");assert(epoch);
    BspShellHost shell;
    sin_port::map_host::NativeBehaviorHost host(shell,*map,epoch);
    sm::SourceMapRuntime runtime(host);
    const auto bsp=make_native_bsp(R"KV(
{"classname" "worldspawn"}
{"classname" "weapon_magnum" "targetname" "pistol"}
{"classname" "weapon_assault_rifle" "targetname" "rifle"}
{"classname" "weapon_scattergun" "targetname" "shotgun"}
)KV");
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok()&&loaded.entities_spawned==4&&host.attached_count()==3&&map->count(epoch)==3);
    const auto makeRef=[&](std::size_t index){const auto e=runtime.entities()[index];return a::MapEntityRef{epoch,e.index,e.generation};};
    const auto mag=makeRef(1),rifle=makeRef(2),scatter=makeRef(3),world=makeRef(0);
    a::NativeWeaponProfile profile{};
    assert(!weapons->profile(world,&profile));
    assert(weapons->profile(mag,&profile)&&profile.kind==a::WeaponMagnum&&profile.clip_size==20&&profile.zoom_fov==50);
    assert(weapons->profile(rifle,&profile)&&profile.kind==a::WeaponAssaultRifle&&profile.clip_size==45&&profile.zoom_fov==40);
    assert(weapons->profile(scatter,&profile)&&profile.kind==a::WeaponScattergun&&profile.clip_size==6&&profile.zoom_fov==60&&profile.clip2_size==-1);
    a::NativeWeaponState state{};
    assert(weapons->state(rifle,&state)&&!state.zoomed&&!state.restore_pending);
    assert(!weapons->reconcile(rifle,2,0));
    const a::MapEntityRef owner{epoch,100,1};
    WeaponEffectFixture fixture{weapons,map,api,rifle,owner,{}};
    const a::NativeWeaponEffects effects{a::weapon_version,sizeof(a::NativeWeaponEffects),
        &WeaponEffectFixture::fov,&WeaponEffectFixture::activity};
    a::NativeWeaponFrame frame{owner,1,1,0,0,0x80000,1,0,0x9d,40};
    a::NativeWeaponReport report{};
    auto step=[&](){return weapons->frame(rifle,&frame,&effects,&fixture,&report);};
    assert(step()&&report.code==a::WeaponEntered&&report.state.zoomed);
    assert((fixture.events==std::vector<int>{40,0xc0}));
    int activity=0;
    assert(weapons->attack_activity(rifle,0,4,&activity)&&activity==0xc1);
    assert(weapons->attack_activity(rifle,1,0,&activity)&&activity==0xc2);
    frame.pressed_buttons=0;frame.reload_active=1;frame.current_activity=0xc0;
    assert(step()&&report.code==a::WeaponExited&&report.interrupted&&report.state.restore_pending&&!report.state.zoomed);
    assert((fixture.events==std::vector<int>{40,0xc0,0,0x9d}));
    assert(weapons->attack_activity(rifle,0,0,&activity)&&activity==0xa4);
    assert(weapons->attack_activity(rifle,0,2,&activity)&&activity==0xbb);
    assert(weapons->attack_activity(rifle,0,3,&activity)&&activity==0xbc);
    assert(weapons->attack_activity(rifle,0,9,&activity)&&activity==0xbd);
    assert(step()&&report.code==a::WeaponUnchanged);
    frame.reload_active=0;frame.player_ready=0;
    assert(step()&&report.code==a::WeaponUnchanged&&report.state.restore_pending);
    frame.player_ready=1;
    assert(step()&&report.code==a::WeaponEntered&&report.automatic_restore&&!report.state.restore_pending);
    // No normal idle activity on exit from a non-idle zoom activity.
    frame.pressed_buttons=0x80000;frame.current_activity=0xc1;
    assert(step()&&report.code==a::WeaponExited&&report.fov_delivered&&!report.activity_attempted);
    frame.vr_mode=1;frame.current_activity=0x9d;
    assert(step()&&report.code==a::WeaponEntered&&!report.fov_attempted&&report.activity_delivered);
    // Failed foreign effect leaves the behavior blocked until observed resync.
    frame.current_activity=0xc0;fixture.fail_activity=true;
    assert(step()&&report.code==a::WeaponEffectsUncertain&&report.state.needs_resync&&report.state.zoomed);
    assert(step()&&report.code==a::WeaponRejected&&!weapons->attack_activity(rifle,0,0,&activity));
    assert(weapons->reconcile(rifle,0,0));fixture.fail_activity=false;
    frame.vr_mode=0;fixture.throw_fov=true;
    assert(step()&&report.code==a::WeaponEffectsUncertain&&report.fov_attempted&&!report.fov_delivered&&!report.activity_attempted);
    assert(weapons->reconcile(rifle,0,0));
    fixture.throw_fov=false;
    assert(weapons->state(mag,&state)&&!state.zoomed);
    assert(weapons->attack_activity(mag,0,9,&activity)&&activity==0xa4);
    assert(weapons->reconcile(scatter,1,0));
    assert(weapons->attack_activity(scatter,1,0,&activity)&&activity==0xc2);
    assert(map->input(rifle,"TurnOn",nullptr,nullptr)==a::MapUnhandledInput);
    assert(runtime.unload().code==sm::SourceMapLoadCode::Unloaded&&host.attached_count()==0);
    assert(!weapons->state(rifle,&state)&&map->end_level(epoch)&&api->end_campaign());
}
struct PumpFixture {
    const a::ScattergunPumpApi* pump;
    const a::MapEntityApi* map;
    a::MapEntityRef weapon,owner;
    std::vector<int> calls;
    float owner_deadline=0.f;
    int fail_at=0;
    bool step(int id){
        calls.push_back(id);
        a::ScattergunSnapshot s{};
        assert(pump->state(weapon,&s)&&!s.pending);
        assert(!pump->before_deploy(weapon,3)&&!map->remove(weapon));
        return fail_at!=static_cast<int>(calls.size());
    }
    static PumpFixture& get(void* c){return *static_cast<PumpFixture*>(c);}
    static int SIN_FIXED_CALL sound(void* c,a::MapEntityRef ref,std::int32_t id,float delay){
        auto& f=get(c);assert(ref.index==f.weapon.index&&id==13&&delay==0.f);return f.step(1);
    }
    static int SIN_FIXED_CALL lookup(void* c,a::MapEntityRef ref,const char* name,std::int32_t* activity){
        auto& f=get(c);assert(ref.index==f.weapon.index&&std::strcmp(name,"ACT_VM_ZOOMED_PUMP")==0);
        *activity=-1;return f.step(2); // original forwards negative lookup too
    }
    static int SIN_FIXED_CALL activity(void* c,a::MapEntityRef ref,std::int32_t id){
        auto& f=get(c);assert(ref.index==f.weapon.index&&(id==0x126||id==-1));return f.step(3);
    }
    static int SIN_FIXED_CALL duration(void* c,a::MapEntityRef ref,float* value){
        auto& f=get(c);assert(ref.index==f.weapon.index);*value=0.5f;return f.step(4);
    }
    static int SIN_FIXED_CALL clock(void* c,float* value){
        *value=10.f;return get(c).step(5);
    }
    static int SIN_FIXED_CALL write(void* c,a::MapEntityRef ref,std::uint32_t kind,float value){
        auto& f=get(c);
        if(kind==a::ScattergunOwnerDeadline){
            assert(ref.index==f.owner.index&&value==10.5f);f.owner_deadline=10.75f;
            return f.step(6);
        }
        assert(ref.index==f.weapon.index);
        if(kind==a::ScattergunPrimaryDeadline){
            assert(value==10.75f);f.owner_deadline=11.f;return f.step(8);
        }
        assert(kind==a::ScattergunSecondaryDeadline&&value==11.f);return f.step(10);
    }
    static int SIN_FIXED_CALL read(void* c,a::MapEntityRef ref,float* value){
        auto& f=get(c);assert(ref.index==f.owner.index);
        *value=f.owner_deadline;return f.step(*value==10.75f?7:9);
    }
};
static void test_scattergun_pump(const a::ScattergunPumpApi* pump,const a::WeaponBehaviorApi* weapons,
        const a::MapEntityApi* map,const a::Api* api){
    assert(pump&&pump->version==1&&pump->size==sizeof(*pump)&&api->begin_campaign(2));
    const auto epoch=map->begin_level("pump");assert(epoch);
    const a::MapEntityRef gun{epoch,1,1},owner{epoch,100,1},mag{epoch,2,1};
    assert(map->create(gun,"weapon_scattergun")==a::MapApplied);
    assert(!pump->before_deploy(gun,6));
    assert(map->spawn(gun)==a::MapApplied&&map->create(mag,"weapon_magnum")==a::MapApplied&&map->spawn(mag)==a::MapApplied);
    a::ScattergunSnapshot state{};
    assert(!pump->state(mag,&state)&&pump->state(gun,&state)&&state.prior_primary==0xffffffffu&&state.prior_secondary==0xffffffffu&&!state.pending);
    PumpFixture fixture{pump,map,gun,owner,{}};
    const a::ScattergunPumpHost host{1,sizeof(a::ScattergunPumpHost),&PumpFixture::sound,&PumpFixture::lookup,
        &PumpFixture::activity,&PumpFixture::duration,&PumpFixture::clock,&PumpFixture::write,&PumpFixture::read};
    a::ScattergunPumpObservation input{owner,6,0,0,1,10.f,10.f};
    a::ScattergunPumpReport report{};
    auto update=[&](){return pump->after_base_update(gun,&input,&host,&fixture,&report);};
    assert(update()&&report.code==a::ScattergunNoPump&&report.primary_changed&&!report.state.pending);
    input.primary_clip=5;input.primary_deadline=11.f;
    assert(update()&&report.code==a::ScattergunWaiting&&report.state.pending&&report.primary_changed);
    input.primary_deadline=10.f;input.reload_active=1;
    assert(update()&&report.code==a::ScattergunWaiting&&!report.primary_changed);
    input.reload_active=0;input.owner_present=0;
    assert(update()&&report.code==a::ScattergunWaiting);
    input.owner_present=1;input.primary_clip=0;
    assert(update()&&report.code==a::ScattergunWaiting&&report.state.pending);
    input.primary_clip=4;
    assert(update()&&report.code==a::ScattergunPumped&&report.activity==0x126&&!report.state.pending);
    assert((fixture.calls==std::vector<int>{1,3,4,5,6,7,8,9,10}));
    assert(report.callbacks_attempted==9&&report.callbacks_delivered==9&&report.requested_deadline==10.5f);
    fixture.calls.clear();input.secondary_state=2;
    assert(update()&&report.code==a::ScattergunNoPump&&!report.secondary_changed&&report.state.prior_secondary==0xffffffffu);
    assert(weapons->reconcile(gun,1,0));
    assert(update()&&report.code==a::ScattergunNoPump&&report.secondary_changed&&!report.state.pending);
    input.secondary_state=1;
    assert(update()&&report.code==a::ScattergunPumped&&report.activity==-1&&report.secondary_changed);
    assert((fixture.calls==std::vector<int>{1,2,3,4,5,6,7,8,9,10}));
    fixture.calls.clear();
    assert(pump->before_deploy(gun,6));input.primary_clip=6;
    assert(update()&&report.code==a::ScattergunNoPump&&!report.primary_changed);
    input.primary_clip=5;fixture.fail_at=3;
    assert(update()&&report.code==a::ScattergunUncertain&&report.callbacks_attempted==3&&report.callbacks_delivered==2&&report.state.needs_resync&&!report.state.pending);
    assert(update()&&report.code==a::ScattergunRejected&&fixture.calls.size()==3);
    state=report.state;state.needs_resync=0;
    assert(pump->reconcile(gun,&state));fixture.fail_at=0;
    assert(update()&&report.code==a::ScattergunNoPump&&fixture.calls.size()==3); // no replay
    assert(map->remove(gun)&&!pump->state(gun,&state)&&map->end_level(epoch)&&api->end_campaign());
}
struct FireFixture {
    const a::MapEntityApi* map;
    const a::ScattergunPumpApi* pump;
    a::MapEntityRef gun,owner;
    std::vector<unsigned> order;
    unsigned clip=5,mode=0,clip_queries=0,clocks=0,pellets=0;
    unsigned fail_pellet=99;
    bool capable=true,vectors=true,uses_clip=true,flip=false;
    static FireFixture& get(void* context){return *static_cast<FireFixture*>(context);}
    void check(a::MapEntityRef g,a::MapEntityRef o){
        assert(g.epoch==gun.epoch&&g.index==gun.index&&g.generation==gun.generation);
        assert(o.epoch==owner.epoch&&o.index==owner.index&&o.generation==owner.generation);
        assert(!map->remove(g)&&!pump->before_deploy(g,6));
    }
    void reset(){order.clear();clip_queries=clocks=pellets=0;}
    static int SIN_FIXED_CALL query(void* c,a::MapEntityRef g,a::MapEntityRef o,
            std::uint32_t op,std::uint32_t arg,a::ScattergunFireValue* out){
        auto& f=get(c);f.check(g,o);f.order.push_back(100+op);
        switch(op){
        case a::FireOwnerCapable:out->word0=f.capable;break;
        case a::FireOwnerMode:out->word0=f.mode;break;
        case a::FireAcquireVectors:out->word0=f.vectors;break;
        case a::FireUsesClip:out->word0=(++f.clip_queries==2&&f.flip)?!f.uses_clip:f.uses_clip;break;
        case a::FireReadClip:out->word0=f.clip;break;
        case a::FireSelectActivity:out->word0=0xc2;break;
        case a::FireInterval:assert(arg==77);out->real0=2.f;break;
        case a::FireClock:out->real0=f.clocks++==0?10.f:12.f;break;
        case a::FireAmmoType:out->word0=92;break;
        case a::FireOwnerAmmoCount:assert(arg==92);out->word0=19;break;
        default:assert(false);return 0;
        }
        return 1;
    }
    static int SIN_FIXED_CALL effect(void* c,a::MapEntityRef g,a::MapEntityRef o,
            std::uint32_t op,const a::ScattergunFireValue* in){
        auto& f=get(c);f.check(g,o);f.order.push_back(200+op);
        switch(op){
        case a::FireStateEvent:assert(in->word0==(f.mode==3?0u:3u)&&in->word1==0);break;
        case a::FireActivity:assert(in->word0==0xc2);break;
        case a::FirePrimaryTimer:assert(in->real0==12.f);break;
        case a::FireSecondaryTimer:assert(in->real0==(f.mode==3?10.2f:14.f));break;
        case a::FireOwnerAnimation:assert(in->word0==5);break;
        case a::FireConsumeClip:--f.clip;break;
        case a::FireConsumeReserve:assert(in->word0==1&&in->word1==41);break;
        case a::FireStoreSecondary:assert(in->word0==19);break;
        case a::FireFeedback:assert(in->word0==6);break;
        case a::FirePellet:
            assert(in->word0==f.pellets&&in->real0==1.f&&in->real1==1500.f);
            if(in->word0==f.fail_pellet)return 0;
            ++f.pellets;break;
        default:break;
        }
        return 1;
    }
};
static void test_scattergun_secondary(const a::ScattergunSecondaryApi* fire,const a::ScattergunPumpApi* pump,
        const a::MapEntityApi* map,const a::Api* api){
    assert(fire&&fire->version==1&&fire->size==sizeof(*fire)&&api->begin_campaign(2));
    const auto epoch=map->begin_level("secondary");assert(epoch);
    const a::MapEntityRef gun{epoch,1,1},owner{epoch,100,1};
    assert(map->create(gun,"weapon_scattergun")==a::MapApplied&&map->spawn(gun)==a::MapApplied);
    FireFixture f{map,pump,gun,owner,{}};
    const a::ScattergunFireHost host{1,sizeof(a::ScattergunFireHost),&FireFixture::query,&FireFixture::effect};
    a::ScattergunFireObservation in{owner,0,77,41};
    a::ScattergunFireReport out{};
    auto run=[&](){return fire->fire(gun,&in,&host,&f,&out);};
    assert(run()&&out.code==a::FireMissingOwner&&f.order.empty());
    in.owner_present=1;f.mode=3;
    assert(run()&&out.code==a::FireModeCooldown&&out.effects_delivered==2&&f.clocks==1);
    assert((f.order==std::vector<unsigned>{100+a::FireOwnerCapable,100+a::FireOwnerMode,
        200+a::FireStateEvent,100+a::FireClock,200+a::FireSecondaryTimer}));
    f.reset();f.mode=0;f.vectors=false;
    assert(run()&&out.code==a::FireNoVectors&&!out.effects_attempted);
    f.reset();f.vectors=true;f.clip=0x80000000u;
    assert(run()&&out.code==a::FireEmptyClip&&out.effects_delivered==1&&f.order.back()==200+a::FireEmpty);
    f.reset();f.clip=5;
    assert(run()&&out.code==a::FireCompleted&&out.pellets_delivered==8&&f.clip==4&&f.clocks==2&&f.clip_queries==2);
    const std::vector<unsigned> expected={
        100+a::FireOwnerCapable,100+a::FireOwnerMode,100+a::FireAcquireVectors,
        100+a::FireUsesClip,100+a::FireReadClip,200+a::FireStateEvent,
        100+a::FireSelectActivity,200+a::FireActivity,100+a::FireInterval,
        100+a::FireClock,200+a::FirePrimaryTimer,100+a::FireClock,200+a::FireSecondaryTimer,
        200+a::FireOwnerCallback,200+a::FireOwnerAnimation,100+a::FireUsesClip,
        200+a::FireConsumeClip,100+a::FireAmmoType,100+a::FireOwnerAmmoCount,
        200+a::FireStoreSecondary,200+a::FirePost,200+a::FireFeedback,200+a::FirePreparePellets};
    assert(f.order.size()==expected.size()+8&&std::equal(expected.begin(),expected.end(),f.order.begin()));
    for(unsigned i=0;i<8;++i)assert(f.order[expected.size()+i]==200+a::FirePellet);
    f.reset();f.flip=true;
    assert(run()&&out.code==a::FireCompleted&&f.clip==4&&f.clip_queries==2&&
        std::find(f.order.begin(),f.order.end(),200+a::FireConsumeReserve)!=f.order.end());
    f.reset();f.capable=false;f.flip=false;
    assert(run()&&out.code==a::FireCompleted&&f.pellets==8&&f.clip_queries==1&&f.clip==4);
    assert(std::find(f.order.begin(),f.order.end(),200+a::FireOwnerAnimation)==f.order.end());
    f.reset();f.fail_pellet=3;
    assert(run()&&out.code==a::FireUncertain&&out.pellets_delivered==3&&out.needs_resync);
    const auto prior=f.order.size();
    assert(run()&&out.code==a::FireRejected&&f.order.size()==prior);
    a::ScattergunSnapshot state{};
    assert(pump->state(gun,&state)&&state.needs_resync);
    state.needs_resync=0;assert(pump->reconcile(gun,&state));
    f.reset();f.fail_pellet=99;
    assert(run()&&out.code==a::FireCompleted&&out.pellets_delivered==8);
    assert(map->remove(gun)&&run()&&out.code==a::FireRejected);
    assert(map->end_level(epoch)&&api->end_campaign());
}
struct GrenadeFixture {
    const a::ARGrenadeApi* api;
    const a::MapEntityApi* map;
    a::MapEntityRef expected;
    std::vector<a::GrenadeEffect> effects;
    unsigned fail_kind=99;
    static int SIN_FIXED_CALL emit(void* c,a::MapEntityRef ref,const a::GrenadeEffect* effect){
        auto& f=*static_cast<GrenadeFixture*>(c);
        assert(ref.epoch==f.expected.epoch&&ref.index==f.expected.index&&ref.generation==f.expected.generation);
        assert(!f.map->remove(ref)&&!f.api->observe_contact(ref,1));
        f.effects.push_back(*effect);return effect->kind!=f.fail_kind;
    }
};
static void test_ar_grenade(const a::ARGrenadeApi* api,const a::MapEntityApi* map,const a::Api* campaign){
    assert(api&&api->version==1&&api->size==sizeof(*api)&&campaign->begin_campaign(2));
    const auto epoch=map->begin_level("grenade");assert(epoch);
    const a::MapEntityRef g{epoch,10,1},g2{epoch,11,1};
    assert(map->create(g,"grenade_ar")==a::MapApplied&&map->spawn(g)==a::MapRejected);
    const a::GrenadeSpawn spawn{{1.f,2.f,3.f},{4.f,6.f,8.f},1.f};
    auto bad=spawn;bad.time=std::numeric_limits<float>::infinity();
    assert(!api->spawn(g,&bad)&&api->spawn(g,&spawn)&&!api->spawn(g,&spawn));
    a::GrenadeSnapshot state{};
    assert(api->state(g,&state)&&state.armed&&!state.contact_flag&&state.damage==100.f&&
        state.blast_radius==250.f&&state.danger_radius==100&&state.next_think==1.1f);
    GrenadeFixture f{api,map,g,{}};
    const a::GrenadeHost host{1,sizeof(a::GrenadeHost),&GrenadeFixture::emit};
    a::GrenadeReport report{};
    assert(api->think(g,1.05f,1,1,&host,&f,&report)&&report.code==a::GrenadeNotDue&&f.effects.empty());
    assert(api->think(g,1.1f,1,1,&host,&f,&report)&&report.code==a::GrenadeApplied&&report.state.danger_radius==115);
    assert(f.effects.size()==1&&f.effects[0].kind==a::GrenadeFlightSound&&f.effects[0].type==8&&
        f.effects[0].origin.x==3.f&&f.effects[0].origin.y==5.f&&f.effects[0].origin.z==7.f&&
        f.effects[0].duration==0.2f&&f.effects[0].radius==115.f&&f.effects[0].channel==1);
    a::NpcSpread position{10.f,20.f,30.f},velocity{1.f,2.f,3.f};
    assert(api->kinematics(g,&position,&velocity));
    assert(api->detonate(g,0,&host,&f,&report)&&report.code==a::GrenadeBlocked);
    a::GrenadeTouch touch{1,1,4,1,0,1};
    assert(api->touch(g,&touch,&host,&f,&report)&&report.code==a::GrenadeBlocked&&report.touch_gate==2);
    touch.other_flags_120=0;touch.virtual_fc_matches=1;
    assert(api->touch(g,&touch,&host,&f,&report)&&report.code==a::GrenadeBlocked&&report.touch_gate==3);
    touch.virtual_fc_matches=0;f.effects.clear();
    assert(api->touch(g,&touch,&host,&f,&report)&&report.code==a::GrenadeApplied&&report.effects_delivered==7);
    assert(!report.state.armed&&!report.state.contact_flag&&report.state.removal_requested);
    const std::vector<unsigned> expected={a::GrenadeSetContact,a::GrenadeClearContact,a::GrenadeTraceEffect,
        a::GrenadeShake,a::GrenadeRadiusDamage,a::GrenadeExplosionSound,a::GrenadeQueueRemove};
    assert(f.effects.size()==expected.size());
    for(unsigned i=0;i<expected.size();++i)assert(f.effects[i].kind==expected[i]);
    assert(f.effects[2].decal==1&&f.effects[3].requested_amplitude==25.f&&f.effects[3].clamped_amplitude==16.f);
    assert(f.effects[4].damage==100.f&&f.effects[4].radius==250.f&&f.effects[4].type==0x40u&&f.effects[4].origin.x==10.f);
    assert(f.effects[5].type==0x02000001u&&f.effects[5].radius==375.f&&f.effects[5].duration==1.f);
    assert(api->touch(g,&touch,&host,&f,&report)&&report.code==a::GrenadeDisarmed&&f.effects.size()==7);
    assert(map->remove(g)&&!api->state(g,&state));
    assert(map->create(g2,"grenade_ar")==a::MapApplied&&api->spawn(g2,&spawn)&&api->observe_contact(g2,1));
    f.expected=g2;f.effects.clear();f.fail_kind=a::GrenadeRadiusDamage;
    assert(api->detonate(g2,2,&host,&f,&report)&&report.code==a::GrenadeUncertain&&report.state.uncertain&&!report.state.armed);
    assert(report.effects_attempted==4&&report.effects_delivered==3&&!report.state.removal_requested);
    assert(api->detonate(g2,2,&host,&f,&report)&&report.code==a::GrenadeRejected&&f.effects.size()==4);
    assert(!api->observe_contact(g2,1)&&map->remove(g2)&&map->end_level(epoch)&&campaign->end_campaign());
}
struct RifleLaunchFixture {
    const a::RifleGrenadeApi* launcher;
    const a::ARGrenadeApi* grenade;
    const a::MapEntityApi* map;
    a::MapEntityRef rifle,owner,next;
    a::RifleLaunchObservation auth{a::RifleOwnerPlayer,1,1,1,0,0,0,a::RifleAmmoFinite,3};
    int prepares=0,commits=0,aborts=0,mode=0;
    float expected_scale=2000.f;
    static RifleLaunchFixture& get(void* c){return *static_cast<RifleLaunchFixture*>(c);}
    static bool same(a::MapEntityRef x,a::MapEntityRef y){
        return x.epoch==y.epoch&&x.index==y.index&&x.generation==y.generation;
    }
    static int SIN_FIXED_CALL authorize(void* c,a::MapEntityRef rifle,a::MapEntityRef owner,a::RifleLaunchObservation* out){
        auto& f=get(c);assert(same(rifle,f.rifle)&&same(owner,f.owner));
        assert(!f.map->remove(rifle));
        *out=f.auth;return 1;
    }
    static int SIN_FIXED_CALL prepare(void* c,a::MapEntityRef owner,const char* classname,const char* model,
            const a::GrenadeSpawn* spawn,a::MapEntityRef* ref){
        auto& f=get(c);assert(same(owner,f.owner));++f.prepares;
        assert(std::strcmp(classname,"grenade_ar")==0&&std::strcmp(model,"models/weapons/grenades/ar_grenade.mdl")==0);
        assert(spawn->origin.x==1.f&&spawn->velocity.x==f.expected_scale&&spawn->velocity.y==0.f&&
            spawn->velocity.z==0.f&&spawn->time==2.f);
        assert(f.map->create(f.next,"grenade_ar")==a::MapRejected);
        if(f.mode==1)return 0;
        *ref=f.next;return 1;
    }
    static int SIN_FIXED_CALL commit(void* c,a::MapEntityRef ref,a::MapEntityRef owner,std::int32_t debit){
        auto& f=get(c);assert(same(ref,f.next)&&same(owner,f.owner));++f.commits;
        a::GrenadeSnapshot s{};
        assert(!f.grenade->state(ref,&s)&&!f.map->remove(ref));
        assert(debit==(f.auth.owner_kind==a::RifleOwnerPlayer?1:0));
        if(f.mode==3)throw 1;
        if(f.mode==2||f.mode==4)return 0;
        f.auth.ammo_remaining-=debit;return 1;
    }
    static int SIN_FIXED_CALL abort(void* c,a::MapEntityRef ref){
        auto& f=get(c);assert(same(ref,f.next));++f.aborts;
        return f.mode!=4;
    }
};
static void test_rifle_grenade(const a::RifleGrenadeApi* rifle,const a::ARGrenadeApi* grenade,
        const a::MapEntityApi* map,const a::Api* api){
    assert(rifle&&rifle->version==1&&rifle->size==sizeof(*rifle)&&api->begin_campaign(2));
    const auto epoch=map->begin_level("rifle_grenade");assert(epoch);
    const a::MapEntityRef weapon{epoch,1,1},owner{epoch,100,1};
    assert(map->create(weapon,"weapon_assault_rifle")==a::MapApplied&&map->spawn(weapon)==a::MapApplied);
    RifleLaunchFixture f{rifle,grenade,map,weapon,owner,{epoch,2,1}};
    const a::RifleLaunchHost host{1,sizeof(a::RifleLaunchHost),&RifleLaunchFixture::authorize,
        &RifleLaunchFixture::prepare,&RifleLaunchFixture::commit,&RifleLaunchFixture::abort};
    a::RifleLaunchInput input{owner,{1.f,2.f,3.f},{1.f,0.f,0.f},2.f};
    a::RifleLaunchReport report{};
    auto fire=[&](){return rifle->launch(weapon,&input,&host,&f,&report);};
    assert(rifle->uncertain(weapon)==0);
    f.auth.selected=0;
    assert(fire()&&report.code==a::RifleUnauthorized&&!f.prepares);
    f.auth.selected=1;f.auth.ammo_kind=a::RifleAmmoUnknown;
    assert(fire()&&report.code==a::RifleAmmoUnverified&&!f.prepares);
    f.auth.ammo_kind=a::RifleAmmoFinite;f.auth.ammo_remaining=0;
    assert(fire()&&report.code==a::RifleNoAmmo&&!f.prepares);
    f.auth.ammo_remaining=3;input.time=std::numeric_limits<float>::infinity();
    assert(fire()&&report.code==a::RifleInvalidInput&&!f.prepares);
    input.time=2.f;f.mode=1;
    assert(fire()&&report.code==a::RiflePrepareRefused&&f.prepares==1&&!f.commits);
    f.mode=2;
    assert(fire()&&report.code==a::RifleCommitRefused&&report.prepared&&report.attached&&
        report.commit_attempted&&!report.committed&&report.aborted&&f.auth.ammo_remaining==3);
    assert(map->count(epoch)==1&&map->create(f.next,"grenade_ar")==a::MapRejected);
    f.next.generation=2;f.mode=0;
    assert(fire()&&report.code==a::RifleLaunched&&report.committed&&report.planned_debit==1&&
        f.auth.ammo_remaining==2&&map->count(epoch)==2);
    a::GrenadeSnapshot state{};
    assert(grenade->state(f.next,&state)&&state.armed&&state.velocity.x==2000.f&&state.next_think==2.1f);
    const auto first=f.next;
    f.next={epoch,3,1};f.expected_scale=1000.f;f.auth.owner_kind=a::RifleOwnerGrunt;
    f.auth.grunt_authorized=0;
    assert(fire()&&report.code==a::RifleUnauthorized);
    f.auth.grunt_authorized=1;
    assert(fire()&&report.code==a::RifleLaunched&&!report.planned_debit&&f.auth.ammo_remaining==2);
    assert(grenade->state(f.next,&state)&&state.armed&&state.velocity.x==1000.f);
    const auto second=f.next;
    f.next={epoch,4,1};f.mode=3;
    assert(fire()&&report.code==a::RifleLaunchUncertain&&report.uncertain&&report.commit_attempted&&!report.committed);
    assert(grenade->state(f.next,&state)&&state.uncertain&&rifle->uncertain(weapon)==1);
    const int calls=f.prepares;
    assert(fire()&&report.code==a::RifleRejected&&f.prepares==calls);
    assert(map->remove(first)&&map->remove(second)&&map->remove(f.next)&&map->remove(weapon));
    assert(rifle->uncertain(weapon)==-1&&map->end_level(epoch)&&api->end_campaign());
}
struct MagnumFixture {
    const a::MagnumApi* magnum;
    const a::MapEntityApi* map;
    a::MapEntityRef gun;
    std::vector<int> order;
    float time=10.f,primary=20.f,pending_time=0.f;
    int count=2,change_count=-1,fail_at=0;
    unsigned clocks=0;
    std::uint32_t damage=0x00400002u,trace_flags=0;
    bool zero_delta=false;
    static MagnumFixture& get(void* c){return *static_cast<MagnumFixture*>(c);}
    bool step(int op){
        order.push_back(op);
        assert(!map->remove(gun)&&!magnum->acknowledge(gun));
        return static_cast<int>(order.size())!=fail_at;
    }
    static int SIN_FIXED_CALL clock(void* c,float* out){
        auto& f=get(c);*out=f.time+static_cast<float>(f.clocks++)*0.125f;return f.step(1);
    }
    static int SIN_FIXED_CALL read_primary(void* c,a::MapEntityRef,float* out){
        auto& f=get(c);*out=f.primary;return f.step(2);
    }
    static int SIN_FIXED_CALL write_primary(void* c,a::MapEntityRef,float value){
        auto& f=get(c);f.primary=value;return f.step(3);
    }
    static int SIN_FIXED_CALL write_count(void* c,a::MapEntityRef,int value){
        auto& f=get(c);f.count=value;return f.step(4);
    }
    static int SIN_FIXED_CALL primary_attack(void* c,a::MapEntityRef){
        auto& f=get(c);assert(f.primary==f.time+0.125f);
        if(f.change_count>=0)f.count=f.change_count;
        return f.step(5);
    }
    static int SIN_FIXED_CALL read_count(void* c,a::MapEntityRef,int* out){
        auto& f=get(c);*out=f.count;return f.step(6);
    }
    static int SIN_FIXED_CALL write_pending(void* c,a::MapEntityRef,float value){
        auto& f=get(c);f.pending_time=value;return f.step(7);
    }
    static int SIN_FIXED_CALL base_update(void* c,a::MapEntityRef){return get(c).step(8);}
    static int SIN_FIXED_CALL base_impact(void* c,a::MapEntityRef,std::uint32_t damage){
        auto& f=get(c);assert(damage==f.damage);return f.step(9);
    }
    static int SIN_FIXED_CALL trace(void* c,a::MapEntityRef,a::MagnumTraceObservation* out){
        auto& f=get(c);*out={f.trace_flags,{1.f,2.f,3.f},f.zero_delta?a::NpcSpread{1.f,2.f,3.f}:a::NpcSpread{4.f,6.f,3.f}};
        return f.step(10);
    }
    static int SIN_FIXED_CALL smoke(void* c,a::MapEntityRef,const a::MagnumSmoke* e){
        auto& f=get(c);
        assert(e->pvs_origin.z==3.f&&e->pvs_origin.x==(f.zero_delta?1.f:4.f));
        assert(std::fabs(e->position.x-(f.zero_delta?0.f:0.6f))<0.000001f&&
            std::fabs(e->position.y-(f.zero_delta?0.f:0.8f))<0.000001f&&e->position.z==0.f);
        assert(e->delay==0.25f&&e->scale==2.5f&&e->framerate==10&&std::strcmp(e->model,"sprites/blueglow1.vmt")==0);
        return f.step(11);
    }
    static int SIN_FIXED_CALL decal(void* c,a::MapEntityRef,const char* name){
        auto& f=get(c);
        if(std::strcmp(name,"DUAGDecal1")==0)return f.step(12);
        assert(std::strcmp(name,"DUAGDecal2")==0);return f.step(13);
    }
};
static void test_magnum(const a::MagnumApi* magnum,const a::MapEntityApi* map,const a::Api* api){
    assert(magnum&&magnum->version==1&&magnum->size==sizeof(*magnum)&&api->begin_campaign(2));
    const auto epoch=map->begin_level("magnum");assert(epoch);
    const a::MapEntityRef gun{epoch,1,1};
    assert(map->create(gun,"weapon_magnum")==a::MapApplied&&map->spawn(gun)==a::MapApplied);
    MagnumFixture f{magnum,map,gun,{}};
    const a::MagnumFrameHost frame_host{1,sizeof(a::MagnumFrameHost),&MagnumFixture::clock,
        &MagnumFixture::read_primary,&MagnumFixture::write_primary,&MagnumFixture::write_count,
        &MagnumFixture::primary_attack,&MagnumFixture::read_count,&MagnumFixture::write_pending,&MagnumFixture::base_update};
    a::MagnumFrameObservation in{0,0.f};a::MagnumFrameReport report{};
    auto step=[&](){return magnum->frame(gun,&in,&frame_host,&f,&report);};
    assert(step()&&report.code==a::MagnumBaseOnly&&f.order==std::vector<int>{8}&&f.clocks==0);
    in={2,11.f};f.order.clear();
    assert(step()&&report.code==a::MagnumBaseOnly&&(f.order==std::vector<int>{1,8}));
    in.pending_deadline=10.f;f.order.clear();f.clocks=0;
    assert(step()&&report.code==a::MagnumDeferredPrimary&&report.primary_delivered&&report.base_delivered&&
        report.pending_after==1&&f.primary==20.f&&f.pending_time==10.25f+0.05f);
    assert((f.order==std::vector<int>{1,2,1,3,4,5,3,6,1,7,8}));
    f.order.clear();f.clocks=0;in={1,10.f};
    assert(step()&&report.code==a::MagnumDeferredPrimary&&report.pending_after==0&&f.clocks==2);
    assert((f.order==std::vector<int>{1,2,1,3,4,5,3,6,8}));
    f.order.clear();f.clocks=0;in={2,10.f};f.change_count=0;
    assert(step()&&report.code==a::MagnumDeferredPrimary&&report.pending_after==0&&f.clocks==2);
    f.order.clear();f.clocks=0;f.fail_at=6;
    assert(step()&&report.code==a::MagnumFrameUncertain&&report.needs_resync&&!report.primary_delivered);
    assert(step()&&report.code==a::MagnumFrameRejected&&f.order.size()==6);
    assert(magnum->acknowledge(gun));f.primary=20.f;f.fail_at=0;f.order.clear();
    const a::MagnumImpactHost impact_host{1,sizeof(a::MagnumImpactHost),&MagnumFixture::base_impact,
        &MagnumFixture::trace,&MagnumFixture::smoke,&MagnumFixture::decal};
    a::MagnumImpactReport impact{};
    auto effect=[&](){return magnum->impact(gun,f.damage,&impact_host,&f,&impact);};
    f.damage=2;
    assert(effect()&&impact.code==a::MagnumImpactBaseOnly&&f.order==std::vector<int>{9});
    f.damage=0x00400002u;f.trace_flags=4;f.order.clear();
    assert(effect()&&impact.code==a::MagnumImpactTraceBlocked&&(f.order==std::vector<int>{9,10}));
    f.trace_flags=2;f.order.clear();
    assert(effect()&&impact.code==a::MagnumImpactDUAG&&impact.smoke_delivered&&impact.decals_delivered==2);
    assert((f.order==std::vector<int>{9,10,11,12,13}));
    f.zero_delta=true;f.order.clear();
    assert(effect()&&impact.code==a::MagnumImpactDUAG);
    f.order.clear();f.fail_at=5;
    assert(effect()&&impact.code==a::MagnumImpactUncertain&&impact.needs_resync&&impact.decals_delivered==1);
    assert(effect()&&impact.code==a::MagnumImpactRejected&&f.order.size()==5);
    assert(magnum->acknowledge(gun)&&map->remove(gun)&&!magnum->acknowledge(gun));
    assert(map->end_level(epoch)&&api->end_campaign());
}
struct AmmoFixture {
    const a::CombatAmmoApi* ammo=nullptr;
    const a::MapEntityApi* map=nullptr;
    a::MapEntityRef owner{};
    std::int32_t counts[32]{};
    std::int32_t maxima[32]{};
    std::vector<int> log;
    int fail_store=-1;
    static AmmoFixture& get(void* c){return *static_cast<AmmoFixture*>(c);}
    void check(a::MapEntityRef ref){
        assert(ref.epoch==owner.epoch&&ref.index==owner.index&&ref.generation==owner.generation);
        assert(!map->end_level(owner.epoch));
    }
    static int SIN_FIXED_CALL live(void* c,a::MapEntityRef ref){
        auto& f=get(c);f.check(ref);f.log.push_back(1);return 1;
    }
    static int SIN_FIXED_CALL max_count(void* c,std::int32_t i,std::int32_t* out){
        auto& f=get(c);assert(i>=0&&i<32);f.log.push_back(100+i);*out=f.maxima[i];return 1;
    }
    static int SIN_FIXED_CALL read(void* c,a::MapEntityRef ref,std::int32_t i,std::int32_t* out){
        auto& f=get(c);f.check(ref);assert(i>=0&&i<32);f.log.push_back(200+i);
        const a::AmmoRequest nested{a::AmmoGetCount,-1,0};a::AmmoReport r{};
        assert(f.ammo->apply(ref,&nested,nullptr,nullptr,&r)&&r.code==a::AmmoRejected);
        *out=f.counts[i];return 1;
    }
    static int SIN_FIXED_CALL notify(void* c,a::MapEntityRef ref){
        auto& f=get(c);f.check(ref);f.log.push_back(2);return 1;
    }
    static int SIN_FIXED_CALL store(void* c,a::MapEntityRef ref,std::int32_t i,std::int32_t value){
        auto& f=get(c);f.check(ref);assert(i>=0&&i<32);f.log.push_back(300+i);
        if(i==f.fail_store)return 0;
        f.counts[i]=value;return 1;
    }
    a::AmmoHost host(){return {1,sizeof(a::AmmoHost),&live,&max_count,&read,&notify,&store};}
    a::AmmoReport apply(std::uint32_t op,int index,int amount=0){
        const a::AmmoRequest req{op,index,amount};const auto cb=host();a::AmmoReport r{};
        assert(ammo->apply(owner,&req,&cb,this,&r));return r;
    }
};
static void test_shared_ammo(const a::CombatAmmoApi* ammo,const a::MapEntityApi* map,const a::Api* api){
    assert(ammo&&ammo->version==1&&ammo->size==sizeof(*ammo)&&api->begin_campaign(2));
    const auto epoch=map->begin_level("ammo");assert(epoch);
    AmmoFixture f;f.ammo=ammo;f.map=map;f.owner={epoch,100,1};
    f.counts[2]=7;f.maxima[2]=40;
    auto r=f.apply(a::AmmoGetCount,-1);
    assert(r.code==a::AmmoRead&&r.value==0&&f.log.empty());
    r=f.apply(a::AmmoGetCount,32);assert(r.code==a::AmmoRejected&&f.log.empty());
    r=f.apply(a::AmmoRemove,-1,0);assert(r.code==a::AmmoUnchanged&&f.log.empty());
    r=f.apply(a::AmmoGetCount,2);
    assert(r.code==a::AmmoRead&&r.value==7&&(f.log==std::vector<int>{1,102,202}));
    f.log.clear();r=f.apply(a::AmmoRemove,2,3);
    assert(r.code==a::AmmoChanged&&r.value==4&&f.counts[2]==4&&r.changed_slots==1);
    assert((f.log==std::vector<int>{1,102,202,2,302}));
    f.log.clear();r=f.apply(a::AmmoRemove,2,9);
    assert(r.code==a::AmmoChanged&&r.value==0&&f.counts[2]==0);
    f.log.clear();r=f.apply(a::AmmoRemove,2,9);
    assert(r.code==a::AmmoUnchanged&&(f.log==std::vector<int>{1,102,202}));
    f.maxima[2]=-2;f.counts[2]=123;f.log.clear();
    r=f.apply(a::AmmoGetCount,2);assert(r.code==a::AmmoRead&&r.infinite&&r.value==999&&(f.log==std::vector<int>{1,102}));
    f.log.clear();r=f.apply(a::AmmoRemove,2,1);
    assert(r.code==a::AmmoUnchanged&&r.infinite&&f.counts[2]==123&&(f.log==std::vector<int>{1,102}));
    f.log.clear();r=f.apply(a::AmmoSet,2,-5);
    assert(r.code==a::AmmoChanged&&f.counts[2]==-5&&(f.log==std::vector<int>{1,202,2,302}));
    f.maxima[2]=40;f.counts[2]=(std::numeric_limits<std::int32_t>::min)();f.log.clear();
    r=f.apply(a::AmmoRemove,2,1);assert(r.code==a::AmmoChanged&&r.value==(std::numeric_limits<std::int32_t>::max)());
    f.counts[0]=1;f.counts[31]=2;f.log.clear();f.fail_store=2;
    r=f.apply(a::AmmoClear,0);
    assert(r.code==a::AmmoUncertain&&r.changed_slots==1&&f.counts[0]==0&&f.counts[2]!=0&&f.counts[31]==2);
    f.fail_store=-1;f.log.clear();r=f.apply(a::AmmoClear,0);
    assert(r.code==a::AmmoChanged&&r.changed_slots==2&&f.counts[31]==0&&f.counts[2]==0);
    // Real original default: omitted max callback uses the shared
    // definition table, with no synthetic max lookup.
    f.log.clear();f.counts[2]=7;auto cb=f.host();cb.max_count=nullptr;
    const a::AmmoRequest request{a::AmmoRemove,2,2};
    assert(ammo->apply(f.owner,&request,&cb,&f,&r)&&r.code==a::AmmoChanged&&r.value==5);
    assert((f.log==std::vector<int>{1,202,2,302}));
    assert(map->end_level(epoch));
    r=f.apply(a::AmmoGetCount,2);assert(r.code==a::AmmoRejected);
    assert(api->end_campaign());
}
struct MagnumSecondaryFixture {
    const a::MapEntityApi* map;
    const a::MagnumApi* magnum;
    a::MapEntityRef gun,owner,other;
    std::vector<unsigned> log;
    bool direct=true,uses_clip=true,zoom=false,fail_fire=false;
    int clip=3,reserve=4,clock=0,random=0,fire_calls=0,impact_calls=0;
    AmmoFixture* shared=nullptr;
    static MagnumSecondaryFixture& get(void* c){return *static_cast<MagnumSecondaryFixture*>(c);}
    void check(a::MapEntityRef ref){
        assert(ref.epoch==gun.epoch&&ref.index==gun.index&&ref.generation==gun.generation);
        assert(!map->remove(gun)&&!magnum->acknowledge(gun));
    }
    void reset(){log.clear();clock=random=0;}
    static int SIN_FIXED_CALL query(void* c,a::MapEntityRef g,a::MapEntityRef,std::uint32_t op,
            std::uint32_t arg,a::MagnumSecondaryValue* v){
        auto& f=get(c);f.check(g);f.log.push_back(100+op);
        switch(op){
        case a::MagSecondaryUsesClip:v->integer=f.uses_clip;break;
        case a::MagSecondaryReadClip:v->integer=f.clip;break;
        case a::MagSecondaryOwnerCapable:v->integer=f.direct;break;
        case a::MagSecondarySelection:v->integer=0xc2;break;
        case a::MagSecondaryInterval:assert(arg==77);v->real=2.f;break;
        case a::MagSecondaryClock:v->real=f.clock++==0?10.f:12.f;break;
        case a::MagSecondaryOrigin:v->vector={1.f,2.f,3.f};break;
        case a::MagSecondaryDirectVector:assert(arg==0x3db27eb6u);v->vector={1.f,0.f,0.f};break;
        case a::MagSecondaryFallbackVector:v->vector={0.f,1.f,0.f};break;
        case a::MagSecondaryAmmoType:v->integer=f.shared?2:92;break;
        case a::MagSecondaryReserveCount:
            if(f.shared){
                assert(arg==2);
                const auto result=f.shared->apply(a::AmmoGetCount,2);
                assert(result.code==a::AmmoRead);
                // Wrong operation and wrong actor are refused even inside a
                // permitted owner query.
                const auto cb=f.shared->host();a::AmmoReport blocked{};
                const a::AmmoRequest remove{a::AmmoRemove,2,1},get{a::AmmoGetCount,2,0};
                assert(f.shared->ammo->apply(f.owner,&remove,&cb,f.shared,&blocked)&&blocked.code==a::AmmoRejected);
                auto other=f.owner;++other.generation;
                assert(f.shared->ammo->apply(other,&get,&cb,f.shared,&blocked)&&blocked.code==a::AmmoRejected);
                v->integer=result.value;
            }else{assert(arg==92);v->integer=f.reserve;}
            break;
        case a::MagSecondarySpread:v->vector={0.1f,0.1f,0.1f};break;
        case a::MagSecondaryZoomed:v->integer=f.zoom;break;
        case a::MagSecondaryOwnerFlags:v->integer=0x800;break;
        case a::MagSecondaryAuxVector:v->vector={0.f,1.f,0.f};break;
        case a::MagSecondaryOwnerAngles:v->vector={10.f,20.f,30.f};break;
        case a::MagSecondaryRandomInt:v->integer=f.random++==0?1:-1;break;
        case a::MagSecondaryRandomFloat:v->real=0.5f;break;
        default:assert(false);return 0;
        }
        return 1;
    }
    static int SIN_FIXED_CALL effect(void* c,a::MapEntityRef g,a::MapEntityRef,std::uint32_t op,
            const a::MagnumSecondaryValue* v,std::int32_t extra){
        auto& f=get(c);f.check(g);f.log.push_back(200+op);
        switch(op){
        case a::MagSecondaryEvent:assert(v->integer==3&&extra==0);break;
        case a::MagSecondaryActivity:assert(v->integer==0xc2);break;
        case a::MagSecondaryPrimaryTimer:assert(v->real==12.f);break;
        case a::MagSecondarySecondaryTimer:assert(v->real==14.f);break;
        case a::MagSecondaryOwnerAnimation:assert(v->integer==5);break;
        case a::MagSecondaryDebitClip:f.clip-=v->integer;break;
        case a::MagSecondaryDebitReserve:
            if(f.shared){assert(extra==2);assert(f.shared->apply(a::AmmoRemove,2,v->integer).code==a::AmmoChanged);}
            else{assert(extra==92);f.reserve-=v->integer;}
            break;
        case a::MagSecondaryFeedback:assert(v->integer==6);break;
        case a::MagSecondaryAngleHelper:assert(v->vector.x==11.f&&v->vector.y==19.f&&v->vector.z==0.f);break;
        case a::MagSecondaryPunch:assert(v->vector.x==-8.f&&v->vector.y==0.5f&&v->vector.z==0.f);break;
        default:break;
        }
        return 1;
    }
    static int SIN_FIXED_CALL base_impact(void* c,a::MapEntityRef g,std::uint32_t damage){
        auto& f=get(c);f.check(g);assert(damage==2);
        ++f.impact_calls;
        // Even a same-entity recursive impact must be rejected.
        a::MagnumImpactReport report{};
        const a::MagnumImpactHost host{1,sizeof(a::MagnumImpactHost),&base_impact,nullptr,nullptr,nullptr};
        assert(f.magnum->impact(g,2,&host,c,&report)&&report.code==a::MagnumImpactRejected);
        return 1;
    }
    static int SIN_FIXED_CALL fire(void* c,a::MapEntityRef owner,const a::MagnumFireRecord* record){
        auto& f=get(c);f.check(record->weapon);assert(owner.index==f.owner.index);
        ++f.fire_calls;f.log.push_back(300);
        assert(record->distance==56755.83984375f&&record->ammo_type==(f.shared?2:92)&&record->tracer_frequency==1&&
            record->force_scale==1.f&&record->origin.x==1.f&&record->spread.x==0.1f);
        a::MagnumImpactReport report{};
        const a::MagnumImpactHost host{1,sizeof(a::MagnumImpactHost),&base_impact,nullptr,nullptr,nullptr};
        assert(f.magnum->impact(f.other,2,&host,c,&report)&&report.code==a::MagnumImpactRejected);
        assert(f.magnum->impact(f.gun,2,&host,c,&report)&&report.code==a::MagnumImpactBaseOnly);
        f.check(f.gun); // nested guard must not release the outer guard
        return !f.fail_fire;
    }
};
static void test_magnum_secondary(const a::MagnumSecondaryApi* secondary,const a::MagnumApi* magnum,
        const a::MapEntityApi* map,const a::Api* api,const a::CombatAmmoApi* ammo){
    assert(secondary&&secondary->version==1&&secondary->size==sizeof(*secondary)&&api->begin_campaign(2));
    const auto epoch=map->begin_level("magnum_secondary");assert(epoch);
    const a::MapEntityRef gun{epoch,1,1},owner{epoch,100,1},other{epoch,2,1};
    assert(map->create(gun,"weapon_magnum")==a::MapApplied&&map->spawn(gun)==a::MapApplied);
    assert(map->create(other,"weapon_magnum")==a::MapApplied&&map->spawn(other)==a::MapApplied);
    MagnumSecondaryFixture f{map,magnum,gun,owner,other,{}};
    const a::MagnumSecondaryHost host{1,sizeof(a::MagnumSecondaryHost),&MagnumSecondaryFixture::query,
        &MagnumSecondaryFixture::effect,&MagnumSecondaryFixture::fire};
    a::MagnumSecondaryObservation input{owner,0,77};
    a::MagnumSecondaryReport report{};
    const auto run=[&](){return secondary->attack(gun,&input,&host,&f,&report);};
    f.clip=0;
    assert(run()&&report.code==a::MagSecondaryEmptyClip&&report.effects_delivered==1&&!report.fire_attempted);
    assert((f.log==std::vector<unsigned>{100+a::MagSecondaryUsesClip,100+a::MagSecondaryReadClip,200+a::MagSecondaryEmpty}));
    f.reset();f.clip=3;
    assert(run()&&report.code==a::MagSecondaryRejected&&!report.effects_attempted);
    f.reset();input.owner_present=1;
    assert(run()&&report.code==a::MagSecondaryCompleted&&f.clip==2&&f.impact_calls==1&&report.record.shots==1);
    assert(report.record.aux_enabled&&report.record.aux_position.x==1.f&&report.record.aux_position.y==6.f);
    const std::vector<unsigned> expected={
        100+a::MagSecondaryUsesClip,100+a::MagSecondaryReadClip,100+a::MagSecondaryOwnerCapable,
        200+a::MagSecondaryEvent,100+a::MagSecondarySelection,200+a::MagSecondaryActivity,
        100+a::MagSecondaryInterval,100+a::MagSecondaryClock,200+a::MagSecondaryPrimaryTimer,
        100+a::MagSecondaryClock,200+a::MagSecondarySecondaryTimer,200+a::MagSecondaryOwnerFire,
        200+a::MagSecondaryOwnerAnimation,100+a::MagSecondaryOrigin,100+a::MagSecondaryDirectVector,
        100+a::MagSecondaryUsesClip,100+a::MagSecondaryReadClip,200+a::MagSecondaryDebitClip,
        100+a::MagSecondaryAmmoType,100+a::MagSecondarySpread,100+a::MagSecondaryZoomed,
        100+a::MagSecondaryOwnerFlags,200+a::MagSecondaryOwnerFlagHelper,100+a::MagSecondaryAuxVector,
        300,200+a::MagSecondaryFeedback,100+a::MagSecondaryOwnerAngles,100+a::MagSecondaryRandomInt,
        100+a::MagSecondaryRandomInt,200+a::MagSecondaryAngleHelper,100+a::MagSecondaryRandomFloat,200+a::MagSecondaryPunch};
    assert(f.log==expected);
    f.reset();f.clip=-1;f.zoom=true;
    assert(run()&&report.code==a::MagSecondaryCompleted&&report.record.shots==-1&&f.clip==0&&!report.record.aux_enabled);
    f.reset();f.uses_clip=false;f.reserve=1;
    assert(run()&&report.code==a::MagSecondaryCompleted&&f.reserve==0&&report.record.shots==1);
    assert(std::count(f.log.begin(),f.log.end(),100+a::MagSecondaryReserveCount)==2);
    // Real cross-export composition: the original reserve query and
    // debit invoke the shared loaded-DLL ammo implementation.
    AmmoFixture shared;shared.ammo=ammo;shared.map=map;shared.owner=owner;
    shared.counts[2]=3;shared.maxima[2]=40;f.shared=&shared;f.reset();
    assert(run()&&report.code==a::MagSecondaryCompleted&&shared.counts[2]==2&&report.record.ammo_type==2);
    assert((shared.log==std::vector<int>{1,102,202,1,102,202,2,302}));
    f.shared=nullptr;
    f.reset();f.direct=false;
    assert(run()&&report.code==a::MagSecondaryCompleted&&report.record.direction.y==1.f&&f.reserve==0);
    assert(std::find(f.log.begin(),f.log.end(),200+a::MagSecondaryOwnerFire)==f.log.end()&&
        std::find(f.log.begin(),f.log.end(),200+a::MagSecondaryFeedback)==f.log.end());
    f.reset();f.fail_fire=true;
    assert(run()&&report.code==a::MagSecondaryUncertain&&report.needs_resync&&report.fire_attempted&&!report.fire_delivered);
    const auto before=f.log.size();
    assert(run()&&report.code==a::MagSecondaryRejected&&f.log.size()==before);
    assert(magnum->acknowledge(gun));
    f.reset();f.fail_fire=false;
    assert(run()&&report.code==a::MagSecondaryCompleted);
    assert(map->remove(gun)&&run()&&report.code==a::MagSecondaryRejected);
    assert(map->end_level(epoch)&&api->end_campaign());
}
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

// Exact MSVC x86 virtual slots, independent synthetic objects. The
// manager models original ownership without deleting its fixture storage.
struct NativeEventFixture {
    virtual ~NativeEventFixture()=default;
    virtual void slot1(){assert(false);}
    virtual void slot2(){assert(false);}
    virtual void slot3(){assert(false);}
    virtual void slot4(){assert(false);}
    virtual void slot5(){assert(false);}
    virtual void slot6(){assert(false);}
    virtual void slot7(){assert(false);}
    virtual void slot8(){assert(false);}
    virtual void slot9(){assert(false);}
    virtual void integer(const char* key,int v){
        keys.emplace_back(key);ints.push_back(v);if(fail_set)throw 1;
    }
    virtual void real(const char* key,float v){keys.emplace_back(key);floats.push_back(v);}
    virtual void string(const char* key,const char* v){keys.emplace_back(key);name=v;}
    std::vector<std::string> keys;
    std::vector<int> ints;
    std::vector<float> floats;
    std::string name;
    bool fail_set=false;
    void reset(){keys.clear();ints.clear();floats.clear();name.clear();}
};
struct NativeEventManagerFixture {
    virtual ~NativeEventManagerFixture()=default;
    virtual void slot1(){assert(false);}
    virtual void slot2(){assert(false);}
    virtual void slot3(){assert(false);}
    virtual void slot4(){assert(false);}
    virtual void slot5(){assert(false);}
    virtual void* create(const char* name,bool force){
        assert(!force&&!live);
        ++creates;event_name=name;
        assert(!api->begin_campaign(1)&&!api->end_campaign()&&!api->set_skill(1));
        if(nested)assert(publish(this,nested)==a::EventRejected);
        if(throw_create)throw 2;
        if(unavailable)return nullptr;
        live=true;event.reset();return &event;
    }
    virtual bool fire(void* e,bool no_broadcast){
        assert(e==&event&&live&&!no_broadcast);
        ++fires;live=false;
        if(throw_fire)throw 3;
        return fire_ok;
    }
    virtual void slot8(){assert(false);}
    virtual void slot9(){assert(false);}
    virtual void release(void* e){
        assert(e==&event&&live);
        ++frees;live=false;if(throw_free)throw 4;
    }
    NativeEventFixture event;
    const a::Api* api=nullptr;
    a::PublishCampaignEvent publish=nullptr;
    const a::NativeTriggerEvent* nested=nullptr;
    std::string event_name;
    int creates=0,fires=0,frees=0;
    bool live=false,unavailable=false,fire_ok=true,throw_create=false,throw_fire=false,throw_free=false;
};
static void test_native_engine_event(HMODULE lib,const a::Api* api,const a::MapEntityApi* map,
                                     const a::CampaignTriggerApi* tr){
    auto publish=reinterpret_cast<a::PublishCampaignEvent>(GetProcAddress(lib,a::event_publish_export_name));
    assert(publish);
    NativeEventManagerFixture manager;manager.api=api;manager.publish=publish;
    a::NativeTriggerEvent event{a::TriggerDynamicEvent,0,5,-1,17,12.5f,1.f,2.f,3.f,"gate"};
    manager.nested=&event;
    assert(!publish(nullptr,&event)&&!publish(&manager,nullptr));
    assert(!publish(reinterpret_cast<void*>(1),&event));
    event.advisor=0;
    assert(publish(&manager,&event)==a::EventRejected&&manager.creates==0);
    event.advisor=-1;
    assert(api->begin_campaign(3));
    assert(publish(&manager,&event)==a::EventFired);
    assert(manager.event_name=="dynamic_difficulty"&&manager.fires==1&&!manager.frees&&!manager.live);
    assert((manager.event.keys==std::vector<std::string>{"descriptor","owner","subject","name","decision","value","x","y","z"}));
    assert((manager.event.ints==std::vector<int>{0,5,-1,17}));
    assert((manager.event.floats==std::vector<float>{12.5f,1.f,2.f,3.f}));
    assert(manager.event.name=="gate");
    manager.fire_ok=false;
    assert(publish(&manager,&event)==a::EventFireReturnedFalse&&manager.fires==2&&!manager.frees);
    manager.fire_ok=true;manager.unavailable=true;
    assert(publish(&manager,&event)==a::EventUnavailable&&manager.fires==2&&!manager.frees);
    manager.unavailable=false;manager.throw_create=true;
    assert(publish(&manager,&event)==a::EventCreateUncertain&&!manager.live);
    manager.throw_create=false;manager.event.fail_set=true;
    assert(publish(&manager,&event)==a::EventDiscarded&&manager.frees==1&&!manager.live);
    manager.throw_free=true;
    assert(publish(&manager,&event)==a::EventCleanupUncertain&&manager.frees==2&&!manager.live);
    manager.throw_free=false;manager.event.fail_set=false;manager.throw_fire=true;
    assert(publish(&manager,&event)==a::EventFireUncertain&&manager.fires==3&&manager.frees==2);
    manager.throw_fire=false;
    // Exercise publication through the real DLL-owned trigger callback.
    const auto epoch=map->begin_level("native_event");
    assert(epoch);
    a::MapEntityRef secret{epoch,21,1};
    NativeTriggerFixture f{map,api,tr,secret,{epoch,5,2},{epoch,7,2}};
    f.native_publish=publish;f.native_manager=&manager;
    f.expected_kind=a::TriggerGameEvent;f.expected_descriptor=11;f.expected_owner=-1;f.expected_name="trigger_secret";
    a::NativeTriggerHost host{1,sizeof(host),&NativeTriggerFixture::live,&NativeTriggerFixture::is_player,
        &NativeTriggerFixture::base,&NativeTriggerFixture::warn,&NativeTriggerFixture::owner,
        &NativeTriggerFixture::special,&NativeTriggerFixture::publish,&NativeTriggerFixture::queue};
    assert(map->create(secret,"trigger_secret")==a::MapApplied);
    a::NativeTriggerSpawn spawn{0,8,0};a::NativeTriggerReport report{};
    assert(tr->spawn(secret,&spawn,&host,&f,&report)&&report.event_published&&report.code==a::TriggerHandled);
    assert(manager.event_name=="game"&&manager.fires==4&&manager.frees==2);
    assert((manager.event.keys==std::vector<std::string>{"descriptor","owner","subject","name"}));
    assert((manager.event.ints==std::vector<int>{11,-1,-1})&&manager.event.floats.empty());
    f.expected_descriptor=12;
    a::NativeTriggerTouch touch{0.f,0.f,0.f,0};
    assert(tr->touch(secret,f.player,&touch,&host,&f,&report)&&report.event_published&&report.removal_queued);
    assert(manager.event.ints[0]==12&&manager.fires==5&&manager.frees==2);
    assert(map->end_level(epoch)&&api->end_campaign());
}

struct NativeNpcRandomFixture {
    virtual void seed(int){assert(false);}
    virtual float random(float lo,float hi){
        assert(lo==0.f&&hi==1.f);++calls;
        assert(!api->set_skill(2)&&!api->end_campaign());
        if(throws)throw 1;
        return sample;
    }
    const a::Api* api=nullptr;
    unsigned calls=0;
    float sample=0.f;
    bool throws=false;
};
// Original VFileSystem012 primary vtable: 0 Read, 2 Open, 3 Close, 7 Size.
// Owned fake storage exercises actual DLL dispatch, including short reads.
struct CombatFileFixture {
    virtual int read(void* out,int count,void* handle){
        assert(live&&handle==this&&count>0);++reads;
        assert(!api->set_skill(2)&&!api->end_campaign()&&!rules->clear());
        if(mode==4)throw 1;
        if(mode==5)return 0;
        if(mode==6)return count+1;
        const auto n=(std::min)(static_cast<std::size_t>(count),(std::min)(data.size()-position,std::size_t{17}));
        std::memcpy(out,data.data()+position,n);position+=n;
        return static_cast<int>(n);
    }
    virtual void slot1(){assert(false);}
    virtual void* open(const char* path,const char* modeName,const char* pathID){
        assert(!live&&std::strcmp(path,"scripts/rules_combat.txt")==0&&
            std::strcmp(modeName,"rb")==0&&std::strcmp(pathID,"GAME")==0);
        ++opens;position=0;
        assert(!rules->clear()&&!api->set_skill(2));
        a::SpreadFileReport nested{};
        assert(load(this,&nested)&&nested.code==a::SpreadFileRejected);
        if(mode==1)return nullptr;
        if(mode==2)throw 1;
        live=true;return this;
    }
    virtual void close(void* handle){
        assert(live&&handle==this);++closes;live=false;
        if(mode==7)throw 1;
    }
    virtual void slot4(){assert(false);}
    virtual void slot5(){assert(false);}
    virtual void slot6(){assert(false);}
    virtual unsigned size(void* handle){
        assert(live&&handle==this);
        if(mode==3)return 262145u;
        if(mode==8)throw 1;
        return static_cast<unsigned>(data.size());
    }
    std::string data;
    std::size_t position=0;
    unsigned opens=0,reads=0,closes=0;
    int mode=0;
    bool live=false;
    const a::Api* api=nullptr;
    const a::AttackSpreadApi* rules=nullptr;
    a::LoadAttackSpreadFromEngine load=nullptr;
};
static void test_native_combat_filesystem(HMODULE lib,const a::Api* api,const a::AttackSpreadApi* rules){
    auto load=reinterpret_cast<a::LoadAttackSpreadFromEngine>(GetProcAddress(lib,a::spread_filesystem_export_name));
    assert(load&&api->begin_campaign(3)&&rules->clear());
    CombatFileFixture f;f.api=api;f.rules=rules;f.load=load;
    f.data=R"KV(AttackSpreadRules {
        class_names { npc_custom rule }
        rule { weapon_class_names { "*" generic }
        generic { target_class_names { "*" target } target 0.4 } }
    })KV";
    a::SpreadFileReport report{};
    assert(!load(&f,nullptr)&&load(nullptr,&report)&&report.code==a::SpreadFileRejected);
    assert(load(reinterpret_cast<void*>(1),&report)&&report.code==a::SpreadFileRejected&&!f.opens);
    assert(load(&f,&report)&&report.code==a::SpreadFileLoaded&&report.read_bytes==f.data.size()&&
        report.expected_bytes==f.data.size()&&report.close_attempted&&report.close_returned&&f.closes==1&&f.reads>1);
    a::AttackSpreadResult out{};
    assert(rules->query("npc_custom","rifle","player",&out)&&out.base==0.4f&&out.x==0.2f);
    const int modes[]={1,2,3,4,5,6,7,8};
    const std::uint32_t expected[]={a::SpreadFileMissing,a::SpreadFileOpenUncertain,a::SpreadFileInvalidLength,
        a::SpreadFileReadFailed,a::SpreadFileReadFailed,a::SpreadFileReadFailed,a::SpreadFileCloseUncertain,a::SpreadFileReadFailed};
    for(unsigned i=0;i<8;++i){
        f.mode=modes[i];const unsigned oldClose=f.closes;
        assert(load(&f,&report)&&report.code==expected[i]&&!f.live);
        assert(f.closes==oldClose+(i>=2?1u:0u));
        assert(report.close_attempted==(i>=2?1u:0u));
        assert(report.close_returned==(i>=2&&f.mode!=7?1u:0u));
        assert(rules->query("npc_custom","rifle","player",&out)&&out.base==0.4f&&out.x==0.2f);
    }
    f.mode=0;f.data="invalid rules";
    assert(load(&f,&report)&&report.code==a::SpreadFileParseFailed&&report.close_returned);
    assert(rules->query("npc_custom","rifle","player",&out)&&out.base==0.4f);
    f.data.clear();
    assert(load(&f,&report)&&report.code==a::SpreadFileInvalidLength&&report.close_returned);
    assert(api->end_campaign()&&rules->clear());
}
class FocusEngine final:public console::EngineCvar {
public:
    console::Variable focus,focusAlias,cone,coneAlias,shotBias,patternFocus,biasMin,biasMax;
    const a::Api* api=nullptr;
    const a::AttackSpreadApi* rules=nullptr;
    mutable std::vector<std::string> lookups;
    bool missingFocus=false,missingCone=false,missingBias=false,missingPattern=false;
    FocusEngine(){
        focus.CreateVariable("ai_spread_cone_focus_time","0.6",0,"",false,0,false,0,nullptr);
        focusAlias.CreateVariable("ai_spread_cone_focus_time","0",0,"",false,0,false,0,nullptr);
        cone.CreateVariable("ai_spread_defocused_cone_multiplier","3.0",0,"",false,0,false,0,nullptr);
        coneAlias.CreateVariable("ai_spread_defocused_cone_multiplier","1.0",0,"",false,0,false,0,nullptr);
        focusAlias.parent=&focus;coneAlias.parent=&cone;
        shotBias.CreateVariable("ai_shot_bias","1.0",0,"",false,0,false,0,nullptr);
        patternFocus.CreateVariable("ai_spread_pattern_focus_time","0.8",0,"",false,0,false,0,nullptr);
        biasMin.CreateVariable("ai_shot_bias_min","-1.0",0,"",false,0,false,0,nullptr);
        biasMax.CreateVariable("ai_shot_bias_max","1.0",0,"",false,0,false,0,nullptr);
    }
    void __thiscall RegisterConCommandBase(console::CommandBase*) noexcept override{assert(false);}
    const char* __thiscall GetCommandLineValue(const char*) noexcept override{assert(false);return nullptr;}
    console::Variable* __thiscall FindMutableVar(const char*) noexcept override{assert(false);return nullptr;}
    const console::Variable* __thiscall FindConstVar(const char* name) const noexcept override{
        assert(!api->set_skill(1)&&!rules->clear());
        lookups.emplace_back(name);
        if(std::strcmp(name,"ai_spread_cone_focus_time")==0)return missingFocus?nullptr:&focusAlias;
        if(std::strcmp(name,"ai_shot_bias")==0)return missingBias?nullptr:&shotBias;
        if(std::strcmp(name,"ai_spread_pattern_focus_time")==0)return missingPattern?nullptr:&patternFocus;
        if(std::strcmp(name,"ai_shot_bias_min")==0)return &biasMin;
        if(std::strcmp(name,"ai_shot_bias_max")==0)return &biasMax;
        assert(std::strcmp(name,"ai_spread_defocused_cone_multiplier")==0);
        return missingCone?nullptr:&coneAlias;
    }
    console::CommandBase* __thiscall GetCommands() noexcept override{assert(false);return nullptr;}
};
static void test_native_focus(HMODULE lib,const a::Api* api,const a::AttackSpreadApi* rules){
    const auto get=reinterpret_cast<a::QueryNpcAttackSpreadFromEngine>(GetProcAddress(lib,a::npc_attack_spread_engine_export_name));
    assert(get&&api->begin_campaign(3)&&rules->clear());
    FocusEngine cvar;cvar.api=api;cvar.rules=rules;
    a::NpcFocusMemory memory{0,0.f,0.f};a::NpcAttackSpreadResult out{};
    assert(get(nullptr,"npc_custom",nullptr,nullptr,&memory,&out)&&out.defocused==0&&cvar.lookups.empty());
    memory={1,0.f,0.f};
    assert(!get(reinterpret_cast<void*>(1),"npc_custom",nullptr,nullptr,&memory,&out));
    assert(get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&out.defocused&&out.focus_multiplier==3.f);
    assert(cvar.lookups.size()==2&&cvar.lookups[0]=="ai_spread_cone_focus_time"&&
        cvar.lookups[1]=="ai_spread_defocused_cone_multiplier");
    assert(std::fabs(out.result.x-0.13053f*0.5f*3.f)<0.000001f);
    memory.curtime=0.6f;cvar.lookups.clear();cvar.missingCone=true;
    assert(get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&!out.defocused&&cvar.lookups.size()==1);
    memory.curtime=0.f;
    assert(!get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&!out.defocused);
    cvar.missingCone=false;cvar.focus.SetFloat(0.f);cvar.lookups.clear();
    assert(get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&!out.defocused&&cvar.lookups.size()==1);
    cvar.focus.SetFloat(0.6f);cvar.focusAlias.parent=&cvar.cone;
    assert(!get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)); // wrong root name
    cvar.focusAlias.parent=&cvar.focus;cvar.focus.parent=&cvar.focusAlias;
    assert(!get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)); // cyclic parent
    cvar.focus.parent=&cvar.focus;cvar.missingFocus=true;
    assert(!get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out));
    assert(api->end_campaign());
}
static void test_native_aim(HMODULE lib,const a::Api* api,const a::AttackSpreadApi* rules){
    const auto get=reinterpret_cast<a::QueryNpcAimFromEngine>(GetProcAddress(lib,a::npc_aim_engine_export_name));
    assert(get&&api->begin_campaign(3)&&rules->clear());
    FocusEngine cvar;cvar.api=api;cvar.rules=rules;
    a::NpcAimMemory memory{{0,0.f,0.f},0.f};a::NpcAimResult out{};
    assert(!get(nullptr,"npc_custom",nullptr,nullptr,&memory,&out));
    assert(get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&out.bias.bias==1.f&&!out.spread.defocused);
    assert(cvar.lookups==std::vector<std::string>{"ai_shot_bias"});
    memory.focus.found=1;cvar.lookups.clear();
    assert(get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out));
    assert(out.bias.path==a::NpcBiasNewEnemy&&out.bias.bias==0.f&&out.spread.defocused);
    assert((cvar.lookups==std::vector<std::string>{"ai_shot_bias","ai_spread_pattern_focus_time",
        "ai_spread_cone_focus_time","ai_spread_defocused_cone_multiplier"}));
    memory={{1,2.f,0.f},1.6f};cvar.lookups.clear();cvar.shotBias.SetFloat(0.5f);
    assert(get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out));
    assert(out.bias.path==a::NpcBiasReacquired&&std::fabs(out.bias.bias-0.25f)<0.000001f&&!out.spread.defocused);
    assert(cvar.lookups.size()==3);
    cvar.missingPattern=true;cvar.lookups.clear();
    assert(!get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&out.bias.path==a::NpcBiasReacquired);
    assert(cvar.lookups.size()==2);
    cvar.missingPattern=false;cvar.missingBias=true;cvar.lookups.clear();
    assert(!get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&cvar.lookups.size()==1);
    cvar.missingBias=false;cvar.missingFocus=true;cvar.lookups.clear();
    assert(!get(&cvar,"npc_custom",nullptr,nullptr,&memory,&out)&&cvar.lookups.size()==3);
    assert(api->end_campaign());
}
struct NativeShotRandom {
    virtual void seed(int){assert(false);}
    virtual float random(float lo,float hi){
        assert(lo==-1.f&&hi==1.f&&index<values.size());
        assert(!api->set_skill(1)&&!api->end_campaign());
        return values[index++];
    }
    const a::Api* api=nullptr;
    std::vector<float> values;
    std::size_t index=0;
};
static void test_native_shot(HMODULE lib,const a::Api* api,const a::AttackSpreadApi* rules){
    const auto get=reinterpret_cast<a::QueryNpcShotFromEngine>(GetProcAddress(lib,a::npc_shot_engine_export_name));
    assert(get&&api->begin_campaign(3)&&rules->clear());
    FocusEngine cvar;cvar.api=api;cvar.rules=rules;
    NativeShotRandom rng;rng.api=api;rng.values={1.f,1.f,1.f,1.f,0.5f,0.5f,0.f,0.f};
    const a::NpcShotBasis basis{{1.f,0.f,0.f},{0.f,1.f,0.f},{0.f,0.f,1.f}};
    a::NpcAimMemory memory{{0,0.f,0.f},0.f};
    a::NpcShotFromEngineResult out{};
    assert(get(&cvar,&rng,"npc_custom","rifle","player",&memory,&basis,&out));
    assert(out.shot.code==a::NpcShotAccepted&&out.shot.attempts==2&&rng.index==8);
    assert(out.aim.bias.bias==1.f&&out.shot.effective_bias==1.f&&out.shot.flatness==0.5f);
    assert(out.shot.direction.x==1.f&&std::fabs(out.shot.direction.y-0.0326325f)<0.000001f);
    assert(out.shot.direction.z==0.f);
    assert((cvar.lookups==std::vector<std::string>{"ai_shot_bias","ai_shot_bias_min","ai_shot_bias_max"}));
    cvar.lookups.clear();cvar.biasMax.parent=&cvar.biasMin;
    assert(get(&cvar,&rng,"npc_custom","rifle","player",&memory,&basis,&out)&&out.shot.code==a::NpcShotRejected&&rng.index==8);
    assert(api->end_campaign());
}
static void test_native_npc_rng(HMODULE lib,const a::Api* api){
    const auto decide=reinterpret_cast<a::DecideNpcWithEngineRandom>(GetProcAddress(lib,a::npc_engine_rng_export_name));
    assert(decide&&api->begin_campaign(1));
    NativeNpcRandomFixture random;random.api=api;
    a::NpcDecisionOutput out{};
    assert(decide(nullptr,a::NpcSecondaryFire,1.f,&out)&&!out.drew_random);
    assert(!decide(reinterpret_cast<void*>(1),a::NpcWearsHelmet,1.f,&out));
    random.sample=0.1f;
    assert(decide(&random,a::NpcWearsHelmet,1.f,&out)&&out.decision==1&&random.calls==1);
    assert(api->set_skill(3));
    random.sample=0.5f;
    assert(decide(&random,a::NpcSecondaryFire,1.f,&out)&&out.decision==1&&random.calls==2);
    random.throws=true;
    assert(!decide(&random,a::NpcSecondaryFire,1.f,&out)&&out.decision==1&&random.calls==3);
    assert(api->end_campaign());
}
#endif
static void test_attack_spread(const a::AttackSpreadApi* rules,const a::Api* api,const char* owner_file){
    assert(rules&&rules->version==1&&rules->size==sizeof(a::AttackSpreadApi));
    assert(rules->clear());
    a::AttackSpreadResult out{};
    assert(!rules->query("test_npc","rifle","player",&out));
    const char fixture[]=R"KV("AttackSpreadRules" {
        "class_names" {"test_npc" "specific" "*" "default"}
        "specific" {
            "weapon_class_names" {"rifle" "r" "*" "w"}
            "r" {"target_class_names" {"player" "p" "*" "other"} "p" "0.2" "other" "0.04"}
            "w" {"target_class_names" {"*" "other"} "other" "0.3"}
        }
        "default" {"weapon_class_names" {"*" "w"}
            "w" {"target_class_names" {"*" "other"} "other" "0.1"}}
    })KV";
    assert(rules->load_text(fixture,sizeof(fixture)-1));
    assert(api->begin_campaign(1));
    auto check=[&](const char* npc,const char* weapon,const char* target,float base,float factor,bool fallback){
        assert(rules->query(npc,weapon,target,&out));
        assert(std::fabs(out.base-base)<0.000001f&&std::fabs(out.x-base*factor)<0.000001f);
        assert(out.x==out.y&&out.y==out.z&&out.used_fallback==static_cast<unsigned>(fallback));
    };
    check("TEST_NPC","RIFLE","player",0.2f,1.f,false);
    check("test_npc","rifle","other",0.04f,1.f,false);
    check("test_npc","pistol","player",0.3f,1.f,false);
    check("unknown","rifle","player",0.1f,1.f,false);
    check(nullptr,nullptr,nullptr,0.1f,1.f,false);
    assert(api->set_skill(2));check("test_npc","rifle","player",0.2f,0.75f,false);
    assert(api->set_skill(3));check("test_npc","rifle","player",0.2f,0.5f,false);
    const char broken[]="AttackSpreadRules { class_names { a missing } }";
    assert(!rules->load_text(broken,sizeof(broken)-1));
    assert(!rules->load_text(fixture,sizeof(fixture)));
    assert(!rules->load_text(nullptr,3));
    check("test_npc","rifle","player",0.2f,0.5f,false);
    assert(rules->clear());check("test_npc","rifle","player",0.13053f,0.5f,true);
    if(owner_file){
        std::ifstream in(owner_file,std::ios::binary);
        assert(in);
        std::string data(256*1024+1,'\0');
        in.read(&data[0],static_cast<std::streamsize>(data.size()));
        assert(!in.bad()&&in.gcount()>0&&in.gcount()<=256*1024);
        assert(rules->load_text(data.data(),static_cast<std::uint32_t>(in.gcount())));
        check("npc_druglab_grunt","weapon_assault_rifle","player",0.13053f,0.5f,false);
        check("npc_druglab_grunt_assault_rifle","weapon_assault_rifle","player",0.08716f,0.5f,false);
        check("npc_druglab_grunt_elite","weapon_assault_rifle","player",0.11320f,0.5f,false);
        check("npc_druglab_grunt","weapon_assault_rifle","npc_jessica",0.04362f,0.5f,false);
        check("player","weapon_magnum","npc_druglab_grunt",0.f,0.5f,false);
        check("unknown","weapon_magnum","player",0.13053f,0.5f,true);
        std::puts("PASS owner combat script spread lookup");
    }
    assert(api->end_campaign()&&rules->clear());
}
static void test_npc_attack_spread(const a::NpcAttackSpreadApi* focus,const a::AttackSpreadApi* rules,const a::Api* api){
    assert(focus&&focus->version==1&&focus->size==sizeof(*focus));
    a::NpcFocusMemory memory{0,0.f,0.f};a::NpcFocusTuning tuning{0.6f,3.f};
    a::NpcAttackSpreadResult out{};
    assert(!focus->query("npc_custom",nullptr,nullptr,&memory,nullptr,&out));
    assert(api->begin_campaign(3)&&rules->clear());
    const char text[]=R"KV(AttackSpreadRules {
        class_names { npc_custom rule }
        rule { weapon_class_names { "*" generic } generic { target_class_names { "*" target } target 0.4 } }
    })KV";
    assert(rules->load_text(text,sizeof(text)-1));
    const auto query=[&](){return focus->query("npc_custom","rifle","player",&memory,&tuning,&out);};
    const auto approximately_equal=[](float x,float y){return std::fabs(x-y)<0.000001f;};
    memory.curtime=std::numeric_limits<float>::quiet_NaN();
    assert(focus->query("npc_custom","rifle","player",&memory,nullptr,&out)&&out.result.x==0.2f&&!out.defocused);
    memory={1,0.f,0.f};
    assert(query()&&out.defocused&&out.elapsed==0.f&&out.focus_multiplier==3.f&&approximately_equal(out.result.x,0.6f));
    assert(out.rules.base==0.4f&&out.rules.x==0.2f&&out.result.x==out.result.y&&out.result.y==out.result.z);
    memory.curtime=-0.25f;assert(query()&&out.defocused&&out.elapsed==0.f&&approximately_equal(out.result.x,0.6f));
    memory.curtime=0.3f;assert(query()&&out.defocused&&approximately_equal(out.focus_multiplier,2.f)&&approximately_equal(out.result.x,0.4f));
    memory.curtime=0.6f;assert(query()&&!out.defocused&&out.result.x==0.2f);
    memory.curtime=0.7f;tuning.defocused_multiplier=std::numeric_limits<float>::quiet_NaN();
    assert(query()&&!out.defocused); // skipped multiplier
    memory.curtime=0.f;assert(!query()&&!out.defocused&&out.result.x==0.2f);
    tuning.focus_time=0.f;memory.curtime=std::numeric_limits<float>::quiet_NaN();
    assert(query()&&!out.defocused); // skipped timing and multiplier
    memory.curtime=0.f;tuning={0.6f,0.5f};
    assert(query()&&!out.defocused&&out.result.x==0.2f);
    tuning.defocused_multiplier=1.f;assert(query()&&!out.defocused);
    tuning={0.6f,3.f};assert(api->set_skill(1));
    assert(query()&&out.defocused&&approximately_equal(out.result.x,1.2f)); // not clamped to 1
    memory.found=2;assert(!query()&&out.defocused);
    assert(api->end_campaign()&&rules->clear());
}
static void test_npc_aim(const a::NpcAimApi* aim,const a::Api* api,const a::AttackSpreadApi* rules){
    assert(aim&&aim->version==1&&aim->size==sizeof(*aim));
    a::NpcAimMemory memory{{0,0.f,0.f},0.f};
    a::NpcAimTuning tuning{{0.6f,3.f},1.f,0.8f};
    a::NpcAimResult out{};
    const auto query=[&](){return aim->query("npc_custom","rifle","player",&memory,&tuning,&out);};
    assert(!query()&&api->begin_campaign(3)&&rules->clear());
    assert(query()&&out.bias.bias==1.f&&out.bias.path==a::NpcBiasUnfocused&&!out.spread.defocused);
    tuning.shot_bias=0.5f;assert(query()&&out.bias.bias==0.5f);
    memory.focus.found=1;
    assert(query()&&out.bias.path==a::NpcBiasNewEnemy&&out.bias.bias==0.f&&out.spread.defocused);
    memory.focus.curtime=0.4f;
    assert(query()&&out.bias.path==a::NpcBiasNewEnemy&&out.bias.scale==0.5f&&out.bias.bias==0.25f);
    memory.focus.curtime=0.8f;memory.time_last_reacquired=0.f;
    assert(query()&&out.bias.path==a::NpcBiasUnfocused&&out.bias.bias==0.5f&&!out.spread.defocused);
    memory={{1,2.f,0.f},1.6f};
    assert(query()&&out.bias.path==a::NpcBiasReacquired&&std::fabs(out.bias.bias-0.25f)<0.000001f);
    memory.time_last_reacquired=2.4f;
    assert(query()&&out.bias.path==a::NpcBiasReacquired&&out.bias.bias<0.f); // original does not clamp reacquire
    memory={{1,-0.1f,0.f},1.f};
    assert(query()&&out.bias.path==a::NpcBiasNewEnemy&&out.bias.bias==0.f&&out.spread.defocused);
    tuning.pattern_focus_time=0.f;
    assert(query()&&out.bias.path==a::NpcBiasUnfocused&&out.bias.bias==0.5f&&out.spread.defocused);
    tuning.pattern_focus_time=std::numeric_limits<float>::quiet_NaN();
    assert(!query()&&out.bias.path==a::NpcBiasUnfocused);
    memory.focus.found=0;
    assert(query()&&out.bias.bias==0.5f&&!out.spread.defocused);
    tuning.shot_bias=std::numeric_limits<float>::infinity();
    assert(!query()&&out.bias.bias==0.5f);
    assert(api->end_campaign());
}
struct ShotSampleFixture {
    const a::Api* api;
    const a::NpcShotApi* shots;
    std::vector<float> values;
    unsigned calls=0;
    int mode=0;
    static int SIN_FIXED_CALL draw(void* context,float lo,float hi,float* out){
        auto& f=*static_cast<ShotSampleFixture*>(context);
        assert(lo==-1.f&&hi==1.f&&!f.api->set_skill(1)&&!f.api->end_campaign());
        ++f.calls;
        if(f.mode==1)return 0;
        if(f.mode==2)throw 1;
        if(f.mode==3){*out=1.f;return 1;}
        if(f.mode==4){*out=2.f;return 1;}
        assert(f.calls<=f.values.size());*out=f.values[f.calls-1];return 1;
    }
};
static void test_npc_shot(const a::NpcShotApi* shots,const a::Api* api){
    assert(shots&&shots->version==1&&shots->size==sizeof(*shots));
    a::NpcShotBasis basis{{1.f,0.f,0.f},{0.f,1.f,0.f},{0.f,0.f,1.f}};
    a::NpcAimResult aim{};aim.bias.bias=1.f;aim.spread.result={0.25f,0.5f,9.f};
    a::NpcShotTuning tuning{-1.f,1.f};
    a::NpcShotReport report{};
    ShotSampleFixture f{api,shots,{1.f,1.f,1.f,1.f,0.5f,0.5f,-0.5f,-0.5f}};
    auto sample=[&](){return shots->sample(&basis,&aim,&tuning,&ShotSampleFixture::draw,&f,&report);};
    assert(sample()&&report.code==a::NpcShotRejected&&f.calls==0);
    assert(api->begin_campaign(3));
    assert(sample()&&report.code==a::NpcShotAccepted&&report.attempts==2&&report.random_calls==8);
    assert(report.effective_bias==1.f&&report.flatness==0.5f&&report.x==0.5f&&report.y==-0.5f);
    assert(report.direction.x==1.f&&report.direction.y==0.125f&&report.direction.z==-0.25f);
    // Bias 0.5 maps to a flat distribution: first weighted draw is
    // still consumed for both axes, including the zero-weight term.
    f.calls=0;f.values={-1.f,0.25f,1.f,0.5f};aim.bias.bias=0.5f;
    assert(sample()&&report.code==a::NpcShotAccepted&&report.random_calls==4);
    assert(report.effective_bias==0.f&&report.flatness==0.f&&report.x==0.25f&&report.y==0.5f);
    // Negative effective bias inverts about the signed unit endpoint.
    f.calls=0;f.values={0.5f,0.5f,-0.5f,-0.5f};aim.bias.bias=-0.4f;
    assert(sample()&&report.code==a::NpcShotAccepted&&report.effective_bias==-1.f);
    assert(report.x==0.5f&&report.y==-0.5f);
    f.calls=0;f.values={1.f,1.f,0.f,0.f};aim.bias.bias=3.f;
    assert(sample()&&report.code==a::NpcShotAccepted&&report.x==1.f&&report.y==0.f); // inclusive circle
    f.calls=0;f.mode=1;
    assert(sample()&&report.code==a::NpcShotRandomRefused&&report.random_calls==1);
    f.calls=0;f.mode=2;
    assert(sample()&&report.code==a::NpcShotRandomRefused&&report.random_calls==1);
    f.calls=0;f.mode=4;
    assert(sample()&&report.code==a::NpcShotInvalidSample&&report.random_calls==1);
    f.calls=0;f.mode=3;
    assert(sample()&&report.code==a::NpcShotAttemptLimit&&report.attempts==a::npc_shot_attempt_limit&&
        report.random_calls==4*a::npc_shot_attempt_limit);
    f.calls=0;basis.forward.x=std::numeric_limits<float>::quiet_NaN();
    assert(sample()&&report.code==a::NpcShotRejected&&!report.random_calls);
    assert(api->end_campaign());
}
int main(int argc,char** argv){
    assert(argc==2||argc==3);
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
#ifdef _WIN32
    auto getMap=reinterpret_cast<a::GetMapEntityApi>(GetProcAddress(lib,a::map_entity_export_name));
#else
    auto getMap=reinterpret_cast<a::GetMapEntityApi>(dlsym(lib,a::map_entity_export_name));
#endif
    assert(getMap&&!getMap(0,sizeof(a::MapEntityApi))&&!getMap(1,0));
    const auto* map=getMap(a::map_entity_version,sizeof(a::MapEntityApi));
    test_map_entities(map,api,globals);
    test_native_bsp_host(map,api,globals);
#ifdef _WIN32
    auto getTriggers=reinterpret_cast<a::GetCampaignTriggerApi>(GetProcAddress(lib,a::trigger_export_name));
#else
    auto getTriggers=reinterpret_cast<a::GetCampaignTriggerApi>(dlsym(lib,a::trigger_export_name));
#endif
    assert(getTriggers&&!getTriggers(0,sizeof(a::CampaignTriggerApi))&&!getTriggers(1,0));
    const auto* triggers=getTriggers(a::trigger_version,sizeof(a::CampaignTriggerApi));
    test_native_triggers(map,api,triggers);
    test_trigger_bsp_host(map,api,triggers);
#ifdef _WIN32
    auto getWeapon=reinterpret_cast<a::GetWeaponBehaviorApi>(GetProcAddress(lib,a::weapon_export_name));
#else
    auto getWeapon=reinterpret_cast<a::GetWeaponBehaviorApi>(dlsym(lib,a::weapon_export_name));
#endif
    assert(getWeapon&&!getWeapon(0,sizeof(a::WeaponBehaviorApi))&&!getWeapon(1,0));
    const auto* weapons=getWeapon(a::weapon_version,sizeof(a::WeaponBehaviorApi));
    test_native_weapon_behavior(weapons,map,api);
#ifdef _WIN32
    auto getPump=reinterpret_cast<a::GetScattergunPumpApi>(GetProcAddress(lib,a::scattergun_export_name));
#else
    auto getPump=reinterpret_cast<a::GetScattergunPumpApi>(dlsym(lib,a::scattergun_export_name));
#endif
    assert(getPump&&!getPump(0,sizeof(a::ScattergunPumpApi))&&!getPump(1,0));
    const auto* pump=getPump(a::scattergun_version,sizeof(a::ScattergunPumpApi));
    test_scattergun_pump(pump,weapons,map,api);
#ifdef _WIN32
    auto getFire=reinterpret_cast<a::GetScattergunSecondaryApi>(GetProcAddress(lib,a::scattergun_fire_export_name));
#else
    auto getFire=reinterpret_cast<a::GetScattergunSecondaryApi>(dlsym(lib,a::scattergun_fire_export_name));
#endif
    assert(getFire&&!getFire(0,sizeof(a::ScattergunSecondaryApi))&&!getFire(1,0));
    test_scattergun_secondary(getFire(a::scattergun_fire_version,sizeof(a::ScattergunSecondaryApi)),pump,map,api);
#ifdef _WIN32
    auto getGrenade=reinterpret_cast<a::GetARGrenadeApi>(GetProcAddress(lib,a::grenade_export_name));
#else
    auto getGrenade=reinterpret_cast<a::GetARGrenadeApi>(dlsym(lib,a::grenade_export_name));
#endif
    assert(getGrenade&&!getGrenade(0,sizeof(a::ARGrenadeApi))&&!getGrenade(1,0));
    const auto* grenade=getGrenade(a::grenade_version,sizeof(a::ARGrenadeApi));
    test_ar_grenade(grenade,map,api);
#ifdef _WIN32
    auto getRifle=reinterpret_cast<a::GetRifleGrenadeApi>(GetProcAddress(lib,a::rifle_launch_export_name));
#else
    auto getRifle=reinterpret_cast<a::GetRifleGrenadeApi>(dlsym(lib,a::rifle_launch_export_name));
#endif
    assert(getRifle&&!getRifle(0,sizeof(a::RifleGrenadeApi))&&!getRifle(1,0));
    test_rifle_grenade(getRifle(a::rifle_launch_version,sizeof(a::RifleGrenadeApi)),grenade,map,api);
#ifdef _WIN32
    auto getMagnum=reinterpret_cast<a::GetMagnumApi>(GetProcAddress(lib,a::magnum_export_name));
#else
    auto getMagnum=reinterpret_cast<a::GetMagnumApi>(dlsym(lib,a::magnum_export_name));
#endif
    assert(getMagnum&&!getMagnum(0,sizeof(a::MagnumApi))&&!getMagnum(1,0));
    const auto* magnum=getMagnum(a::magnum_version,sizeof(a::MagnumApi));
    test_magnum(magnum,map,api);
#ifdef _WIN32
    auto getMagSecondary=reinterpret_cast<a::GetMagnumSecondaryApi>(GetProcAddress(lib,a::magnum_secondary_export_name));
#else
    auto getMagSecondary=reinterpret_cast<a::GetMagnumSecondaryApi>(dlsym(lib,a::magnum_secondary_export_name));
#endif
    assert(getMagSecondary&&!getMagSecondary(0,sizeof(a::MagnumSecondaryApi))&&!getMagSecondary(1,0));
#ifdef _WIN32
    auto getAmmo=reinterpret_cast<a::GetCombatAmmoApi>(GetProcAddress(lib,a::ammo_export_name));
#else
    auto getAmmo=reinterpret_cast<a::GetCombatAmmoApi>(dlsym(lib,a::ammo_export_name));
#endif
    assert(getAmmo&&!getAmmo(0,sizeof(a::CombatAmmoApi))&&!getAmmo(1,0));
    const auto* ammo=getAmmo(a::ammo_version,sizeof(a::CombatAmmoApi));
    test_shared_ammo(ammo,map,api);
    namespace defs=sin_port::original_ammo;
#ifdef _WIN32
    auto getDefs=reinterpret_cast<defs::GetApi>(GetProcAddress(lib,defs::export_name));
#else
    auto getDefs=reinterpret_cast<defs::GetApi>(dlsym(lib,defs::export_name));
#endif
    assert(getDefs&&!getDefs(0,sizeof(defs::Api))&&!getDefs(1,0));
    const auto* definitions=getDefs(defs::version,sizeof(defs::Api));
    assert(definitions&&definitions->definition_count()==17&&definitions->index("arnaDe")==6);
    defs::Definition duag{};
    assert(definitions->definition(9,&duag)&&duag.damage_type==0x00400002u&&duag.max_carry==8);
    assert(definitions->index("unknown")==-1&&definitions->max_carry(-1)==0);
    test_magnum_secondary(getMagSecondary(a::magnum_secondary_version,sizeof(a::MagnumSecondaryApi)),magnum,map,api,ammo);
#ifdef _WIN32
    auto getCombat=reinterpret_cast<a::GetCombatDamageApi>(GetProcAddress(lib,a::combat_export_name));
#else
    auto getCombat=reinterpret_cast<a::GetCombatDamageApi>(dlsym(lib,a::combat_export_name));
#endif
    assert(getCombat&&!getCombat(0,sizeof(a::CombatDamageApi))&&!getCombat(1,0));
    test_combat_damage(getCombat(a::combat_version,sizeof(a::CombatDamageApi)),api,globals,map);
#ifdef _WIN32
    auto getNpc=reinterpret_cast<a::GetNpcRulesApi>(GetProcAddress(lib,a::npc_rules_export_name));
#else
    auto getNpc=reinterpret_cast<a::GetNpcRulesApi>(dlsym(lib,a::npc_rules_export_name));
#endif
    assert(getNpc&&!getNpc(0,sizeof(a::NpcRulesApi))&&!getNpc(1,0));
    test_npc_rules(getNpc(a::npc_rules_version,sizeof(a::NpcRulesApi)),api);
#ifdef _WIN32
    auto getSpread=reinterpret_cast<a::GetAttackSpreadApi>(GetProcAddress(lib,a::attack_spread_export_name));
#else
    auto getSpread=reinterpret_cast<a::GetAttackSpreadApi>(dlsym(lib,a::attack_spread_export_name));
#endif
    assert(getSpread&&!getSpread(0,sizeof(a::AttackSpreadApi))&&!getSpread(1,0));
    const auto* spread=getSpread(a::attack_spread_version,sizeof(a::AttackSpreadApi));
    test_attack_spread(spread,api,argc==3?argv[2]:nullptr);
#ifdef _WIN32
    auto getFocus=reinterpret_cast<a::GetNpcAttackSpreadApi>(GetProcAddress(lib,a::npc_attack_spread_export_name));
#else
    auto getFocus=reinterpret_cast<a::GetNpcAttackSpreadApi>(dlsym(lib,a::npc_attack_spread_export_name));
#endif
    assert(getFocus&&!getFocus(0,sizeof(a::NpcAttackSpreadApi))&&!getFocus(1,0));
    test_npc_attack_spread(getFocus(a::npc_attack_spread_version,sizeof(a::NpcAttackSpreadApi)),spread,api);
#ifdef _WIN32
    auto getAim=reinterpret_cast<a::GetNpcAimApi>(GetProcAddress(lib,a::npc_aim_export_name));
#else
    auto getAim=reinterpret_cast<a::GetNpcAimApi>(dlsym(lib,a::npc_aim_export_name));
#endif
    assert(getAim&&!getAim(0,sizeof(a::NpcAimApi))&&!getAim(1,0));
    test_npc_aim(getAim(a::npc_aim_version,sizeof(a::NpcAimApi)),api,spread);
#ifdef _WIN32
    auto getShot=reinterpret_cast<a::GetNpcShotApi>(GetProcAddress(lib,a::npc_shot_export_name));
#else
    auto getShot=reinterpret_cast<a::GetNpcShotApi>(dlsym(lib,a::npc_shot_export_name));
#endif
    assert(getShot&&!getShot(0,sizeof(a::NpcShotApi))&&!getShot(1,0));
    test_npc_shot(getShot(a::npc_shot_version,sizeof(a::NpcShotApi)),api);
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
    test_native_combat_filesystem(lib,api,getSpread(a::attack_spread_version,sizeof(a::AttackSpreadApi)));
    test_native_focus(lib,api,spread);
    test_native_aim(lib,api,spread);
    test_native_shot(lib,api,spread);
    test_native_npc_rng(lib,api);
    test_native_engine_event(lib,api,map,triggers);
    test_engine_skill(lib,api);
    test_configured_campaign(lib,api,globals);
#endif
    assert(FreeLibrary(lib));
#else
    assert(dlclose(lib)==0);
#endif
    std::puts("PASS actual loaded module fixed difficulty campaign service");
}
