#include "sin_compat/weapon_viewmodel_lifecycle.hpp"

namespace sin_compat {

namespace {
constexpr int kMaxDefensiveSlots = 32;
}

EnsureViewModelResult ensure_viewmodel_slot(
    IWeaponViewModelLifecycleHost& host, EntityRef player, int slot) {
    if (!host.is_live(player) || !host.is_player(player)) {
        return EnsureViewModelResult::InvalidPlayer;
    }
    const int count = host.viewmodel_slot_count(player);
    if (count <= 0 || count > kMaxDefensiveSlots || slot < 0 || slot >= count) {
        return EnsureViewModelResult::InvalidSlot;
    }
    if (host.is_live(host.viewmodel_at(player, slot))) {
        return EnsureViewModelResult::AlreadyPresent;
    }

    const EntityRef created = host.create_named_entity("viewmodel");
    if (!host.is_live(created)) {
        return EnsureViewModelResult::CreationFailed;
    }
    if (!host.configure_viewmodel(created, player, slot)) {
        host.remove_viewmodel(created);
        return EnsureViewModelResult::ConfigurationFailed;
    }
    if (!host.store_viewmodel_handle(player, slot, created)) {
        host.remove_viewmodel(created);
        return EnsureViewModelResult::HandleCommitFailed;
    }
    return EnsureViewModelResult::Created;
}

void clear_viewmodel_slots(IWeaponViewModelLifecycleHost& host, EntityRef player) {
    if (!host.is_live(player) || !host.is_player(player)) return;
    const int count = host.viewmodel_slot_count(player);
    if (count < 0 || count > kMaxDefensiveSlots) return;
    for (int slot = count; slot-- > 0;) {
        const EntityRef model = host.viewmodel_at(player, slot);
        if (host.is_live(model)) {
            host.remove_viewmodel(model);
        }
        // Explicitly clear stale handles too. This is a defensive native-port
        // policy, not a claim about every original SiN cleanup path.
        host.clear_viewmodel_handle(player, slot);
    }
}

}  // namespace sin_compat
