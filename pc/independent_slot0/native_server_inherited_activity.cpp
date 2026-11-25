#include "native_server_inherited_activity.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponActivityApi* __cdecl
SiN_GetServerWeaponActivityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityChoiceApi* __cdecl
SiN_GetServerActivityChoiceApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool probe(void* actor,int activity,const ServerInheritedActivityBindings& b,ServerInheritedActivityResult& out) noexcept {
    const auto* t=b.sequences;
    if(!t||!t->metadata||!t->activities||!t->events)return false;
    const auto* choice=SiN_GetServerActivityChoiceApi(1,sizeof(ServerActivityChoiceApi));
    if(!choice)return false;
    const ServerActivityCompositeView* view=nullptr;
    const int status=t->metadata(t->context,actor,&view);
    if(status<0||(status>0&&!view))return false;
    if(!status)view=nullptr;
    ServerActivityChoiceResult cr{};
    ++out.probes;
    if(!choice->choose(t->activities,t->events,view,activity,read<int>(actor,0x3ec),true,&t->random,&cr))return false;
    out.sequence=cr.selected;return true;
}
bool posture(void* target,const ServerInheritedActivityBindings& b,ServerInheritedActivityResult& out,int& value) noexcept {
    value=0;
    if(!target)return true;
    unsigned size=0;
    if(!b.target_extent||!b.target_extent(b.context,target,&size)||size<0x3a6)return false;
    value=read<std::int16_t>(target,0x3a4);
    out.target_posture=static_cast<unsigned>(value);
    return true;
}
int __cdecl translate(void* actor,unsigned bytes,int input,const ServerInheritedActivityBindings* b,ServerInheritedActivityResult* out) noexcept {
    if(!out)return 0;*out={};out->activity=input;out->sequence=-1;
    if(!actor||bytes<0xa1c||!b)return 0;
    const auto* weapon=SiN_GetServerWeaponActivityApi(1,sizeof(ServerWeaponActivityApi));
    if(!weapon||!weapon->capabilities(actor,bytes,&b->weapons,&out->capabilities))return 0;
    if(!(out->capabilities&0x08000000u))return 1;
    const bool idle=input==3||(input==1&&(read<std::uint8_t>(actor,0x988)&2));
    if(input!=66&&!idle)return 1;
    void* target=sin_independent::EntityHandleView(b->weapons.entries,b->weapons.count).Resolve(read<std::uint32_t>(actor,0xa18));
    out->target_live=target!=nullptr;
    if(input==66){
        // 2203da40: +154 is queried before target posture, only for live target.
        if(!target)return 1;
        if(!b->reload_context)return 0;
        const int context=b->reload_context(b->context,actor);
        if(context<0)return 0;if(!context)return 1;
        int kind=0;if(!posture(target,*b,*out,kind))return 0;
        if(kind!=100&&kind!=101)return 1;
        if(!probe(actor,67,*b,*out))return 0;
        if(out->sequence==-1)return 1;
        if(!b->reload_trace||!b->reload_trace(b->context,actor,target,67,&out->trace_fraction)||!std::isfinite(out->trace_fraction))return 0;
        out->trace_called=1;
        // Original FUCOMPP against 1.0: finite unequal -> lower reload.
        if(out->trace_fraction!=1.f)out->activity=67;
        return 1;
    }
    int kind=0;if(!posture(target,*b,*out,kind))return 0;
    const int chosen=kind==100?4:kind==101?5:3;
    if(!probe(actor,chosen,*b,*out))return 0;
    out->activity=out->sequence==-1?1:chosen;
    return 1;
}
const ServerInheritedActivityApi api{1,sizeof(ServerInheritedActivityApi),&translate};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerInheritedActivityApi=_SiN_GetServerInheritedActivityApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerInheritedActivityApi* __cdecl
SiN_GetServerInheritedActivityApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerInheritedActivityApi)?&api:nullptr;
}
