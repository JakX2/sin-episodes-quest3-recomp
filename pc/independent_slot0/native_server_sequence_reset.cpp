#include "native_server_sequence_reset.h"
#include <cmath>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool dirty(void* actor,const ServerSequenceResetBindings& b) noexcept {
    if(read<std::uint8_t>(actor,0x1f0)){write<std::uint8_t>(actor,0x200,1);return true;}
    const auto word=read<std::uint32_t>(actor,0x16c);
    if(!word)return true;
    void* p=nullptr;unsigned extent=0;
    if(!b.storage||!b.storage(b.context,word,&p,&extent)||!p||extent<4)return false;
    write(p,0,read<std::uint32_t>(p,0)|1u);return true;
}
template<class T> bool assign(void* a,unsigned off,T value,const ServerSequenceResetBindings& b) noexcept {
    if(read<T>(a,off)==value)return true;
    if(!dirty(a,b))return false;
    write(a,off,value);return true;
}
bool valid(void* a,unsigned bytes,const ServerSequenceResetBindings* b) noexcept {
    return a&&bytes>=0x46c&&b&&b->duration&&b->distance&&b->flags&&b->register_events;
}
bool reset_impl(void* a,const ServerSequenceResetBindings& b,ServerSequenceResetResult& out) noexcept {
    // 220f1a60: normalize sentinel, derive speed and looping, then reset
    // playback/completion/baseline and both three-bit parity fields.
    if(read<int>(a,0x3ec)==-1&&!assign(a,0x3ec,0,b))return false;
    const int seq=read<int>(a,0x3ec);
    double duration=0,distance=0;unsigned flags=0;
    if(!b.duration(b.context,a,seq,&duration)||!std::isfinite(duration))return false;
    const float span=static_cast<float>(duration);
    if(!std::isfinite(span)||(duration>0.&&span==0.f))return false;
    if(duration>0.&&(!b.distance(b.context,a,seq,&distance)||!std::isfinite(distance)))return false;
    const float speed=duration>0.?static_cast<float>(distance/span):0.f;
    if(!std::isfinite(speed))return false;
    write(a,0x394,speed);
    if(!b.flags(b.context,a,seq,&flags))return false;
    write<std::uint8_t>(a,0x3e1,static_cast<std::uint8_t>(flags&1));
    if(!assign(a,0x3c0,1.f,b))return false;
    write<std::uint8_t>(a,0x3e0,0);write(a,0x398,0.f);
    for(unsigned off:{0x464u,0x468u})
        if(!assign(a,off,(read<std::uint32_t>(a,off)+1u)&7u,b))return false;
    out.reset=1;
    return b.register_events(b.context,a,seq)!=0;
}
int __cdecl reset(void* a,unsigned bytes,const ServerSequenceResetBindings* b,ServerSequenceResetResult* out) noexcept {
    if(!out)return 0;*out={};
    return valid(a,bytes,b)&&reset_impl(a,*b,*out);
}
int __cdecl set(void* a,unsigned bytes,int sequence,const ServerSequenceResetBindings* b,ServerSequenceResetResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!valid(a,bytes,b))return 0;
    const int old=read<int>(a,0x3ec);
    const bool old_loop=read<std::uint8_t>(a,0x3e1)!=0;
    if(b->debug_enabled&&(read<std::uint8_t>(a,0xa9)&0x10)){
        if(!b->debug)return 0;b->debug(b->context,a,old,sequence);
    }
    // 220f2300 uses the OLD loop flag; same looping sequence does not
    // reset, changed looping sequence preserves its current cycle.
    if(!old_loop){
        const float cycle=read<float>(a,0x3e8);
        if(!std::isfinite(cycle))return 0;
        if(cycle!=0.f){if(!assign(a,0x3e8,0.f,*b))return 0;out->cycle_changed=1;}
    }
    if(old!=sequence){
        if(!assign(a,0x3ec,sequence,*b))return 0;out->sequence_changed=1;
    }
    return (old==sequence&&old_loop)||reset_impl(a,*b,*out);
}
const ServerSequenceResetApi api{1,sizeof(ServerSequenceResetApi),&reset,&set};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerSequenceResetApi=_SiN_GetServerSequenceResetApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerSequenceResetApi* __cdecl
SiN_GetServerSequenceResetApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerSequenceResetApi)?&api:nullptr;
}
