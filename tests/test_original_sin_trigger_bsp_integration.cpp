// Phase 527: end-to-end ORIGINAL-FREE Source BSP entity-lump -> actual SiN
// gameplay trigger factory -> map KeyValues -> native-host Spawn -> game
// descriptor11 -> player Touch -> descriptor12 -> deferred removal.
//
// The host and BSP are fabricated. This exercises actual project C++
// parser and gameplay logic, NOT original 2006 game engine initialization,
// Source CBaseTrigger allocations, real map loading or Quest hardware.
#include "sin_compat/original_sin_trigger_entities.hpp"
#include "sin_compat/source_map_runtime.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace sin_compat;
using namespace sin_compat::map;
using namespace sin_compat::map_entities;

namespace {
constexpr std::size_t kBspHeader=1036U;
void store_u32(std::vector<std::uint8_t>& data,std::size_t off,
               std::uint32_t value) {
    for(unsigned i=0;i<4;++i)
        data[off+i]=static_cast<std::uint8_t>((value>>(8*i))&0xFFu);
}
std::vector<std::uint8_t> fake_bsp(std::string_view entities) {
    std::vector<std::uint8_t> data(kBspHeader+entities.size()+1,0u);
    data[0]='V';data[1]='B';data[2]='S';data[3]='P';
    store_u32(data,4,20);
    store_u32(data,8,static_cast<std::uint32_t>(kBspHeader));
    store_u32(data,12,static_cast<std::uint32_t>(entities.size()+1));
    std::memcpy(data.data()+kBspHeader,entities.data(),entities.size());
    return data;
}
struct EventSnapshot {
    TriggerEventName event=TriggerEventName::Game;
    int descriptor=-1,owner=0,decision=0;
    float value=0;
    std::string name;
};
struct FixtureHost final:
    ISourceMapEntityHost,IOriginalTriggerHost,IGameEntityRegistrar {
    struct Callback {SpawnCallback call=nullptr;void* context=nullptr;};
    struct Entity {EntityRef ref{};std::string classname;};
    std::map<std::string,Callback> registry{};
    std::unordered_map<std::uint32_t,Entity> live{};
    std::vector<EntityRef> removal_queue{};
    std::vector<EventSnapshot> events{};
    std::vector<std::string> warnings{};
    int manager_calls=0;
    std::uint32_t simulated_native_spawnflags_0xB4=0;
    std::vector<std::string> steps{};
    EntityRef player{997u,1u};
    OriginalTriggerEntityFactory* game=nullptr;
    std::uint32_t next_index=1;
    bool base_ok=true;
    // This is a SYNTHETIC engine-owned field snapshot, not a map KV.
    std::uint32_t simulated_native_flags_0x120=0x08u;
    int base_calls=0;
    int activated=0;
    bool is_live(EntityRef e) const override {
        if(e==player)return true;
        auto it=live.find(e.index);
        return it!=live.end() && it->second.ref==e;
    }
    bool is_player(EntityRef e) const override {return e==player;}
    int current_difficulty_owner_index() const override {return -1;}
    bool initialize_native_base_trigger(EntityRef e) override {
        if(!is_live(e))return false;
        ++base_calls;steps.emplace_back("base-spawn");
        return base_ok;
    }
    bool perform_special_trigger_base_action(EntityRef) override {
        return false; // never guess the unimplemented original base helper
    }
    void report_original_warning(std::string_view text) override {
        warnings.emplace_back(text);
        steps.emplace_back("warn");
    }
    bool publish(const TriggerEvent& e) override {
        steps.emplace_back("event");
        events.push_back({e.event,e.descriptor,e.owner,
                          e.decision,e.value,std::string(e.name)});
        return true;
    }
    bool mark_difficulty_manager() override {
        ++manager_calls;
        steps.emplace_back("mark-difficulty");
        return true;
    }
    bool queue_remove(EntityRef e) override {
        if(!is_live(e))return false;
        steps.emplace_back("queue-remove");
        removal_queue.push_back(e);
        return true;
    }
    bool install(std::string_view n,SpawnCallback cb,void* context) override {
        return registry.emplace(std::string(n),Callback{cb,context}).second;
    }
    bool uninstall(std::string_view n,SpawnCallback cb,void* context) override {
        auto it=registry.find(std::string(n));
        if(it==registry.end() || it->second.call!=cb ||
           it->second.context!=context)return false;
        registry.erase(it);
        return true;
    }
    bool create_entity(std::string_view classname,EntityRef& out) override {
        const auto entry=registry.find(std::string(classname));
        if(entry==registry.end())return false;
        EntityRef ref{next_index++,1u};
        live.emplace(ref.index,Entity{ref,std::string(classname)});
        const auto s=entry->second.call(entry->second.context,
                                         {classname,ref});
        if(!s.ok()){
            live.erase(ref.index);
            return false;
        }
        out=ref;
        steps.emplace_back("create");
        return true;
    }
    bool key_value(EntityRef e,std::string_view key,
                   std::string_view value) override {
        auto it=live.find(e.index);
        if(it==live.end() || it->second.ref!=e)return false;
        if(key=="targetname")steps.emplace_back("targetname");
        return game && game->apply_map_keyvalue(e,key,value)==
            TriggerCode::Handled;
    }
    bool dispatch_spawn(EntityRef e) override {
        auto it=live.find(e.index);
        if(it==live.end() || it->second.ref!=e||!game)return false;
        if(it->second.classname=="trigger_secret") {
            const auto result=game->run_secret_spawn(e,
                {false,simulated_native_flags_0x120});
            return result.handled();
        }
        if(it->second.classname=="trigger_dynamic_difficulty") {
            const auto result=game->run_dynamic_spawn(e,
                {false,0u,simulated_native_spawnflags_0xB4});
            return result.handled();
        }
        return false;
    }
    bool activate(EntityRef e) override {
        if(!is_live(e))return false;
        ++activated;steps.emplace_back("activate");
        return true;
    }
    bool remove_entity(EntityRef e) override {
        auto it=live.find(e.index);
        if(it==live.end()||it->second.ref!=e)return false;
        if(!game || game->release(e)!=TriggerCode::Handled)return false;
        live.erase(it);steps.emplace_back("remove");
        return true;
    }
    void flush_deferred_remove() {
        for(EntityRef e:removal_queue)
            assert(remove_entity(e));
        removal_queue.clear();
    }
};
void test_loaded_bsp_triggers_secret_spawn_and_touch() {
    FixtureHost h{};
    OriginalTriggerEntityFactory adapter(h);
    h.game=&adapter;
    assert(adapter.install(h)==RegistrationCode::Installed);
    SourceMapRuntime runtime(h,16u);
    const auto bsp=fake_bsp(
        "{\"classname\" \"trigger_secret\" "
        "\"targetname\" \"secret_intro_01\" }");
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok() && loaded.entities_created==1 &&
           loaded.key_values_applied==1 &&
           loaded.entities_spawned==1 && loaded.entities_activated==1);
    assert(h.base_calls==1&&h.activated==1&&h.events.size()==1&&
           h.events[0].descriptor==11&&
           h.events[0].name=="secret_intro_01");
    const EntityRef trigger=runtime.entities()[0];
    assert(h.is_live(trigger));
    assert(h.game->touch(trigger,h.player).handled());
    assert(h.events.size()==2&&h.events[1].descriptor==12&&
           h.events[1].name=="secret_intro_01");
    assert(h.removal_queue.size()==1);
    h.flush_deferred_remove();
    assert(!h.is_live(trigger));
    assert(runtime.unload().code==SourceMapLoadCode::Unloaded);
    assert(adapter.instance_count()==0);
    assert(adapter.uninstall()==RegistrationCode::Uninstalled);
}
void test_dynamic_difficulty_map_keyvalues_spawn_and_touch() {
    FixtureHost h{};
    OriginalTriggerEntityFactory adapter(h);
    h.game=&adapter;
    assert(adapter.install(h)==RegistrationCode::Installed);
    SourceMapRuntime runtime(h,8u);
    h.simulated_native_spawnflags_0xB4=0x400u;
    const auto bsp=fake_bsp(
        "{\"classname\" \"trigger_dynamic_difficulty\" "
        "\"targetname\" \"difficulty_intro_01\" "
        "\"EstimatedTime\" \"12.5\" "
        "\"EstimatedDifficulty\" \"7\" "
        "\"EndOfGame\" \"1\" }");
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok() && loaded.entities_created==1 &&
           loaded.key_values_applied==4 &&
           loaded.entities_spawned==1 && loaded.entities_activated==1);
    // Dynamic Spawn runs base CBaseTrigger and the authentic warning
    // bit10, but NEVER emits a dynamic_difficulty event prematurely.
    assert(h.base_calls==1&&h.events.empty()&&h.manager_calls==0);
    assert(h.warnings==std::vector<std::string>{
        "DD Trigger with don't destroy set\n"
    });
    const EntityRef dynamic=runtime.entities()[0];
    assert(adapter.touch(dynamic,h.player).handled());
    assert(h.events.size()==1);
    assert(h.events[0].event==TriggerEventName::DynamicDifficulty &&
           h.events[0].descriptor==0 &&
           h.events[0].name=="difficulty_intro_01" &&
           h.events[0].decision==7 && h.events[0].value==12.5f);
    assert(h.manager_calls==1&&h.removal_queue.size()==1);
    h.flush_deferred_remove();
    assert(runtime.unload().code==SourceMapLoadCode::Unloaded);
    assert(adapter.instance_count()==0);
    assert(adapter.uninstall()==RegistrationCode::Uninstalled);
}
void test_rejected_invalid_map_key_rolls_back_native_entity() {
    FixtureHost h{};
    OriginalTriggerEntityFactory adapter(h);
    h.game=&adapter;
    assert(adapter.install(h)==RegistrationCode::Installed);
    SourceMapRuntime runtime(h,8u);
    auto bsp=fake_bsp(
        "{\"classname\" \"trigger_secret\" "
        "\"targetname\" \"secret_intro_02\" "
        "\"EstimatedDifficulty\" \"5\" }");
    const auto result=runtime.load_bsp(bsp.data(),bsp.size());
    assert(result.code==SourceMapLoadCode::HostKeyValueFailed &&
           result.property_index==2 && result.entities_removed==1);
    assert(h.live.empty()&&adapter.instance_count()==0);
    assert(h.events.empty()&&h.base_calls==0);
    // A rejected map must never poison the following valid load.
    h.simulated_native_flags_0x120=0;
    bsp=fake_bsp(
        "{\"classname\" \"trigger_secret\" "
        "\"targetname\" \"valid_again\" }");
    assert(runtime.load_bsp(bsp.data(),bsp.size()).ok());
    assert(h.base_calls==1&&h.events.empty());
    assert(runtime.unload().code==SourceMapLoadCode::Unloaded);
    assert(adapter.uninstall()==RegistrationCode::Uninstalled);
}
void test_native_base_spawn_refusal_rolls_back_map() {
    FixtureHost h{};
    OriginalTriggerEntityFactory adapter(h);
    h.game=&adapter;
    assert(adapter.install(h)==RegistrationCode::Installed);
    SourceMapRuntime runtime(h,8u);
    h.base_ok=false;
    const auto bsp=fake_bsp(
        "{\"classname\" \"trigger_secret\" "
        "\"targetname\" \"secret_intro_03\" }");
    const auto failure=runtime.load_bsp(bsp.data(),bsp.size());
    assert(failure.code==SourceMapLoadCode::HostSpawnFailed &&
           failure.entities_removed==1);
    assert(h.live.empty()&&adapter.instance_count()==0);
    assert(h.base_calls==1&&h.events.empty());
    assert(adapter.uninstall()==RegistrationCode::Uninstalled);
}
} // namespace
int main(){
    test_loaded_bsp_triggers_secret_spawn_and_touch();
    test_dynamic_difficulty_map_keyvalues_spawn_and_touch();
    test_rejected_invalid_map_key_rolls_back_native_entity();
    test_native_base_spawn_refusal_rolls_back_map();
    std::puts("PASS original-free BSP20 -> map runtime -> authentic SiN secret Spawn11/Touch12 and dynamic Spawn warning/Touch event -> deferred remove/rollback");
    return 0;
}
