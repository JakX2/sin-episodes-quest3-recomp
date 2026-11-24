#pragma once
#include "native_client_phoneme_window.h"
#include "native_client_phoneme_emphasis.h"
namespace sin_port::native_weapon_tables {
inline constexpr char client_phoneme_source_export[]="SiN_GetClientPhonemeSourceApi";
inline constexpr unsigned client_phoneme_source_offset=0x520,client_phoneme_source_count_offset=0x540;
inline constexpr unsigned client_phoneme_max_sources=4;
struct ClientPhonemeSentence {
    float duration;
    const ClientPhonemeInterval* intervals;unsigned interval_count;
    const ClientEmphasisSample* emphasis;unsigned emphasis_count;
};
struct ClientPhonemeResolvedBanks {
    bool weak,strong;
    ClientPhonemeBank banks[3]; // weak, normal, strong, remapped indices
};
struct ClientPhonemeSourceBindings {
    void* context;
    // Audio service: 1 found, 0 missing, -1 invalid. Views remain live for
    // the duration of apply. No decoder or original audio invocation here.
    int (__cdecl *resolve)(void*,std::uint32_t,ClientPhonemeSentence*) noexcept;
    // 1 resolved, 0 missing required normal setting, -1 invalid.
    int (__cdecl *lookup)(void*,std::uint16_t,ClientPhonemeResolvedBanks*) noexcept;
};
struct ClientPhonemeSourceApi {
    std::uint32_t version,size;
    // 240d0970. Full C_BaseFlex object, supplied delay/window cvars and
    // adaptive flag. Accumulates onto the existing 128 projected weights.
    int (__cdecl *apply)(const void*,unsigned,float,float,bool,const ClientPhonemeSourceBindings*,float*) noexcept;
};
using GetClientPhonemeSourceApi=const ClientPhonemeSourceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
