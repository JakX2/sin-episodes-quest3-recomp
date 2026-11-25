#include "native_server_weapon_event_sound.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool emit(void* weapon,int sound,float time,const ServerWeaponEventSoundBindings& b,ServerWeaponEventSoundResult& out) noexcept {
    if(!b.emit||!b.emit(b.context,weapon,sound,time))return false;
    ++out.emitted;out.last_sound=sound;out.last_time=time;return true;
}
int __cdecl play(void* weapon,unsigned bytes,unsigned kind,int sound,float time,const ServerWeaponEventSoundBindings* b,ServerWeaponEventSoundResult* out) noexcept {
    if(!out)return 0;*out={};out->last_sound=-1;
    if(!weapon||bytes<0x4f0||kind>WeaponActivitySmg||!b||!std::isfinite(time))return 0;
    if((kind==WeaponActivitySmg||kind==WeaponActivityChaingun)&&sound==1){
        if(bytes<0x58c)return 0;
        const int mode=read<int>(weapon,0x588);
        if(mode==1)return emit(weapon,1,time,*b,*out);
        if(mode!=3)return 1;
        const int target=read<int>(weapon,0x584);
        int maximum=0;
        if(!b->max_clip||!b->max_clip(b->context,weapon,&maximum))return 0;
        const int clip=read<int>(weapon,0x4ec);
        if(clip<target)return emit(weapon,1,time,*b,*out);
        if(target==maximum)return emit(weapon,5,time,*b,*out);
        return 1;
    }
    return emit(weapon,sound,time,*b,*out);
}
int __cdecl assault_tick(void* weapon,unsigned bytes,float now,const ServerWeaponEventSoundBindings* b,ServerWeaponEventSoundResult* out) noexcept {
    if(!out)return 0;*out={};out->last_sound=-1;
    if(!weapon||bytes<0x584||!b||!std::isfinite(now))return 0;
    float next=read<float>(weapon,0x580);
    const float last=read<float>(weapon,0x2c),prev=read<float>(weapon,0x28);
    if(!std::isfinite(next)||!std::isfinite(last)||!std::isfinite(prev))return 0;
    if(next<now){next=now;write(weapon,0x580,next);}
    double dt=static_cast<double>(last)-prev;
    dt=dt<0.?0.:dt>.2?.2:dt;
    // First comparison uses the unrounded delta, second reloads its f32 spill.
    for(unsigned i=0;i<2;++i){
        const double elapsed=i?static_cast<float>(dt):dt;
        if(static_cast<double>(now)+elapsed<=next)break;
        if(!b->interval||!emit(weapon,2,next,*b,*out))return 0;
        double interval=0;
        if(!b->interval(b->context,weapon,&interval)||!std::isfinite(interval))return 0;
        // Original reloads +580 after sound/interval callbacks.
        next=static_cast<float>(static_cast<double>(read<float>(weapon,0x580))+interval);
        if(!std::isfinite(next))return 0;
        write(weapon,0x580,next);
    }
    return 1;
}
const ServerWeaponEventSoundApi api{1,sizeof(ServerWeaponEventSoundApi),&play,&assault_tick};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerWeaponEventSoundApi=_SiN_GetServerWeaponEventSoundApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponEventSoundApi* __cdecl
SiN_GetServerWeaponEventSoundApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerWeaponEventSoundApi)?&api:nullptr;
}
