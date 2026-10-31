/* Phase 68: compile-time ABI checks, independent of any Source engine header.
 * Can compile as a freestanding AArch64 Android object without NDK libc.
 * This is intentionally a C translation unit, not a runnable Android test.
 */
#include "sin_compat/host_abi_v1.h"

#if defined(__cplusplus)
#define SIN_STATIC_ASSERT static_assert
#else
#define SIN_STATIC_ASSERT _Static_assert
#endif

SIN_STATIC_ASSERT(SIN_COMPAT_ABI_VERSION_1 == 1, "v1 id must not change");
SIN_STATIC_ASSERT(sizeof(sin_entity_ref_v1) == 8, "ref must encode two u32 values");
SIN_STATIC_ASSERT(offsetof(sin_entity_ref_v1, generation) == 4, "ref generation offset");
SIN_STATIC_ASSERT(sizeof(sin_ammo_v1) == 16, "ammo quartet layout");
SIN_STATIC_ASSERT(offsetof(sin_ammo_v1, reserve2) == 12, "ammo reserve2 offset");
SIN_STATIC_ASSERT(sizeof(((sin_weapon_config_v1*)0)->origin_offset) == 12, "origin float array");
SIN_STATIC_ASSERT(offsetof(sin_host_callbacks_v1, version) == 0, "callbacks version start");
SIN_STATIC_ASSERT(offsetof(sin_host_callbacks_v1, struct_size) == 4, "callbacks size start");

#if UINTPTR_MAX == UINT64_MAX
SIN_STATIC_ASSERT(sizeof(sin_weapon_config_v1) == 32, "64bit config layout");
SIN_STATIC_ASSERT(offsetof(sin_weapon_config_v1, zoom_fov) == 20, "64bit zoom offset");
SIN_STATIC_ASSERT(offsetof(sin_weapon_config_v1, clip2_size) == 28, "64bit clip2 offset");
SIN_STATIC_ASSERT(sizeof(sin_host_callbacks_v1) == 184, "64bit callback table width");
SIN_STATIC_ASSERT(offsetof(sin_host_callbacks_v1, is_live) == 8, "64bit callback start");
SIN_STATIC_ASSERT(offsetof(sin_host_callbacks_v1, bind_viewmodel) == 160, "64bit bind callback offset");
SIN_STATIC_ASSERT(offsetof(sin_host_callbacks_v1, set_flatscreen_fov) == 176, "64bit last callback offset");
#endif

int sin_abi_v1_layout_compiles(void) { return (int)SIN_COMPAT_ABI_VERSION_1 - 1; }

int main(void) {return sin_abi_v1_layout_compiles();}
