#pragma once
#include "native_server_studio_event_registration.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_event_dictionary_export[]="SiN_GetServerEventDictionaryApi";
// Port-owned bounded storage, not the retail symbol-table ABI.
struct ServerEventDictionaryEntry {
    int id;
    unsigned flags;
    bool private_event;
    char name[128];
};
struct ServerEventDictionary {
    ServerEventDictionaryEntry* entries;
    unsigned capacity,count,generation;
    int highest;
};
struct ServerEventDictionaryApi {
    std::uint32_t version,size;
    int (__cdecl *lookup)(const ServerEventDictionary*,const char*,int*) noexcept;
    int (__cdecl *flags)(const ServerEventDictionary*,int,unsigned*) noexcept;
    int (__cdecl *shared)(ServerEventDictionary*,const char*,int,unsigned) noexcept;
    // 1 success, 0 collision/exhaustion/invalid; output -1 on failure.
    int (__cdecl *private_id)(ServerEventDictionary*,const char*,int*) noexcept;
    int (__cdecl *clear)(ServerEventDictionary*) noexcept;
    int (__cdecl *reset_highest)(ServerEventDictionary*) noexcept;
    int (__cdecl *seed)(ServerEventDictionary*) noexcept;
    int (__cdecl *apply)(ServerEventDictionary*,void*,unsigned,unsigned,ServerStudioEventRegistrationResult*) noexcept;
};
using GetServerEventDictionaryApi=const ServerEventDictionaryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
