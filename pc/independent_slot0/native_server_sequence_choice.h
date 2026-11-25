#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char server_sequence_choice_export[]="SiN_GetServerSequenceChoiceApi";
// Resolved studio descriptor +10/+14 values, after activity registration.
// Indices are actual sequence indices in descriptor traversal order.
struct ServerSequenceChoiceEntry {int activity,weight;};
struct ServerSequenceChoiceBindings {
    void* context;
    // Same inclusive endpoints as original random service; zero total
    // intentionally calls with (0,-1). Result ignored when total is zero.
    int (__cdecl *random)(void*,int low,int high,int sequence_index,int*) noexcept;
};
struct ServerSequenceChoiceApi {
    std::uint32_t version,size;
    // 220eb2c0: strict greatest absolute weight; first tie wins.
    // 220eb1c0: weighted reservoir, current negative weight is sticky.
    // Null metadata is original return 0; nonnull empty returns -1.
    int (__cdecl *choose)(const ServerSequenceChoiceEntry*,unsigned count,int activity,int current,bool looping,const ServerSequenceChoiceBindings*,int*) noexcept;
};
using GetServerSequenceChoiceApi=const ServerSequenceChoiceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
