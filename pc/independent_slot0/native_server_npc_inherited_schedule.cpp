#include "native_server_npc_inherited_schedule.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
struct Component {const ServerNpcInheritedScheduleBindings* b;std::uint32_t token;};
int __cdecl translate(void* a,unsigned n,int id,const ServerNpcInheritedScheduleBindings* b,ServerNpcInheritedScheduleResult* out) noexcept{
    if(!out)return 0;*out={};out->input=id;out->output=id;out->component_local=out->phase=-1;
    if(!a||n<0xc1c||!b||b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    const auto token=rd<std::uint32_t>(a,0xc18);
    if(token){
        out->component=1;out->component_local=id;
        if(id>=npc_global_condition_base||id==-1){
            const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
            Component context{b,token};
            ServerNpcConditionMapBindings map{&context,
                [](void* p,void*,const void** out,unsigned* bytes) noexcept -> int{
                    auto& c=*static_cast<Component*>(p);return c.b->component_descriptor&&c.b->component_descriptor(c.b->context,c.token,out,bytes);},
                [](void* p,std::uint32_t word,const void** out,unsigned* bytes) noexcept -> int{
                    auto& c=*static_cast<Component*>(p);return c.b->storage&&c.b->storage(c.b->context,word,out,bytes);}
            };
            if(!ns||!ns->to_local(a,id,&map,&out->component_local))return 0;
        }
        return b->component_translate&&b->component_translate(b->context,token,out->component_local,&out->output);
    }
    if(id==2){
        const int state=rd<int>(a,0x850);if(state==2)out->output=9;else if(state==3)out->output=15;return 1;
    }
    if(id!=0x36)return 1;
    void* script=sin_independent::EntityHandleView(b->entities.entries,b->entities.count).Resolve(rd<std::uint32_t>(a,0x970));
    if(!script){
        if(!b->reset_script||!b->reset_script(b->context,a))return 0;
        out->reset=1;out->output=1;return 1;
    }
    out->controller_live=1;unsigned bytes=0;
    if(!b->extent||!b->extent(b->context,script,&bytes)||bytes<0x3b0)return 0;
    out->phase=rd<int>(script,0x3ac);constexpr int ids[]{0x3a,0x37,0x38,0x39,0x3a,0x3b};
    if(out->phase>=0&&out->phase<6)out->output=ids[out->phase];
    return 1;
}
int __cdecl burst_range(unsigned kind,int* low,int* high) noexcept{
    if(!low||!high)return 0;*low=*high=0;
    constexpr int lo[]{1,2,1,1,1,1,1,1,4},hi[]{1,5,3,1,1,1,1,3,7};
    if(kind>WeaponActivitySmg)return 0;*low=lo[kind];*high=hi[kind];return 1;
}
int __cdecl burst(void* w,const ServerNpcInheritedScheduleBindings* b,int* out) noexcept{
    if(!out)return 0;*out=0;
    unsigned kind=0;int lo=0,hi=0;
    if(!w||!b||!b->entities.classify||!b->entities.classify(b->entities.context,w,&kind)||!burst_range(kind,&lo,&hi)||!b->random)return 0;
    // Retail calls RNG even when lo==hi.
    return b->random(b->context,lo,hi,out)&&*out>=lo&&*out<=hi;
}
const ServerNpcInheritedScheduleApi api{1,sizeof(ServerNpcInheritedScheduleApi),&translate,&burst_range,&burst};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcInheritedScheduleApi=_SiN_GetServerNpcInheritedScheduleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcInheritedScheduleApi* __cdecl SiN_GetServerNpcInheritedScheduleApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcInheritedScheduleApi)?&api:nullptr;
}
