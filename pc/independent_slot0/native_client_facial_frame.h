#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char client_facial_frame_export[]="SiN_GetClientFacialFrameApi";
inline constexpr unsigned client_facial_object_bytes=0xbf8;
inline constexpr unsigned client_facial_network_count=64;
inline constexpr unsigned client_facial_global_count=128;
// Already-resolved controller metadata: original 20-byte studio controller
// row +8 global index, +c minimum, +10 maximum. No model loading here.
struct ClientFacialController {int global;float minimum,maximum;};
struct ClientFacialFrameBindings {
    void* context;
    // Original 240ce6b0 registry lookup of "blink". Caller owns registry.
    int (__cdecl *lookup)(void*,const char*,int*) noexcept;
};
struct ClientFacialFrameApi {
    std::uint32_t version,size;
    // Original 240d0a70 (secondary this = base+4). Caller provides full
    // base entity, 128-float scratch output and resolved controller metadata.
    // No controller rows: zero output, no blink processing.
    int (__cdecl *project)(void*,unsigned,const ClientFacialController*,unsigned,float,float,const ClientFacialFrameBindings*,float*) noexcept;
    // Original 240d0c79..0d04 descriptor smoothing stage. Caller owns
    // previous/current arrays, enables smoothing only when original gate permits.
    int (__cdecl *smooth)(float*,const float*,unsigned,float) noexcept;
};
using GetClientFacialFrameApi=const ClientFacialFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
