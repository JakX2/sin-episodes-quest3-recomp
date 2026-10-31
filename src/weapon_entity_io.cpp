#include "sin_compat/weapon_entity_io.hpp"

namespace sin_compat {

HideWeaponInputResult process_hide_weapon_input(IWeaponEntityIOHost& host,
                                                 EntityRef weapon) {
    if (!host.is_live(weapon)) {
        return HideWeaponInputResult::IgnoredInvalidWeapon;
    }
    const EntityRef owner = host.owner_of(weapon);
    if (!host.is_live(owner)) {
        return HideWeaponInputResult::IgnoredInvalidOwner;
    }
    const EntityRef selected = host.selected_weapon_of(owner);
    if (!host.is_live(selected) || selected != weapon) {
        return HideWeaponInputResult::IgnoredNotSelected;
    }
    host.set_weapon_nodraw(weapon, true);
    return HideWeaponInputResult::Applied;
}

} // namespace sin_compat
