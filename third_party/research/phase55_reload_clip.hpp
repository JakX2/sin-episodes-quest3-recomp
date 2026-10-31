#pragma once
#include <algorithm>
#include <cstdint>

// Clean-room, guarded reload accounting reference for Phase 55.
// This is NOT a decompilation of Ritual's functions and is not yet game-integrated.
namespace sin_phase55 {
struct WeaponInfo {
    int clip_size = -1;
    int clip2_size = -1;
    int default_clip = -1;
    int default_clip2 = -1;
    bool melee_weapon = false;  // WeaponInfo +0x6c8, parsed from "MeleeWeapon"
};
struct Weapon {
    int clip = 0;
    int clip2 = 0;
    int reserve = 0;
    int reserve2 = 0;
    bool reloads_singly = false; // weapon instance +0x508, initialized false in named constructors
    bool reload_active = false;   // weapon instance +0x4ac
    int stage = 0;                // weapon instance +0x4a8
};
struct Transfer { int primary=0; int secondary=0; };

inline int reload_quantity(int capacity, int current, int reserve) {
    if (capacity == -1 || capacity <= 0 || current < 0 || reserve <= 0) return 0;
    return std::min(std::max(0, capacity - current), reserve);
}

// Synthetic equivalent of the two-channel bounded transfer policy visible
// in shared slot 222. Real engine eligibility, dirty flags and owner lookup
// must be applied by the integration layer before this is called.
inline Transfer transfer_bulk(Weapon& w, const WeaponInfo& info, bool owner_valid) {
    if (!owner_valid) return {};
    Transfer t;
    t.primary = reload_quantity(info.clip_size,w.clip,w.reserve);
    t.secondary = reload_quantity(info.clip2_size,w.clip2,w.reserve2);
    w.clip += t.primary;
    w.reserve -= t.primary;
    w.clip2 += t.secondary;
    w.reserve2 -= t.secondary;
    return t;
}

// Confirmed *branch ordering* when the independently enabled +0x508 mode
// reaches its time gate: an attack with a loaded round aborts before insert;
// otherwise a single primary round can transfer, then stages are scheduled.
// Does not model every game-time, owner, ammo-index or animation fallback gate.
enum class ShellEvent { NotReady, Interrupted, Inserted, Exhausted };
inline ShellEvent process_one_round(Weapon& w, const WeaponInfo& info,
                                   bool owner_valid, bool deadline_reached,
                                   std::uint32_t held_buttons) {
    if (!w.reloads_singly || !w.reload_active || !owner_valid || !deadline_reached)
        return ShellEvent::NotReady;
    if ((held_buttons & 0x801u) && w.clip > 0) {
        w.reload_active=false;
        return ShellEvent::Interrupted;
    }
    if (reload_quantity(info.clip_size,w.clip,w.reserve) == 0)
        return ShellEvent::Exhausted;
    ++w.clip;
    --w.reserve;
    return ShellEvent::Inserted;
}

// Based on the end of slot 223, after reload cleanup: the primary clip
// check gates the ordinary reset; "MeleeWeapon" provides another reset path.
inline bool reset_stage_on_cleanup(const WeaponInfo& info, int primary_clip) {
    return (info.clip_size != -1 && primary_clip > 0) || info.melee_weapon;
}
} // namespace sin_phase55
