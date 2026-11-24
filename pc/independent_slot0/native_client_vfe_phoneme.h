#pragma once
#include "native_client_vfe.h"
#include "native_client_phoneme_source.h"
namespace sin_port::native_weapon_tables {
inline constexpr char client_vfe_phoneme_export[]="SiN_GetClientVfePhonemeApi";
struct ClientVfePhonemeBank {ClientVfeView generic,character;};
struct ClientVfePhonemeBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,ClientPhonemeSentence*) noexcept;
    ClientVfeMap map;
    // Original order weak, required normal, strong; caller owns loaded bytes.
    ClientVfePhonemeBank banks[3];
};
struct ClientVfePhonemeApi {
    std::uint32_t version,size;
    int (__cdecl *apply)(const void*,unsigned,float,float,bool,const ClientVfePhonemeBindings*,float*) noexcept;
};
using GetClientVfePhonemeApi=const ClientVfePhonemeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
