#pragma once
// Phase529: production-facing host boundary between generic Source BSP
// entity-lump instantiation and actual SiN-only server trigger behavior.
// This IS NOT a Source engine entity allocator or a playable server.dll.
// Native CBaseTrigger allocations, datadesc, save/network and source
// vtables must be supplied by an authenticated Source-compatible host.
#include "sin_compat/original_sin_trigger_entities.hpp"
#include "sin_compat/source_map_runtime.hpp"

#include <cstdint>

namespace sin_compat::map_entities {

struct NativeTriggerSnapshot {
    TriggerVec3 world_origin{};
    std::uint32_t base_flags_0xB8=0;
    std::uint32_t native_flags_0xB4=0;
    std::uint32_t native_bits_0x120=0;
    bool engine_requests_removal=false;
};
class ITriggerNativeState {
public:
    virtual ~ITriggerNativeState()=default;
    // Must acquire these fields from a LIVE real engine-owned CBaseTrigger,
    // not from guessed Source keyvalues or the behavior adapter itself.
    virtual bool read_native_trigger_snapshot(
        EntityRef ref,NativeTriggerSnapshot& snapshot) const=0;
};

// Decoration of an already functional engine-owned ISourceMapEntityHost.
// Ordinary entities continue to use that real host unmodified. SiN triggers
// are accepted ONLY if the host really constructs the native classname,
// i.e. no stand-in prop or dummy alias. This adapter attaches the portable
// gameplay behavior AFTER native allocation, then defers to the native
// host for all original map KeyValues, activation and lifetime operations.
//
// The host's native CBaseTrigger::Spawn must be invoked EXACTLY ONCE by the
// IOriginalTriggerHost::initialize_native_base_trigger() callback. We
// therefore do not call native_map.dispatch_spawn again for these triggers.
// Access single-threadedly on the engine game thread, like Source callbacks.
class OriginalTriggerMapBridge final : public map::ISourceMapEntityHost {
public:
    OriginalTriggerMapBridge(map::ISourceMapEntityHost& native_map,
                             ITriggerNativeState& state,
                             OriginalTriggerEntityFactory& behavior) noexcept;
    OriginalTriggerMapBridge(const OriginalTriggerMapBridge&)=delete;
    OriginalTriggerMapBridge& operator=(const OriginalTriggerMapBridge&)=delete;

    bool is_live(EntityRef e) const override;
    bool create_entity(std::string_view classname,EntityRef& out) override;
    bool key_value(EntityRef e,std::string_view key,
                   std::string_view value) override;
    bool dispatch_spawn(EntityRef e) override;
    bool activate(EntityRef e) override;
    bool remove_entity(EntityRef e) override;

    TriggerTouchReport player_or_npc_touch(EntityRef trigger,
                                          EntityRef actor) noexcept;

private:
    bool read_and_refresh(EntityRef e,
                          NativeTriggerSnapshot& out) noexcept;
    map::ISourceMapEntityHost& native_map_;
    ITriggerNativeState& state_;
    OriginalTriggerEntityFactory& behavior_;
};
} // namespace sin_compat::map_entities
