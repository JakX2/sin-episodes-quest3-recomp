#pragma once
#include "native_vehicle_think.h"
namespace sin_port::native_weapon_tables {
inline constexpr char camera_link_export[]="SiN_GetCameraLinkApi";
enum class CameraLinkFieldKind:unsigned {Key,Saved,Input};
struct CameraLinkField {const char* name;CameraLinkFieldKind kind;unsigned type,offset;};
struct CameraLinkSchema {const char* classname;unsigned bytes,count;const CameraLinkField* fields;};
struct CameraLinkBindings {
    void* context;
    // Engine base lifecycle and global list allocation remain host-owned.
    int (__cdecl *base_construct)(void*,void* link,unsigned argument) noexcept;
    int (__cdecl *register_link)(void*,void* link) noexcept;
    int (__cdecl *unregister_link)(void*,void* link) noexcept;
    int (__cdecl *base_destroy)(void*,void* link) noexcept;
    int (__cdecl *base_resolve_names)(void*,void* link) noexcept;
    // Pooled string conversion returns zero for empty string.
    int (__cdecl *convert)(void*,unsigned type,std::uint32_t word,std::uint32_t*) noexcept;
    int (__cdecl *find_named)(void*,std::uint32_t string_word,void**) noexcept;
    int (__cdecl *is_point_camera)(void*,void* entity,unsigned*) noexcept;
    int (__cdecl *handle)(void*,void* entity,std::uint32_t*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t handle,void**) noexcept;
};
struct CameraLinkApi {
    std::uint32_t version,size;
    const CameraLinkSchema* (__cdecl *schema)() noexcept;
    int (__cdecl *construct)(HologramControl*,const CameraLinkBindings*) noexcept;
    int (__cdecl *destroy)(HologramControl*,const CameraLinkBindings*) noexcept;
    int (__cdecl *key)(HologramControl*,const char*,unsigned type,std::uint32_t word) noexcept;
    int (__cdecl *input)(HologramControl*,const CameraLinkBindings*,const char*,unsigned type,std::uint32_t word) noexcept;
    // Original virtual slot 34, not ordinary Spawn or Activate.
    int (__cdecl *resolve_names)(HologramControl*,const CameraLinkBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetCameraLinkApi=const CameraLinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
