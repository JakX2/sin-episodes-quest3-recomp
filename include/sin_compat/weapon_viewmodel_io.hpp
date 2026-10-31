#pragma once

#include "sin_compat/weapon_entity_io.hpp"

namespace sin_compat {

// Host-side opt-in viewmodel extension. This abstraction does not encode an
// original x86 entity ABI or assert which SourceVR fork the project will use.
class IWeaponViewModelHost : public IWeaponEntityIOHost {
public:
    ~IWeaponViewModelHost() override = default;

    // The original related-entity lookup is conditional on an owner type check.
    virtual bool is_player_owner(EntityRef owner) const = 0;

    // The original 2006 SiN build registers two m_hViewModel handle slots,
    // confirmed independently in server/client metadata (Phase 64). Use the
    // actual selected host engine's bounds, not hard-coded PE32 offsets or an
    // assumption that other SourceVR builds have identical viewmodel counts.
    // Host must obtain this index from the live weapon and reject invalid or
    // out-of-range slots *before* indexing an engine-side viewmodel array.
    virtual int viewmodel_index_of(EntityRef weapon) const = 0;
    virtual bool is_valid_viewmodel_index(EntityRef owner, int index) const = 0;

    // Must resolve the current index+generation of the player's viewmodel;
    // invalid handles and unrelated entities must never be returned as live.
    virtual EntityRef viewmodel_of(EntityRef owner, int index) const = 0;
    virtual bool is_viewmodel_owned_by(EntityRef model, EntityRef owner) const = 0;
    virtual void set_viewmodel_nodraw(EntityRef model, bool hidden) = 0;
};

// First preserves the Phase 62 HideWeapon active-selection guard and weapon
// effect. When a valid player viewmodel is available, hides that entity too.
// Absence of a viewmodel must not prevent the world weapon's effect update.
HideWeaponInputResult process_hide_weapon_input_with_viewmodel(
    IWeaponViewModelHost& host, EntityRef weapon);

}  // namespace sin_compat
