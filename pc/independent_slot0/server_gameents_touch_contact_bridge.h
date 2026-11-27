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
    // Original 0x22221080 copies the 0x54 trace into global contact state
    // BEFORE either 0x2221f350 directed-contact method. Host owns globals.
    bool (*publish_trace)(void*,const GameEntsTouchTrace*) noexcept=nullptr;
    ContactCleanupState* contacts=nullptr;
    const EntityHandleEntry* entries=nullptr;
    std::size_t entry_count=0;
    TouchMarkHooks touch{};
    // Optional on the independent core; mandatory for bounded dispatch.
    // Attest the TouchEntity storage *before* accessing its fields, then
    // attest the returned CBaseEntity pointer before contact callbacks.
    bool (*authorized_touch)(void*,const TouchEntity*) noexcept=nullptr;
    bool (*authorized_entity)(void*,const CBaseEntity*) noexcept=nullptr;
    void* authorization_context=nullptr;
    // Optional on the standalone touch core; the bounded dispatch requires
    // a trusted edict/entity live-generation association check.
    bool (*authorized_pair)(void*,const void*,const CBaseEntity*) noexcept=nullptr;
    void* pair_authorization_context=nullptr;
};
struct GameEntsContactBridgeResult {
    GameEntsTouchEntryResult entry{};
    TouchTraceState contact_trace{};
    TouchMarkResult contact_status=TouchMarkResult::InvalidInput;
    unsigned resolved_calls=0,refresh_calls=0,contact_calls=0,publish_calls=0,
             live_reads=0;
};

namespace detail {
struct GameEntsContactFrame {
    const GameEntsContactBridgeHooks& hooks;
    GameEntsContactBridgeResult& result;
    TouchEntity* first=nullptr;
    TouchEntity* second=nullptr;
    const void* first_source_edict=nullptr;
    const void* second_source_edict=nullptr;
};
inline bool AuthorizedContact(const GameEntsContactFrame& frame,
                              const TouchEntity* entity) noexcept {
    if(!entity)return false;
    if(frame.hooks.authorized_touch&&
       !frame.hooks.authorized_touch(frame.hooks.authorization_context,entity))
        return false;
    if(!entity->entity)return false;
    if(frame.hooks.authorized_entity &&
       !frame.hooks.authorized_entity(frame.hooks.authorization_context,
                                      entity->entity))return false;
    // Reverify both mappings across origin refresh and trace callbacks.
    // This blocks two owned entities being crossed through a valid touch
    // object, including an allocator address reused under a new serial.
    if(frame.hooks.authorized_pair) {
        if(entity==frame.first&&frame.first_source_edict&&
           !frame.hooks.authorized_pair(
               frame.hooks.pair_authorization_context,
               frame.first_source_edict,entity->entity))return false;
        if(entity==frame.second&&frame.second_source_edict&&
           !frame.hooks.authorized_pair(
               frame.hooks.pair_authorization_context,
               frame.second_source_edict,entity->entity))return false;
    }
    return true;
}
inline bool ResolveContact(void* p,const void* edict,
                           GameEntsTouchEntity* out) noexcept {
    auto& frame=*static_cast<GameEntsContactFrame*>(p);
    TouchEntity* entity=nullptr;
    if(!frame.hooks.resolve(frame.hooks.context,edict,&entity))return false;
    const unsigned call=++frame.result.resolved_calls;
    if(call==1) {
        frame.first=entity;
        frame.first_source_edict=edict;
    } else if(call==2) {
        frame.second=entity;
        frame.second_source_edict=edict;
    } else return false;
    if(!entity)return true;
    if(!AuthorizedContact(frame,entity))return false;
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
    if((entity!=frame.first&&entity!=frame.second)||
       !AuthorizedContact(frame,entity))return false;
    ++frame.result.refresh_calls;
    if(!frame.hooks.touch.refreshOrigin(*entity,frame.hooks.touch.context))
        return false;
    // The owner must actually clear its dirty flag. Otherwise the core
    // would refresh the same entity a second time during pair insertion.
    if(!AuthorizedContact(frame,entity)||entity->dirtyOrigin)return false;
    view->dirty_origin=false;
    view->origin[0]=entity->origin.x;
    view->origin[1]=entity->origin.y;
    view->origin[2]=entity->origin.z;
    return true;
}
// The retail method reads both +b8 flags and +2bc..+2c4 origins
// from live actors after any earlier refresh side effects. Do not retain
// stale edict-resolution snapshots across a host callback.
inline bool ReadLiveContactOrigin(void* p,GameEntsTouchEntity* view) noexcept {
    auto& frame=*static_cast<GameEntsContactFrame*>(p);
    if(!view||!view->opaque)return false;
    auto* entity=static_cast<TouchEntity*>(view->opaque);
    if((entity!=frame.first&&entity!=frame.second)||
       !AuthorizedContact(frame,entity))return false;
    ++frame.result.live_reads;
    view->dirty_origin=entity->dirtyOrigin;
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
       !original_trace||!frame.hooks.contacts||
       !AuthorizedContact(frame,frame.first)||
       !AuthorizedContact(frame,frame.second))return false;
    auto& first=*frame.first;
    auto& second=*frame.second;
    // Independent safety: verify that the pair core will produce the
    // midpoint already stored by the original wrapper BEFORE publication
    // or list mutation. Never report a post-commit mismatch as a failure.
    const float expected[3]{
        (first.origin.x+second.origin.x)*0.5f,
        (first.origin.y+second.origin.y)*0.5f,
        (first.origin.z+second.origin.z)*0.5f
    };
    for(unsigned i=0;i<3;++i)
        if(!std::isfinite(expected[i])||
           expected[i]!=original_trace->ReadFloat(0x0c+i*4))
            return false;
    // Source's 0x22221080 publishes the global contact trace before
    // either directed +0x2221f350 call. This stays engine-owned.
    ++result.publish_calls;
    if(!frame.hooks.publish_trace(frame.hooks.context,original_trace))
        return false;
    const auto status=MarkTouchPair(first,second,*frame.hooks.contacts,
        EntityHandleView(frame.hooks.entries,frame.hooks.entry_count),
        result.contact_trace,frame.hooks.touch);
    result.contact_status=status;
    return status==TouchMarkResult::Recorded||status==TouchMarkResult::Ignored;
}
} // namespace detail

inline GameEntsTouchEntryStatus MarkEntitiesAsTouchingWithContactGraph(
    const void* first_edict,const void* second_edict,
    const GameEntsContactBridgeHooks& hooks,
    GameEntsContactBridgeResult& result) noexcept {
    result={};
    result.contact_status=TouchMarkResult::InvalidInput;
    if(!hooks.resolve||!hooks.init_trace||!hooks.publish_trace||!hooks.contacts||
       !hooks.entries||hooks.entry_count==0||!hooks.touch.Complete())
        return GameEntsTouchEntryStatus::InvalidInput;
    detail::GameEntsContactFrame frame{hooks,result};
    const GameEntsTouchEntryHooks entry{
        &frame,&detail::ResolveContact,&detail::InitContactTrace,
        &detail::RefreshContactOrigin,&detail::MarkContactPair,
        &detail::ReadLiveContactOrigin
    };
    return MarkEntitiesAsTouchingEntry(first_edict,second_edict,entry,result.entry);
}
} // namespace sin_independent
