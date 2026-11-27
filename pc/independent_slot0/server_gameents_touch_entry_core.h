#pragma once
// Original-guided ServerGameEnts001::MarkEntitiesAsTouching entry wrapper.
// This models the 0x221e9380 control flow, NOT a Source edict/entity overlay.
// Engine owns edict->networkable->entity, trace initialization, origin refresh,
// and the 0x22221080 contact service. No retail factory is exported.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
struct GameEntsTouchTrace {
    std::array<unsigned char,0x54> data{};
    void WriteFloat(std::size_t offset,float value) noexcept {
        std::memcpy(data.data()+offset,&value,sizeof(value));
    }
    float ReadFloat(std::size_t offset) const noexcept {
        float value=0;
        std::memcpy(&value,data.data()+offset,sizeof(value));
        return value;
    }
};
static_assert(sizeof(GameEntsTouchTrace)==0x54,
              "Original trace initializer 22285a10 writes 21 dwords");

struct GameEntsTouchEntity {
    void* opaque=nullptr;
    bool dirty_origin=false;
    float origin[3]{};
};
struct GameEntsTouchEntryHooks {
    void* context=nullptr;
    // Host implements edict+0xc -> interface virtual+0x14 resolution.
    bool (*resolve)(void*,const void*,GameEntsTouchEntity*) noexcept=nullptr;
    // Host implements constructor 22285a10 (including global defaults).
    bool (*init_trace)(void*,GameEntsTouchTrace*) noexcept=nullptr;
    // Host implements dirty-origin method 2213fa60 for live entities.
    bool (*refresh_origin)(void*,GameEntsTouchEntity*) noexcept=nullptr;
    // Host owns 22221080, which globally publishes the trace and
    // dispatches 2221f350 twice, first -> second and second -> first.
    bool (*mark_contacts)(void*,void*,void*,const GameEntsTouchTrace*) noexcept=nullptr;
    // Original 221e93d3 checks second +b8 AFTER trace initialization,
    // then 221e93eb reads first +b8 AFTER any second-origin refresh.
    // The owner can reread flags and origin from live entity state. This
    // remains optional for legacy synthetic fixtures, but a real engine
    // adapter should provide it (especially across aliasing/callbacks).
    bool (*sync_live_origin)(void*,GameEntsTouchEntity*) noexcept=nullptr;
};
enum class GameEntsTouchEntryStatus {
    Completed, NoEntities, InvalidInput, HostFailure, InvalidOrigin
};
struct GameEntsTouchEntryResult {
    unsigned resolved=0,trace_initialized=0,second_refreshes=0,
             first_refreshes=0,mark_calls=0;
    float midpoint[3]{};
    GameEntsTouchTrace trace{};
};

inline GameEntsTouchEntryStatus MarkEntitiesAsTouchingEntry(
    const void* first_edict,const void* second_edict,
    const GameEntsTouchEntryHooks& hooks,
    GameEntsTouchEntryResult& result) noexcept {
    result={};
    if(!hooks.resolve||!hooks.init_trace||!hooks.refresh_origin||
       !hooks.mark_contacts) return GameEntsTouchEntryStatus::InvalidInput;
    GameEntsTouchEntity first{},second{};
    // The retail wrapper calls +0x14 on both edict networkable objects.
    // Null edict or null networkable produces an absent entity.
    if(first_edict) {
        if(!hooks.resolve(hooks.context,first_edict,&first))
            return GameEntsTouchEntryStatus::HostFailure;
        result.resolved+=first.opaque!=nullptr;
    }
    if(second_edict) {
        if(!hooks.resolve(hooks.context,second_edict,&second))
            return GameEntsTouchEntryStatus::HostFailure;
        result.resolved+=second.opaque!=nullptr;
    }
    if(!first.opaque||!second.opaque)
        return GameEntsTouchEntryStatus::NoEntities;
    if(!hooks.init_trace(hooks.context,&result.trace))
        return GameEntsTouchEntryStatus::HostFailure;
    result.trace_initialized=1;
    // Retail reads flags live at 221e93d3, after constructing the trace.
    if(hooks.sync_live_origin &&
       !hooks.sync_live_origin(hooks.context,&second))
        return GameEntsTouchEntryStatus::HostFailure;
    if(second.dirty_origin) {
        ++result.second_refreshes;
        if(!hooks.refresh_origin(hooks.context,&second))
            return GameEntsTouchEntryStatus::HostFailure;
    }
    // The retail second-origin refresh can clear the first actor's dirty
    // bit when both edicts name the SAME entity. Reuse its new snapshot,
    // not the stale snapshot taken during initial edict resolution.
    if(first.opaque==second.opaque) first=second;
    // Original 221e93eb rereads FIRST's dirty flags only now, rather
    // than using the earlier edict-resolution snapshot. The callback
    // also handles a second refresh affecting an unrelated first actor.
    if(hooks.sync_live_origin &&
       !hooks.sync_live_origin(hooks.context,&first))
        return GameEntsTouchEntryStatus::HostFailure;
    if(first.dirty_origin) {
        ++result.first_refreshes;
        if(!hooks.refresh_origin(hooks.context,&first))
            return GameEntsTouchEntryStatus::HostFailure;
    }
    // Original 221e9400..223c loads the refreshed actor origins live.
    // Refresh here before computing the midpoint if a host supplies the
    // owner-backed state reader. No third-party entity memory is guessed.
    if(hooks.sync_live_origin &&
       (!hooks.sync_live_origin(hooks.context,&first) ||
        !hooks.sync_live_origin(hooks.context,&second)))
        return GameEntsTouchEntryStatus::HostFailure;
    if(first.opaque==second.opaque) second=first;
    // Original x87 stores each sum to float before multiplying by 0.5.
    // Its midpoint sits at +0x0c/+0x10/+0x14 in the 0x54-byte trace.
    for(std::size_t i=0;i<3;++i) {
        if(!std::isfinite(first.origin[i])||!std::isfinite(second.origin[i]))
            return GameEntsTouchEntryStatus::InvalidOrigin;
        volatile float rounded_sum=first.origin[i]+second.origin[i];
        const float midpoint=rounded_sum*0.5f;
        if(!std::isfinite(midpoint))
            return GameEntsTouchEntryStatus::InvalidOrigin;
        result.midpoint[i]=midpoint;
        result.trace.WriteFloat(0x0c+i*4,midpoint);
    }
    ++result.mark_calls;
    if(!hooks.mark_contacts(hooks.context,first.opaque,second.opaque,&result.trace))
        return GameEntsTouchEntryStatus::HostFailure;
    return GameEntsTouchEntryStatus::Completed;
}
} // namespace sin_independent
