#include "sin_compat/grunt_tactical_controls.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
using namespace sin_compat;
struct Host : IGruntTacticalHost {
    std::map<std::uint32_t,std::uint32_t> gen;
    EntityRef add(std::uint32_t index, std::uint32_t generation=1) {
        gen[index]=generation;return {index,generation};
    }
    bool is_live(EntityRef ref) const override {
        const auto it=gen.find(ref.index);
        return it!=gen.end()&&it->second==ref.generation;
    }
};
int main() {
    Host host;
    GruntTacticalControls controls(host);
    const auto grunt=host.add(7);
    constexpr std::uint32_t other_flag=0x40000000;
    assert(controls.register_grunt(grunt,{"npc_sintek_grunt","",other_flag,5,0}));
    auto state=controls.snapshot(grunt,GruntAlertState::Idle);
    assert(state.valid && state.effective_spawnflags==other_flag);
    assert(!state.grenade_auxiliary_cleared);
    assert(controls.apply_input(grunt,"LookOff")==GruntCommandResult::Applied);
    state=controls.snapshot(grunt,GruntAlertState::Idle);
    assert(state.effective_spawnflags==(other_flag|kGruntStartLookOff));
    assert(state.sight==GruntSightPreference::SquadEyes);
    assert(controls.apply_input(grunt,"LookOff")==GruntCommandResult::NoChange);
    assert(controls.apply_input(grunt,"LookOn")==GruntCommandResult::Applied);
    state=controls.snapshot(grunt,GruntAlertState::Idle);
    assert(state.effective_spawnflags==other_flag);
    assert(state.sight==GruntSightPreference::OwnEyes);
    assert(controls.apply_input(grunt,"StartPatrolling")==GruntCommandResult::Applied);
    assert(controls.snapshot(grunt,GruntAlertState::Idle).patrol_requested);
    assert(controls.apply_input(grunt,"StopPatrolling")==GruntCommandResult::Applied);
    assert(!controls.snapshot(grunt,GruntAlertState::Idle).patrol_requested);
    // Actual InputThrowGrenades checks a typed variant of integer type 6.
    assert(controls.apply_typed_grenade_input(grunt,6,true)==GruntCommandResult::Applied);
    state=controls.snapshot(grunt,GruntAlertState::Idle);
    assert(state.aggressive_grenades_requested && state.grenade_auxiliary_cleared);
    assert(controls.apply_typed_grenade_input(grunt,6,true)==GruntCommandResult::NoChange);
    assert(controls.apply_typed_grenade_input(grunt,2,true)==GruntCommandResult::Applied);
    assert(!controls.snapshot(grunt,GruntAlertState::Idle).aggressive_grenades_requested);
    assert(controls.apply_typed_grenade_input(grunt,2,false)==GruntCommandResult::NoChange);
    assert(controls.apply_input(grunt,"ThrowGrenades","true")==GruntCommandResult::Applied);
    assert(controls.apply_input(grunt,"ThrowGrenades","bad")==GruntCommandResult::InvalidParameter);
    assert(controls.snapshot(grunt,GruntAlertState::Idle).aggressive_grenades_requested);
    assert(controls.apply_typed_grenade_input(grunt,-1,true)==GruntCommandResult::Applied);
    assert(controls.apply_typed_grenade_input(grunt,6,false)==GruntCommandResult::NoChange);
    assert(controls.snapshot(grunt,GruntAlertState::Idle).effective_spawnflags==other_flag);
    // No memory is dereferenced after entity identity is recycled.
    host.gen[7]=2;
    assert(controls.apply_typed_grenade_input(grunt,6,true)==GruntCommandResult::InvalidGrunt);
    assert(!controls.snapshot(grunt,GruntAlertState::Idle).valid);
    const EntityRef replacement{7,2};
    assert(controls.register_grunt(replacement,{"npc_sintek_grunt","",
                                              kGruntStartLookOff|other_flag,5,0}));
    assert(controls.snapshot(replacement,GruntAlertState::Idle).effective_spawnflags==
           (kGruntStartLookOff|other_flag));
    assert(controls.apply_input(replacement,"LookOn")==GruntCommandResult::Applied);
    assert(controls.snapshot(replacement,GruntAlertState::Idle).effective_spawnflags==other_flag);
    controls.remove_grunt(grunt);
    assert(controls.grunt_count()==1);
    return 0;
}
