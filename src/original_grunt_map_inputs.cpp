#include "sin_compat/original_grunt_map_inputs.hpp"

namespace sin_compat::ai::grunt {

OriginalGruntMapInputResult dispatch_original_grunt_map_input(
    OriginalGruntEntityToken entity,
    OriginalGruntMapInput input,
    const OriginalGruntMapInputValue& value,
    IOriginalGruntMapInputHost& host) noexcept {
    if (!host.is_live(entity)) {
        return OriginalGruntMapInputResult::MissingEntity;
    }

    switch (input) {
    case OriginalGruntMapInput::StartPatrolling:
        host.set_should_patrol(entity, true);
        return OriginalGruntMapInputResult::Applied;

    case OriginalGruntMapInput::StopPatrolling:
        host.set_should_patrol(entity, false);
        return OriginalGruntMapInputResult::Applied;

    case OriginalGruntMapInput::LookOff:
        host.update_look_flag(entity, original_grunt_look_flag_mask, true);
        return OriginalGruntMapInputResult::Applied;

    case OriginalGruntMapInput::LookOn:
        host.update_look_flag(entity, original_grunt_look_flag_mask, false);
        return OriginalGruntMapInputResult::Applied;

    case OriginalGruntMapInput::Assault:
        if (!value.string_value.has_value()) {
            return OriginalGruntMapInputResult::MissingInputValue;
        }
        host.invoke_assault_helper(
            entity, *value.string_value, original_grunt_assault_helper_argument);
        return OriginalGruntMapInputResult::Applied;

    case OriginalGruntMapInput::ThrowGrenades:
        if (!value.boolean_value.has_value()) {
            return OriginalGruntMapInputResult::MissingInputValue;
        }
        // Preserve the observed FIELD_BOOLEAN write order.
        host.set_grenade_height(entity, original_grunt_throw_grenade_height_reset);
        host.set_grenade_happy(entity, *value.boolean_value);
        return OriginalGruntMapInputResult::Applied;
    }

    return OriginalGruntMapInputResult::InvalidInput;
}

} // namespace sin_compat::ai::grunt
