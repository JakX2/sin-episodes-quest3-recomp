#include "sin_compat/original_sin_trigger_map_bridge.hpp"

namespace sin_compat::map_entities {
namespace {
bool is_trigger_key(std::string_view key) noexcept {
    return key=="targetname" || key=="EstimatedTime" ||
           key=="EstimatedDifficulty" || key=="EndOfGame";
}
}
OriginalTriggerMapBridge::OriginalTriggerMapBridge(
    map::ISourceMapEntityHost& native_map,ITriggerNativeState& state,
    OriginalTriggerEntityFactory& behavior) noexcept
    :native_map_(native_map),state_(state),behavior_(behavior){}

bool OriginalTriggerMapBridge::is_live(EntityRef e) const {
    return native_map_.is_live(e);
}
bool OriginalTriggerMapBridge::create_entity(
    std::string_view classname,EntityRef& out) {
    // First, actual engine class factory must produce a LIVE entity.
    // No fallback to prop_dynamic/ambient_generic or a dummy game class.
    if(!native_map_.create_entity(classname,out))return false;
    if(!native_map_.is_live(out))return false;
    if(!OriginalTriggerEntityFactory::supports(classname))return true;
    // There is only one attaching path. Native create_entity must NOT
    // independently call OriginalTriggerEntityFactory::install callbacks.
    const SpawnRequest request{classname,out};
    return behavior_.spawn(request).ok();
}
bool OriginalTriggerMapBridge::key_value(
    EntityRef e,std::string_view key,std::string_view value) {
    // Native entity must first parse/own its real keyvalue state.
    // Preserve Source's duplicate keys and ordinary inherited options.
    if(!native_map_.key_value(e,key,value))return false;
    TriggerKind kind{};
    if(!behavior_.kind_for(e,kind))return true;
    if(!is_trigger_key(key))return true;
    if(kind==TriggerKind::Secret && key!="targetname")
        return true; // already handled by the real engine DATADESC
    return behavior_.apply_map_keyvalue(e,key,value)==TriggerCode::Handled;
}
bool OriginalTriggerMapBridge::read_and_refresh(
    EntityRef e,NativeTriggerSnapshot& out) noexcept {
    try {
        if(!native_map_.is_live(e) ||
           !state_.read_native_trigger_snapshot(e,out))return false;
        return behavior_.refresh_native_fields(
            e,out.world_origin,out.base_flags_0xB8)==TriggerCode::Handled;
    } catch (...) {return false;}
}
bool OriginalTriggerMapBridge::dispatch_spawn(EntityRef e) {
    TriggerKind kind{};
    if(!behavior_.kind_for(e,kind))
        return native_map_.dispatch_spawn(e);
    NativeTriggerSnapshot s{};
    if(!read_and_refresh(e,s))return false;
    const TriggerSpawnInput args{
        s.engine_requests_removal,s.native_bits_0x120,s.native_flags_0xB4
    };
    const auto result=kind==TriggerKind::Secret?
        behavior_.run_secret_spawn(e,args):
        behavior_.run_dynamic_spawn(e,args);
    // The original suppression branch can queue deletion here. The
    // native engine host owns teardown and SourceMapRuntime treats a
    // removed entity differently from an activated ordinary entity.
    // Refuse a false partial Spawn rather than fabricating success.
    return result.handled();
}
bool OriginalTriggerMapBridge::activate(EntityRef e) {
    return native_map_.activate(e);
}
bool OriginalTriggerMapBridge::remove_entity(EntityRef e) {
    // Remove native engine allocation FIRST, then its portable behavior.
    // If the host refuses deletion, leave its generation-bound behavior
    // intact so SourceMapRuntime's existing rollback can be retried.
    bool live=false;
    try {live=native_map_.is_live(e);} catch (...) {return false;}
    if(live){
        try {(void)native_map_.remove_entity(e);}
        catch (...) {}
    }
    try {
        if(native_map_.is_live(e))return false;
    } catch (...) {return false;}
    // Idempotent release: ordinary Source entities were never attached.
    (void)behavior_.release(e);
    return true;
}
TriggerTouchReport OriginalTriggerMapBridge::player_or_npc_touch(
    EntityRef trigger,EntityRef actor) noexcept {
    TriggerKind kind{};
    if(!behavior_.kind_for(trigger,kind))
        return {TriggerCode::UnknownEntity};
    NativeTriggerSnapshot s{};
    if(!read_and_refresh(trigger,s))
        return {TriggerCode::HostRefused};
    return behavior_.touch(trigger,actor);
}
} // namespace sin_compat::map_entities
