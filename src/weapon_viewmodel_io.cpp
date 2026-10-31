#include "sin_compat/weapon_viewmodel_io.hpp"

namespace sin_compat {

HideWeaponInputResult process_hide_weapon_input_with_viewmodel(
    IWeaponViewModelHost& host, EntityRef weapon) {
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

    // Do not assume every owner has a viewmodel, or that viewmodel slot 0 is
    // always used by the currently selected weapon.
    EntityRef related{};
    bool have_related = false;
    if (host.is_player_owner(owner)) {
        const int index = host.viewmodel_index_of(weapon);
        if (host.is_valid_viewmodel_index(owner, index)) {
            const EntityRef candidate = host.viewmodel_of(owner, index);
            if (host.is_live(candidate) && host.is_viewmodel_owned_by(candidate, owner)) {
                related = candidate;
                have_related = true;
            }
        }
    }
    // Phase 62: zero argument adds NODRAW to weapon unconditionally after the
    // selection gate. Phase 63: conditionally apply it to the related model.
    host.set_weapon_nodraw(weapon, true);
    if (have_related) {
        host.set_viewmodel_nodraw(related, true);
    }
    return HideWeaponInputResult::Applied;
}

}  // namespace sin_compat
