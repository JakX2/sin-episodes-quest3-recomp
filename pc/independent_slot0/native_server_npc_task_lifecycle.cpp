#include "native_server_npc_task_lifecycle.h"
#include <cstring>
#include <cmath>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool finish_cleanup(void* a,float now,const ServerNpcTaskLifecycleBindings& b,ServerNpcTaskLifecycleResult& out) noexcept{
    if(!read<std::uint8_t>(a,0x9ec))return true;
    // 220720c0: timer already running only when +4 and deadline>now.
    const float deadline=read<float>(a,0x9a8);
    if(!std::isfinite(deadline))return false;
    if(!read<std::uint8_t>(a,0x9ac)||deadline<=now){
        const unsigned lo=read<std::uint16_t>(a,0x9b0),hi=read<std::uint16_t>(a,0x9b2);
        unsigned draw=0;double delay=0;
        if(!b.random_int||!b.random_float||!b.random_int(b.context,lo,hi,&draw)||draw>65535)return false;
        write(a,0x9ae,static_cast<std::uint16_t>(draw));
        if(!b.random_float(b.context,read<float>(a,0x9b4),read<float>(a,0x9b8),&delay)||!std::isfinite(delay))return false;
        const float next=static_cast<float>(now+delay);if(!std::isfinite(next))return false;
        write<std::uint8_t>(a,0x9ac,1);write(a,0x9a8,next);out.randomized=1;
    }
    write<std::uint8_t>(a,0x9ec,0);out.cleanup=1;
    return b.cleanup&&b.cleanup(b.context,a);
}
int __cdecl complete(void* a,unsigned bytes,float now,bool force,const ServerNpcTaskLifecycleBindings* b,ServerNpcTaskLifecycleResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||bytes<0x9f4||!b||!std::isfinite(now)||!finish_cleanup(a,now,*b,*out))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
    if(!force&&(!cond||!cond->test(a,bytes,0x21,NpcConditionCurrent,&b->conditions,&cr)))return 0;
    if(force||!cr.value){write(a,0x7a4,4);out->completed=1;}
    out->status=read<int>(a,0x7a4);return 1;
}
int __cdecl fail(void* a,unsigned bytes,float now,int code,const ServerNpcTaskLifecycleBindings* b,ServerNpcTaskLifecycleResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||bytes<0xbf4||!b||!std::isfinite(now)||!finish_cleanup(a,now,*b,*out))return 0;
    if(b->record_failure){
        std::uint32_t name=0;if(!b->failure_name||!b->failure_name(b->context,code,&name))return 0;
        write(a,0xbe4,name);write(a,0xbf0,0u);write(a,0xbec,read<std::uint32_t>(a,0x798));out->recorded=1;
    }
    write(a,0x7b0,code);
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
    if(!cond||!cond->store(a,bytes,0x21,NpcConditionCurrent,true,&b->conditions,&cr))return 0;
    write<std::uint8_t>(a,0x989,read<std::uint8_t>(a,0x989)&0xdf);
    out->failed=1;out->status=read<int>(a,0x7a4);return 1;
}
int __cdecl animation_finished(const void* a,unsigned bytes,unsigned* out) noexcept{
    if(!out)return 0;*out=0;
    if(!a||bytes<0x888)return 0;
    *out=read<std::uint8_t>(a,0x3e0)&&read<int>(a,0x3ec)==read<int>(a,0x884);return 1;
}
const ServerNpcTaskLifecycleApi api{1,sizeof(ServerNpcTaskLifecycleApi),&complete,&fail,&animation_finished};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcTaskLifecycleApi=_SiN_GetServerNpcTaskLifecycleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl
SiN_GetServerNpcTaskLifecycleApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcTaskLifecycleApi)?&api:nullptr;
}
