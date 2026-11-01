#include "sin_compat/npc_maker_placement_input_bridge.hpp"
#include <utility>
namespace sin_compat {
NpcMakerInputDispatch dispatch_npc_maker_placement_input(
    NpcMakerRuntime& makers, NpcMakerPlacementHostAdapter& placements,
    EntityRef maker, const std::string& input, const std::string& parameter) {
    if (input == "ChangeDestinationGroup") {
        // Empty group is allowed: subsequent automatic spawn uses radius.
        return {true, placements.change_destination_group(maker, parameter)
            ? NpcMakerResult::Applied : NpcMakerResult::InvalidMaker, {}};
    }
    if (input == "SpawnNPCInRadius" || input == "SpawnNPCInLine") {
        auto action=placements.spawn_with_mode(makers, maker,
            input == "SpawnNPCInRadius" ? SpawnPlacementMode::Radius : SpawnPlacementMode::Line);
        return {true, action.result, std::move(action.events)};
    }
    return dispatch_npc_maker_input(makers,maker,input,parameter);
}
} // namespace sin_compat
