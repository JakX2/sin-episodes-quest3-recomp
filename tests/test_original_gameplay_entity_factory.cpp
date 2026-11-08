#include "sin_compat/original_gameplay_entity_factory.hpp"
#include "sin_compat/original_assault_rifle_profile.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace sin_compat;
using namespace sin_compat::map_entities;

struct OriginalHost final : IGameplayEntityHost {
    EntityRef player{4,2};
    EntityRef weapon_ref{9,8};
    EntityRef grenade_ref{33,10};
    EntityRef owner{};
    EntityRef selected{};
    bool player_live=true;
    bool weapon_live=true;
    bool grenade_live=true;
    bool setter_success=true;
    int setter_calls=0;
    SpawnCode callback_reentry=SpawnCode::Created;
    GameplayEntityFactory* on_set_reenter=nullptr;

    bool is_live(EntityRef e) const override {
        return (player_live && e==player) ||
               (weapon_live && e==weapon_ref) ||
               (grenade_live && e==grenade_ref);
    }
    EntityRef owner_of(EntityRef w) const override {
        return w==weapon_ref?owner:EntityRef{};
    }
    EntityRef selected_weapon_of(EntityRef e) const override {
        return e==player?selected:EntityRef{};
    }
    void set_weapon_nodraw(EntityRef,bool) override {}
    bool is_player_owner(EntityRef e) const override {return e==player;}
    int viewmodel_index_of(EntityRef) const override {return 0;}
    bool is_valid_viewmodel_index(EntityRef,int i) const override {return i==0;}
    EntityRef viewmodel_of(EntityRef,int) const override {return {};}
    bool is_viewmodel_owned_by(EntityRef,EntityRef) const override {return false;}
    void set_viewmodel_nodraw(EntityRef,bool) override {}
    bool is_player(EntityRef e) const override {return e==player;}
    int viewmodel_slot_count(EntityRef) const override {return 0;}
    EntityRef viewmodel_at(EntityRef,int) const override {return {};}
    EntityRef create_named_entity(const char*) override {return {};}
    bool configure_viewmodel(EntityRef,EntityRef,int) override {return false;}
    bool store_viewmodel_handle(EntityRef,int,EntityRef) override {return false;}
    void remove_viewmodel(EntityRef) override {}
    void clear_viewmodel_handle(EntityRef,int) override {}
    int resolve_sequence(int a) override {return a<0?-1:a+100;}
    void bind_viewmodel(const std::string&,const std::array<float,3>&) override {}
    void play_sequence(int) override {}
    void set_flatscreen_fov(int,float) override {}

    bool write_original_grenade_contact_flag(EntityRef ref,bool set) override {
        assert(ref==grenade_ref && set);
        ++setter_calls;
        if (on_set_reenter)
            callback_reentry=on_set_reenter->remove(grenade_ref);
        return setter_success;
    }
};

struct FakeRegistrar final : IGameEntityRegistrar {
    struct Entry {SpawnCallback callback;void* context;};
    std::map<std::string,Entry> entries;
    bool reject_grenade=false;
    bool reject_rifle=false;
    bool reject_uninstall=false;
    int installed_count=0;
    int removed_count=0;
    bool install(std::string_view name,SpawnCallback callback,void* context) override {
        if ((reject_rifle && name=="weapon_assault_rifle") ||
            (reject_grenade && name=="grenade_ar")) return false;
        const auto inserted=entries.emplace(std::string(name),Entry{callback,context});
        if (inserted.second) ++installed_count;
        return inserted.second;
    }
    bool uninstall(std::string_view name,SpawnCallback callback,
                   void* context) override {
        if (reject_uninstall) return false;
        const auto it=entries.find(std::string(name));
        if (it==entries.end() || it->second.callback!=callback ||
            it->second.context!=context) return false;
        entries.erase(it);
        ++removed_count;
        return true;
    }
    SpawnResult create(std::string_view name,const SpawnRequest& args) const {
        const auto it=entries.find(std::string(name));
        if (it==entries.end())
            return {SpawnCode::UnknownOrUnimplementedClass,Kind::AssaultRifle};
        return it->second.callback(it->second.context,args);
    }
};

struct BlastHost final : ar_grenade::IHost {
    std::vector<std::string> events;
    int damage=0,removed=0;
    void on_flight_ai_sound(const ar_grenade::AiSoundEvent&) override {
        events.emplace_back("flight");
    }
    void on_original_trace_effect(ar_grenade::Decal) override {
        events.emplace_back("trace");
    }
    void on_screen_shake_feedback(const ar_grenade::ScreenShakeEvent& e) override {
        assert(e.engine_clamped_amplitude==16 &&
               !e.move_tracked_hmd);
        events.emplace_back("shake");
    }
    void on_radius_damage(const ar_grenade::BlastDamageEvent& e) override {
        assert(e.damage==100 && e.radius==250 && e.damage_type==0x40);
        ++damage;events.emplace_back("damage");
    }
    void on_explosion_ai_sound(const ar_grenade::AiSoundEvent& e) override {
        assert(e.type==0x02000001 && e.radius==375);
        events.emplace_back("combat");
    }
    void on_remove_projectile() override {
        ++removed;events.emplace_back("remove");
    }
};

