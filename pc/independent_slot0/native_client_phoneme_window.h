#pragma once
#include <cstdint>
#include <cmath>
namespace sin_port::native_weapon_tables {
inline constexpr char client_phoneme_window_export[]="SiN_GetClientPhonemeWindowApi";
// Resolved sentence phoneme: start/end at +0/+4, unsigned code at +8.
struct ClientPhonemeInterval {float start,end;std::uint16_t code,reserved;};
using ClientPhonemeEmit=void (__cdecl *)(void*,std::uint16_t,float) noexcept;
struct ClientPhonemeWindowApi {
    std::uint32_t version,size;
    // 240d07f0: window is in/out, can widen as the ordered rows are visited.
    // adaptive is true when the original cvar is zero or studio flag 0x10 is set.
    int (__cdecl *sample)(const ClientPhonemeInterval*,unsigned,float,float*,bool,void*,ClientPhonemeEmit) noexcept;
};
using GetClientPhonemeWindowApi=const ClientPhonemeWindowApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Pure policy; caller supplies validated row storage and positive finite window.
inline void sample_client_phonemes(const ClientPhonemeInterval* rows,unsigned count,float time,
    float& window,bool adaptive,void* context,ClientPhonemeEmit emit) noexcept {
    for(unsigned i=0;i<count;++i){
        const auto& row=rows[i];
        if(adaptive&&time>row.start&&time<row.end&&i+1<count){
            const double next=static_cast<double>(rows[i+1].end)-time;
            const float duration=static_cast<float>(static_cast<double>(row.end)-row.start);
            const double candidate=next<duration?next:duration;
            if(window<=candidate)window=static_cast<float>(candidate);
        }
        const double inverse=1.0/static_cast<double>(window);
        const float begin=static_cast<float>((static_cast<double>(row.start)-time)*inverse);
        const double end=(static_cast<double>(row.end)-time)*inverse;
        if(begin<1.0&&end>0.0){
            const double clipped_end=end>1.0?1.0:end;
            const float clipped_begin=begin<0.f?0.f:begin;
            emit(context,row.code,static_cast<float>(clipped_end-clipped_begin));
        }
    }
}
}
