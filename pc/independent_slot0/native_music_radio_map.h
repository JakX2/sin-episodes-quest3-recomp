#pragma once
#include "native_music_radio.h"
namespace sin_port::native_weapon_tables {
inline constexpr char music_radio_map_export[]="SiN_GetMusicRadioMapApi";
enum class MusicFieldKind:unsigned {Saved,Key,Think,Input};
struct MusicRadioField {const char* name;MusicFieldKind kind;unsigned type,offset;};
struct MusicRadioSchema {const char* classname;unsigned object_bytes,count;const MusicRadioField* fields;};
struct MusicRadioTrack {int station;char sound[64];};
static_assert(sizeof(MusicRadioTrack)==68);
struct MusicRadioMapBindings {
    MusicRadioBindings playback;
    // Existing engine KeyValues service. Null token means original
    // missing/unloadable file, a completed no-op. Preserve duplicate
    // sibling names and original enumeration order.
    int (__cdecl *open_script)(void*,std::uint32_t script_word,void** token) noexcept;
    int (__cdecl *entry)(void*,void* token,unsigned index,unsigned* present,const char** station,const char** sound) noexcept;
    int (__cdecl *close_script)(void*,void* token) noexcept;
    // Native x86 vector allocator is host-owned. Append one empty
    // 56-byte station or copy one 68-byte track at end, update count.
    // Array addresses may change; the DLL rereads them after callback.
    int (__cdecl *append_station)(void*,void* radio) noexcept;
    int (__cdecl *append_track)(void*,void* radio,int station,const MusicRadioTrack*) noexcept;
    int (__cdecl *precache)(void*,const char*) noexcept;
    int (__cdecl *base_spawn)(void*,void*) noexcept;
    int (__cdecl *base_activate)(void*,void*) noexcept;
    int (__cdecl *install_think)(void*,void*,const char* function_name) noexcept;
};
struct MusicRadioMapApi {
    std::uint32_t version,size;
    const MusicRadioSchema* (__cdecl *schema)() noexcept;
    int (__cdecl *key)(HologramControl*,const char*,unsigned type,std::uint32_t word) noexcept;
    int (__cdecl *precache)(HologramControl*,const MusicRadioMapBindings*) noexcept;
    int (__cdecl *spawn)(HologramControl*,const MusicRadioMapBindings*) noexcept;
    int (__cdecl *activate)(HologramControl*,const MusicRadioMapBindings*) noexcept;
    int (__cdecl *input)(HologramControl*,const MusicRadioMapBindings*,const char*,unsigned type,std::uint32_t word) noexcept;
};
using GetMusicRadioMapApi=const MusicRadioMapApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
