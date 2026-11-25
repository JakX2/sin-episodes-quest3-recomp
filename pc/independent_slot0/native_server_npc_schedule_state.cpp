#include "native_server_npc_schedule_state.h"
#include <cstring>
#include <cmath>
#include <initializer_list>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
const ServerNpcConditionApi* conditions() noexcept{return SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));}
bool resolve(const ServerNpcScheduleBindings& b,std::uint32_t token,unsigned min,const void** p) noexcept {
    *p=nullptr;unsigned bytes=0;return token&&b.storage&&b.storage(b.context,token,p,&bytes)&&*p&&bytes>=min;
}
int __cdecl clear(void* a,unsigned bytes) noexcept {
    if(!a||bytes<0x84c)return 0;
    constexpr unsigned fields[]{0x7a8,0x7ac,0x7a4,0x79c,0x798,0x7a0,0x7b4};
    for(unsigned off:fields)write(a,off,0u);
    write<std::uint8_t>(a,0x7b8,0);write<std::uint8_t>(a,0x7b9,0);
    std::memset(static_cast<unsigned char*>(a)+0x82c,0xff,32);
    return 1;
}
bool inspect_impl(void* a,unsigned bytes,const ServerNpcScheduleBindings* b,ServerNpcScheduleStateResult* out,bool task) noexcept {
    if(!out)return false;*out={};out->global=-1;
    if(!a||bytes<0x7bc||!b)return false;
    out->schedule=read<std::uint32_t>(a,0x798);out->index=read<int>(a,0x7a0);out->status=read<int>(a,0x7a4);
    if(!out->schedule)return true;
    const void* schedule=nullptr;
    if(!resolve(*b,out->schedule,0x30,&schedule))return false;
    out->global=read<int>(schedule,0);out->count=read<int>(schedule,8);
    if(out->count<0||out->count>65535||out->index<0||out->index>out->count)return false;
    out->complete=out->index==out->count;
    if(task&&!out->complete){
        const void* tasks=nullptr;
        if(!resolve(*b,read<std::uint32_t>(schedule,4),static_cast<unsigned>(out->count)*8,&tasks))return false;
        out->task=read<ServerNpcScheduleTask>(tasks,8u*out->index);
        out->has_task=1;
    }
    return true;
}
int __cdecl inspect(void* a,unsigned bytes,const ServerNpcScheduleBindings* b,ServerNpcScheduleStateResult* out) noexcept {
    return inspect_impl(a,bytes,b,out,true);
}
int __cdecl install(void* a,unsigned bytes,std::uint32_t token,float now,const ServerNpcScheduleBindings* b,ServerNpcScheduleStateResult* out) noexcept {
    if(!out)return 0;*out={};out->global=-1;
    if(!a||bytes<0x98a||!b||!b->reset_movement||!std::isfinite(now))return 0;
    const void* schedule=nullptr;const auto* api=conditions();
    if(!api||!resolve(*b,token,0x30,&schedule))return 0;
    const int count=read<int>(schedule,8);
    if(count<0||count>65535)return 0;
    write(a,0x7a8,now);write(a,0x7ac,now);write(a,0x798,token);
    for(unsigned off:{0x7a0u,0x7b4u,0x7a4u,0x7bcu})write(a,off,0u);
    write<std::uint8_t>(a,0x7b8,0);write<std::uint8_t>(a,0x7b9,0);
    ServerNpcConditionResult cr{};
    if(!api->test(a,bytes,1,NpcConditionCurrent,&b->conditions,&cr))return 0;
    const bool keep=cr.value!=0;
    std::memset(static_cast<unsigned char*>(a)+0x7cc,0,32);
    if(keep&&!api->store(a,bytes,1,NpcConditionCurrent,true,&b->conditions,&cr))return 0;
    write<std::uint8_t>(a,0x84d,0);
    if(!b->reset_movement(b->context,a,read<std::uint32_t>(a,0x8f8)))return 0;
    std::memset(static_cast<unsigned char*>(a)+0x82c,0xff,32);
    write<std::uint8_t>(a,0x989,read<std::uint8_t>(a,0x989)&0xdf);
    if(!inspect_impl(a,bytes,b,out,false))return 0;
    out->preserved_condition=keep;return 1;
}
int __cdecl advance(void* a,unsigned bytes,const ServerNpcScheduleBindings* b,ServerNpcScheduleStateResult* out) noexcept {
    if(!out)return 0;*out={};out->global=-1;
    if(!a||bytes<0xbf4||!inspect_impl(a,bytes,b,out,false)||!out->schedule||out->complete)return 0;
    const int next=out->index+1;
    write(a,0x7a0,next);write(a,0x7a4,0u);write(a,0x7b4,0u);
    if(next==out->count){
        write(a,0xbec,0u);write(a,0xbf0,0u);
        const auto* api=conditions();ServerNpcConditionResult cr{};
        if(!api||!api->store(a,bytes,0x22,NpcConditionCurrent,true,&b->conditions,&cr))return 0;
    }
    return inspect_impl(a,bytes,b,out,false);
}
int __cdecl interrupt(void* a,unsigned bytes,int id,bool current,const ServerNpcScheduleBindings* b,unsigned* out) noexcept {
    if(!out)return 0;*out=0;
    if(!a||bytes<0x7ec||!b)return 0;
    const auto token=read<std::uint32_t>(a,0x798);if(!token)return 1;
    const auto* api=conditions();ServerNpcConditionResult cr{};
    if(!api||!api->map(a,id,&b->conditions,&cr))return 0;
    if(!cr.mapped)return 1;
    const unsigned word=static_cast<unsigned>(cr.bit)/32,mask=1u<<(cr.bit%32);
    if(current&&!(read<unsigned>(a,0x7cc+4*word)&mask))return 1;
    const void* schedule=nullptr;
    if(!resolve(*b,token,0x30,&schedule))return 0;
    *out=(read<unsigned>(schedule,0xc+4*word)&mask)!=0;return 1;
}
const ServerNpcScheduleStateApi api{1,sizeof(ServerNpcScheduleStateApi),&clear,&install,&inspect,&advance,&interrupt};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleStateApi=_SiN_GetServerNpcScheduleStateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl
SiN_GetServerNpcScheduleStateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcScheduleStateApi)?&api:nullptr;
}
