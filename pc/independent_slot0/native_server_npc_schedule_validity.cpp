#include "native_server_npc_schedule_validity.h"
#include <cmath>
#include <cstring>
#include <initializer_list>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcBehaviorSelectionApi* __cdecl
SiN_GetServerNpcBehaviorSelectionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl validate(void* actor,unsigned bytes,float now,const ServerNpcScheduleValidityBindings* b,ServerNpcScheduleValidityResult* out) noexcept {
    if(!out)return 0;*out={};out->first_bit=out->first_global=-1;
    if(!actor||bytes<0xbf4||!b||!std::isfinite(now))return 0;
    const auto token=read<std::uint32_t>(actor,0x798);
    if(!token){out->reason=ScheduleMissing;return 1;}
    const void* schedule=nullptr;unsigned extent=0;
    if(!b->schedule.storage||!b->schedule.storage(b->schedule.context,token,&schedule,&extent)||!schedule||extent<0x2c)return 0;
    const int count=read<int>(schedule,8);
    if(count<0)return 0;
    if(!count){out->reason=ScheduleEmpty;return 1;}
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* behavior=SiN_GetServerNpcBehaviorSelectionApi(1,sizeof(ServerNpcBehaviorSelectionApi));
    if(!cond||!behavior)return 0;
    auto store=[&](int id,unsigned bank,bool value) noexcept {
        ServerNpcConditionResult cr{};return cond->store(actor,bytes,id,bank,value,&b->schedule.conditions,&cr)!=0;
    };
    auto test=[&](int id,unsigned& value) noexcept {
        ServerNpcConditionResult cr{};if(!cond->test(actor,bytes,id,NpcConditionCurrent,&b->schedule.conditions,&cr))return false;
        value=cr.value;return true;
    };
    std::memcpy(static_cast<unsigned char*>(actor)+0x7ec,static_cast<const unsigned char*>(schedule)+0xc,32);
    if(read<int>(actor,0x850)!=4){
        out->builder_called=1;
        unsigned yes=0;
        if(!behavior->schedule_is(actor,bytes,0x68,true,&b->schedule,&yes))return 0;
        if(!yes&&!behavior->schedule_is(actor,bytes,0x69,true,&b->schedule,&yes))return 0;
        out->burning=yes;
        if(!yes){
            if(!b->base_build||!b->base_build(b->context,actor))return 0;
            const float deadline=read<float>(actor,0x60c),speed=read<float>(actor,0x394);
            if(!std::isfinite(deadline)||!std::isfinite(speed))return 0;
            if(now<deadline)for(int id:{0x14,0x15})if(!store(id,NpcConditionInterrupt,false))return 0;
            for(int id:{0x46,0x47,0x4c,0x48})if(!store(id,NpcConditionInterrupt,true))return 0;
            if(speed==0.f){
                if(!behavior->schedule_is(actor,bytes,0x4e,true,&b->schedule,&yes))return 0;
                if(!yes&&!store(0x37,NpcConditionInterrupt,true))return 0;
            }
            bool mobile=false;if(!b->mobility||!b->mobility(b->context,1.f,&mobile))return 0;
            if(mobile){if(!test(0x4d,yes))return 0;if(yes&&!store(0x4d,NpcConditionCurrent,true))return 0;}
        }
        if(b->jetpack){if(!store(0x47,NpcConditionInterrupt,false))return 0;out->jetpack_clear=1;}
    }
    if(!store(0x3f,NpcConditionInterrupt,true))return 0;
    for(unsigned i=0;i<8;++i){
        unsigned overlap=read<unsigned>(actor,0x7cc+4*i)&read<unsigned>(actor,0x7ec+4*i);
        for(unsigned j=0;j<32;++j)if(overlap&(1u<<j)){
            if(!out->matched){out->first_bit=static_cast<int>(32*i+j);out->first_global=1000000000+out->first_bit;}
            ++out->matched;
        }
    }
    if(out->matched){
        out->reason=ScheduleInterrupted;
        if(b->record_interrupt){
            write(actor,0xbf0,read<std::uint32_t>(actor,0x798));write(actor,0xbec,0u);
            std::uint32_t name=0;
            if(!b->name_token||!b->name_token(b->context,actor,out->first_global,&name))return 0;
            write(actor,0xbe8,name?name:b->unknown_name);out->recorded=1;
        }
        return 1;
    }
    unsigned yes=0;
    if(!test(0x22,yes))return 0;
    if(yes){out->reason=ScheduleCompleted;return 1;}
    if(!test(0x21,yes))return 0;
    if(yes){out->reason=ScheduleFailed;return 1;}
    out->reason=ScheduleValid;out->valid=1;return 1;
}
const ServerNpcScheduleValidityApi api{1,sizeof(ServerNpcScheduleValidityApi),&validate};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleValidityApi=_SiN_GetServerNpcScheduleValidityApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleValidityApi* __cdecl
SiN_GetServerNpcScheduleValidityApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcScheduleValidityApi)?&api:nullptr;
}
