#pragma once

#include "sin_compat/weapon_entity_io.hpp"

namespace sin_compat {

// The original SiN server registers "viewmodel" and maintains player-owned
// m_hViewModel[2]. This interface is portable: no x86 offsets, EHANDLE bit
// encoding, Source engine class layout, or object lifetime ABI is assumed.
class IWeaponViewModelLifecycleHost {
public:
    virtual ~IWeaponViewModelLifecycleHost() = default;
    virtual bool is_live(EntityRef entity) const = 0;
    virtual bool is_player(EntityRef entity) const = 0;
    virtual int viewmodel_slot_count(EntityRef player) const = 0;
    virtual EntityRef viewmodel_at(EntityRef player, int slot) const = 0;

    // Host creates an entity of the *registered* class name; only the host may
    // decide how and when the engine actually performs spawn/activation.
    virtual EntityRef create_named_entity(const char* classname) = 0;
    virtual bool configure_viewmodel(EntityRef model, EntityRef player, int slot) = 0;
    virtual bool store_viewmodel_handle(EntityRef player, int slot, EntityRef model) = 0;
    virtual void remove_viewmodel(EntityRef model) = 0;
    virtual void clear_viewmodel_handle(EntityRef player, int slot) = 0;
};

enum class EnsureViewModelResult {
    InvalidPlayer,
    InvalidSlot,
    AlreadyPresent,
    CreationFailed,
    ConfigurationFailed,
    HandleCommitFailed,
    Created,
};

// The original helper lazily creates a named viewmodel when a slot's EHANDLE
// cannot resolve a live entity. The port avoids unsafe array indexing and
// rolls back failed host operations.
EnsureViewModelResult ensure_viewmodel_slot(
    IWeaponViewModelLifecycleHost& host, EntityRef player, int slot);

// Original SiN cleanup visits both slots in descending order, destroys live
// referenced entities and invalidates their handles. Host stores and bounds
// are authoritative; no raw 2006 pointers are dereferenced here.
void clear_viewmodel_slots(IWeaponViewModelLifecycleHost& host, EntityRef player);

}  // namespace sin_compat
