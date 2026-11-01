#include "sin_compat/grunt_tactical_controls.hpp"
#include "sin_compat/entity_io_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
using namespace sin_compat;
struct Scene final : IEntityOutputHost, IGruntTacticalHost {
    std::map<std::uint32_t,std::uint32_t> generations;
    std::map<std::string,EntityRef> names;
    GruntTacticalControls* controls=nullptr;
    EntityRef trigger{},grunt{},player{};
    int observed=0;
    EntityRef add(std::uint32_t id,const std::string& name) {
        generations[id]=1;EntityRef e{id,1};names[name]=e;return e;
    }
    bool is_live(EntityRef e) const override {
        auto it=generations.find(e.index);
        return it!=generations.end() && it->second==e.generation;
    }
    std::vector<EntityRef> resolve_targets(const std::string& name,
                                           EntityRef activator,EntityRef caller) override {
        if(name=="!activator")return {activator};
        if(name=="!caller")return {caller};
        auto it=names.find(name);
        return it==names.end()?std::vector<EntityRef>{}:std::vector<EntityRef>{it->second};
    }
    void accept_input(EntityRef target,const std::string& input,const std::string& param,
                      EntityRef activator,EntityRef caller) override {
        assert(target==grunt && caller==trigger && activator==player);
        const auto result=controls->apply_input(target,input,param);
        assert(result==GruntCommandResult::Applied);
        ++observed;
    }
};
int main() {
    Scene scene;
    scene.trigger=scene.add(1,"trigger");
    scene.grunt=scene.add(2,"grunt");
    scene.player=scene.add(3,"player");
    GruntTacticalControls controls(scene);
    EntityIoRuntime io(scene);
    scene.controls=&controls;
    assert(controls.register_grunt(scene.grunt,
            {"npc_sintek_grunt","weapon_scattergun",kGruntStartLookOff,2,10}));
    // Invented encounter output chain, using *real* FGD input names.
    assert(io.connect(scene.trigger,"OnStartTouch",{"grunt","LookOn","",0.0,1}));
    assert(io.connect(scene.trigger,"OnStartTouch",{"grunt","StartPatrolling","",0.1,1}));
    assert(io.connect(scene.trigger,"OnStartTouch",{"grunt","ThrowGrenades","1",0.2,1}));
    assert(io.fire(scene.trigger,"OnStartTouch",scene.player)==3);
    assert(io.advance_to(0).inputs_delivered==1);
    assert(controls.snapshot(scene.grunt,GruntAlertState::Idle).sight==
           GruntSightPreference::OwnEyes);
    assert(io.advance_to(0.1).inputs_delivered==1);
    assert(controls.snapshot(scene.grunt,GruntAlertState::Idle).patrol_requested);
    assert(!controls.snapshot(scene.grunt,GruntAlertState::Combat).patrol_requested);
    assert(io.advance_to(0.2).inputs_delivered==1);
    assert(controls.snapshot(scene.grunt,GruntAlertState::Combat).aggressive_grenades_requested);
    assert(controls.report_grenade_thrown(scene.grunt)==GruntCommandResult::Applied);
    assert(controls.snapshot(scene.grunt,GruntAlertState::Idle).grenades_remaining==1);
    assert(io.fire(scene.trigger,"OnStartTouch",scene.player)==0);
    assert(scene.observed==3);
    scene.generations[2]=2;
    assert(!controls.snapshot(scene.grunt,GruntAlertState::Idle).valid);
    controls.remove_grunt(scene.grunt);
    assert(controls.grunt_count()==0);
}
