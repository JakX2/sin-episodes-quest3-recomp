#pragma once
#include "sin_compat/npc_spawn_placement.hpp"
#include "sin_compat/npc_maker_input_bridge.hpp"
#include <string>
namespace sin_compat {
// SDK npc_template_maker-specific commands plus Phase86 BaseNPCMaker inputs.
// This is host-neutral: engine collision, visibility and cloning are callbacks.
NpcMakerInputDispatch dispatch_npc_maker_placement_input(
    NpcMakerRuntime& maker_runtime, NpcMakerPlacementHostAdapter& placements,
    EntityRef maker, const std::string& input, const std::string& parameter);
} // namespace sin_compat
