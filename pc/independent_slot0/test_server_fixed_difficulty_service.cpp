// Load the actual built module. No linking against its implementation.
#include "server_fixed_difficulty_service.h"
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
    auto getCombat=reinterpret_cast<a::GetCombatDamageApi>(GetProcAddress(lib,a::combat_export_name));
#else
    auto getCombat=reinterpret_cast<a::GetCombatDamageApi>(dlsym(lib,a::combat_export_name));
#endif
    assert(getCombat&&!getCombat(0,sizeof(a::CombatDamageApi))&&!getCombat(1,0));
    test_combat_damage(getCombat(a::combat_version,sizeof(a::CombatDamageApi)),api,globals,map);
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
