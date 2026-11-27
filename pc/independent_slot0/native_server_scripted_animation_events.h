#pragma once
#include "native_server_animation_events.h"
#include "native_server_map_entity.h"
namespace sin_port::native_weapon_tables {
// An opt-in version of the original base/overlay event traversal.
// Script events use the borrowed 64-byte studio options span, all other
// events use the existing virtual emit callback.
struct ServerScriptedAnimationDispatchBindings {
    const ServerAnimationEventBindings* animation;
    const native_map::ScriptedNpcAnimationEventBindings* script;
};
struct ServerScriptedAnimationDispatchResult {
    ServerAnimationEventResult animation;
    unsigned completed,script_events,fallback_events;
    native_map::ScriptedNpcAnimationEventResult last;
};
inline constexpr char server_scripted_animation_dispatch_export[]="SiN_DispatchScriptedNpcAnimationEvents";
using DispatchScriptedNpcAnimationEvents=int (__cdecl *)(void*,unsigned,void*,unsigned,float,
    const ServerScriptedAnimationDispatchBindings*,ServerScriptedAnimationDispatchResult*) noexcept;
}
