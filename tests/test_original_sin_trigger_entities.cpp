// Real original 2006 SiN trigger business logic, exercised with an
// ORIGINAL-FREE native host. No Source entity ABI or proprietary data.
#include "sin_compat/original_sin_trigger_entities.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
using namespace sin_compat;
using namespace sin_compat::map_entities;

struct FakeRegistrar final: IGameEntityRegistrar {
    struct Entry{SpawnCallback callback;void* context;};
    std::map<std::string,Entry> entries;
    bool fail_secret=false,fail_uninstall=false;
    bool install(std::string_view n,SpawnCallback cb,void* ctx) override {
        if(fail_secret && n=="trigger_secret")return false;
        return entries.emplace(std::string(n),Entry{cb,ctx}).second;
    }
    bool uninstall(std::string_view n,SpawnCallback cb,void* ctx) override {
        if(fail_uninstall)return false;
        auto it=entries.find(std::string(n));
        if(it==entries.end()||it->second.callback!=cb||
           it->second.context!=ctx)return false;
        entries.erase(it);
        return true;
    }
    SpawnResult create(std::string_view n,EntityRef e,
                       ar_grenade::Vec3 pos={}) const {
        auto it=entries.find(std::string(n));
        if(it==entries.end())return {SpawnCode::UnknownOrUnimplementedClass,Kind::AssaultRifle};
        return it->second.callback(it->second.context,{n,e,pos});
    }
};
struct Captured {
    TriggerEventName event;
    int descriptor=0,owner=0,advisor=0,decision=0;
    float value=0;
    TriggerVec3 origin{};
    std::string name;
};
struct Host final:IOriginalTriggerHost {
    EntityRef dynamic{40,2},secret{41,3},player{7,2},npc{9,1};
    bool event_ok=true,remove_ok=true,manager_ok=true,special_ok=true,base_ok=true;
    bool throw_owner=false;
    int event_calls=0,remove_calls=0,manager_calls=0,special_calls=0,base_calls=0;
    OriginalTriggerEntityFactory* reentry=nullptr;
    TriggerTouchReport reentry_result{};
    std::vector<std::string> order;
    std::vector<Captured> events;
    std::vector<std::string> warnings;
    bool is_live(EntityRef e) const override {
        return e==dynamic||e==secret||e==player||e==npc;
    }
    bool is_player(EntityRef e) const override{return e==player;}
    void report_original_warning(std::string_view msg) override {
        warnings.emplace_back(msg);
        order.emplace_back("warn");
    }
    bool initialize_native_base_trigger(EntityRef e) override {
        assert(e==secret||e==dynamic);
        ++base_calls;order.emplace_back("base-spawn");
        return base_ok;
    }
    int current_difficulty_owner_index() const override {
        if(throw_owner)throw std::runtime_error("owner getter failure");
        return 42;
    }
    bool perform_special_trigger_base_action(EntityRef e) override {
        assert(e==dynamic);
        ++special_calls;order.emplace_back("base-special");
        return special_ok;
    }
    bool publish(const TriggerEvent& e) override {
        ++event_calls;order.emplace_back("publish");
        events.push_back({e.event,e.descriptor,e.owner,e.advisor,e.decision,
                          e.value,e.origin,std::string(e.name)});
        if(reentry)reentry_result=reentry->touch(dynamic,player);
        return event_ok;
    }
    bool mark_difficulty_manager() override {
        ++manager_calls;order.emplace_back("manager");
        return manager_ok;
    }
    bool queue_remove(EntityRef e) override {
        assert(e==dynamic||e==secret);
        ++remove_calls;order.emplace_back("remove");
        return remove_ok;
    }
};
int main(){
    static_assert(sizeof(TriggerVec3)==12,"original Source Vector three f32s");
    assert(OriginalTriggerEntityFactory::supports("trigger_dynamic_difficulty"));
    assert(OriginalTriggerEntityFactory::supports("trigger_secret"));
    assert(!OriginalTriggerEntityFactory::supports("trigger_multiple"));
    assert(!OriginalTriggerEntityFactory::supports("npc_druglab_grunt_pistol"));
    Host host{};
    FakeRegistrar reg{};
    OriginalTriggerEntityFactory game(host,2);
    assert(game.install(reg)==RegistrationCode::Installed);
    assert(game.install(reg)==RegistrationCode::AlreadyInstalled);
    assert(game.installed()&&reg.entries.size()==2);

    // Only a REAL host entity is allowed to acquire a behavior instance.
    assert(reg.create("trigger_dynamic_difficulty",{4,2}).code==
           SpawnCode::HostEntityNotLive);
    assert(reg.create("trigger_dynamic_difficulty",host.dynamic).ok());
    assert(reg.create("trigger_secret",host.secret).ok());
    assert(reg.create("trigger_dynamic_difficulty",{50,1}).code==
           SpawnCode::CapacityExceeded);
    assert(game.spawn({"trigger_dynamic_difficulty",{40,3}}).code==
           SpawnCode::DuplicateIndex);
    assert(game.instance_count()==2);
    assert(reg.create("trigger_dynamic_difficulty",host.secret).code==
           SpawnCode::DuplicateIndex);

    TriggerConfig c{};
    c.targetname="arena_balance_trigger";
    c.origin={32.0f,64.0f,96.0f};
    c.estimated_time=0.75f;
    c.estimated_difficulty=5;
    c.end_of_game=1;
    c.base_special_flag_0x800=true;
    assert(game.configure(host.dynamic,c)==TriggerCode::Handled);
    TriggerConfig bad=c;
    bad.estimated_time=std::numeric_limits<float>::quiet_NaN();
    assert(game.configure(host.dynamic,bad)==TriggerCode::InvalidConfig);
    assert(game.touch(host.dynamic,host.npc).code==TriggerCode::NonPlayer);
    assert(game.touch(host.dynamic,{0xffffffffu,0xffffffffu}).code==
           TriggerCode::InvalidActor);
    assert(host.event_calls==0&&host.remove_calls==0);

    // Unknown special base-action code is NOT silently skipped: an
    // engine that cannot implement it refuses before the game event.
    host.special_ok=false;
    assert(game.touch(host.dynamic,host.player).code==TriggerCode::HostRefused);
    assert(host.event_calls==0&&host.remove_calls==0);
    host.special_ok=true;
    host.reentry=&game;
    auto dyn=game.touch(host.dynamic,host.player);
    assert(dyn.handled()&&dyn.event_published&&dyn.manager_signaled&&
           dyn.removal_queued);
    assert(host.reentry_result.code==TriggerCode::Reentrant);
    assert((host.order==std::vector<std::string>{"base-special","base-special","publish","manager","remove"}));
    assert(host.events.size()==1&&host.events[0].event==
           TriggerEventName::DynamicDifficulty);
    assert(host.events[0].name=="arena_balance_trigger");
    assert(host.events[0].descriptor==0&&host.events[0].owner==42&&
           host.events[0].advisor==-1&&host.events[0].decision==5&&
           host.events[0].value==0.75f&&host.events[0].origin.x==32.0f);
    assert(game.touch(host.dynamic,host.player).code==
           TriggerCode::AlreadyConsumed);
    assert(game.configure(host.dynamic,c)==TriggerCode::AlreadyConsumed);
    assert(host.remove_calls==1&&host.event_calls==1&&host.manager_calls==1);
    host.reentry=nullptr;

    // Secret consumes only player touch; publishes the original "game"
    // descriptor=12 event, with fixed owner=-1 and name, then removal.
    assert(game.touch(host.secret,host.npc).code==TriggerCode::NonPlayer);
    assert(game.touch(host.secret,host.player).handled());
    assert(host.event_calls==2&&host.remove_calls==2&&host.manager_calls==1);
    assert(host.events[1].event==TriggerEventName::Game);
    assert(host.events[1].descriptor==12&&host.events[1].owner==-1&&
           host.events[1].name=="trigger_secret");

    assert(game.release({host.dynamic.index,host.dynamic.generation+1})==
           TriggerCode::UnknownEntity);
    assert(game.release(host.dynamic)==TriggerCode::Handled);
    assert(game.release(host.secret)==TriggerCode::Handled);
    assert(game.instance_count()==0);
    assert(game.uninstall()==RegistrationCode::Uninstalled&&reg.entries.empty());

    // The actual 2006 DATADESC map keys are parsed by the independent
    // host adapter. No fabricated "Difficulty" or "Value" KeyValues.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        assert(f.apply_map_keyvalue(h.dynamic,"EstimatedTime","12.5")==
               TriggerCode::Handled);
        assert(f.apply_map_keyvalue(h.dynamic,"EstimatedDifficulty","7")==
               TriggerCode::Handled);
        assert(f.apply_map_keyvalue(h.dynamic,"EndOfGame","1")==
               TriggerCode::Handled);
        assert(f.apply_map_keyvalue(h.dynamic,"targetname","change_phase_01")==
               TriggerCode::Handled);
        assert(f.apply_map_keyvalue(h.dynamic,"EstimatedDifficulty","7oops")==
               TriggerCode::InvalidConfig);
        assert(f.apply_map_keyvalue(h.dynamic,"EstimatedTime","nan")==
               TriggerCode::InvalidConfig);
        assert(f.apply_map_keyvalue(h.dynamic,"EstimatedTime","1e200")==
               TriggerCode::InvalidConfig);
        assert(f.apply_map_keyvalue(h.dynamic,"EstimatedDifficulty","2147483648")==
               TriggerCode::InvalidConfig);
        assert(f.apply_map_keyvalue(h.dynamic,"MissingKey","5")==
               TriggerCode::UnknownKey);
        assert(f.touch(h.dynamic,h.player).handled());
        assert(h.events.size()==1);
        assert(h.events[0].value==12.5f&&h.events[0].decision==7&&
               h.events[0].name=="change_phase_01");
        assert(h.manager_calls==1);
        assert(f.apply_map_keyvalue(h.dynamic,"EndOfGame","0")==
               TriggerCode::AlreadyConsumed);
    }
    // Original x86 CMP BYTE [this+0x478],0 specifically tests only
    // the LOW BYTE of the 32-bit DATADESC EndOfGame integer.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        assert(f.apply_map_keyvalue(h.dynamic,"EndOfGame","256")==
               TriggerCode::Handled);
        assert(f.touch(h.dynamic,h.player).handled());
        assert(h.manager_calls==0&&h.remove_calls==1);
    }
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        assert(f.apply_map_keyvalue(h.secret,"EstimatedTime","5")==
               TriggerCode::UnknownKey);
        assert(f.apply_map_keyvalue(h.secret,"targetname","secret_01")==
               TriggerCode::Handled);
        assert(f.touch(h.secret,h.player).handled());
        assert(h.events[0].name=="secret_01"&&h.events[0].descriptor==12);
    }
    // Phase527: Original CTriggerSecret::Spawn (slot25) is a SEPARATE
    // engine callback from our host factory allocation. It sends the
    // SAME 'game' event but descriptor=11 on +0x120 spawnflags bit3.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        assert(f.apply_map_keyvalue(h.secret,"targetname","first_secret")==
               TriggerCode::Handled);
        auto spawned=f.run_secret_spawn(h.secret,{false,0x08u});
        assert(spawned.handled()&&spawned.base_initialized&&
               spawned.event_published&&!spawned.removal_queued);
        assert(h.base_calls==1&&h.event_calls==1&&h.remove_calls==0);
        assert((h.order==std::vector<std::string>{"base-spawn","publish"}));
        assert(h.events[0].event==TriggerEventName::Game &&
               h.events[0].descriptor==11&&h.events[0].owner==-1 &&
               h.events[0].name=="first_secret");
        assert(f.run_secret_spawn(h.secret,{false,0x08u}).code==
               TriggerCode::AlreadyConsumed);
        assert(h.event_calls==1);
        // A later player Touch produces distinct original descriptor12
        // exactly once, followed by deferred entity removal.
        assert(f.touch(h.secret,h.npc).code==TriggerCode::NonPlayer);
        assert(f.touch(h.secret,h.player).handled());
        assert(h.events.size()==2&&h.events[1].descriptor==12&&
               h.remove_calls==1);
        assert(f.touch(h.secret,h.player).code==TriggerCode::AlreadyConsumed);
    }
    // Original BIT3 predicate, not any nonzero spawnflags.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        auto no_event=f.run_secret_spawn(h.secret,{false,0x100u});
        assert(no_event.handled()&&no_event.base_initialized&&
               !no_event.event_published&&!no_event.removal_queued);
        assert(h.base_calls==1&&h.event_calls==0);
    }
    // Original global vslot +0x88 returns true: immediate queued
    // deletion request, skipping CBaseTrigger::Spawn and event11.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        auto removed=f.run_secret_spawn(h.secret,{true,0x08u});
        assert(removed.handled()&&removed.removal_queued);
        assert(h.base_calls==0&&h.event_calls==0&&h.remove_calls==1);
        assert(f.touch(h.secret,h.player).code==TriggerCode::AlreadyConsumed);
    }
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        h.remove_ok=false;
        auto first=f.run_secret_spawn(h.secret,{true,0x08u});
        assert(first.code==TriggerCode::RemovalPending);
        assert(h.event_calls==0&&h.base_calls==0&&h.remove_calls==1);
        h.remove_ok=true;
        assert(f.retry_pending_remove(h.secret)==TriggerCode::Handled);
        assert(h.event_calls==0&&h.remove_calls==2);
    }
    // If genuine native CBaseTrigger Spawn cannot run, do not fabricate
    // event11 or latch a completed spawn; host may retry later.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        h.base_ok=false;
        assert(f.run_secret_spawn(h.secret,{false,0x08u}).code==
               TriggerCode::HostRefused);
        assert(h.base_calls==1&&h.event_calls==0);
        h.base_ok=true;
        assert(f.run_secret_spawn(h.secret,{false,0x08u}).handled());
        assert(h.base_calls==2&&h.event_calls==1);
        assert(f.run_secret_spawn(h.secret,{false,0}).code==
               TriggerCode::AlreadyConsumed);
    }
    // Event creator may reject original descriptor11 while the native
    // base Spawn still completes. It is not re-emitted on retries.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        h.event_ok=false;
        const auto out=f.run_secret_spawn(h.secret,{false,0x08u});
        assert(out.code==TriggerCode::EventUnavailable&&
               out.base_initialized&&!out.event_published);
        assert(f.run_secret_spawn(h.secret,{false,0x08u}).code==
               TriggerCode::AlreadyConsumed);
        assert(h.event_calls==1&&h.base_calls==1);
    }
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        assert(f.run_secret_spawn(h.dynamic,{false,0x08u}).code==
               TriggerCode::WrongClass);
        assert(h.base_calls==0&&h.event_calls==0);
    }
    // Phase528: the ORIGINAL CTriggerDynamicDifficulty::Spawn never
    // publishes a dynamic_difficulty event. It first runs CBaseTrigger
    // Spawn and logs a warning if native field this+0xB4 has bit10.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        auto out=f.run_dynamic_spawn(h.dynamic,{false,0,0x400u});
        assert(out.handled()&&out.base_initialized&&out.warning_emitted);
        assert(h.base_calls==1&&h.event_calls==0&&h.remove_calls==0);
        assert(h.warnings==std::vector<std::string>{
            "DD Trigger with don't destroy set\n"
        });
        assert((h.order==std::vector<std::string>{"base-spawn","warn"}));
        assert(f.run_dynamic_spawn(h.dynamic,{false,0,0x400u}).code==
               TriggerCode::AlreadyConsumed);
        // Dynamic event still happens once, later, ONLY on player Touch.
        assert(f.touch(h.dynamic,h.player).handled());
        assert(h.events[0].event==TriggerEventName::DynamicDifficulty);
        assert(h.remove_calls==1&&h.event_calls==1);
    }
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        assert(f.run_dynamic_spawn(h.dynamic,{false,0,0x800u}).handled());
        assert(h.warnings.empty()&&h.base_calls==1);
    }
    // Global +0x88 veto requests removal BEFORE base Spawn and warning.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        auto out=f.run_dynamic_spawn(h.dynamic,{true,0,0x400u});
        assert(out.handled()&&out.removal_queued);
        assert(h.base_calls==0&&h.warnings.empty()&&h.remove_calls==1);
        assert(f.touch(h.dynamic,h.player).code==TriggerCode::AlreadyConsumed);
    }
    // Conservative failure handling: base initialization rejected;
    // after host repair, retry original base method once and latch.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        h.base_ok=false;
        assert(f.run_dynamic_spawn(h.dynamic,{false,0,0x400u}).code==
               TriggerCode::HostRefused);
        assert(h.warnings.empty()&&h.base_calls==1);
        h.base_ok=true;
        assert(f.run_dynamic_spawn(h.dynamic,{false,0,0x400u}).handled());
        assert(h.warnings.size()==1&&h.base_calls==2);
        assert(f.run_secret_spawn(h.dynamic,{false,0,0}).code==
               TriggerCode::WrongClass);
    }
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        assert(f.run_dynamic_spawn(h.secret,{false,0,0x400u}).code==
               TriggerCode::WrongClass);
        assert(h.warnings.empty()&&h.base_calls==0);
    }
    // Event-manager failure did not stop original trigger removal.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        h.event_ok=false;
        const auto result=f.touch(h.dynamic,h.player);
        assert(result.code==TriggerCode::EventUnavailable);
        assert(!result.event_published&&result.removal_queued);
        assert(h.event_calls==1&&h.remove_calls==1);
    }
    // A failed engine removal remains pending. Retry ONLY the engine
    // removal, never emit the original game event twice.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_secret",h.secret}).ok());
        h.remove_ok=false;
        const auto first=f.touch(h.secret,h.player);
        assert(first.code==TriggerCode::RemovalPending);
        assert(f.touch(h.secret,h.player).code==TriggerCode::AlreadyConsumed);
        assert(f.retry_pending_remove(h.secret)==TriggerCode::RemovalPending);
        assert(h.event_calls==1&&h.remove_calls==2);
        h.remove_ok=true;
        assert(f.retry_pending_remove(h.secret)==TriggerCode::Handled);
        assert(f.retry_pending_remove(h.secret)==TriggerCode::AlreadyConsumed);
        assert(h.event_calls==1&&h.remove_calls==3);
    }
    // Manager signaling refusal is NOT permission to skip engine remove.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        TriggerConfig c{};c.end_of_game=1;
        assert(f.configure(h.dynamic,c)==TriggerCode::Handled);
        h.manager_ok=false;
        const auto result=f.touch(h.dynamic,h.player);
        assert(result.code==TriggerCode::ManagerRefused);
        assert(h.event_calls==1&&h.remove_calls==1&&h.manager_calls==1);
    }
    // The owner field is optional: failed original-owner resolution
    // degrades to source-observed -1 without suppressing the event.
    {
        Host h{};
        OriginalTriggerEntityFactory f(h);
        assert(f.spawn({"trigger_dynamic_difficulty",h.dynamic}).ok());
        h.throw_owner=true;
        assert(f.touch(h.dynamic,h.player).handled());
        assert(h.events[0].owner==-1);
    }
    // Atomic registration rollback and explicit retry if it failed.
    {
        Host h{};
        FakeRegistrar r{};
        OriginalTriggerEntityFactory f(h);
        r.fail_secret=true;
        assert(f.install(r)==RegistrationCode::HostRejected);
        assert(r.entries.empty()&&!f.installed());
        r.fail_uninstall=true;
        assert(f.install(r)==RegistrationCode::PartialRollbackFailed);
        assert(r.entries.size()==1&&!f.installed());
        r.fail_uninstall=false;
        assert(f.uninstall()==RegistrationCode::Uninstalled);
        r.fail_secret=false;
        assert(f.install(r)==RegistrationCode::Installed);
        assert(f.uninstall()==RegistrationCode::Uninstalled);
    }
    std::puts("PASS original SiN trigger_dynamic_difficulty and trigger_secret: player gate, typed event fields, real host removal, reentrancy, rollback (synthetic native host)");
}
