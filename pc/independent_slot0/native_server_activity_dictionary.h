#pragma once
#include "native_server_event_dictionary.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_activity_dictionary_export[]="SiN_GetServerActivityDictionaryApi";
// Separate owner and generation from the animation-event registry. Reuse
// the bounded name storage, not the original activity 8-byte vector ABI.
struct ServerActivityDictionary {
    ServerEventDictionary names;
    int last_shared_request;
};
struct ServerActivitySeed {int id;const char* name;};
struct ServerActivityDictionaryApi {
    std::uint32_t version,size;
    int (__cdecl *lookup)(const ServerActivityDictionary*,const char*,int*) noexcept;
    int (__cdecl *shared)(ServerActivityDictionary*,const char*,int) noexcept;
    int (__cdecl *private_id)(ServerActivityDictionary*,const char*,int*) noexcept;
    int (__cdecl *clear)(ServerActivityDictionary*) noexcept;
    int (__cdecl *reset_highest)(ServerActivityDictionary*) noexcept;
    // Full seed extracted from the owner-authenticated binary, exactly
    // IDs 0..920, unique ASCII names. No hardcoded modern SDK mapping.
    int (__cdecl *seed)(ServerActivityDictionary*,const ServerActivitySeed*,unsigned) noexcept;
};
using GetServerActivityDictionaryApi=const ServerActivityDictionaryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
