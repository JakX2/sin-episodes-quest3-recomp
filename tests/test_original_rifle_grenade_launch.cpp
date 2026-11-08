#include "sin_compat/original_rifle_grenade_launch.hpp"
#include "sin_compat/original_assault_rifle_profile.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

using namespace sin_compat;
using namespace sin_compat::ar_grenade_launch;

class SessionHost final : public IWeaponSessionHost {
public:
    EntityRef owner{4,2};
    EntityRef weapon{9,8};
    EntityRef selected = weapon;
    bool owner_live = true;
    bool weapon_live = true;
    bool player = true;

    bool is_live(EntityRef e) const override {
        return (e==owner && owner_live) || (e==weapon && weapon_live);
    }
    EntityRef owner_of(EntityRef) const override { return owner; }
    EntityRef selected_weapon_of(EntityRef) const override { return selected; }
    void set_weapon_nodraw(EntityRef, bool) override {}
    bool is_player_owner(EntityRef) const override { return player; }
    int viewmodel_index_of(EntityRef) const override { return 0; }
    bool is_valid_viewmodel_index(EntityRef, int i) const override {return i==0;}
    EntityRef viewmodel_of(EntityRef, int) const override {return {}; }
    bool is_viewmodel_owned_by(EntityRef,EntityRef) const override {return false;}
    void set_viewmodel_nodraw(EntityRef,bool) override {}
    bool is_player(EntityRef) const override {return player;}
    int viewmodel_slot_count(EntityRef) const override {return 0;}
    EntityRef viewmodel_at(EntityRef,int) const override {return {};}
    EntityRef create_named_entity(const char*) override {return {};}
    bool configure_viewmodel(EntityRef,EntityRef,int) override {return false;}
    bool store_viewmodel_handle(EntityRef,int,EntityRef) override {return false;}
    void remove_viewmodel(EntityRef) override {}
    void clear_viewmodel_handle(EntityRef,int) override {}
    int resolve_sequence(int activity) override {return activity>=0 ? activity+100: -1;}
    void bind_viewmodel(const std::string&,const std::array<float,3>&) override {}
    void play_sequence(int) override {}
    void set_flatscreen_fov(int,float) override {}
};

class WorldHost final : public ILaunchHost {
public:
    EntityRef owner{4,2};
    EntityRef weapon{9,8};
    EntityRef staged{33,10};
    OwnerKind kind = OwnerKind::Player;
    AmmoSnapshot ammo{AmmoKind::Finite,4};
    bool owner_live = true;
    bool weapon_live = true;
    bool allow_grunt = true;
    bool fail_prepare = false;
    bool fail_commit = false;
    int ammo_checks = 0;
    int prepared = 0;
    int committed = 0;
    int aborted = 0;
    int ammo_spent = 0;
    int last_debit = -1;
    std::string classname, model;
    ar_grenade::Vec3 origin{}, velocity{};
    std::vector<std::string> operations;
    std::function<void()> on_prepare;

    bool is_live(EntityRef r) const noexcept override {
        return (r==owner && owner_live)||(r==weapon && weapon_live);
    }
    EntityRef owner_of(EntityRef) const noexcept override {return owner;}
    OwnerKind owner_kind(EntityRef) const noexcept override {return kind;}
    AmmoSnapshot player_secondary_ammo(EntityRef r,
            std::string_view type) const noexcept override {
        assert(r==owner && type=="ARNADE");
        // All calls in this test use a mutable external ammo ledger.
        ++const_cast<WorldHost*>(this)->ammo_checks;
        return ammo;
    }
    bool approve_grunt_launch(EntityRef r,EntityRef w) const noexcept override {
        return allow_grunt && r==owner && w==weapon;
    }
    EntityRef prepare_projectile(EntityRef r,std::string_view entity_name,
                                 std::string_view requested_model,
                                 const ar_grenade::Vec3& pos,
                                 const ar_grenade::Vec3& vel) noexcept override {
        assert(r==owner);
        ++prepared;
        operations.emplace_back("prepare");
        classname=std::string(entity_name);
        model=std::string(requested_model);
        origin=pos;
        velocity=vel;
        if (on_prepare) on_prepare();
        return fail_prepare ? EntityRef{} : staged;
    }
    bool commit_projectile(EntityRef handle,EntityRef r,
                           std::int32_t debit) noexcept override {
        assert(handle==staged && r==owner);
        ++committed;
        operations.emplace_back("commit");
        last_debit=debit;
        if (fail_commit) return false;
        // Emulates only the documented atomic host contract. It is not the
        // original Source engine's ammo/alloc implementation.
        ammo_spent+=debit;
        if (debit && ammo.kind==AmmoKind::Finite) ammo.remaining-=debit;
        return true;
    }
    void abort_prepared_projectile(EntityRef handle) noexcept override {
        assert(handle==staged);
        ++aborted;
        operations.emplace_back("abort");
    }
};

