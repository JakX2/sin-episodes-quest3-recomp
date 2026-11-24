#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char radio_message_export[]="SiN_GetRadioMessageApi";
// Original global dispatcher scalar layout, independent of its scratch
// sound-name buffer. Owner serializes this state and all message calls.
struct RadioDispatcher {std::uint32_t pending=0xffffffffu,current=0xffffffffu,other=0xffffffffu;int stage=6,kind=0;float deadline=-1.0f;};
static_assert(sizeof(RadioDispatcher)==0x18);
enum class RadioInput:unsigned {Speaker,Priority,Message,Dispatch,Remove};
struct RadioValue {unsigned type;std::uint32_t word;};
struct RadioBindings {
    void* context;
    // Message word represents a host-owned stable string identity.
    int (__cdecl *string)(void*,std::uint32_t,const char**) noexcept;
    int (__cdecl *convert_variant)(void*,const RadioValue*,std::uint32_t*) noexcept;
    int (__cdecl *precache)(void*,const char*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void** entity,unsigned* bytes) noexcept;
    int (__cdecl *handle)(void*,void* entity,std::uint32_t*) noexcept;
    int (__cdecl *max_clients)(void*,int*) noexcept;
    int (__cdecl *single_player)(void*,void**) noexcept;
    int (__cdecl *output)(void*,void* message,void* player,unsigned offset) noexcept;
    // Original dispatcher reset/delivery helper 22288d70(0,0,0).
    int (__cdecl *clear_delivery)(void*) noexcept;
};
struct RadioSound {char text[256];std::uint32_t speaker_word;unsigned present;};
struct RadioMessageApi {
    std::uint32_t version,size;
    int (__cdecl *initialize)(HologramControl*) noexcept;
    int (__cdecl *input)(HologramControl*,RadioDispatcher*,const RadioBindings*,RadioInput,const RadioValue*) noexcept;
    int (__cdecl *heard)(HologramControl*,const RadioBindings*,unsigned value) noexcept;
    // Original state transition leaves, not the full playback dispatcher.
    int (__cdecl *promote)(RadioDispatcher*,const RadioBindings*) noexcept;
    int (__cdecl *sound)(const RadioDispatcher*,const RadioBindings*,unsigned stage,RadioSound*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetRadioMessageApi=const RadioMessageApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char radio_playback_export[]="SiN_GetRadioPlaybackApi";
struct RadioPlaybackBindings {
    RadioBindings message;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *participant)(void*,void* player,unsigned*) noexcept;
    // Existing recipient/audio service. Original channel 4, volume 1,
    // recipient filter parameter .8; returns actual duration.
    int (__cdecl *emit)(void*,void* player,const char* sound,int channel,float volume,float filter_parameter,float* duration) noexcept;
    // Original 22288d70 delivery helper. Host handles exactly one
    // connected player and PoliceRadioMessage transport.
    int (__cdecl *notify)(void*,unsigned active,int priority,const char* speaker) noexcept;
};
struct RadioPlaybackControl {
    RadioDispatcher* state;
    unsigned busy=0,blocked=0;
    unsigned input_latch=0; // shared per server, original 2259e7c8
};
struct RadioPlaybackApi {
    std::uint32_t version,size;
    // Request kind3 only when stage6 (original 222e4290).
    int (__cdecl *request)(RadioPlaybackControl*) noexcept;
    // Original player button words +938,+93c,+940; preserve latch
    // before scheduler tick. Other player frame work remains host-owned.
    int (__cdecl *player_input)(RadioPlaybackControl*,unsigned word938,unsigned word93c,unsigned word940) noexcept;
    int (__cdecl *tick)(RadioPlaybackControl*,const RadioPlaybackBindings*) noexcept;
    int (__cdecl *acknowledge)(RadioPlaybackControl*) noexcept;
};
using GetRadioPlaybackApi=const RadioPlaybackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

}
