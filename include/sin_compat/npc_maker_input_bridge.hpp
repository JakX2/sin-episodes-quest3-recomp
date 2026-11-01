#pragma once
// Exact SDK BaseNPCMaker input names mapped to an independent portable model.
// Not Source variant_t, IEntityFactory, or original NPC maker input code.
#include "sin_compat/npc_maker_runtime.hpp"
#include <string>
namespace sin_compat {
struct NpcMakerInputDispatch {
    bool recognized = false;
    NpcMakerResult result = NpcMakerResult::InvalidSetting;
    std::vector<NpcMakerEvent> events;
};
// Numeric text is parsed in a locale-independent decimal format. Incorrect
// types, trailing characters, overflows or nonfinite values are rejected.
NpcMakerInputDispatch dispatch_npc_maker_input(
    NpcMakerRuntime& runtime, EntityRef maker,
    const std::string& input, const std::string& parameter);
} // namespace sin_compat
