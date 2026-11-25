#include "native_server_npc_state_reconsider.h"
#include <cstring>
#include <cmath>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool init(void* a,unsigned bytes,const ServerNpcStateBindings* b,ServerNpcStateResult* out) noexcept {
    if(!out)return false;*out={};out->current=out->ideal=-1;
    if(!a||bytes<0x9c8||!b||b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return false;
    out->current=read<int>(a,0x850);out->ideal=read<int>(a,0x858);return true;
}
void* enemy(void* a,const ServerNpcStateBindings& b) noexcept {
    return sin_independent::EntityHandleView(b.entities.entries,b.entities.count).Resolve(read<std::uint32_t>(a,0x8a0));
}
bool test(void* a,unsigned bytes,int id,const ServerNpcStateBindings& b,unsigned& value) noexcept {
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
    if(!api||!api->test(a,bytes,id,NpcConditionCurrent,&b.schedule.conditions,&cr))return false;
    value=cr.value;return true;
}
int __cdecl should_reconsider(void* a,unsigned bytes,const ServerNpcStateBindings* b,ServerNpcStateResult* out) noexcept {
    if(!init(a,bytes,b,out))return 0;
    if(out->ideal==7||(out->ideal==4&&out->current!=4))return 1;
    unsigned complete=0;if(!test(a,bytes,0x22,*b,complete))return 0;
    if(!complete){out->reconsider=1;return 1;}
    const auto token=read<std::uint32_t>(a,0x798);
    if(token){
        const void* s=nullptr;unsigned n=0;
        if(!b->schedule.storage||!b->schedule.storage(b->schedule.context,token,&s,&n)||!s||n<0x14)return 0;
        // Original checks fixed global bit34 of the declared mask, not mapped local ID.
        if(read<unsigned>(s,0x10)&4u){out->reconsider=1;return 1;}
    }
    out->reconsider=(out->current==3&&!enemy(a,*b))||((out->current==1||out->current==2)&&enemy(a,*b));
    return 1;
}
int __cdecl choose_ideal(void* a,unsigned bytes,const ServerNpcStateBindings* b,ServerNpcStateResult* out) noexcept {
    if(!init(a,bytes,b,out))return 0;
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!api)return 0;
    const int current=out->current;
    if(current==3){
        void* e=enemy(a,*b);unsigned lost=0;if(!test(a,bytes,0x1d,*b,lost))return 0;
        if(!e){
            if(!lost){ServerNpcConditionResult cr{};if(!api->store(a,bytes,0x44,NpcConditionCurrent,true,&b->schedule.conditions,&cr))return 0;out->patrol_set=1;}
            out->ideal=2;return 1;
        }
        if(lost){if(!b->lost_enemy||!b->lost_enemy(b->context,a,enemy(a,*b)))return 0;out->lost_enemy_called=1;}
    }
    const auto squad=read<std::uint32_t>(a,0x9c4);
    if(squad&&(current==1||current==2)){
        unsigned yes=0;if(!test(a,bytes,0x19,*b,yes))return 0;
        if(yes){if(!b->squad_enemy||!b->squad_enemy(b->context,squad,enemy(a,*b)))return 0;out->squad_called=1;}
    }
    if(current==7){out->ideal=7;return 1;}
    if(current==1||current==2||current==4){
        int result=-1;if(!b->state_query||!b->state_query(b->context,a,current,&result))return 0;
        if(result!=-1)out->ideal=result;
    }else if(current==3&&!enemy(a,*b))out->ideal=2;
    return 1;
}
int __cdecl apply(void* a,unsigned bytes,int desired,float now,const ServerNpcStateBindings* b,ServerNpcStateResult* out) noexcept {
    if(!init(a,bytes,b,out)||!std::isfinite(now))return 0;
    const int old=out->current;
    if(old!=desired)write(a,0x854,now);
    if(desired==1&&enemy(a,*b)){
        if(!b->clear_enemy||!b->clear_enemy(b->context,a))return 0;
        out->enemy_cleared=1;
    }
    const bool changed=read<int>(a,0x850)!=desired;
    write(a,0x850,desired);write(a,0x858,desired);out->ideal=desired;out->changed=changed;
    if(changed&&(!b->state_changed||!b->state_changed(b->context,a,old,desired)))return 0;
    return 1;
}
int __cdecl set_enemy(void* a,unsigned bytes,void* target,bool notify,float now,const ServerNpcEnemyStateBindings* b,ServerNpcEnemyStateResult* out) noexcept {
    if(!out)return 0;*out={};out->old_handle=out->new_handle=0xffffffffu;
    if(!a||bytes<0xc04||!b||!std::isfinite(now)||b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    auto resolve=[&](){return static_cast<void*>(registry.Resolve(read<std::uint32_t>(a,0x8a0)));};
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    ServerNpcConditionResult cr{};
    out->old_handle=read<std::uint32_t>(a,0x8a0);
    out->changed=resolve()!=target;
    if(out->changed){
        if(!b->enemy_changing||!b->enemy_changing(b->context,a))return 0;
        out->notified=1;
        const auto squad=read<std::uint32_t>(a,0x9c4);
        if(squad){
            if(!b->release_squad_slot||!b->release_squad_slot(b->context,squad,resolve(),read<int>(a,0x9cc)))return 0;
            write(a,0x9cc,-1);out->squad_released=1;
        }
        write<std::uint8_t>(a,0x8b0,0);
        if(target&&notify){
            if(!cond->store(a,bytes,0x19,NpcConditionCurrent,true,&b->conditions,&cr))return 0;
            out->new_condition_set=cr.mapped;
        }
    }
    if(target){
        if(!b->entity_handle||!b->entity_handle(b->context,target,&out->new_handle)||
           registry.Resolve(out->new_handle)!=target)return 0;
    }
    write(a,0x8a0,out->new_handle);write(a,0x8a4,now);
    write(a,0xbf8,-1.f);write(a,0xbfc,0u);write(a,0xc00,0u);out->reset_fields=1;
    if(!target){
        if(!cond->store(a,bytes,0x19,NpcConditionCurrent,false,&b->conditions,&cr))return 0;
        out->new_condition_cleared=cr.mapped;
    }
    return 1;
}
int __cdecl clear_combat_conditions(void* a,unsigned bytes,const ServerNpcConditionMapBindings* b,ServerNpcEnemyConditionResult* out) noexcept{
    if(!out)return 0;*out={};if(!a||bytes<0x7ec||!b)return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    ServerNpcConditionResult cr{};
    if(!cond->test(a,bytes,0x15,NpcConditionCurrent,b,&cr))return 0;
    out->prior_preserved=cr.value;
    constexpr int ids[]={0x14,0x15,0x16,0x17,0x27,0x28,0x29,0x2a,0x2b};
    for(int id:ids){
        if(!cond->store(a,bytes,id,NpcConditionCurrent,false,b,&cr))return 0;
        out->cleared+=cr.changed;
    }
    if(out->prior_preserved){
        if(!cond->store(a,bytes,0x15,NpcConditionCurrent,true,b,&cr))return 0;
        out->restored=cr.mapped;
    }
    return 1;
}
const ServerNpcEnemyStateApi enemy_state_api{1,sizeof(ServerNpcEnemyStateApi),&set_enemy,&clear_combat_conditions};
const ServerNpcStateApi api{1,sizeof(ServerNpcStateApi),&should_reconsider,&choose_ideal,&apply};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcStateReconsiderApi=_SiN_GetServerNpcStateReconsiderApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcStateApi* __cdecl
SiN_GetServerNpcStateReconsiderApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcStateApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyStateApi=_SiN_GetServerNpcEnemyStateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyStateApi* __cdecl
SiN_GetServerNpcEnemyStateApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyStateApi)?&enemy_state_api:nullptr;
}
