#pragma once
#include "native_server_activity_translate.h"
#include "native_server_weapon_activity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_inherited_activity_export[]="SiN_GetServerInheritedActivityApi";
struct ServerInheritedActivityBindings {
    ServerWeaponActivityBindings weapons;
    void* context;
    // Extent of a live entity already resolved by EntityHandleView.
    int (__cdecl *target_extent)(void*,void* target,unsigned*) noexcept;
    // Original +154 availability, 1 present / 0 absent / negative failure.
    int (__cdecl *reload_context)(void*,void* actor) noexcept;
    // Original 2203da40 lower-view trace boundary. This service includes
    // the +630 view vector and the engine trace, not a fabricated fraction.
    int (__cdecl *reload_trace)(void*,void* actor,void* target,int lower_activity,float*) noexcept;
    // Reuse the exact same activity metadata and RNG as the outer cascade.
    const ServerNpcActivityTranslateBindings* sequences;
};
struct ServerInheritedActivityResult {
    int activity,sequence;
    unsigned capabilities,target_live,target_posture,probes,trace_called;
    float trace_fraction;
};
struct ServerInheritedActivityApi {
    std::uint32_t version,size;
    // 2203cfc0, verified common +5c0/+714/+718 virtuals for Grunt/Jetpack.
    int (__cdecl *translate)(void*,unsigned,int,const ServerInheritedActivityBindings*,ServerInheritedActivityResult*) noexcept;
};
using GetServerInheritedActivityApi=const ServerInheritedActivityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
