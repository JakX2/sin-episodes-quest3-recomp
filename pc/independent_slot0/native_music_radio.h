#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char music_radio_export[]="SiN_GetMusicRadioApi";
inline constexpr unsigned music_radio_bytes=0x3bc;
enum class MusicRadioInput:unsigned {On,Off,Next,Previous,Station,Volume,Cycle};
struct MusicRadioBindings {
    void* context;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *schedule)(void*,void* radio,float when) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *handle)(void*,void* radio,std::uint32_t*) noexcept;
    // Original all-recipient reliable filter, channel5, sound level60.
    // Existing engine supplies speaker index, origin, and actual duration.
    int (__cdecl *emit)(void*,void* radio,void* speaker,const char* sound,float volume,unsigned channel,unsigned level,unsigned flags,float* duration) noexcept;
    int (__cdecl *stop)(void*,void* speaker,unsigned channel,const char* sound) noexcept;
};
struct MusicRadioApi {
    std::uint32_t version,size;
    // Constructor scalar slice. Default script is host-owned string
    // word for scripts/radio.txt; station vector allocation host-owned.
    int (__cdecl *initialize)(HologramControl*,std::uint32_t default_script) noexcept;
    int (__cdecl *input)(HologramControl*,const MusicRadioBindings*,MusicRadioInput,int value) noexcept;
    int (__cdecl *tick)(HologramControl*,const MusicRadioBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetMusicRadioApi=const MusicRadioApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
