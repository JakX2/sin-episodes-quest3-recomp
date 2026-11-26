#pragma once
#include <cstdint>
#include "server_entity_handle_core.h"
namespace sin_port::native_vgui_button {
// Authenticated client PE identities. Never execute these addresses.
inline constexpr char export_name[]="SiN_GetClientVguiButtonApi";
inline constexpr unsigned panel_vtable=0x24295b54u,screen_vtable=0x2429586cu,screen_networkable_vtable=0x242957ccu;
struct Bindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *create_event)(void*,const char*,bool,void**) noexcept;
    int (__cdecl *set_int)(void*,void*,const char*,int) noexcept;
    int (__cdecl *set_string)(void*,void*,const char*,const char*) noexcept;
    // A true result transfers ownership; on failure caller still owns it.
    int (__cdecl *fire_event)(void*,void*,bool) noexcept;
    int (__cdecl *discard_event)(void*,void*) noexcept;
};
struct Result {
    unsigned create_called,created,resolved,extent_read,index_native,int_set,string_set,fired,discarded,base_noop,completed;
    int index;
};
struct Api {
    std::uint32_t version,size;
    int (__cdecl *command)(void*,unsigned,const char*,const Bindings*,Result*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
