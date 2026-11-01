#include "sin_compat/grunt_tactical_controls.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
using namespace sin_compat;
struct Host final : IGruntTacticalHost {
    std::map<std::uint32_t,std::uint32_t> generations;
    EntityRef add(std::uint32_t index,std::uint32_t generation=1) {
        generations[index]=generation;return {index,generation};
    }
    bool is_live(EntityRef ref) const override {
        auto it=generations.find(ref.index);
        return it!=generations.end() && it->second==ref.generation;
    }
};
int main() {
    Host host;
    GruntTacticalControls grunts(host);
    const auto sintek=host.add(1);
    assert(grunts.register_grunt(sintek,{"npc_sintek_grunt","",0,5,0}));
    auto snap=grunts.snapshot(sintek,GruntAlertState::Idle);
    assert(snap.valid && snap.sight==GruntSightPreference::OwnEyes);
    assert(snap.equipment=="weapon_assault_rifle" && snap.grenades_remaining==5);
    assert(snap.secondary_fire_allowed && !snap.patrol_requested);
    assert(grunts.apply_input(sintek,"LookOff")==GruntCommandResult::Applied);
    assert(grunts.apply_input(sintek,"LookOff")==GruntCommandResult::NoChange);
    assert(grunts.snapshot(sintek,GruntAlertState::Combat).sight==GruntSightPreference::SquadEyes);
    assert(grunts.apply_input(sintek,"StartPatrolling")==GruntCommandResult::Applied);
    assert(grunts.snapshot(sintek,GruntAlertState::Idle).patrol_requested);
    assert(grunts.snapshot(sintek,GruntAlertState::Alert).patrol_requested);
    assert(!grunts.snapshot(sintek,GruntAlertState::Combat).patrol_requested);
    assert(grunts.apply_input(sintek,"StopPatrolling")==GruntCommandResult::Applied);
    assert(!grunts.snapshot(sintek,GruntAlertState::Idle).patrol_requested);
    assert(grunts.apply_input(sintek,"LookOn")==GruntCommandResult::Applied);
    assert(grunts.snapshot(sintek,GruntAlertState::Idle).sight==GruntSightPreference::OwnEyes);
    assert(grunts.apply_input(sintek,"ThrowGrenades","true")==GruntCommandResult::Applied);
    assert(grunts.snapshot(sintek,GruntAlertState::Idle).aggressive_grenades_requested);
    assert(grunts.apply_input(sintek,"ThrowGrenades","1")==GruntCommandResult::NoChange);
    assert(grunts.apply_input(sintek,"ThrowGrenades","0")==GruntCommandResult::Applied);
    assert(!grunts.snapshot(sintek,GruntAlertState::Idle).aggressive_grenades_requested);
    for(const auto& invalid : {"", "yes", "2", "True-ish", "1 ", "-1"}) {
        assert(grunts.apply_input(sintek,"ThrowGrenades",invalid)==GruntCommandResult::InvalidParameter);
    }
    assert(grunts.apply_input(sintek,"LookOff","oops")==GruntCommandResult::InvalidParameter);
    assert(grunts.apply_input(sintek,"Unknown")==GruntCommandResult::UnknownInput);
    for(int i=0;i<5;++i)assert(grunts.report_grenade_thrown(sintek)==GruntCommandResult::Applied);
    assert(grunts.report_grenade_thrown(sintek)==GruntCommandResult::NoGrenades);
    assert(!grunts.snapshot(sintek,GruntAlertState::Combat).grenades_available);
    const auto limited=host.add(2,4);
    assert(grunts.register_grunt(limited,{"npc_druglab_grunt_elite","",
            kGruntStartLookOff|kGruntDontSecondaryFire,kGruntUnlimitedGrenades,24}));
    snap=grunts.snapshot(limited,GruntAlertState::Idle);
    assert(snap.valid && snap.equipment=="weapon_chaingun");
    assert(snap.sight==GruntSightPreference::SquadEyes);
    assert(!snap.secondary_fire_allowed && snap.unlimited_grenades && snap.grenades_available);
    assert(snap.grenade_height==24);
    for(int i=0;i<100;++i)assert(grunts.report_grenade_thrown(limited)==GruntCommandResult::Applied);
    assert(grunts.snapshot(limited,GruntAlertState::Combat).grenades_remaining==kGruntUnlimitedGrenades);
    host.generations[2]=5;
    assert(!grunts.snapshot(limited,GruntAlertState::Idle).valid);
    assert(grunts.apply_input(limited,"LookOn")==GruntCommandResult::InvalidGrunt);
    assert(grunts.report_grenade_thrown(limited)==GruntCommandResult::InvalidGrunt);
    const EntityRef replacement{2,5};
    assert(grunts.register_grunt(replacement,{"npc_druglab_grunt_pistol","",0,0,0}));
    assert(grunts.snapshot(replacement,GruntAlertState::Idle).equipment=="weapon_npc_pistol");
    assert(grunts.report_grenade_thrown(replacement)==GruntCommandResult::NoGrenades);
    grunts.remove_grunt(limited);
    assert(grunts.snapshot(replacement,GruntAlertState::Idle).valid);
    grunts.remove_grunt(replacement);
    assert(!grunts.snapshot(replacement,GruntAlertState::Idle).valid);

    auto third=host.add(3);
    assert(!grunts.register_grunt(third,{"npc_combine_soldier","",0,5,0}));
    assert(!grunts.register_grunt(third,{"npc_sintek_grunt","weapon_chaingun",0,5,0}));
    assert(!grunts.register_grunt(third,{"npc_sintek_grunt","",0,11,0}));
    assert(!grunts.register_grunt(third,{"npc_sintek_grunt","",0,-1,0}));
    assert(!grunts.register_grunt(third,{"npc_sintek_grunt","",0,5,NAN}));
    assert(!grunts.register_grunt(third,{"npc_sintek_grunt","",0,5,-1}));
    assert(!grunts.register_grunt(EntityRef{},{"npc_sintek_grunt","",0,5,0}));
    assert(grunts.register_grunt(third,{"npc_druglab_grunt_scattergun","",0,3,0}));
    assert(!grunts.register_grunt(third,{"npc_sintek_grunt","",0,5,0}));
    assert(grunts.snapshot(third,GruntAlertState::Idle).equipment=="weapon_scattergun");
    assert(grunt_default_equipment("npc_druglab_grunt_assault_rifle")=="weapon_assault_rifle");
    assert(grunt_equipment_allowed("npc_sintek_grunt","weapon_npc_pistol"));
    assert(!grunt_equipment_allowed("npc_druglab_grunt_elite","weapon_npc_pistol"));
    assert(grunts.grunt_count()==2); // initial SinTEK grunt and third grunt remain after cleanup
}
