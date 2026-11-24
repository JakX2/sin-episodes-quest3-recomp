#pragma once
#include "native_action_monitor.h"
namespace sin_port::native_weapon_tables {
inline constexpr char action_listener_export[]="SiN_GetActionListenerApi";
enum class ActionRoute:unsigned {Look,Use,Weapon,Move,Health,Game,Map,Difficulty,Activate};
struct ActionListenerBindings {
    void* context;
    int (__cdecl *prepare)(void*) noexcept;
    // Original registers server-only event names. Callback success is
    // transport success, not AddListener's ignored return value.
    int (__cdecl *subscribe)(void*,void* listener,const char*,unsigned server) noexcept;
    int (__cdecl *unsubscribe)(void*,void* listener) noexcept;
    int (__cdecl *get_int)(void*,const void* event,const char*,int fallback,int*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    // Actual entity virtual +120, not a classname approximation.
    int (__cdecl *participant)(void*,void*,unsigned*) noexcept;
    // Event GetName; a stable string for duration of the call.
    int (__cdecl *name)(void*,const void*,const char**) noexcept;
    int (__cdecl *get_string)(void*,const void*,const char*,const char* fallback,const char**) noexcept;
    int (__cdecl *get_float)(void*,const void*,const char*,float fallback,float*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *sample)(void*,void*,ActionEntitySample*) noexcept;
    // Synchronous action record sink (2215df00); record is an original
    // field layout but deliberately has no original vtable/pointers.
    int (__cdecl *record)(void*,ActionRoute,const void*,unsigned bytes) noexcept;
    // Other virtual route handlers stay host-owned.
    int (__cdecl *route)(void*,ActionRoute,const void*,int,void* owner,void* subject) noexcept;
};
struct ActionListenerApi {
    std::uint32_t version,size;
    int (__cdecl *subscribe)(void* listener,const ActionListenerBindings*) noexcept;
    int (__cdecl *unsubscribe)(void* listener,const ActionListenerBindings*) noexcept;
    int (__cdecl *dispatch)(const ActionListenerBindings*,const void* event) noexcept;
};
using GetActionListenerApi=const ActionListenerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
