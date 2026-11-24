#pragma once
#include "native_server_composed_facial_think.h"
#include "native_server_vfe_remap.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_named_facial_think_export[]="SiN_GetServerNamedFacialThinkApi";
struct ServerNamedFacialThinkBindings {
    ServerComposedFacialThinkBindings composed;
    const char* const* controller_names;
    unsigned controller_count;
    ServerVfeRemapCache* remap; // caller-owned per actor, clear on layout or bank lifetime change
};
struct ServerNamedFacialThinkApi {
    std::uint32_t version,size;
    // Removes hosted controller lookup from all three recovered native leaves.
    int (__cdecl *run)(void*,unsigned,float,const ServerNamedFacialThinkBindings*) noexcept;
};
using GetServerNamedFacialThinkApi=const ServerNamedFacialThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