int main() {
    static_assert(OriginalAssaultRifle::clip_size==45);
    assert(GameplayEntityFactory::supports("weapon_assault_rifle"));
    assert(GameplayEntityFactory::supports("grenade_ar"));
    assert(!GameplayEntityFactory::supports("trigger_dynamic_difficulty"));
    assert(!GameplayEntityFactory::supports("npc_druglab_grunt_pistol"));
    assert(!GameplayEntityFactory::supports("weapon_magnum"));
    assert(!GameplayEntityFactory::supports("WEAPON_ASSAULT_RIFLE"));

    OriginalHost host;
    FakeRegistrar registrar;
    GameplayEntityFactory bridge(host,2);
    assert(bridge.install(registrar)==RegistrationCode::Installed);
    assert(bridge.installed());
    assert(registrar.entries.size()==2);
    assert(bridge.install(registrar)==RegistrationCode::AlreadyInstalled);
    assert(registrar.create("trigger_dynamic_difficulty",
                {"trigger_dynamic_difficulty",{50,10}}).code ==
           SpawnCode::UnknownOrUnimplementedClass);
    assert(bridge.instance_count()==0);

    // A weapon placed by the map initially has no owner. It becomes the
    // correct live gameplay object only after the host binds its player.
    const SpawnRequest rifle_request{"weapon_assault_rifle",host.weapon_ref};
    assert(!host.owner_of(host.weapon_ref).generation ||
           host.owner_of(host.weapon_ref)!=host.player);
    const auto rifle_result=registrar.create("weapon_assault_rifle",rifle_request);
    assert(rifle_result.ok() && rifle_result.kind==Kind::AssaultRifle);
    assert(bridge.instance_count()==1 && bridge.weapon(host.weapon_ref)==nullptr);
    assert(bridge.grenade(host.weapon_ref)==nullptr);
    assert(bridge.bind_weapon(host.weapon_ref,host.player)==SpawnCode::HostRejected);
    host.owner=host.player;
    host.selected=host.weapon_ref;
    assert(bridge.bind_weapon(host.weapon_ref,host.player)==SpawnCode::Created);
    assert(bridge.weapon(host.weapon_ref)!=nullptr);
    assert(bridge.weapon(host.weapon_ref)->can_launch_rifle_grenade());
    assert(bridge.weapon(host.weapon_ref)->runtime().config().viewmodel_name==
           OriginalAssaultRifle::viewmodel);
    assert(bridge.weapon(host.weapon_ref)->runtime().config().ammo.clip_size==45);
    assert(bridge.bind_weapon(host.weapon_ref,host.player)==SpawnCode::AlreadyBound);

    // Same entity index with a new generation cannot inherit the previous
    // player's ammo, session or model without a full old-generation cleanup.
    assert(bridge.spawn({"weapon_assault_rifle",{9,9}}).code==
           SpawnCode::DuplicateIndex);
    assert(bridge.spawn({"npc_druglab_grunt_pistol",{34,10}}).code==
           SpawnCode::UnknownOrUnimplementedClass);

    const SpawnRequest grenade_request{
        "grenade_ar",host.grenade_ref,{10,20,30},{2,4,6},1.0f
    };
    assert(registrar.create("grenade_ar",grenade_request).ok());
    assert(bridge.instance_count()==2);
    assert(bridge.grenade(host.grenade_ref)!=nullptr);
    assert(bridge.grenade(host.grenade_ref)->armed());
    assert(!bridge.original_grenade_touch_flag(host.grenade_ref));
    assert(bridge.spawn({"grenade_ar",{40,5}}).code==
           SpawnCode::CapacityExceeded);

    // Touch only after the original verified raw field/bit/owner predicates.
    BlastHost blast;
    ar_grenade_touch::OriginalTouchObservation contact{};
    contact.other_entity_present=true;
    contact.other_field_11c=7;
    contact.other_flags_120=0x04;
    contact.other_virtual_fc_result_nonnull=true;
    contact.other_virtual_fc_matches_local_helper=false;
    assert(!bridge.grenade_touch(host.grenade_ref,contact,
             ar_grenade::Decal::Scorch,blast).dispatched());
    assert(host.setter_calls==0 && blast.damage==0);
    contact.other_flags_120=0;
    contact.other_virtual_fc_matches_local_helper=true;
    assert(!bridge.grenade_touch(host.grenade_ref,contact,
             ar_grenade::Decal::Scorch,blast).dispatched());
    assert(host.setter_calls==0);

    // A caller cannot forge the projectile's separate networked original
    // +0x795 flag via an input observation.
    contact.grenade_original_field_795=true;
    assert(!bridge.grenade_touch(host.grenade_ref,contact,
             ar_grenade::Decal::Scorch,blast).dispatched());
    assert(!bridge.original_grenade_touch_flag(host.grenade_ref));

    contact.other_virtual_fc_matches_local_helper=false;
    host.setter_success=false;
    const auto denied=bridge.grenade_touch(
        host.grenade_ref,contact,ar_grenade::Decal::Scorch,blast);
    assert(denied.code==SpawnCode::HostRejected);
    assert(host.setter_calls==1 && blast.damage==0);
    assert(bridge.grenade(host.grenade_ref)->armed());
    assert(!bridge.original_grenade_touch_flag(host.grenade_ref));
    host.setter_success=true;

    // Original network change setter runs before the real portable one-shot
    // explosion; reentrant host destruction is rejected during that call.
    host.on_set_reenter=&bridge;
    const auto accepted=bridge.grenade_touch(
        host.grenade_ref,contact,ar_grenade::Decal::Scorch,blast);
    assert(accepted.dispatched());
    assert(host.callback_reentry==SpawnCode::Reentrant);
    assert(host.setter_calls==2);
    assert(bridge.original_grenade_touch_flag(host.grenade_ref));
    assert(blast.damage==1 && blast.removed==1);
    assert((blast.events==std::vector<std::string>{
        "trace","shake","damage","combat","remove"
    }));
    assert(!bridge.grenade_touch(host.grenade_ref,contact,
             ar_grenade::Decal::Scorch,blast).dispatched());
    assert(blast.damage==1 && blast.removed==1);
    host.on_set_reenter=nullptr;

    assert(bridge.unbind_weapon(host.weapon_ref)==SpawnCode::Created);
    assert(bridge.weapon(host.weapon_ref)==nullptr);
    assert(bridge.unbind_weapon(host.weapon_ref)==SpawnCode::NotFound);
    assert(bridge.remove({host.weapon_ref.index,host.weapon_ref.generation+1})==
           SpawnCode::NotFound);
    assert(bridge.remove(host.weapon_ref)==SpawnCode::Created);
    assert(bridge.remove(host.grenade_ref)==SpawnCode::Created);
    assert(bridge.instance_count()==0);
    assert(bridge.remove(host.grenade_ref)==SpawnCode::NotFound);
    assert(bridge.uninstall()==RegistrationCode::Uninstalled);
    assert(registrar.entries.empty() && registrar.removed_count==2);

    // Failed native-host registration must not leave one fake classname.
    {
        OriginalHost local;
        FakeRegistrar fail;
        GameplayEntityFactory candidate(local);
        fail.reject_grenade=true;
        assert(candidate.install(fail)==RegistrationCode::HostRejected);
        assert(fail.entries.empty() && !candidate.installed());
        fail.reject_grenade=false;
        assert(candidate.install(fail)==RegistrationCode::Installed);
        assert(candidate.uninstall()==RegistrationCode::Uninstalled);
    }
    {
        OriginalHost local;
        FakeRegistrar fail;
        GameplayEntityFactory candidate(local);
        fail.reject_grenade=true;
        fail.reject_uninstall=true;
        assert(candidate.install(fail)==RegistrationCode::PartialRollbackFailed);
        assert(fail.entries.count("weapon_assault_rifle")==1);
        fail.reject_uninstall=false;
        assert(candidate.uninstall()==RegistrationCode::Uninstalled);
        assert(fail.entries.empty());
    }
    {
        OriginalHost local;
        FakeRegistrar limited;
        GameplayEntityFactory candidate(local,0);
        assert(candidate.spawn({"weapon_assault_rifle",local.weapon_ref}).code==
               SpawnCode::CapacityExceeded);
        assert(candidate.spawn({"npc_sintek_grunt",local.weapon_ref}).code==
               SpawnCode::UnknownOrUnimplementedClass);
    }
    {
        OriginalHost local;
        FakeRegistrar tmp;
        GameplayEntityFactory candidate(local);
        assert(candidate.spawn({"grenade_ar",{}}).code==
               SpawnCode::InvalidEntity);
        SpawnRequest invalid{"grenade_ar",local.grenade_ref,{0,0,0},{0,0,0},1.0f};
        invalid.game_time_seconds=1.0e38f;
        assert(candidate.spawn(invalid).code==SpawnCode::InvalidProjectileKinematics);
        assert(candidate.instance_count()==0);
        local.grenade_live=false;
        invalid.game_time_seconds=1.0f;
        assert(candidate.spawn(invalid).code==SpawnCode::HostEntityNotLive);
        assert(candidate.instance_count()==0);
    }
    std::puts("PASS: original SiN named rifle/grenade entity factories create actual portable gameplay (mock host only)");
}