static LaunchInput sample(const SessionHost& h) {
    return {h.owner,h.weapon,{10.0f,20.0f,30.0f},
            {1.0f,0.5f,-0.25f},3.0f};
}

int main() {
    static_assert(original_player_direction_multiplier==2000.0f);
    static_assert(original_grunt_direction_multiplier==1000.0f);
    static_assert(original_player_secondary_debit==1);
    static_assert(original_secondary_ammo_name=="ARNADE");
    assert(std::string(ar_grenade::entity_classname)=="grenade_ar");
    SessionHost session_host{};
    WorldHost world{};
    const auto input=sample(session_host);
    WeaponSession session(session_host,session_host.owner,session_host.weapon,
        WeaponKind::AssaultRifle,OriginalAssaultRifle::make_runtime_config(),true);
    Launcher launcher{};
    ar_grenade::Runtime grenade{};

    assert(session.can_launch_rifle_grenade());
    session.runtime().ammo().reserve2=99;
    const LaunchResult fired=launcher.fire_player(session,world,input,grenade);
    assert(fired.ok() && fired.code==LaunchCode::Launched);
    assert(fired.projectile==world.staged);
    assert(fired.player_ammo_debit==1 && fired.direction_multiplier==2000);
    assert(world.ammo_checks==1 && world.ammo_spent==1);
    assert(world.ammo.remaining==3);
    assert(session.runtime().ammo().reserve2==99); // owner ammo is authoritative
    assert(world.prepared==1 && world.committed==1 && world.aborted==0);
    assert((world.operations==std::vector<std::string>{"prepare","commit"}));
    assert(world.classname=="grenade_ar");
    assert(world.model=="models/weapons/grenades/ar_grenade.mdl");
    assert(world.origin.x==10 && world.origin.y==20 && world.origin.z==30);
    assert(world.velocity.x==2000 && world.velocity.y==1000 &&
           world.velocity.z==-500);
    assert(grenade.spawned() && grenade.armed());
    assert(grenade.velocity().x==world.velocity.x);
    assert(std::fabs(grenade.next_think_seconds()-3.1f)<0.0001f);

    // The same runtime cannot silently be replaced with an unrelated shot.
    assert(launcher.fire_player(session,world,input,grenade).code==
           LaunchCode::ProjectileAlreadyBound);
    assert(world.prepared==1 && world.ammo_spent==1);

    // All failures here are BEFORE engine entity preparation.
    {
        WorldHost no_ammo{};
        no_ammo.ammo.remaining=0;
        ar_grenade::Runtime output{};
        assert(launcher.fire_player(session,no_ammo,input,output).code==
               LaunchCode::NoSecondaryAmmo);
        assert(no_ammo.prepared==0 && no_ammo.ammo_spent==0);
        no_ammo.ammo.kind=AmmoKind::InfiniteOrUnknown;
        assert(launcher.fire_player(session,no_ammo,input,output).code==
               LaunchCode::AmmoSemanticsUnverified);
        no_ammo.ammo.kind=AmmoKind::InvalidIndex;
        assert(launcher.fire_player(session,no_ammo,input,output).code==
               LaunchCode::AmmoSemanticsUnverified);
        no_ammo.kind=OwnerKind::GruntBase;
        assert(launcher.fire_player(session,no_ammo,input,output).code==
               LaunchCode::OwnerNotAuthorized);
        assert(!output.spawned());
    }
    {
        WorldHost blocked{};
        ar_grenade::Runtime output{};
        session_host.selected={77,2};
        assert(!session.can_launch_rifle_grenade());
        assert(launcher.fire_player(session,blocked,input,output).code==
               LaunchCode::WeaponNotSelectedOrReady);
        session_host.selected=session_host.weapon;
        session_host.weapon_live=false;
        assert(launcher.fire_player(session,blocked,input,output).code==
               LaunchCode::WeaponNotSelectedOrReady);
        session_host.weapon_live=true;
        blocked.owner_live=false;
        assert(launcher.fire_player(session,blocked,input,output).code==
               LaunchCode::OwnerNotAuthorized);
        blocked.owner_live=true;
        auto bad=input;
        bad.weapon={99,1};
        assert(launcher.fire_player(session,blocked,bad,output).code==
               LaunchCode::WrongWeaponOrOwner);
        bad=input;
        bad.direction.x=std::numeric_limits<float>::infinity();
        assert(launcher.fire_player(session,blocked,bad,output).code==
               LaunchCode::InvalidPhysicsInput);
        bad=input;
        bad.direction.x=std::numeric_limits<float>::max();
        assert(launcher.fire_player(session,blocked,bad,output).code==
               LaunchCode::InvalidPhysicsInput);
        bad=input;
        bad.game_time_seconds=std::numeric_limits<float>::max();
        assert(launcher.fire_player(session,blocked,bad,output).code==
               LaunchCode::InvalidPhysicsInput);
        assert(blocked.prepared==0 && blocked.committed==0);
    }
    {
        WorldHost failed{};
        ar_grenade::Runtime output{};
        failed.fail_prepare=true;
        assert(launcher.fire_player(session,failed,input,output).code==
               LaunchCode::EntityPreparationFailed);
        assert(!output.spawned() && failed.ammo_spent==0 && failed.aborted==0);
        failed.fail_prepare=false;
        failed.fail_commit=true;
        assert(launcher.fire_player(session,failed,input,output).code==
               LaunchCode::TransactionFailed);
        assert(!output.spawned() && failed.ammo_spent==0);
        assert(failed.aborted==1);
        assert((failed.operations==std::vector<std::string>{
            "prepare","prepare","commit","abort"
        }));
    }
    {
        WorldHost recurse{};
        ar_grenade::Runtime output{}, nested{};
        LaunchCode nested_code=LaunchCode::Launched;
        recurse.on_prepare=[&] {
            nested_code=launcher.fire_player(session,recurse,input,nested).code;
        };
        assert(launcher.fire_player(session,recurse,input,output).ok());
        assert(nested_code==LaunchCode::RecursiveLaunch);
        assert(recurse.ammo_spent==1 && recurse.prepared==1 &&
               output.spawned() && !nested.spawned());
    }
    {
        SessionHost other{};
        WorldHost host{};
        WeaponSession magnum(other,other.owner,other.weapon,WeaponKind::Magnum,
            OriginalAssaultRifle::make_runtime_config(),true);
        ar_grenade::Runtime output{};
        assert(!magnum.can_launch_rifle_grenade());
        assert(launcher.fire_player(magnum,host,input,output).code==
               LaunchCode::WrongWeaponOrOwner);
    }
    {
        SessionHost active{};
        WorldHost host{};
        WeaponSession reloading(active,active.owner,active.weapon,
            WeaponKind::AssaultRifle,OriginalAssaultRifle::make_runtime_config(),true);
        reloading.runtime().ammo().clip=3;
        reloading.runtime().ammo().reserve=10;
        assert(reloading.on_reload_begin(5,6));
        assert(!reloading.can_launch_rifle_grenade());
        ar_grenade::Runtime output{};
        assert(launcher.fire_player(reloading,host,sample(active),output).code==
               LaunchCode::WeaponNotSelectedOrReady);
        reloading.on_reload_end();
        assert(reloading.can_launch_rifle_grenade());
        assert(reloading.on_holster());
        assert(!reloading.can_launch_rifle_grenade());
        assert(launcher.fire_player(reloading,host,sample(active),output).code==
               LaunchCode::WeaponNotSelectedOrReady);
        assert(host.prepared==0);
    }
    {
        // Original CNPC_GruntBase owner path selects the 1000 multiplier;
        // it bypasses the identified player-side RemoveAmmo(1) call, but
        // still requires explicit world/AI approval. No invented infinite ammo.
        WorldHost npc{};
        npc.kind=OwnerKind::GruntBase;
        ar_grenade::Runtime output{};
        assert(launcher.fire_grunt(npc,input,output).ok());
        assert(output.armed() && output.velocity().x==1000 &&
               output.velocity().y==500 && output.velocity().z==-250);
        assert(npc.ammo_checks==0 && npc.ammo_spent==0 && npc.last_debit==0);
        assert(npc.prepared==1 && npc.committed==1);
        ar_grenade::Runtime blocked{};
        npc.allow_grunt=false;
        assert(launcher.fire_grunt(npc,input,blocked).code==
               LaunchCode::OwnerNotAuthorized);
        assert(npc.prepared==1 && !blocked.spawned());
        npc.allow_grunt=true;
        npc.kind=OwnerKind::Player;
        assert(launcher.fire_grunt(npc,input,blocked).code==
               LaunchCode::WrongWeaponOrOwner);
        assert(npc.prepared==1);
    }
}
