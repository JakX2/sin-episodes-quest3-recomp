#pragma once
// Composition of the original ServerGameEnts001 +0x08 entry and the
// existing independent reciprocal-contact graph. The original delegates
// its final call to 0x22221080 (then 0x2221f350 twice).
// Entity memory, edict resolution, trace defaults and node allocation
// remain owned by the caller. No retail GameEnts factory is exported.
#include "server_gameents_touch_entry_core.h"
#include "server_touch_mark_core.h"
#include <cstddef>

namespace sin_independent {
struct GameEntsContactBridgeHooks {
    void* context=nullptr;
    // Owner maps a live edict/networkable to owned, stable TouchEntity.
    bool (*resolve)(void*,const void*,TouchEntity**) noexcept=nullptr;
    // Owner initializes the full 0x54-byte trace, including global defaults.
    bool (*init_trace)(void*,GameEntsTouchTrace*) noexcept=nullptr;
    ContactCleanupState* contacts=nullptr;
    const EntityHandleEntry* entries=nullptr;
    std::size_t entry_count=0;
    TouchMarkHooks touch{};
};
struct GameEntsContactBridgeResult {
    GameEntsTouchEntryResult entry{};
    TouchTraceState contact_trace{};
    TouchMarkResult contact_status=TouchMarkResult::InvalidInput;
    unsigned resolved_calls=0,refresh_calls=0,contact_calls=0;
};

namespace detail {
struct GameEntsContactFrame {
    const GameEntsContactBridgeHooks& hooks;
    GameEntsContactBridgeResult& result;
    TouchEntity* first=nullptr;
    TouchEntity* second=nullptr;
};
inline bool ResolveContact(void* p,const void* edict,
                           GameEntsTouchEntity* out) noexcept {
    auto& frame=*static_cast<GameEntsContactFrame*>(p);
    TouchEntity* entity=nullptr;
    if(!frame.hooks.resolve(frame.hooks.context,edict,&entity))return false;
    const unsigned call=++frame.result.resolved_calls;
    if(call==1)frame.first=entity;
    else if(call==2)frame.second=entity;
    else return false;
    if(!entity)return true;
    if(!entity->entity)return false;
    out->opaque=entity;
    out->dirty_origin=entity->dirtyOrigin;
    out->origin[0]=entity->origin.x;
    out->origin[1]=entity->origin.y;
    out->origin[2]=entity->origin.z;
    return true;
}
inline bool InitContactTrace(void* p,GameEntsTouchTrace* trace) noexcept {
    auto& frame=*static_cast<GameEntsContactFrame*>(p);
    return frame.hooks.init_trace(frame.hooks.context,trace);
}
inline bool RefreshContactOrigin(void* p,GameEntsTouchEntity* view) noexcept {
    auto& frame=*static_cast<GameEntsContactFrame*>(p);
    auto* entity=static_cast<TouchEntity*>(view->opaque);
    if(!entity||(entity!=frame.first&&entity!=frame.second))return false;
    ++frame.result.refresh_calls;
    if(!frame.hooks.touch.refreshOrigin(*entity,frame.hooks.touch.context))
        return false;
    // The owner must actually clear its dirty flag. Otherwise the core
    // would refresh the same entity a second time during pair insertion.
    if(entity->dirtyOrigin)return false;
    view->dirty_origin=false;
    view->origin[0]=entity->origin.x;
    view->origin[1]=entity->origin.y;
    view->origin[2]=entity->origin.z;
    return true;
}
inline bool MarkContactPair(void* p,void* a,void* b,
                            const GameEntsTouchTrace* original_trace) noexcept {
    auto& frame=*static_cast<GameEntsContactFrame*>(p);
    auto& result=frame.result;
    ++result.contact_calls;
    if(!a||!b||a!=frame.first||b!=frame.second||
       !original_trace||!frame.hooks.contacts)return false;
    auto& first=*frame.first;
    auto& second=*frame.second;
    // Source's 0x22221080 publishes global trace state before dispatching
    // two directed links. The independent graph owns those links, while
    // the wrapper retains the original initialized trace on the result.
    const auto status=MarkTouchPair(first,second,*frame.hooks.contacts,
        EntityHandleView(frame.hooks.entries,frame.hooks.entry_count),
        result.contact_trace,frame.hooks.touch);
    result.contact_status=status;
    if(status!=TouchMarkResult::Recorded&&status!=TouchMarkResult::Ignored)
        return false;
    return result.contact_trace.end.x==original_trace->ReadFloat(0x0c) &&
           result.contact_trace.end.y==original_trace->ReadFloat(0x10) &&
           result.contact_trace.end.z==original_trace->ReadFloat(0x14) &&
           result.contact_trace.fraction==1.f;
}
} // namespace detail

inline GameEntsTouchEntryStatus MarkEntitiesAsTouchingWithContactGraph(
    const void* first_edict,const void* second_edict,
    const GameEntsContactBridgeHooks& hooks,
    GameEntsContactBridgeResult& result) noexcept {
    result={};
    result.contact_status=TouchMarkResult::InvalidInput;
    if(!hooks.resolve||!hooks.init_trace||!hooks.contacts||
       !hooks.entries||hooks.entry_count==0||!hooks.touch.Complete())
        return GameEntsTouchEntryStatus::InvalidInput;
    detail::GameEntsContactFrame frame{hooks,result};
    const GameEntsTouchEntryHooks entry{
        &frame,&detail::ResolveContact,&detail::InitContactTrace,
        &detail::RefreshContactOrigin,&detail::MarkContactPair
    };
    return MarkEntitiesAsTouchingEntry(first_edict,second_edict,entry,result.entry);
}
} // namespace sin_independent
