#pragma once
#include "native_server_flex_weight.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_flex_animation_export[]="SiN_GetServerFlexAnimationApi";
inline constexpr unsigned server_flex_scene_extent=0x608;
struct ServerFlexAnimationBindings {
    void* context;
    const ServerFlexRange* ranges;unsigned count;
    ServerFlexWeightBindings dirty;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // 220f5840 returns controller zero on missing name; the API performs
    // that fallback itself. -1 means no match, -2 means transport failure.
    int (__cdecl *controller)(void*,const char*) noexcept;
    // Original 2210d970: normalized sample at event-relative time.
    // Range conversion and stereo balance are implemented natively.
    int (__cdecl *sample)(void*,void*,unsigned,float,float*) noexcept;
    // Original 22110f80 event intensity (scene time).
    int (__cdecl *intensity)(void*,void*,float,float*) noexcept;
};
struct ServerFlexAnimationApi {
    std::uint32_t version,size;
    // 220f5d50 / 220f5d80 with scene background flag and global time
    // supplied. Actor's +604 timestamp and record +20 fade are native.
    int (__cdecl *fade)(void*,unsigned,void*,unsigned,bool,float,bool,float*) noexcept;
    // 220f84e0 / 220f71b0, event +320 lazy binding, record +0c active.
    int (__cdecl *apply)(void*,unsigned,void*,unsigned,void*,unsigned,float,float,bool,const ServerFlexAnimationBindings*) noexcept;
    // 220f8450 weight decay prepass, not event-vector dispatch.
    int (__cdecl *decay)(void*,unsigned,const ServerFlexRange*,unsigned,const ServerFlexWeightBindings*) noexcept;
};
using GetServerFlexAnimationApi=const ServerFlexAnimationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
