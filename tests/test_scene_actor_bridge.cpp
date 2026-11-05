#include "sin_compat/scene_actor_bridge.hpp"
#include <cassert>
#include <string>

using namespace sin_compat;

struct FakeHost final : ISceneActorHost {
    bool fail_create=false, fail_model=false, can_choreo=true, fail_scene=false;
    int creates=0, removed=0, throws=0, holsters=0, hp=-1;
    std::string base, original, model, name, scene, scene_actor, item;
    int primary=-99, secondary=-99;
    SceneActorRef create_from_existing_base(const char *b,const char *o) override {
        ++creates; base=b; original=o;
        return fail_create ? SceneActorRef{} : SceneActorRef{7, 2};
    }
    bool set_model(SceneActorRef,const char *m) override {model=m;return !fail_model;}
    bool set_health(SceneActorRef,int h) override {hp=h;return true;}
    bool set_targetname(SceneActorRef,const char *n) override {name=n;return true;}
    bool choreography_capable(SceneActorRef) override {return can_choreo;}
    bool play_scene(SceneActorRef,const char *p,const char *n) override {
        scene=p; scene_actor=n; return !fail_scene;
    }
    bool holster_player_weapon() override {++holsters;return true;}
    bool throw_item_at_player(SceneActorRef,const char *c,int p,int s) override {
        ++throws; item=c;primary=p;secondary=s;return true;
    }
    void remove_actor(SceneActorRef) override {++removed;}
};

int main() {
    {
        FakeHost h; SceneActorBridge jc(h,SceneActorKind::JC);
        assert(jc.begin_scene("scenes/SE1_Intro/jc_calls.vcd","JC") == SceneActorResult::InvalidState);
        assert(jc.spawn({"JC_Hospital", ""}) == SceneActorResult::Ok);
        assert(h.base=="npc_citizen" && h.original=="npc_jc" && h.hp==8);
        assert(h.model=="models/characters/JC/jc_torso.mdl" && h.name=="JC_Hospital");
        assert(jc.begin_scene("scenes/se1_finale/teaser_jc_hospital.vcd","JC_Hospital") == SceneActorResult::Ok);
        assert(h.scene_actor=="JC_Hospital");
        assert(jc.spawn({}) == SceneActorResult::InvalidState);
        SceneActorInput empty{};
        assert(jc.input("ThrowItemAtPlayer",empty)==SceneActorResult::UnsupportedInput);
    }
    {
        FakeHost h; SceneActorBridge j(h,SceneActorKind::Jessica);
        assert(j.spawn({"jess01", ""})==SceneActorResult::Ok);
        assert(h.base=="npc_alyx" && h.hp==80);
        assert(h.model=="models/characters/Jessica/Jessica.mdl");
        SceneActorInput i;
        i.type=SceneActorInput::Type::String;i.string_value="weapon_assault_rifle";
        assert(j.input("SetPrimaryAmmoCountToThrow",i)==SceneActorResult::WrongInputType);
        assert(j.input("ThrowItemAtPlayer",i)==SceneActorResult::Ok);
        assert(j.pending_throw() && h.throws==0);
        i.type=SceneActorInput::Type::Integer;i.integer_value=45;
        assert(j.input("SetPrimaryAmmoCountToThrow",i)==SceneActorResult::Ok);
        i.integer_value=6;
        assert(j.input("SetSecondaryAmmoCountToThrow",i)==SceneActorResult::Ok);
        assert(j.animation_event("some_other_event")==SceneActorResult::UnsupportedInput);
        assert(j.animation_event("AE_JESSICA_THROW_ITEM")==SceneActorResult::Ok);
        assert(h.throws==1 && h.item=="weapon_assault_rifle" && h.primary==45 && h.secondary==6);
        assert(j.animation_event("AE_JESSICA_THROW_ITEM")==SceneActorResult::NoPendingThrow);
        assert(!j.pending_throw());
        i.type=SceneActorInput::Type::Void;
        assert(j.input("HolsterWeapon",i)==SceneActorResult::Ok && h.holsters==1);
        assert(j.begin_scene("scenes/se1_highrise/jess_wakeupblade.vcd","!Target1")==SceneActorResult::Ok);
        assert(h.scene_actor=="!Target1");
    }
    {
        FakeHost h; SceneActorBridge g(h,SceneActorKind::Generic);
        assert(g.spawn({})==SceneActorResult::InvalidArgument);
        assert(h.creates==0);
        assert(g.spawn({"actor_1", "models/npc/citizen.mdl"})==SceneActorResult::Ok);
        assert(h.base=="npc_citizen" && h.hp==-1 && h.model=="models/npc/citizen.mdl");
    }
    {
        FakeHost h;h.can_choreo=false;SceneActorBridge jc(h,SceneActorKind::JC);
        assert(jc.spawn({})==SceneActorResult::HostFailed);
        assert(h.creates==1 && h.removed==1 && !jc.actor().valid());
    }
    {
        FakeHost h;h.fail_model=true;SceneActorBridge jc(h,SceneActorKind::JC);
        assert(jc.spawn({})==SceneActorResult::HostFailed);
        assert(h.removed==1);
    }
    {
        FakeHost h;h.fail_create=true;SceneActorBridge jc(h,SceneActorKind::JC);
        assert(jc.spawn({})==SceneActorResult::HostFailed);
        assert(h.removed==0);
    }
    {
        FakeHost h;h.fail_scene=true;SceneActorBridge jc(h,SceneActorKind::JC);
        assert(jc.spawn({})==SceneActorResult::Ok);
        assert(jc.begin_scene("scenes/x.vcd","JC")==SceneActorResult::HostFailed);
        assert(jc.begin_scene("","JC")==SceneActorResult::InvalidArgument);
    }
}