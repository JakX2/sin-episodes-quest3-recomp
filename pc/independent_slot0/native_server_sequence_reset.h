#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char server_sequence_reset_export[]="SiN_GetServerSequenceResetApi";
struct ServerSequenceResetBindings {
    void* context;
    int (__cdecl *duration)(void*,void*,int,double*) noexcept;
    int (__cdecl *distance)(void*,void*,int,double*) noexcept;
    int (__cdecl *flags)(void*,void*,int,unsigned*) noexcept;
    // Original +16c dirty word, only when +1f0 is clear.
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original tail 220f1ba0..1c13, resolve and register sequence events.
    int (__cdecl *register_events)(void*,void*,int) noexcept;
    // Optional diagnostic branch 220f2313, caller resolves global switch.
    bool debug_enabled;
    void (__cdecl *debug)(void*,void*,int old_sequence,int new_sequence) noexcept;
};
struct ServerSequenceResetResult {unsigned reset,cycle_changed,sequence_changed;};
struct ServerSequenceResetApi {
    std::uint32_t version,size;
    int (__cdecl *reset)(void*,unsigned,const ServerSequenceResetBindings*,ServerSequenceResetResult*) noexcept;
    int (__cdecl *set)(void*,unsigned,int,const ServerSequenceResetBindings*,ServerSequenceResetResult*) noexcept;
};
using GetServerSequenceResetApi=const ServerSequenceResetApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
