#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char server_studio_event_registration_export[]="SiN_GetServerStudioEventRegistrationApi";
struct ServerStudioEventRegistrationBindings {
    void* context;
    // Original event dictionary lookup: id=-1 means not found.
    int (__cdecl *lookup)(void*,const char* name,int* id) noexcept;
    int (__cdecl *register_new)(void*,const char* name,int* id) noexcept;
    int (__cdecl *type_flags)(void*,int id,unsigned*) noexcept;
};
struct ServerStudioEventRegistrationResult {
    unsigned visited,named,existing,created;
    int failed_index;
};
struct ServerStudioEventRegistrationApi {
    std::uint32_t version,size;
    // 220ea730: descriptor within a stable writable metadata span, not
    // model acquisition/loading. All relative fields checked before use.
    int (__cdecl *apply)(void* storage,unsigned bytes,unsigned descriptor_offset,
        const ServerStudioEventRegistrationBindings*,ServerStudioEventRegistrationResult*) noexcept;
};
using GetServerStudioEventRegistrationApi=const ServerStudioEventRegistrationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
