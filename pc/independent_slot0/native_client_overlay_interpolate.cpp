#include "native_client_overlay_interpolate.h"
#include "client_overlay_cycle_math.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayHistoryApi* __cdecl
SiN_GetClientOverlayHistoryApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {
    T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;
}
bool finite(const ClientOverlaySample& s) noexcept {
    return std::isfinite(s.cycle)&&std::isfinite(s.previous_cycle)&&std::isfinite(s.weight);
}
float mix(float a,float b,float t) noexcept {
    return static_cast<float>(static_cast<double>(a)+(static_cast<double>(b)-a)*t);
}
float clamp(float v,float lo,float hi) noexcept {return v<lo?lo:(v>hi?hi:v);}
void sanitize(ClientOverlaySample& s) noexcept {
    // 240c2d40: these are deliberately wider than network limits.
    if(s.sequence<-1)s.sequence=-1;else if(s.sequence>1000)s.sequence=1000;
    s.cycle=clamp(s.cycle,-2.f,2.f);s.previous_cycle=clamp(s.previous_cycle,-2.f,2.f);
    s.weight=clamp(s.weight,-5.f,5.f);
}
ClientOverlaySample mixed(const ClientOverlaySample& a,const ClientOverlaySample& b,float t) noexcept {
    auto out=b;out.unknown10=0;
    out.cycle=mix(a.cycle,b.cycle,t);out.weight=mix(a.weight,b.weight,t);
    return out;
}
int __cdecl evaluate(const ClientOverlayHistorySelection* s,bool looping,ClientOverlaySample* dst) noexcept {
    if(!s||!dst||!s->older||!s->newer||!std::isfinite(s->fraction)||s->fraction<0.f||s->fraction>2.f||
       !finite(s->older->sample)||!finite(s->newer->sample))return 0;
    const auto& a=s->older->sample;const auto& b=s->newer->sample;
    ClientOverlaySample out{};
    if(s->older==s->newer)out=b;
    else{
        out=mixed(a,b,s->fraction);
        if(s->hermite){
            if(!s->earlier||!finite(s->earlier->sample)||!std::isfinite(s->earlier->time)||
               !std::isfinite(s->older->time)||!std::isfinite(s->newer->time))return 0;
            auto p=s->earlier->sample;
            // 240c3130: normalize the previous interval to the current
            // interval by nonlooping two-sample interpolation, even for a
            // looping output. No normalization for tiny/unchanged gap.
            const float old_gap=static_cast<float>(static_cast<double>(s->older->time)-s->earlier->time);
            const float gap=static_cast<float>(static_cast<double>(s->newer->time)-s->older->time);
            if(!std::isfinite(old_gap)||!std::isfinite(gap))return 0;
            if(std::fabs(static_cast<double>(gap)-old_gap)>0.0001f&&old_gap>0.0001f)
                p=mixed(p,a,static_cast<float>(1.-static_cast<double>(gap)/old_gap));
            if(!finite(p)||!overlay_cycle_math::cubic(s->fraction,p.cycle,a.cycle,b.cycle,looping,out.cycle))return 0;
        }else if(looping&&!overlay_cycle_math::linear(s->fraction,a.cycle,b.cycle,out.cycle))return 0;
    }
    if(!finite(out))return 0;
    sanitize(out);*dst=out;return 1;
}
// Extrapolation-disabled single overlay watcher path. History ownership
// remains in the host; no implicit allocation or destructive traversal.
int __cdecl apply_clocked(void* watcher,unsigned bytes,void* row,unsigned row_bytes,float time,
    const ClientOverlayInterpolationClock* clock,const ClientOverlayInterpolateBindings* b,int* wrote,int* raw_copy) noexcept {
    if(!wrote||!raw_copy)return 0;
    *wrote=0;*raw_copy=0;
    if(!clock||!std::isfinite(clock->last_network_time))return 0;
    if(!watcher||bytes<0x44||!row||row_bytes<sizeof(ClientOverlaySample)||!std::isfinite(time)||
       !b||!b->history.resolve||!b->prune||read<void*>(watcher,4)!=row||read<int>(watcher,0x38)!=1)return 0;
    const float delay=read<float>(watcher,0x3c);
    const float cutoff=static_cast<float>(static_cast<double>(time)-delay-0.05f);
    if(!std::isfinite(cutoff))return 0;
    const auto* history=SiN_GetClientOverlayHistoryApi(1,sizeof(ClientOverlayHistoryApi));
    ClientOverlayHistorySelection s{};
    if(!history||!history->select(read<std::uint32_t>(watcher,0x30),read<int>(watcher,0x2c),time,delay,&b->history,&s))return 0;
    if(!s.newer||!s.older)return 1;
    ClientOverlaySample out{};
    bool use_raw=false;
    // 240c2a56..240c2ac5. This specific overlay specialization does not
    // project velocity: both paths of 240c2b10 copy the newer payload.
    const float network_snapshot=read<float>(watcher,0x28);
    if(clock->allow_extrapolation&&s.older==s.newer&&s.newer->next&&
       delay>0.000001f&&std::isfinite(network_snapshot)&&clock->last_network_time<=network_snapshot){
        const auto word=s.newer->next;
        auto* prior=b->history.resolve(b->history.context,word);
        if(!prior||!std::isfinite(prior->time))return 0;
        const bool linked=prior->previous!=word||prior->next!=word;
        use_raw=linked&&prior!=s.newer&&prior->time!=0.f;
    }
    if(use_raw){
        if(!finite(s.newer->sample))return 0;
        out=s.newer->sample;
    }else if(!evaluate(&s,read<std::uint8_t>(watcher,0x34)!=0,&out))return 0;
    std::memcpy(row,&out,sizeof(out));*wrote=1;*raw_copy=use_raw?1:0;
    b->prune(b->history.context,watcher,cutoff);
    return 1;
}
int __cdecl apply(void* watcher,unsigned bytes,void* row,unsigned row_bytes,float time,const ClientOverlayInterpolateBindings* b,int* wrote) noexcept {
    const ClientOverlayInterpolationClock clock{false,0.f};
    int raw=0;
    return apply_clocked(watcher,bytes,row,row_bytes,time,&clock,b,wrote,&raw);
}
const ClientOverlayClockedInterpolateApi clocked_api{1,sizeof(ClientOverlayClockedInterpolateApi),&apply_clocked};
const ClientOverlayInterpolateApi api{1,sizeof(ClientOverlayInterpolateApi),&evaluate,&apply};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayInterpolateApi=_SiN_GetClientOverlayInterpolateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayInterpolateApi* __cdecl
SiN_GetClientOverlayInterpolateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayInterpolateApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayClockedInterpolateApi=_SiN_GetClientOverlayClockedInterpolateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayClockedInterpolateApi* __cdecl
SiN_GetClientOverlayClockedInterpolateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayClockedInterpolateApi)?&clocked_api:nullptr;
}
