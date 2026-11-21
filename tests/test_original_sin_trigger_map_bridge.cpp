#include "sin_compat/original_sin_trigger_map_bridge.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace sin_compat;
using namespace sin_compat::map;
using namespace sin_compat::map_entities;

namespace {
struct Event {
    TriggerEventName kind=TriggerEventName::Game;
    int descriptor=-1,decision=0;
    float value=0;
    std::string name{};
    TriggerVec3 origin{};
};
struct FakeNativeEngine final : ISourceMapEntityHost,IOriginalTriggerHost,
                                ITriggerNativeState {
    struct NativeEntity {
        EntityRef identity{};
        std::string classname;
        std::vector<std::pair<std::string,std::string>> keys;
    };
    std::map<std::uint32_t,NativeEntity> allocated;
    std::vector<EntityRef> deletion_queue;
    std::vector<Event> events;
    std::vector<std::string> call_order;
    std::uint32_t next_index=2;
    EntityRef player{600,1},npc{601,1};
    bool refuse_snapshot=false,refuse_base=false,refuse_native_factory=false;
    bool refuse_removal=false,global_suppress=false;
    int native_base_spawns=0,special_calls=0,manager_calls=0;
    int native_normal_spawns=0,activated=0;
    TriggerVec3 scene_origin{11.0f,22.0f,33.0f};
    bool is_live(EntityRef e) const override {
        if(e==player || e==npc)return true;
        auto it=allocated.find(e.index);
        return it!=allocated.end()&&it->second.identity==e;
    }
    bool is_player(EntityRef e) const override{return e==player;}
    int current_difficulty_owner_index() const override{return 17;}
    bool initialize_native_base_trigger(EntityRef e) override {
        if(!is_live(e) || refuse_base)return false;
        ++native_base_spawns;
        call_order.emplace_back("native-base-spawn");
        return true;
    }
    bool perform_special_trigger_base_action(EntityRef e) override {
        if(!is_live(e))return false;
        ++special_calls;call_order.emplace_back("special");
        return true;
    }
    bool mark_difficulty_manager() override {
        ++manager_calls;call_order.emplace_back("manager");
        return true;
    }
    void report_original_warning(std::string_view s) override {
        assert(s=="DD Trigger with don't destroy set\n");
        call_order.emplace_back("warning");
    }
    bool publish(const TriggerEvent& e) override {
        call_order.emplace_back("publish");
        events.push_back({e.event,e.descriptor,e.decision,e.value,
                          std::string(e.name),e.origin});
        return true;
    }
    bool queue_remove(EntityRef e) override {
        if(!is_live(e))return false;
        deletion_queue.push_back(e);
        call_order.emplace_back("queue-remove");
        return true;
    }
    bool read_native_trigger_snapshot(
        EntityRef e,NativeTriggerSnapshot& out) const override {
        if(refuse_snapshot||!is_live(e))return false;
        auto it=allocated.find(e.index);
        if(it==allocated.end())return false;
        out.world_origin=scene_origin;
        out.base_flags_0xB8=
            it->second.classname=="trigger_dynamic_difficulty"?0x800u:0u;
        out.native_flags_0xB4=
            it->second.classname=="trigger_dynamic_difficulty"?0x400u:0u;
        out.native_bits_0x120=
            it->second.classname=="trigger_secret"?0x8u:0u;
        out.engine_requests_removal=global_suppress;
        return true;
    }
    bool create_entity(std::string_view classname,EntityRef& out) override {
        if(refuse_native_factory)return false;
        if(classname!="trigger_secret" &&
           classname!="trigger_dynamic_difficulty" &&
           classname!="logic_relay")return false;
        EntityRef e{next_index++,1};
        allocated.emplace(e.index,NativeEntity{e,std::string(classname),{}});
        out=e;
        call_order.emplace_back("native-create");
        return true;
    }
    bool key_value(EntityRef e,std::string_view k,std::string_view v) override {
        auto it=allocated.find(e.index);
        if(it==allocated.end()||it->second.identity!=e)return false;
        it->second.keys.emplace_back(std::string(k),std::string(v));
        call_order.emplace_back("native-key");
        return true;
    }
    bool dispatch_spawn(EntityRef e) override {
        auto it=allocated.find(e.index);
        if(it==allocated.end()||it->second.identity!=e)return false;
        if(it->second.classname!="logic_relay")return false;
        ++native_normal_spawns;
        call_order.emplace_back("native-normal-spawn");
        return true;
    }
    bool activate(EntityRef e) override {
        if(!is_live(e))return false;
        ++activated;
        call_order.emplace_back("native-activate");
        return true;
    }
    bool remove_entity(EntityRef e) override {
        if(refuse_removal)return false;
        auto it=allocated.find(e.index);
        if(it==allocated.end()||it->second.identity!=e)return false;
        allocated.erase(it);
        call_order.emplace_back("native-remove");
        return true;
    }
    void drain(OriginalTriggerMapBridge& bridge) {
        for(auto e:deletion_queue)assert(bridge.remove_entity(e));
        deletion_queue.clear();
    }
};
SourceMapEntity entity(
    std::initializer_list<std::pair<const char*,const char*>> fields) {
    SourceMapEntity e;
    for(const auto& p:fields)e.properties.push_back({p.first,p.second});
    return e;
}
void test_original_gameplay_triggers_in_native_host() {
    FakeNativeEngine native;
    OriginalTriggerEntityFactory gameplay(native);
    OriginalTriggerMapBridge bridge(native,native,gameplay);
    SourceMapRuntime map(bridge,16u);
    SourceBspEntityDocument doc;
    doc.entities={
        entity({{"classname","logic_relay"},{"targetname","relay_safe"}}),
        entity({{"classname","trigger_secret"},{"targetname","secret_entrance"}}),
        entity({{"classname","trigger_dynamic_difficulty"},
                {"targetname","dynamic_gate"},{"EstimatedTime","14.5"},
                {"EstimatedDifficulty","3"},{"EndOfGame","1"}})
    };
    const auto r=map.load_document(doc);
    assert(r.ok()&&r.entities_created==3&&r.key_values_applied==6);
    assert(r.entities_spawned==3&&r.entities_activated==3);
    assert(gameplay.instance_count()==2);
    // Stock Source entities are owned by the original host, not replaced.
    assert(native.native_normal_spawns==1&&native.native_base_spawns==2);
    assert(native.allocated.size()==3);
    assert(native.events.size()==1&&
           native.events[0].kind==TriggerEventName::Game&&
           native.events[0].descriptor==11&&
           native.events[0].name=="secret_entrance");
    // The dynamic trigger does not publish until a genuine player touch.
    const EntityRef secret=map.entities()[1];
    const EntityRef dynamic=map.entities()[2];
    assert(bridge.player_or_npc_touch(dynamic,native.npc).code==
           TriggerCode::NonPlayer);
    assert(bridge.player_or_npc_touch(secret,native.player).handled());
    assert(bridge.player_or_npc_touch(dynamic,native.player).handled());
    assert(native.events.size()==3);
    assert(native.events[1].descriptor==12&&
           native.events[1].name=="secret_entrance");
    assert(native.events[2].kind==TriggerEventName::DynamicDifficulty);
    assert(native.events[2].decision==3&&native.events[2].value==14.5f);
    assert(native.events[2].origin.x==11.0f&&
           native.events[2].origin.y==22.0f&&
           native.events[2].origin.z==33.0f);
    assert(native.manager_calls==1&&native.special_calls==1);
    assert(native.deletion_queue.size()==2);
    native.drain(bridge);
    assert(gameplay.instance_count()==0);
    assert(!native.is_live(secret)&&!native.is_live(dynamic));
    // SourceMapRuntime now only has its normal logic_relay to release.
    assert(map.unload().code==SourceMapLoadCode::Unloaded);
    assert(native.allocated.empty());
}
void test_native_factory_refusal_is_not_a_fake_class() {
    FakeNativeEngine native;
    OriginalTriggerEntityFactory gameplay(native);
    OriginalTriggerMapBridge bridge(native,native,gameplay);
    SourceMapRuntime map(bridge);
    native.refuse_native_factory=true;
    SourceBspEntityDocument d;
    d.entities={entity({{"classname","trigger_secret"}})};
    const auto r=map.load_document(d);
    assert(r.code==SourceMapLoadCode::HostCreateFailed);
    assert(native.allocated.empty()&&gameplay.instance_count()==0);
    native.refuse_native_factory=false;
    d.entities={entity({{"classname","unknown_game_entity"}})};
    assert(map.load_document(d).code==SourceMapLoadCode::HostCreateFailed);
    assert(native.allocated.empty()&&gameplay.instance_count()==0);
}
void test_rejected_native_field_snapshot_and_failed_base_spawn_rollback() {
    for(bool missing_snapshot:{false,true}) {
        FakeNativeEngine native;
        OriginalTriggerEntityFactory gameplay(native);
        OriginalTriggerMapBridge bridge(native,native,gameplay);
        SourceMapRuntime map(bridge);
        native.refuse_snapshot=missing_snapshot;
        native.refuse_base=!missing_snapshot;
        SourceBspEntityDocument d;
        d.entities={entity({{"classname","trigger_dynamic_difficulty"},
                            {"EstimatedTime","1.0"},{"EndOfGame","0"}})};
        const auto r=map.load_document(d);
        assert(r.code==SourceMapLoadCode::HostSpawnFailed);
        assert(r.entities_removed==1);
        assert(native.allocated.empty()&&gameplay.instance_count()==0);
        assert(native.events.empty());
    }
}
void test_invalid_trigger_key_native_rollback_and_retry() {
    FakeNativeEngine native;
    OriginalTriggerEntityFactory gameplay(native);
    OriginalTriggerMapBridge bridge(native,native,gameplay);
    SourceMapRuntime map(bridge);
    SourceBspEntityDocument d;
    d.entities={
        entity({{"classname","logic_relay"}}),
        entity({{"classname","trigger_dynamic_difficulty"},
                {"EstimatedTime","not-a-number"}})
    };
    const auto fail=map.load_document(d);
    assert(fail.code==SourceMapLoadCode::HostKeyValueFailed&&
           fail.entities_removed==2);
    assert(native.allocated.empty()&&gameplay.instance_count()==0);
    assert(native.native_normal_spawns==0);
    d.entities={entity({{"classname","trigger_dynamic_difficulty"},
                        {"EstimatedTime","4.25"}})};
    assert(map.load_document(d).ok());
    assert(native.events.empty()); // no event during dynamic Spawn
    assert(map.unload().code==SourceMapLoadCode::Unloaded);
    assert(gameplay.instance_count()==0);
}
void test_native_removal_failure_preserves_behavior_for_retry() {
    FakeNativeEngine native;
    OriginalTriggerEntityFactory gameplay(native);
    OriginalTriggerMapBridge bridge(native,native,gameplay);
    SourceMapRuntime map(bridge);
    SourceBspEntityDocument d;
    d.entities={entity({{"classname","trigger_secret"}})};
    assert(map.load_document(d).ok());
    const auto e=map.entities()[0];
    native.refuse_removal=true;
    assert(map.unload().code==SourceMapLoadCode::PartialUnload);
    assert(gameplay.instance_count()==1&&native.is_live(e));
    native.refuse_removal=false;
    assert(map.unload().code==SourceMapLoadCode::Unloaded);
    assert(gameplay.instance_count()==0&&!native.is_live(e));
}
} // namespace
int main() {
    test_original_gameplay_triggers_in_native_host();
    test_native_factory_refusal_is_not_a_fake_class();
    test_rejected_native_field_snapshot_and_failed_base_spawn_rollback();
    test_invalid_trigger_key_native_rollback_and_retry();
    test_native_removal_failure_preserves_behavior_for_retry();
    std::puts("PASS SiN native-map bridge: true-host-first factory, normal Source entities, original game event/trigger KeyValues, real-state snapshot and rollback");
}
