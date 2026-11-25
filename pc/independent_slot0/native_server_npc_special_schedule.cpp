#include "native_server_npc_special_schedule.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl select(void* a,unsigned n,const ServerNpcSpecialScheduleBindings* b,ServerNpcSpecialScheduleResult* out) noexcept{
    if(!out)return 0;*out={};out->schedule=-1;
    if(!a||n<0x974||!b)return 0;
    const auto& reg=b->state.entities;
    if(reg.count>sin_independent::kEntityHandleCapacity||(reg.count&&!reg.entries))return 0;
    out->state=read<int>(a,0x850);
    if(out->state!=4&&out->state!=7)return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    ServerNpcConditionResult cr{};
    if(!cond||!cond->test(a,n,0x3a,NpcConditionCurrent,&b->state.schedule.conditions,&cr))return 0;
    if(cr.value){
        out->override_condition=1;write(a,0x2a4,1.f);
        if(!b->clear_parent||!b->clear_parent(b->context,a))return 0;
        out->parent_cleared=1;out->schedule=0x4c;return 1;
    }
    if(out->state==4){
        void* script=sin_independent::EntityHandleView(reg.entries,reg.count).Resolve(read<std::uint32_t>(a,0x970));
        if(script){out->controller_live=1;out->schedule=0x36;return 1;}
        if(!b->reset_script||!b->reset_script(b->context,a))return 0;
        out->script_reset=1;out->schedule=1;return 1;
    }
    bool yes=false;
    if(!b->death_query||!b->death_query(b->context,a,&yes))return 0;
    out->death_query=1;
    if(!yes&&read<int>(a,0x8c)==2){out->schedule=0;return 1;}
    if(!b->death_prepare||!b->death_prepare(b->context,a,0,1))return 0;
    out->death_prepared=1;out->schedule=yes?0x34:0x33;return 1;
}
const ServerNpcSpecialScheduleApi api{1,sizeof(ServerNpcSpecialScheduleApi),&select};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSpecialScheduleApi=_SiN_GetServerNpcSpecialScheduleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSpecialScheduleApi* __cdecl SiN_GetServerNpcSpecialScheduleApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSpecialScheduleApi)?&api:nullptr;
}
