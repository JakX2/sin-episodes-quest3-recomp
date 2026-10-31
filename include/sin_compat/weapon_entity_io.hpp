#pragma once

// Independently authored, engine-neutral host adapter for the guarded SiN
// HideWeapon entity input. Does not replicate Source engine classes/ABI.

#include <cstdint>

namespace sin_compat {

struct EntityRef {
    std::uint32_t index = UINT32_MAX;
    std::uint32_t generation = UINT32_MAX;
};

inline bool operator==(EntityRef a, EntityRef b) {
    return a.index == b.index && a.generation == b.generation;
}
inline bool operator!=(EntityRef a, EntityRef b) { return !(a == b); }

class IWeaponEntityIOHost {
public:
    virtual ~IWeaponEntityIOHost() = default;
    // Must verify index and generation; a reused slot is not the same entity.
    virtual bool is_live(EntityRef entity) const = 0;
    virtual EntityRef owner_of(EntityRef weapon) const = 0;
    virtual EntityRef selected_weapon_of(EntityRef owner) const = 0;
    // Source-specific implementation may use EF_NODRAW (0x20) via native APIs.
    // Related-entity effects and other runtime state remain host responsibilities.
    virtual void set_weapon_nodraw(EntityRef weapon, bool hidden) = 0;
};

enum class HideWeaponInputResult {
    IgnoredInvalidWeapon,
    IgnoredInvalidOwner,
    IgnoredNotSelected,
    Applied
};

// Guards reflect the observed 2006 handler; the native host owns handle
// representation and effect application. Not a drop-in DLL replacement.
HideWeaponInputResult process_hide_weapon_input(IWeaponEntityIOHost& host,
                                                 EntityRef weapon);

} // namespace sin_compat
