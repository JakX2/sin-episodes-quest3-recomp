#include "native_server_npc_state_reconsider.h"
#include <cstring>
#include <cmath>
#include <limits>
#include <initializer_list>
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
struct NativeStateRoute {
    const ServerNpcGruntNativeStateBindings& bindings;
    ServerNpcGruntNativeStateResult& out;
    float now;
};
bool native_state_kill(void*,unsigned,void*,const ServerNpcStateBindings&,const NativeStateRoute&) noexcept;
bool native_state_broadcast(std::uint32_t,void*,const ServerNpcStateBindings&,const NativeStateRoute&) noexcept;
bool native_state_clear(void*,unsigned,const ServerNpcStateBindings&,const NativeStateRoute&) noexcept;
int choose_ideal_impl(void* a,unsigned bytes,const ServerNpcStateBindings* b,ServerNpcStateResult* out,const NativeStateRoute* native=nullptr) noexcept {
    if(!init(a,bytes,b,out))return 0;
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!api)return 0;
    const int current=out->current;
    if(current==3){
        void* e=enemy(a,*b);unsigned lost=0;if(!test(a,bytes,0x1d,*b,lost))return 0;
        if(!e){
            if(!lost){ServerNpcConditionResult cr{};if(!api->store(a,bytes,0x44,NpcConditionCurrent,true,&b->schedule.conditions,&cr))return 0;out->patrol_set=1;}
            out->ideal=2;return 1;
        }
        if(lost){
            if(native){if(!native_state_kill(a,bytes,enemy(a,*b),*b,*native))return 0;}
            else if(!b->lost_enemy||!b->lost_enemy(b->context,a,enemy(a,*b)))return 0;
            out->lost_enemy_called=1;
        }
    }
    const auto squad=read<std::uint32_t>(a,0x9c4);
    if(squad&&(current==1||current==2)){
        unsigned yes=0;if(!test(a,bytes,0x19,*b,yes))return 0;
        if(yes){
            if(native){if(!native_state_broadcast(squad,enemy(a,*b),*b,*native))return 0;}
            else if(!b->squad_enemy||!b->squad_enemy(b->context,squad,enemy(a,*b)))return 0;
            out->squad_called=1;
        }
    }
    if(current==7){out->ideal=7;return 1;}
    if(current==1||current==2||current==4){
        int result=-1;if(!b->state_query||!b->state_query(b->context,a,current,&result))return 0;
        if(result!=-1)out->ideal=result;
    }else if(current==3&&!enemy(a,*b))out->ideal=2;
    return 1;
}
int __cdecl choose_ideal(void* a,unsigned bytes,const ServerNpcStateBindings* b,ServerNpcStateResult* out) noexcept{
    return choose_ideal_impl(a,bytes,b,out);
}
int apply_impl(void* a,unsigned bytes,int desired,float now,const ServerNpcStateBindings* b,ServerNpcStateResult* out,const NativeStateRoute* native=nullptr) noexcept {
    if(!init(a,bytes,b,out)||!std::isfinite(now))return 0;
    const int old=out->current;
    if(old!=desired)write(a,0x854,now);
    if(desired==1&&enemy(a,*b)){
        if(native){if(!native_state_clear(a,bytes,*b,*native))return 0;}
        else if(!b->clear_enemy||!b->clear_enemy(b->context,a))return 0;
        out->enemy_cleared=1;
    }
    const bool changed=read<int>(a,0x850)!=desired;
    write(a,0x850,desired);write(a,0x858,desired);out->ideal=desired;out->changed=changed;
    if(changed&&(!b->state_changed||!b->state_changed(b->context,a,old,desired)))return 0;
    return 1;
}
int __cdecl apply(void* a,unsigned bytes,int desired,float now,const ServerNpcStateBindings* b,ServerNpcStateResult* out) noexcept{
    return apply_impl(a,bytes,desired,now,b,out);
}
int __cdecl clear_combat_conditions(void*,unsigned,const ServerNpcConditionMapBindings*,ServerNpcEnemyConditionResult*) noexcept;
int __cdecl slots_release(std::uint32_t,void*,int,float,const ServerNpcGruntSquadSlotsBindings*,ServerNpcGruntSquadSlotsResult*) noexcept;
struct SquadReleaseRoute {
    const ServerNpcGruntSquadSlotsBindings& bindings;
    ServerNpcGruntPerceptionSlotsResult& out;
};
bool release_slot(std::uint32_t token,void* target,int slot,float now,const ServerNpcEnemyStateBindings& b,const SquadReleaseRoute* route) noexcept{
    if(!route)return b.release_squad_slot&&b.release_squad_slot(b.context,token,target,slot);
    auto bound=route->bindings;bound.entries=b.entries;bound.count=b.count;
    ++route->out.release_calls;
    return slots_release(token,target,slot,now,&bound,&route->out.last_release)!=0;
}
int set_enemy_impl(void* a,unsigned bytes,void* target,bool notify,float now,const ServerNpcEnemyStateBindings* b,ServerNpcEnemyStateResult* out,bool native_notify,const SquadReleaseRoute* slots=nullptr) noexcept {
    if(!out)return 0;*out={};out->old_handle=out->new_handle=0xffffffffu;
    if(!a||bytes<0xc04||!b||!std::isfinite(now)||b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    auto resolve=[&](){return static_cast<void*>(registry.Resolve(read<std::uint32_t>(a,0x8a0)));};
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    ServerNpcConditionResult cr{};
    out->old_handle=read<std::uint32_t>(a,0x8a0);
    out->changed=resolve()!=target;
    if(out->changed){
        if(native_notify){ServerNpcEnemyConditionResult result{};if(!clear_combat_conditions(a,bytes,&b->conditions,&result))return 0;}
        else if(!b->enemy_changing||!b->enemy_changing(b->context,a))return 0;
        out->notified=1;
        const auto squad=read<std::uint32_t>(a,0x9c4);
        if(squad){
            if(!release_slot(squad,resolve(),read<int>(a,0x9cc),now,*b,slots))return 0;
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
int __cdecl set_enemy(void* a,unsigned bytes,void* target,bool notify,float now,const ServerNpcEnemyStateBindings* b,ServerNpcEnemyStateResult* out) noexcept{
    return set_enemy_impl(a,bytes,target,notify,now,b,out,false);
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
bool memory_storage(const ServerNpcEnemyMemoryBindings* b,std::uint32_t token,unsigned minimum,void*& p,unsigned& extent) noexcept{
    p=nullptr;extent=0;
    return token&&b&&b->storage&&b->storage(b->context,token,&p,&extent)&&p&&extent>=minimum;
}
int __cdecl memory_lookup(const void* memory,unsigned bytes,std::uint32_t key,bool fallback,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryLookup* out) noexcept{
    if(!out)return 0;*out={};out->index=255;out->key=key==0x394u?0:key;
    // Constructor 2206b0b1 assigns pointer-token comparator 22068b40.
    // Do not invoke arbitrary original function pointers or inherit global comparator state.
    if(!memory||bytes<0x1c||!b||read<std::uint32_t>(memory,0)!=0x22068d70u||read<std::uint32_t>(memory,0x18)!=0x22068b40u)return 0;
    const unsigned root=read<std::uint8_t>(memory,0x10);
    const unsigned capacity=read<unsigned>(memory,8);
    void* nodes=nullptr;unsigned extent=0;
    if(root!=255&&(!capacity||!memory_storage(b,read<std::uint32_t>(memory,4),12,nodes,extent)))return 0;
    auto find=[&](std::uint32_t needle){
        bool seen[255]{};
        unsigned i=root;
        while(i!=255){
            if(i>=capacity||seen[i]||extent<12*(i+1))return false;
            seen[i]=true;++out->visited;
            const auto* node=static_cast<const unsigned char*>(nodes)+12*i;
            const auto candidate=read<std::uint32_t>(node,4);
            if(needle==candidate){out->index=i;out->matched=1;out->record=read<std::uint32_t>(node,8);return true;}
            i=read<std::uint8_t>(node,needle<candidate?0:1);
        }
        return true;
    };
    if(!find(out->key))return 0;
    // A matching node with a null record is still a match: no second search.
    if(!out->matched&&fallback){
        out->fallback_attempted=1;if(!find(0))return 0;out->fallback_used=out->matched;
    }
    return 1;
}
bool memory_record(const ServerNpcEnemyMemoryBindings* b,const ServerNpcEnemyMemoryLookup& out,unsigned minimum,void*& p) noexcept{
    unsigned extent=0;return memory_storage(b,out.record,minimum,p,extent);
}
int __cdecl memory_timestamp(const void* memory,unsigned bytes,std::uint32_t key,unsigned kind,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryLookup* out,float* stamp) noexcept{
    if(!out||!stamp)return 0;*out={};out->index=255;*stamp=0;
    constexpr unsigned offsets[]={0x1c,0x20,0x24,0x2c,0x30};
    if(kind>=5)return 0;if(!key)return 1;
    if(!memory_lookup(memory,bytes,key,true,b,out))return 0;
    if(!out->record){*stamp=-std::numeric_limits<float>::max();return 1;}
    void* record=nullptr;const unsigned offset=offsets[kind];
    if(!memory_record(b,*out,offset+4,record))return 0;
    *stamp=read<float>(record,offset);return 1;
}
int __cdecl memory_position(void* memory,unsigned bytes,std::uint32_t key,bool second,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryLookup* out,float result[3]) noexcept{
    if(!result||!out||!memory||bytes<0x3c)return 0;
    if(!memory_lookup(memory,bytes,key,true,b,out))return 0;
    const unsigned cache=second?0x30:0x24,source=second?0x10:4;
    if(out->record){
        void* record=nullptr;if(!memory_record(b,*out,source+12,record))return 0;
        unsigned char snapshot[12];std::memcpy(snapshot,static_cast<const unsigned char*>(record)+source,12);
        std::memcpy(static_cast<unsigned char*>(memory)+cache,snapshot,12);
    }
    std::memcpy(result,static_cast<const unsigned char*>(memory)+cache,12);return 1;
}
int __cdecl memory_touch(const void* memory,unsigned bytes,std::uint32_t key,float now,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryLookup* out) noexcept{
    if(!std::isfinite(now)||!memory_lookup(memory,bytes,key,true,b,out))return 0;
    if(out->record){void* record=nullptr;if(!memory_record(b,*out,0x30,record))return 0;write(record,0x2c,now);}
    return 1;
}
int __cdecl memory_recent(const void* memory,unsigned bytes,std::uint32_t key,float now,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryLookup* out,bool* recent) noexcept{
    if(!out||!recent)return 0;*out={};out->index=255;*recent=false;
    if(!std::isfinite(now))return 0;if(!key)return 1;
    if(!memory_lookup(memory,bytes,key,true,b,out))return 0;
    if(!out->record){*recent=true;return 1;}
    if(bytes<0x20)return 0;
    void* record=nullptr;if(!memory_record(b,*out,0x20,record))return 0;
    const float stamp=read<float>(record,0x1c),lifetime=read<float>(memory,0x1c);
    if(!std::isfinite(stamp)||!std::isfinite(lifetime))return 0;
    *recent=static_cast<double>(stamp)+lifetime>now;return 1;
}
bool memory_key(const ServerNpcEnemyMemoryQueryBindings& b,void* entity,std::uint32_t& key) noexcept{
    key=0;return !entity||(b.entity_key&&b.entity_key(b.context,entity,&key)&&key&&key!=0x394u);
}
int __cdecl query_last_seen(void* context,std::uint32_t token,void* entity,bool* found,float* stamp) noexcept{
    if(!found||!stamp)return 0;*found=false;*stamp=0;
    if(!context)return 0;
    if(!entity)return 1;
    const auto& b=*static_cast<const ServerNpcEnemyMemoryQueryBindings*>(context);
    void* memory=nullptr;unsigned extent=0;std::uint32_t key=0;
    if(!memory_key(b,entity,key)||!memory_storage(&b.storage,token,0x1c,memory,extent))return 0;
    ServerNpcEnemyMemoryLookup result{};
    if(!memory_timestamp(memory,extent,key,NpcMemoryStamp1c,&b.storage,&result,stamp))return 0;
    *found=result.record!=0;return 1;
}
int __cdecl query_aim_vector(void* actor,unsigned bytes,const ServerNpcEnemyMemoryQueryBindings* b,ServerNpcEnemyMemoryLookup* out,float result[3]) noexcept{
    if(!out||!result)return 0;*out={};out->index=255;result[0]=result[1]=result[2]=0;
    if(!actor||bytes<0x988||!b||b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    void* target=sin_independent::EntityHandleView(b->entries,b->count).Resolve(read<std::uint32_t>(actor,0x8a0));
    void* memory=nullptr;unsigned extent=0;std::uint32_t key=0;
    if(!memory_key(*b,target,key)||!memory_storage(&b->storage,read<std::uint32_t>(actor,0x984),0x3c,memory,extent))return 0;
    return memory_position(memory,extent,key,false,&b->storage,out,result);
}
int __cdecl grunt_relation(void*,unsigned,void*,bool,const ServerNpcGruntRelationBindings*,ServerNpcGruntRelationResult*) noexcept;
int __cdecl grunt_valid(void*,unsigned,void*,const ServerNpcGruntRelationBindings*,ServerNpcGruntValidityResult*) noexcept;
int __cdecl unreachable_enemy(void*,unsigned,void*,float,const ServerNpcUnreachableEnemyBindings*,ServerNpcUnreachableEnemyResult*) noexcept;
bool relation_grunt_class(std::uint32_t) noexcept;
int __cdecl sensed_contains(const void*,unsigned,void*,const ServerNpcSensedEntitiesBindings*,bool*) noexcept;
int best_enemy_impl(void* actor,unsigned bytes,float now,const ServerNpcBestEnemyBindings* b,ServerNpcBestEnemyResult* out,const ServerNpcGruntRelationBindings* rel,const ServerNpcUnreachableEnemyBindings* cache,ServerNpcGruntBestEnemyResult* stats,const ServerNpcSensedEntitiesBindings* senses=nullptr) noexcept{
    if(!out)return 0;*out={};out->priority=-1000;out->distance_squared=0x10000000;out->unreachable=1;
    if(!actor||bytes<0xc1c||!b||!std::isfinite(now)||b->memory.count>sin_independent::kEntityHandleCapacity||(b->memory.count&&!b->memory.entries))return 0;
    const auto component=read<unsigned>(actor,0xc18);
    if(component){out->component_called=1;return b->component_best&&b->component_best(b->context,component,&out->enemy);}
    const auto registry=sin_independent::EntityHandleView(b->memory.entries,b->memory.count);
    void* memory=nullptr;unsigned memory_bytes=0;
    if(!memory_storage(&b->memory.storage,read<unsigned>(actor,0x984),0x1c,memory,memory_bytes)||
       read<unsigned>(memory,0)!=0x22068d70u||read<unsigned>(memory,0x18)!=0x22068b40u)return 0;
    const auto capacity=read<unsigned>(memory,8);unsigned index=read<std::uint8_t>(memory,0x10);
    if(index==255)return 1;
    void* nodes=nullptr;unsigned node_bytes=0;
    if(!capacity||!memory_storage(&b->memory.storage,read<unsigned>(memory,4),12,nodes,node_bytes))return 0;
    unsigned stack[255]{},depth=0;bool seen[255]{};
    auto node=[&](unsigned i){return static_cast<const unsigned char*>(nodes)+12*i;};
    auto query_bool=[&](auto fn,void* entity,bool& value){return fn&&fn(b->context,actor,entity,&value);};
    auto relation_value=[&](void* entity,bool disposition,int& value){
        if(!rel){auto fn=disposition?b->disposition:b->priority;return fn&&fn(b->context,actor,entity,&value);}
        if(disposition)++stats->disposition_queries;else ++stats->priority_queries;
        ServerNpcGruntRelationResult rr{};
        if(!grunt_relation(actor,bytes,entity,disposition,rel,&rr))return false;
        value=disposition?rr.disposition:rr.priority;return true;
    };
    auto priority=[&](void* entity,int& value){return relation_value(entity,false,value);};
    auto valid=[&](void* entity,bool& value){
        if(!rel)return query_bool(b->valid,entity,value)!=0;
        ++stats->validity_queries;ServerNpcGruntValidityResult vr{};
        if(!grunt_valid(actor,bytes,entity,rel,&vr))return false;value=vr.valid!=0;return true;
    };
    auto unreachable_query=[&](void* entity,bool& value){
        if(!cache)return query_bool(b->unreachable,entity,value)!=0;
        ++stats->cache_queries;ServerNpcUnreachableEnemyResult ur{};
        if(!unreachable_enemy(actor,bytes,entity,now,cache,&ur))return false;value=ur.unreachable!=0;return true;
    };
    auto visibility=[&](void* entity,bool& value){
        if(senses){
            void* owner=nullptr;unsigned n=0;
            if(!memory_storage(&senses->storage,read<unsigned>(actor,0x898),0x60,owner,n)||!sensed_contains(owner,n,entity,senses,&value))return false;
        }else if(!b->sensed||!b->sensed(b->context,read<unsigned>(actor,0x898),entity,&value))return false;
        return value||(b->visible&&b->visible(b->context,actor,entity,0x4081,&value));
    };
    auto distance=[&](void* entity,int& value){
        float p[3]{},q[3]{};
        if(!b->origin||!b->origin(b->context,actor,p)||!b->origin(b->context,entity,q))return false;
        double sum=0;for(unsigned k=0;k<3;++k){
            if(!std::isfinite(p[k])||!std::isfinite(q[k]))return false;
            const float d=static_cast<float>(static_cast<double>(q[k])-p[k]);
            if(!std::isfinite(d))return false;sum+=static_cast<double>(d)*d;
        }
        // x87 FISTP with truncate control; out-of-range integer indefinite.
        value=sum>=2147483648.0?std::numeric_limits<int>::min():static_cast<int>(sum);return true;
    };
    while(index!=255||depth){
        while(index!=255){
            if(index>=capacity||seen[index]||node_bytes<12*(index+1)||depth>=255)return 0;
            seen[index]=true;stack[depth++]=index;index=read<std::uint8_t>(node(index),0);
        }
        const auto i=stack[--depth];const auto* nd=node(i);index=read<std::uint8_t>(nd,1);
        ++out->visited;
        void* record=nullptr;unsigned record_bytes=0;
        if(!memory_storage(&b->memory.storage,read<unsigned>(nd,8),0x37,record,record_bytes))return 0;
        void* entity=registry.Resolve(read<unsigned>(record,0));
        if(!entity){++out->stale;continue;}
        unsigned extent=0;if(!b->extent||!b->extent(b->context,entity,&extent)||extent<0xc0)return 0;
        if(read<int>(entity,0x8c)!=0){++out->dead;continue;}
        if(read<unsigned>(entity,0xbc)&0x8000u){++out->disposition_rejected;continue;}
        int disp=0;if(!relation_value(entity,true,disp))return 0;
        if(disp!=1){if(!relation_value(entity,true,disp))return 0;if(disp!=2){++out->disposition_rejected;continue;}}
        bool yes=false;if(!valid(entity,yes))return 0;if(!yes){++out->invalid;continue;}
        const float cutoff=read<float>(actor,0x8c4),stamp=read<float>(record,0x1c),valid_after=read<float>(record,0x28);
        if(!std::isfinite(cutoff)||!std::isfinite(stamp)||!std::isfinite(valid_after))return 0;
        if(cutoff>0&&stamp<cutoff){++out->old;continue;}
        if(valid_after>now){++out->not_yet_valid;continue;}
        unsigned key=0;if(!memory_key(b->memory,entity,key))return 0;
        ServerNpcEnemyMemoryLookup lookup{};
        if(!memory_lookup(memory,memory_bytes,key,false,&b->memory.storage,&lookup))return 0;
        if(lookup.record){void* rec=nullptr;if(!memory_record(&b->memory.storage,lookup,0x36,rec))return 0;
            if(read<std::uint8_t>(rec,0x35)){++out->eluded;continue;}}
        if(!relation_value(entity,true,disp))return 0;
        if(disp==2&&!read<std::uint8_t>(record,0x36)){
            void* current=registry.Resolve(read<unsigned>(actor,0x8a0));unsigned current_key=0;float last=0;
            if(!memory_key(b->memory,current,current_key)||!memory_timestamp(memory,memory_bytes,current_key,NpcMemoryStamp20,&b->memory.storage,&lookup,&last))return 0;
            if(last==-std::numeric_limits<float>::max()){++out->feared;continue;}
        }
        bool unreachable=false;if(!unreachable_query(entity,unreachable))return 0;
        if(!out->unreachable&&unreachable){++out->unreachable_rejected;continue;}
        int rank=0,dist=0;bool update_distance=true;
        if(out->unreachable&&!unreachable){
            if(!visibility(entity,yes)||!priority(entity,rank)||!distance(entity,dist))return 0;
            out->visible=yes;out->unreachable=0;
        }else{
            if(!priority(entity,rank))return 0;
            if(rank>out->priority){
                // Retail priority upgrade retains the previous visibility cache.
                if(!priority(entity,rank)||!distance(entity,dist))return 0;
            }else{
                if(!priority(entity,rank))return 0;
                if(rank!=out->priority)continue;
                if(!distance(entity,dist))return 0;
                const bool closer=dist<out->distance_squared;
                if(!closer&&out->visible)continue;
                if(!visibility(entity,yes))return 0;
                if(closer?(!yes&&out->visible):!yes)continue;
                // Retail 2203d925 writes visibility to the unreachable temporary,
                // 2203d929 writes distance to the priority temporary; both are overwritten.
                // Keep the comparison caches at esp+11 and esp+1c unchanged.
                update_distance=false;
                if(!priority(entity,rank))return 0;
            }
            out->unreachable=unreachable;
        }
        out->enemy=entity;out->priority=rank;if(update_distance)out->distance_squared=dist;++out->replaced;
    }
    return 1;
}
int __cdecl best_enemy(void* actor,unsigned bytes,float now,const ServerNpcBestEnemyBindings* b,ServerNpcBestEnemyResult* out) noexcept{
    return best_enemy_impl(actor,bytes,now,b,out,nullptr,nullptr,nullptr);
}
int grunt_best_enemy_impl(void* actor,unsigned bytes,float now,const ServerNpcBestEnemyBindings* b,const ServerNpcGruntRelationBindings* rel,const ServerNpcUnreachableEnemyBindings* cache,ServerNpcGruntBestEnemyResult* out,const ServerNpcSensedEntitiesBindings* senses) noexcept{
    if(!out)return 0;*out={};
    if(!b||!rel||!cache||bytes<0xdb2||!relation_grunt_class(rel->original_vtable))return 0;
    auto rb=*rel;auto cb=*cache;
    rb.entries=cb.entries=b->memory.entries;rb.count=cb.count=b->memory.count;
    return best_enemy_impl(actor,bytes,now,b,&out->selection,&rb,&cb,out,senses);
}
int __cdecl grunt_best_enemy(void* actor,unsigned bytes,float now,const ServerNpcBestEnemyBindings* b,const ServerNpcGruntRelationBindings* rel,const ServerNpcUnreachableEnemyBindings* cache,ServerNpcGruntBestEnemyResult* out) noexcept{
    return grunt_best_enemy_impl(actor,bytes,now,b,rel,cache,out,nullptr);
}
int __cdecl unreachable_enemy(void* actor,unsigned bytes,void* target,float now,const ServerNpcUnreachableEnemyBindings* b,ServerNpcUnreachableEnemyResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0x924||!target||!b||!std::isfinite(now)||b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    int count=read<int>(actor,0x920);out->initial_count=out->final_count=count;
    if(count<0)return 0;if(!count)return 1;
    void* records=nullptr;unsigned extent=0;
    if(!memory_storage(&b->storage,read<unsigned>(actor,0x914),20,records,extent)||static_cast<unsigned>(count)>extent/20)return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    auto row=[&](int i){return static_cast<unsigned char*>(records)+20*static_cast<unsigned>(i);};
    auto remove=[&](int i){
        // Original copies all five dwords from the last active element.
        std::memmove(row(i),row(count-1),20);--count;write(actor,0x920,count);out->final_count=count;
    };
    for(int i=count-1;i>=0;--i){
        ++out->visited;auto* r=row(i);void* entity=registry.Resolve(read<unsigned>(r,0));
        if(!entity){remove(i);++out->stale_removed;continue;}
        if(entity!=target)continue;
        const float expiry=read<float>(r,4);if(!std::isfinite(expiry))return 0;
        if(now>expiry){remove(i);++out->expired_removed;return 1;}
        float origin[3]{};if(!b->origin||!b->origin(b->context,target,origin))return 0;
        out->origin_called=1;double sum=0;
        for(unsigned j=0;j<3;++j){
            const float prior=read<float>(r,8+4*j);if(!std::isfinite(prior)||!std::isfinite(origin[j]))return 0;
            const double delta=static_cast<double>(origin[j])-prior;sum+=delta*delta;
        }
        out->distance_squared=sum;
        if(sum<=14400.0){out->unreachable=1;return 1;}
        remove(i);++out->moved_removed;return 1;
    }
    return 1;
}
int __cdecl query_best_enemy(void* context,void* actor,std::uint32_t component,void** result) noexcept{
    if(!result)return 0;*result=nullptr;
    if(!context)return 0;
    auto& q=*static_cast<ServerNpcBestEnemyQueryContext*>(context);q.last={};
    if(!actor||q.actor_bytes<0xc1c||component!=read<unsigned>(actor,0xc18))return 0;
    if(!best_enemy(actor,q.actor_bytes,q.now,q.bindings,&q.last))return 0;
    *result=q.last.enemy;return 1;
}
bool finite_vec(const float* v) noexcept{return v&&std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
int __cdecl initialize_memory_record(void* record,unsigned bytes,std::uint32_t handle,const float initial[3],const float position[3],float now,float lifetime,float delay,bool seen,ServerNpcEnemyMemoryRecordResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!record||bytes<0x38||!finite_vec(initial)||!finite_vec(position)||!std::isfinite(now)||!std::isfinite(lifetime)||!std::isfinite(delay))return 0;
    const float stamp=seen?now:static_cast<float>(static_cast<double>(now)-(static_cast<double>(lifetime)+.01));
    const float eligibility=delay>0?static_cast<float>(static_cast<double>(now)+delay):0.f;
    if(!std::isfinite(stamp)||!std::isfinite(eligibility))return 0;
    // The original constructor writes every field except padding at +37.
    float pos[3],fallback[3];std::memcpy(pos,position,12);std::memcpy(fallback,initial,12);
    write(record,0,handle);std::memcpy(static_cast<unsigned char*>(record)+4,pos,12);
    std::memcpy(static_cast<unsigned char*>(record)+0x10,fallback,12);
    write(record,0x1c,stamp);write(record,0x20,stamp);write(record,0x24,stamp);
    write(record,0x28,eligibility);write(record,0x2c,0.f);
    write(record,0x30,seen?now:-std::numeric_limits<float>::max());
    write<std::uint8_t>(record,0x34,handle==0xffffffffu);write<std::uint8_t>(record,0x35,0);write<std::uint8_t>(record,0x36,0);
    out->initialized=out->position_written=out->changed_stamp_written=1;
    out->seen_written=out->first_seen_written=seen;return 1;
}
int __cdecl update_memory_record(void* record,unsigned bytes,const float position[3],float now,bool seen,ServerNpcEnemyMemoryRecordResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!record||bytes<0x38||!finite_vec(position)||!std::isfinite(now))return 0;
    float pos[3];std::memcpy(pos,position,12);
    double sq[3]{};
    for(unsigned i=0;i<3;++i){
        const float before=read<float>(record,4+4*i);if(!std::isfinite(before))return 0;
        const float delta=static_cast<float>(static_cast<double>(before)-pos[i]);
        if(!std::isfinite(delta))return 0;sq[i]=static_cast<double>(delta)*delta;
    }
    const float stamp=seen?now:read<float>(record,0x1c);
    const float first=read<float>(record,0x30);
    if(!std::isfinite(stamp)||!std::isfinite(first))return 0;
    if(seen){write(record,0x1c,now);out->seen_written=1;}
    write<std::uint8_t>(record,0x35,0);out->eluded_cleared=1;
    const double sum=(sq[2]+sq[1])+sq[0];out->distance_squared=sum;
    if(sum>57600.0||(sum>2304.0&&static_cast<double>(now)-stamp>4.0)){
        write(record,0x24,now);out->changed_stamp_written=1;
    }
    if(sum>144.0){std::memcpy(static_cast<unsigned char*>(record)+4,pos,12);out->position_written=1;}
    if(seen&&first==-std::numeric_limits<float>::max()){write(record,0x30,now);out->first_seen_written=1;}
    return 1;
}
int __cdecl memory_expiry(const void* record,unsigned bytes,bool live,bool npc,int state,float now,float lifetime,ServerNpcEnemyMemoryExpiryResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!record||bytes<0x37||!std::isfinite(now)||!std::isfinite(lifetime))return 0;
    if(!live&&!read<std::uint8_t>(record,0x34)){out->stale=out->remove=1;return 1;}
    if(live&&npc&&state==7){out->dead=out->remove=1;return 1;}
    if(read<std::uint8_t>(record,0x36)){out->protected_record=1;return 1;}
    const float stamp=read<float>(record,0x1c);if(!std::isfinite(stamp))return 0;
    out->expired=static_cast<double>(stamp)+lifetime<now;out->remove=out->expired;return 1;
}
int __cdecl maintain_memory(void* memory,unsigned bytes,float now,const ServerNpcEnemyMemoryMaintenanceBindings* b,ServerNpcEnemyMemoryMaintenanceResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!memory||bytes<0x24||!b||!std::isfinite(now)||b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    if(read<unsigned>(memory,0)!=0x22068d70u||read<unsigned>(memory,0x18)!=0x22068b40u)return 0;
    const auto capacity=read<unsigned>(memory,8);unsigned i=read<std::uint8_t>(memory,0x10);
    if(i==255)return 1;
    void* nodes=nullptr;unsigned extent=0;
    if(!capacity||!memory_storage(&b->storage,read<unsigned>(memory,4),12,nodes,extent))return 0;
    // Preflight traversal: equivalent in-order successor sequence for stable keys/indices.
    unsigned order[255]{},stack[255]{},depth=0,count=0;bool seen[255]{};
    auto node=[&](unsigned index){return static_cast<unsigned char*>(nodes)+12*index;};
    while(i!=255||depth){
        while(i!=255){
            if(i>=capacity||seen[i]||extent<12*(i+1)||depth>=255)return 0;
            seen[i]=true;stack[depth++]=i;i=read<std::uint8_t>(node(i),0);
        }
        i=stack[--depth];order[count++]=i;i=read<std::uint8_t>(node(i),1);
    }
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    const float recent=read<float>(memory,0x1c),lifetime=read<float>(memory,0x20);
    if(!std::isfinite(recent)||!std::isfinite(lifetime))return 0;
    for(unsigned k=0;k<count;++k){
        const unsigned index=order[k];++out->visited;
        const auto token=read<unsigned>(node(index),8);
        void* record=nullptr;unsigned n=0;
        if(!memory_storage(&b->storage,token,0x37,record,n))return 0;
        void* entity=registry.Resolve(read<unsigned>(record,0));bool npc=false;int state=0;
        if(entity&&(!b->npc_state||!b->npc_state(b->context,entity,&npc,&state)))return 0;
        ServerNpcEnemyMemoryExpiryResult er{};
        if(!memory_expiry(record,n,entity!=nullptr,npc,state,now,lifetime,&er))return 0;
        if(er.remove){
            if(!b->erase||!b->erase(b->context,memory,bytes,index,token))return 0;
            ++out->removed;out->stale+=er.stale;out->dead+=er.dead;out->expired+=er.expired;continue;
        }
        entity=registry.Resolve(read<unsigned>(record,0));if(!entity)continue;
        unsigned entity_bytes=0;
        if(!b->extent||!b->extent(b->context,entity,&entity_bytes)||entity_bytes<0xc0)return 0;
        const float stamp=read<float>(record,0x1c);if(!std::isfinite(stamp))return 0;
        auto refresh=[&](unsigned off){
            float pos[3]{};++out->origin_calls;
            if(!b->origin||!b->origin(b->context,entity,pos)||!finite_vec(pos))return false;
            std::memcpy(static_cast<unsigned char*>(record)+off,pos,12);return true;
        };
        if(static_cast<double>(stamp)+recent>=now&&!(read<unsigned>(entity,0xbc)&0x8000u)){
            if(!refresh(4))return 0;++out->position_refreshed;
        }
        if(now<=stamp){if(!refresh(0x10))return 0;++out->secondary_refreshed;}
    }
    return 1;
}
// Indexed red/black representation at 22069fa0/220695b0 and 2206ab90/2206a000.
// Mutate a local copy; publish only after checking every tree/free-list invariant.
struct MemoryTree {
    static constexpr unsigned nil=255;
    unsigned char data[255*12]{};
    unsigned capacity=0,root=nil,count=0,free_head=nil,high=0,rotations=0;
    unsigned char* backing=nullptr;
    unsigned char* memory=nullptr;
    unsigned char* node(unsigned i){return data+12*i;}
    unsigned edge(unsigned i,unsigned d){return i==nil?nil:node(i)[d];}
    unsigned parent(unsigned i){return edge(i,2);}
    unsigned color(unsigned i){return i==nil?1:node(i)[3];}
    void edge_set(unsigned i,unsigned d,unsigned j){if(i!=nil)node(i)[d]=static_cast<unsigned char>(j);}
    void paint(unsigned i,unsigned v){edge_set(i,3,v);}
    std::uint32_t key(unsigned i){return read<unsigned>(node(i),4);}
    unsigned record(unsigned i){return read<unsigned>(node(i),8);}
    bool open(void* m,unsigned bytes,const ServerNpcEnemyMemoryBindings* b){
        if(!m||bytes<0x1c||!b||read<unsigned>(m,0)!=0x22068d70u||read<unsigned>(m,0x18)!=0x22068b40u)return false;
        memory=static_cast<unsigned char*>(m);capacity=read<unsigned>(m,8);
        root=memory[0x10];count=memory[0x11];free_head=memory[0x12];high=memory[0x13];
        if(high>capacity||count>high)return false;
        unsigned extent=0;void* p=nullptr;
        if(capacity){
            if(!memory_storage(b,read<unsigned>(m,4),12,p,extent))return false;
            if(high>extent/12)return false;
            capacity=capacity>255?255:capacity;if(capacity>extent/12)capacity=extent/12;
            backing=static_cast<unsigned char*>(p);std::memcpy(data,backing,12*capacity);
        }
        return true;
    }
    int walk(unsigned i,unsigned p,std::uint64_t lo,std::uint64_t hi,bool* used,unsigned& active){
        if(i==nil)return 1;
        if(i>=high||used[i]||parent(i)!=p||color(i)>1)return -1;
        const std::uint64_t k=static_cast<std::uint64_t>(key(i))+1;
        if(k<=lo||k>=hi)return -1;
        used[i]=true;++active;
        const auto l=edge(i,0),r=edge(i,1);
        if((l!=nil&&l>=high)||(r!=nil&&r>=high))return -1;
        if(!color(i)&&(!color(l)||!color(r)))return -1;
        int a=walk(l,i,lo,k,used,active),b=walk(r,i,k,hi,used,active);
        return a<0||a!=b?-1:a+static_cast<int>(color(i));
    }
    bool check(unsigned& height){
        if(root!=nil&&(root>=high||color(root)!=1))return false;
        bool used[255]{};unsigned active=0,freed=0;
        int h=walk(root,nil,0,0x100000001ull,used,active);
        if(h<0||active!=count)return false;
        for(unsigned i=free_head;i!=nil;i=edge(i,1)){
            if(i>=high||used[i]||edge(i,0)!=i)return false;
            used[i]=true;++freed;
        }
        if(active+freed!=high)return false;height=static_cast<unsigned>(h);return true;
    }
    void replace_child(unsigned p,unsigned old,unsigned v){
        if(p==nil)root=v;else edge_set(p,edge(p,0)==old?0:1,v);
        edge_set(v,2,p);
    }
    void rotate(unsigned x,unsigned direction){
        const unsigned y=edge(x,1-direction),p=parent(x),inner=edge(y,direction);
        edge_set(x,1-direction,inner);edge_set(inner,2,x);
        replace_child(p,x,y);edge_set(y,direction,x);edge_set(x,2,y);++rotations;
    }
    unsigned find(std::uint32_t k){
        unsigned i=root;while(i!=nil&&key(i)!=k)i=edge(i,k<key(i)?0:1);return i;
    }
    bool insert(std::uint32_t k,std::uint32_t token,ServerNpcEnemyMemoryTreeResult& out){
        if(find(k)!=nil||!token||count==255)return false;
        unsigned z=free_head;
        if(z!=nil){free_head=edge(z,1);out.reused=1;}
        else{if(high>=capacity||high==255)return false;z=high++;}
        unsigned p=nil,i=root;while(i!=nil){p=i;i=edge(i,k<key(i)?0:1);}
        node(z)[0]=node(z)[1]=255;node(z)[2]=static_cast<unsigned char>(p);node(z)[3]=0;
        write(node(z),4,k);write(node(z),8,token);
        if(p==nil)root=z;else edge_set(p,k<key(p)?0:1,z);
        ++count;out.index=z;out.key=k;out.record=token;
        while(z!=root&&!color(parent(z))){
            p=parent(z);unsigned g=parent(p),d=edge(g,0)==p?0:1,u=edge(g,1-d);
            if(!color(u)){paint(p,1);paint(u,1);paint(g,0);z=g;}
            else{
                if(z==edge(p,1-d)){z=p;rotate(z,d);p=parent(z);g=parent(p);}
                paint(p,1);paint(g,0);rotate(g,1-d);
            }
        }
        paint(root,1);return true;
    }
    bool erase(unsigned z,ServerNpcEnemyMemoryTreeResult& out){
        if(z>=high||edge(z,0)==z||find(key(z))!=z)return false;
        out.index=z;out.key=key(z);out.record=record(z);
        unsigned y=z,old_color=color(y),x=nil,p=nil;
        if(edge(z,0)==nil){x=edge(z,1);p=parent(z);replace_child(p,z,x);}
        else if(edge(z,1)==nil){x=edge(z,0);p=parent(z);replace_child(p,z,x);}
        else{
            y=edge(z,1);while(edge(y,0)!=nil)y=edge(y,0);
            old_color=color(y);x=edge(y,1);
            if(parent(y)==z){p=y;edge_set(x,2,y);}
            else{
                p=parent(y);replace_child(p,y,x);
                edge_set(y,1,edge(z,1));edge_set(edge(y,1),2,y);
            }
            replace_child(parent(z),z,y);edge_set(y,0,edge(z,0));edge_set(edge(y,0),2,y);paint(y,color(z));
        }
        // Nil has no mutable global parent: carry the parent explicitly.
        if(old_color==1){
            while(x!=root&&color(x)==1){
                if(p==nil)return false;
                unsigned d=edge(p,0)==x?0:1,w=edge(p,1-d);
                if(!color(w)){paint(w,1);paint(p,0);rotate(p,d);w=edge(p,1-d);}
                if(color(edge(w,0))&&color(edge(w,1))){
                    paint(w,0);x=p;p=parent(x);
                }else{
                    if(color(edge(w,1-d))){paint(edge(w,d),1);paint(w,0);rotate(w,1-d);w=edge(p,1-d);}
                    paint(w,color(p));paint(p,1);paint(edge(w,1-d),1);rotate(p,d);x=root;p=nil;
                }
            }
            paint(x,1);
        }
        edge_set(z,0,z);edge_set(z,1,free_head);free_head=z;--count;return true;
    }
    void result(ServerNpcEnemyMemoryTreeResult& out,unsigned height){
        out.root=root;out.count=count;out.free_head=free_head;out.high_water=high;out.black_height=height;out.rotations=rotations;
    }
    void publish(){
        if(high)std::memcpy(backing,data,high*12);
        memory[0x10]=static_cast<unsigned char>(root);memory[0x11]=static_cast<unsigned char>(count);
        memory[0x12]=static_cast<unsigned char>(free_head);memory[0x13]=static_cast<unsigned char>(high);
        write(memory,0x14,read<unsigned>(memory,4));
    }
};
int tree_operation(void* memory,unsigned bytes,unsigned operation,unsigned value,unsigned record,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryTreeResult* out) noexcept{
    if(!out)return 0;*out={};out->index=255;MemoryTree tree;unsigned height=0;
    if(!tree.open(memory,bytes,b)||!tree.check(height))return 0;
    if(!operation){tree.result(*out,height);return 1;}
    if(operation==1?!tree.insert(value,record,*out):!tree.erase(value,*out))return 0;
    if(!tree.check(height))return 0;
    tree.publish();tree.result(*out,height);out->changed=1;return 1;
}
int __cdecl tree_validate(void* m,unsigned n,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryTreeResult* out) noexcept{return tree_operation(m,n,0,0,0,b,out);}
int __cdecl tree_insert(void* m,unsigned n,std::uint32_t k,std::uint32_t record,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryTreeResult* out) noexcept{return tree_operation(m,n,1,k,record,b,out);}
int __cdecl tree_erase(void* m,unsigned n,unsigned index,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryTreeResult* out) noexcept{return tree_operation(m,n,2,index,0,b,out);}
int __cdecl upsert_memory(void* memory,unsigned bytes,void* entity,const float position[3],float now,float delay,bool seen,const ServerNpcEnemyMemoryMutationBindings* b,ServerNpcEnemyMemoryMutationResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!memory||bytes<0x40||!b||!finite_vec(position)||!std::isfinite(now)||!std::isfinite(delay)||
       b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    if(entity&&(!b->entity_key||!b->entity_key(b->context,entity,&out->key)||!out->key||out->key==0x394u))return 0;
    ServerNpcEnemyMemoryLookup lookup{};
    if(!memory_lookup(memory,bytes,out->key,false,&b->storage,&lookup))return 0;
    out->token=lookup.record;out->creation_counter=read<unsigned>(memory,0x3c);
    if(lookup.record){
        void* record=nullptr;unsigned extent=0;
        if(!memory_storage(&b->storage,lookup.record,0x38,record,extent))return 0;
        if(!update_memory_record(record,extent,position,now,seen,&out->record))return 0;
        out->updated=1;return 1;
    }
    // A matching null node is corrupt in the unique-key portable representation.
    // Reject before allocation rather than creating the retail duplicate-key case.
    if(lookup.matched||!b->allocate||!b->release||!finite_vec(b->constructor_vector))return 0;
    MemoryTree tree;unsigned height=0;
    if(!tree.open(memory,bytes,&b->storage)||!tree.check(height)||tree.count==255||
       (tree.free_head==MemoryTree::nil&&tree.high>=tree.capacity))return 0;
    std::uint32_t handle=0xffffffffu;
    if(entity&&(!b->entity_handle||!b->entity_handle(b->context,entity,&handle)||registry.Resolve(handle)!=entity))return 0;
    const float recent=read<float>(memory,0x1c);if(!std::isfinite(recent))return 0;
    void* record=nullptr;unsigned extent=0;std::uint32_t token=0;
    if(!b->allocate(b->context,0x38,&token,&record,&extent))return 0;
    out->allocated=1;out->token=token;
    auto rollback=[&](){if(token&&b->release(b->context,token))out->released=1;return 0;};
    if(!token||!record||extent<0x38)return rollback();
    // Storage resolver must agree with allocator, so lookup cannot reach a foreign buffer.
    void* verified=nullptr;unsigned verified_extent=0;
    if(!memory_storage(&b->storage,token,0x38,verified,verified_extent)||verified!=record)return rollback();
    if(!initialize_memory_record(record,extent,handle,b->constructor_vector,position,now,recent,delay,seen,&out->record))return rollback();
    if(!tree_insert(memory,bytes,out->key,token,&b->storage,&out->tree))return rollback();
    out->creation_counter+=1u;write(memory,0x3c,out->creation_counter);out->created=1;return 1;
}
bool remove_memory_index(void* memory,unsigned bytes,unsigned index,std::uint32_t token,const ServerNpcEnemyMemoryMutationBindings& b,ServerNpcEnemyMemoryTreeResult& result,unsigned& released) noexcept{
    MemoryTree tree;unsigned height=0;
    if(!tree.open(memory,bytes,&b.storage)||!tree.check(height)||index>=tree.high||tree.edge(index,0)==index||
       tree.record(index)!=token||!token||!b.release)return false;
    result={};if(!tree.erase(index,result)||!tree.check(height))return false;
    // Precompute verified unlink, then perform retail pool release followed by publish.
    if(!b.release(b.context,token))return false;released=1;
    tree.publish();tree.result(result,height);result.changed=1;return true;
}
int __cdecl remove_memory(void* memory,unsigned bytes,std::uint32_t key,const ServerNpcEnemyMemoryMutationBindings* b,ServerNpcEnemyMemoryMutationResult* out) noexcept{
    if(!out)return 0;*out={};out->key=key;
    if(!memory||bytes<0x40||!b)return 0;
    out->creation_counter=read<unsigned>(memory,0x3c);
    // 2206bdd0 uses raw key, unlike lookup's 0x394 normalization.
    MemoryTree tree;unsigned height=0;if(!tree.open(memory,bytes,&b->storage)||!tree.check(height))return 0;
    unsigned index=tree.find(key);if(index==MemoryTree::nil)return 1;
    out->token=tree.record(index);
    if(!remove_memory_index(memory,bytes,index,out->token,*b,out->tree,out->released))return 0;
    out->removed=1;return 1;
}
int __cdecl erase_memory_record(void* context,void* memory,unsigned bytes,unsigned index,std::uint32_t token) noexcept{
    if(!context)return 0;
    ServerNpcEnemyMemoryTreeResult result{};unsigned released=0;
    return remove_memory_index(memory,bytes,index,token,*static_cast<const ServerNpcEnemyMemoryMutationBindings*>(context),result,released);
}
int __cdecl initialize_memory_owner(void* memory,unsigned bytes,std::uint32_t token,unsigned capacity,const float initial[3],const ServerNpcEnemyMemoryBindings* b) noexcept{
    if(!memory||bytes<0x40||!finite_vec(initial)||capacity>255||((token==0)!=(capacity==0)))return 0;
    if(capacity){void* nodes=nullptr;unsigned extent=0;if(!memory_storage(b,token,12*capacity,nodes,extent))return 0;}
    // Caller must supply fresh/unowned storage; this is not a destructor or reset of a live tree.
    float vector[3];std::memcpy(vector,initial,12);
    write(memory,0,0x22068d70u);write(memory,4,token);write(memory,8,capacity);write(memory,0xc,0u);
    write<std::uint8_t>(memory,0x10,255);write<std::uint8_t>(memory,0x11,0);
    write<std::uint8_t>(memory,0x12,255);write<std::uint8_t>(memory,0x13,0);
    write(memory,0x14,token);write(memory,0x18,0x22068b40u);
    write(memory,0x1c,1.75f);write(memory,0x20,60.f);
    std::memcpy(static_cast<unsigned char*>(memory)+0x24,vector,12);
    std::memcpy(static_cast<unsigned char*>(memory)+0x30,vector,12);
    write(memory,0x3c,0u);return 1;
}
int __cdecl memory_control(void* memory,unsigned bytes,std::uint32_t key,unsigned operation,float time,std::uint8_t flag,const ServerNpcEnemyMemoryBindings* b,ServerNpcEnemyMemoryControlResult* out) noexcept{
    if(!out)return 0;*out={};
    if(operation>NpcMemorySetProtected||(operation==NpcMemorySetEligibility&&!std::isfinite(time)))return 0;
    if(!memory_lookup(memory,bytes,key,false,b,&out->lookup))return 0;
    if(!out->lookup.record)return 1;
    if(operation==NpcMemoryExists){out->value=1;return 1;}
    void* record=nullptr;
    if(!memory_record(b,out->lookup,0x37,record))return 0;
    switch(operation){
        case NpcMemoryEluded:out->value=read<std::uint8_t>(record,0x35);break;
        case NpcMemoryMarkEluded:write<std::uint8_t>(record,0x35,1);out->written=1;break;
        case NpcMemorySetEligibility:write(record,0x28,time);out->written=1;break;
        case NpcMemorySetProtected:write(record,0x36,flag);out->written=1;break;
        default:return 0;
    }
    return 1;
}
bool relation_grunt_class(std::uint32_t v) noexcept{
    switch(v){case 0x2248df04:case 0x224954a4:case 0x2249aaa4:case 0x224a416c:
    case 0x2248c514:case 0x2248e83c:case 0x2249964c:case 0x224a4a4c:return true;default:return false;}
}
int __cdecl grunt_relation(void* actor,unsigned bytes,void* target,bool disposition,const ServerNpcGruntRelationBindings* b,ServerNpcGruntRelationResult* out) noexcept{
    if(!out)return 0;*out={};out->index=0xffffffffu;out->actor_class=out->target_class=-1;out->disposition=4;
    if(!actor||bytes<0xdb2||!b||!relation_grunt_class(b->original_vtable)||b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    if(!target)return !disposition||!read<std::uint8_t>(actor,0xdb1);
    int count=read<int>(actor,0x644);
    void* rows=nullptr;unsigned extent=0;
    if(count>0&&(!memory_storage(&b->storage,read<unsigned>(actor,0x638),16,rows,extent)||static_cast<unsigned>(count)>extent/16))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    auto row=[&](unsigned i){return static_cast<const unsigned char*>(rows)+16*i;};
    const void* selected=nullptr;
    for(int i=0;i<count;++i){++out->visited;if(registry.Resolve(read<unsigned>(row(i),0))==target){selected=row(i);out->source=1;out->index=i;break;}}
    auto classify=[&](void* e,int& k){++out->class_queries;return b->classify&&b->classify(b->context,e,&k);};
    if(!selected){
        if(!classify(target,out->target_class))return 0;
        if(out->target_class){
            for(int i=0;i<count;++i){
                if(!classify(target,out->target_class))return 0;
                if(out->target_class==read<int>(row(i),4)){selected=row(i);out->source=2;out->index=i;break;}
            }
        }
        if(!selected){
            if(!classify(actor,out->actor_class)||!classify(target,out->target_class))return 0;
            if(out->actor_class<0||out->actor_class>=15||out->target_class<0||out->target_class>=15)return 0;
            unsigned n=0;const void* matrix=nullptr;
            if(!b->matrix_row||!b->matrix_row(b->context,out->actor_class,&matrix,&n)||!matrix||n<15*16)return 0;
            selected=static_cast<const unsigned char*>(matrix)+16*out->target_class;out->source=3;out->index=out->target_class;
        }
    }
    out->disposition=read<int>(selected,8);out->priority=read<int>(selected,12);
    if(disposition&&read<std::uint8_t>(actor,0xdb1)){
        bool yes=false;out->overridden=out->player_called=1;
        if(!b->is_player||!b->is_player(b->context,target,&yes))return 0;
        if(!yes){out->special_called=1;if(!b->special_target||!b->special_target(b->context,target,&yes))return 0;}
        out->disposition=yes?1:4;
    }
    return 1;
}
int __cdecl grunt_valid(void* actor,unsigned bytes,void* target,const ServerNpcGruntRelationBindings* b,ServerNpcGruntValidityResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xc1c||!b||!relation_grunt_class(b->original_vtable))return 0;
    bool yes=false;const auto component=read<unsigned>(actor,0xc18);
    if(component){out->component_called=1;if(!b->component_valid||!b->component_valid(b->context,component,target,&yes))return 0;out->valid=yes;return 1;}
    void* npc=nullptr;if(!b->as_npc||!b->as_npc(b->context,target,&npc))return 0;
    if(!npc){out->valid=1;return 1;}
    out->npc_found=1;unsigned n=0;std::uint32_t vt=0;
    if(!b->npc_identity||!b->npc_identity(b->context,npc,&vt,&n))return 0;
    if(relation_grunt_class(vt)){
        if(n<0x86c)return 0;out->native_npc_gate=1;out->valid=read<int>(npc,0x868)<=1;return 1;
    }
    out->external_gate=1;
    if(!b->other_npc_gate||!b->other_npc_gate(b->context,npc,actor,&yes))return 0;out->valid=yes;return 1;
}
constexpr unsigned char original_disposition[15][15]={
    {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4},
    {4,3,3,3,4,4,1,1,3,1,1,1,4,4,4},
    {4,3,3,3,4,4,1,1,3,1,1,1,4,4,4},
    {4,3,3,3,4,1,1,1,3,1,1,1,4,4,4},
    {4,4,4,4,4,4,2,2,4,2,2,2,4,4,4},
    {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4},
    {4,1,1,1,4,4,3,3,1,1,1,3,4,4,4},
    {4,1,1,1,4,4,3,3,1,4,4,3,4,4,4},
    {4,3,3,3,4,4,1,1,3,1,1,1,4,4,4},
    {4,1,1,1,1,4,1,4,1,3,1,4,1,4,4},
    {4,1,1,1,1,4,1,4,1,1,3,1,1,4,4},
    {4,1,1,1,4,4,3,3,1,1,1,3,4,4,4},
    {4,3,4,4,4,4,4,4,4,2,2,4,4,4,4},
    {4,1,1,1,4,4,1,1,1,1,1,1,4,4,4},
    {4,1,1,1,4,4,1,1,1,1,1,1,4,4,4},
};
int __cdecl initialize_relation_matrix(ServerNpcRelationCell* cells,unsigned count) noexcept{
    if(!cells||count<225)return 0;
    for(int i=0;i<15;++i)for(int j=0;j<15;++j){
        auto& row=cells[15*i+j];
        row.handle=0xffffffffu;row.classification=0;
        row.disposition=original_disposition[i][j];
        row.priority=j==1&&(i==6||i==7||i==10)?100:0;
    }
    return 1;
}
int __cdecl set_relation_matrix(ServerNpcRelationCell* cells,unsigned count,int source,int target,int disposition,int priority) noexcept{
    if(!cells||count<225||source<0||source>=15||target<0||target>=15)return 0;
    auto& cell=cells[15*source+target];cell.disposition=disposition;cell.priority=priority;return 1;
}
int __cdecl relation_matrix_row(void* context,int source,const void** row,unsigned* bytes) noexcept{
    if(!row||!bytes)return 0;*row=nullptr;*bytes=0;
    if(!context||source<0||source>=15)return 0;
    *row=static_cast<ServerNpcRelationCell*>(context)+15*source;*bytes=240;return 1;
}
const ServerNpcRelationMatrixApi matrix_api{1,sizeof(ServerNpcRelationMatrixApi),&initialize_relation_matrix,&set_relation_matrix,&relation_matrix_row};
struct SquadReportRoute {
    const ServerNpcGruntSquadReportBindings& bindings;
    ServerNpcGruntSquadReportResult& out;
};
void reset_perception_speech(ServerNpcGruntPerceptionSpeechResult&) noexcept;
struct PerceptionSpeechRoute {
    const ServerNpcGruntPerceptionSpeechBindings& bindings;
    ServerNpcGruntPerceptionSpeechResult& out;
    const SquadReleaseRoute* slots=nullptr;
    const SquadReportRoute* reports=nullptr;
};
int __cdecl sentence_dispatch_request(void*,unsigned,float,const char*,int,unsigned,const ServerNpcGruntSentenceBindings*,const ServerNpcGruntSentenceRequestBindings*,ServerNpcGruntSentenceResult*,ServerNpcGruntSentenceRequestResult*) noexcept;
int __cdecl grunt_speak_request(void*,unsigned,unsigned,float,const ServerNpcGruntSpeechBindings*,const ServerNpcGruntSentenceBindings*,const ServerNpcGruntSentenceRequestBindings*,ServerNpcGruntRequestSpeechResult*) noexcept;
bool perception_sentence(void* a,unsigned n,float now,bool idle,unsigned vt,const sin_independent::EntityHandleEntry* entries,unsigned count,const PerceptionSpeechRoute& route) noexcept{
    auto sentence=route.bindings.sentence;sentence.speech.original_vtable=vt;sentence.entries=entries;sentence.count=count;
    auto& result=idle?route.out.idle:route.out.refind;
    auto& request=idle?route.out.idle_request:route.out.refind_request;
    ++(idle?route.out.idle_calls:route.out.refind_calls);
    return sentence_dispatch_request(a,n,now,idle?"GRUNT_IDLE":"GRUNT_REFIND_ENEMY",idle?0:2,2,&sentence,&route.bindings.request,&result,&request)!=0;
}
int grunt_memory_report_impl(void* actor,unsigned bytes,void* target,const float position[3],void* informer,float now,const ServerNpcGruntMemoryReportBindings* b,ServerNpcGruntMemoryReportResult* out,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0x9c8||!b||!relation_grunt_class(b->original_vtable))return 0;
    if(read<std::uint8_t>(actor,0xb6)&1){out->blocked=1;return 1;}
    const bool firsthand=!informer||informer==actor;out->firsthand=firsthand;
    const auto token=read<unsigned>(actor,0x984);
    if(!token){out->no_memory=out->retail_return=1;return 1;}
    if(!finite_vec(position)||!std::isfinite(now))return 0;
    auto& mb=b->mutation;void* memory=nullptr;unsigned extent=0;
    if(!memory_storage(&mb.storage,token,0x40,memory,extent))return 0;
    unsigned key=0;if(target&&(!mb.entity_key||!mb.entity_key(mb.context,target,&key)||!key||key==0x394u))return 0;
    ServerNpcEnemyMemoryControlResult cr{};
    if(!memory_control(memory,extent,key,NpcMemoryEluded,0,0,&mb.storage,&cr))return 0;
    if(cr.value){
        if(speech){
            if(!perception_sentence(actor,bytes,now,false,b->original_vtable,b->mutation.entries,b->mutation.count,*speech))return 0;
        }else if(!b->cue||!b->cue(b->context,actor,"GRUNT_REFIND_ENEMY",2,2))return 0;
        out->refind_cue=1;
    }
    const int state=read<int>(actor,0x850);
    if(firsthand)out->delay=state==2||state==3?b->alert_delay:b->idle_delay;
    if(!std::isfinite(out->delay))return 0;
    if(!upsert_memory(memory,extent,target,position,now,out->delay,firsthand,&mb,&out->mutation))return 0;
    out->retail_return=out->mutation.created;
    if(!firsthand){
        if(target&&out->retail_return&&read<int>(actor,0x850)==1){
            write(actor,0x864,0u);write(actor,0x85c,0u);out->idle_reset=1;
        }
        return 1;
    }
    const auto squad=read<unsigned>(actor,0x9c4);
    if(!target||!squad)return 1;
    void* squad_bytes=nullptr;unsigned n=0;
    if(!memory_storage(&mb.storage,squad,0x4c,squad_bytes,n)||mb.count>sin_independent::kEntityHandleCapacity||(mb.count&&!mb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    // 220a9680 forwards every firsthand report, even an existing record update.
    for(int i=0;i<read<int>(squad_bytes,0x48);++i){
        if(i>=16)return 0;
        ++out->squad_members;void* member=registry.Resolve(read<unsigned>(squad_bytes,8+4*i));
        if(member==actor)continue;
        // Retail dereferences an invalid member; bounded adapter rejects it.
        if(!member)return 0;
        if(speech&&speech->reports){
            const auto& route=*speech->reports;auto& stats=route.out;const auto& rb=route.bindings;
            unsigned vt=0,extent=0;ServerNpcGruntMemoryReportBindings bound{};
            if(!rb.member_report||!rb.member_report(rb.context,member,&vt,&extent,&bound))return 0;
            if(relation_grunt_class(vt)){
                bound.original_vtable=vt;bound.mutation.entries=mb.entries;bound.mutation.count=mb.count;
                reset_perception_speech(stats.last_dialogue);
                // A recipient gets the original sender as informer. No nested forwarding.
                const PerceptionSpeechRoute recipient{rb.perception.dialogue,stats.last_dialogue};
                ++stats.native_reports;
                if(!grunt_memory_report_impl(member,extent,target,position,actor,now,&bound,&stats.last_member,&recipient))return 0;
                stats.created+=stats.last_member.mutation.created;stats.updated+=stats.last_member.mutation.updated;stats.blocked+=stats.last_member.blocked;
            }else{
                ++stats.external_reports;
                if(!rb.other_report||!rb.other_report(rb.context,member,target,position,actor))return 0;
            }
            ++stats.forwarded;
        }else if(!b->forward||!b->forward(b->context,member,target,position,actor))return 0;
        ++out->forwarded;
    }
    return 1;
}
int __cdecl grunt_memory_report(void* actor,unsigned bytes,void* target,const float position[3],void* informer,float now,const ServerNpcGruntMemoryReportBindings* b,ServerNpcGruntMemoryReportResult* out) noexcept{
    return grunt_memory_report_impl(actor,bytes,target,position,informer,now,b,out);
}
int __cdecl grunt_enemy_choice_gate(void* actor,unsigned bytes,float now,const ServerNpcGruntEnemyChoiceGateBindings* b,ServerNpcGruntEnemyChoiceGateResult* out) noexcept{
    if(!out)return 0;*out={};out->condition=-1;
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now)||!relation_grunt_class(b->relation.original_vtable)||
       b->memory.count>sin_independent::kEntityHandleCapacity||(b->memory.count&&!b->memory.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->memory.entries,b->memory.count);
    void* target=registry.Resolve(read<unsigned>(actor,0x8a0));void* memory=nullptr;unsigned n=0;
    if(!memory_storage(&b->memory.storage,read<unsigned>(actor,0x984),0x40,memory,n))return 0;
    out->counter=read<unsigned>(memory,0x3c);
    auto choose=[&](unsigned reason){out->choose=1;out->reason=reason;return 1;};
    auto sync=[&](){write(actor,0x8c0,out->counter);out->counter_written=1;};
    if(!target){sync();return choose(NpcChoiceNoEnemy);}
    // Retail changed-counter branch returns immediately without synchronizing.
    if(read<unsigned>(actor,0x8c0)!=out->counter)return choose(NpcChoiceMemoryChanged);
    sync();
    unsigned key=0;if(!memory_key(b->memory,target,key))return 0;
    ServerNpcEnemyMemoryControlResult cr{};
    if(!memory_control(memory,n,key,NpcMemoryEluded,0,0,&b->memory.storage,&cr))return 0;
    if(cr.value)return choose(NpcChoiceEluded);
    auto relation=b->relation;relation.entries=b->memory.entries;relation.count=b->memory.count;
    ServerNpcGruntRelationResult rr{};
    if(!grunt_relation(actor,bytes,target,true,&relation,&rr))return 0;
    if(rr.disposition!=1){
        if(!grunt_relation(actor,bytes,target,true,&relation,&rr))return 0;
        if(rr.disposition!=2)return choose(NpcChoiceDisposition);
    }
    ServerNpcGruntValidityResult vr{};
    if(!grunt_valid(actor,bytes,target,&relation,&vr))return 0;
    if(!vr.valid)return choose(NpcChoiceInvalid);
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto test=[&](int id,unsigned& value){
        ServerNpcConditionResult r{};++out->condition_tests;
        if(!cond->test(actor,bytes,id,NpcConditionCurrent,&b->conditions,&r))return false;
        value=r.value;return true;
    };
    for(int id:{7,9,0x20,8}){
        unsigned yes=0;if(!test(id,yes))return 0;
        if(yes){out->condition=id;return choose(NpcChoiceCondition);}
    }
    unsigned extent=0;if(!b->extent||!b->extent(b->context,target,&extent)||extent<0x90)return 0;
    if(read<int>(target,0x8c)!=0){
        if(read<std::uint8_t>(actor,0x8b0)){
            const float deadline=read<float>(actor,0x8ac);if(!std::isfinite(deadline))return 0;
            if(static_cast<double>(now)-deadline>-.001)return choose(NpcChoiceDeathTimer);
        }else{
            float delay=read<float>(actor,0x8b4),maximum=read<float>(actor,0x8b8);
            if(!std::isfinite(delay)||!std::isfinite(maximum))return 0;
            if(maximum!=0.f&&(!b->random||!b->random(b->context,delay,maximum,&delay)))return 0;
            const float deadline=static_cast<float>(static_cast<double>(now)+delay);
            if(!std::isfinite(deadline))return 0;
            write(actor,0x8ac,deadline);write<std::uint8_t>(actor,0x8b0,1);out->death_timer_started=1;
        }
    }
    ServerNpcEnemyMemoryLookup lookup{};
    if(!memory_lookup(memory,n,key,false,&b->memory.storage,&lookup))return 0;
    void* record=nullptr;if(lookup.record&&!memory_record(&b->memory.storage,lookup,0x34,record))return 0;
    const float periodic=read<float>(actor,0x8bc);if(!std::isfinite(periodic))return 0;
    if(static_cast<double>(now)-periodic>-.001){
        const float deadline=static_cast<float>(static_cast<double>(now)+1.5);
        if(!std::isfinite(deadline))return 0;
        write(actor,0x8bc,deadline);out->periodic_timer_written=1;
        unsigned yes=0;if(!test(0x21,yes))return 0;
        if(yes){out->condition=0x21;return choose(NpcChoicePeriodicCondition);}
        if(!record){sync();return choose(NpcChoiceMissingRecord);}
        const float first=read<float>(record,0x30),last=read<float>(record,0x1c);
        if(!std::isfinite(first)||!std::isfinite(last))return 0;
        if(first==-std::numeric_limits<float>::max())return choose(NpcChoiceNeverSeen);
        if(static_cast<double>(now)-last>10.)return choose(NpcChoiceOldSight);
    }
    if(!record){sync();return choose(NpcChoiceMissingRecord);}
    const float eligibility=read<float>(record,0x28);if(!std::isfinite(eligibility))return 0;
    if(eligibility>=now){sync();return choose(NpcChoiceEligibility);}
    return 1;
}
int grunt_enemy_switch_impl(void* actor,unsigned bytes,float now,const ServerNpcGruntEnemySwitchBindings* b,ServerNpcGruntEnemySwitchResult* out,const ServerNpcSensedEntitiesBindings* senses,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now)||!relation_grunt_class(b->gate.relation.original_vtable)||
       b->best.memory.count>sin_independent::kEntityHandleCapacity||(b->best.memory.count&&!b->best.memory.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->best.memory.entries,b->best.memory.count);
    void* current=registry.Resolve(read<unsigned>(actor,0x8a0));void* candidate=current;
    out->had_enemy=out->has_enemy=current!=nullptr;out->old_flags=out->new_flags=read<unsigned>(actor,0x988);
    const bool had_flags=(out->old_flags&0x18000u)!=0,special_flag=(out->old_flags&0x10000u)!=0;
    out->forced_missing=had_flags&&!current;
    auto gate=b->gate;gate.memory=b->best.memory;gate.conditions=b->schedule.conditions;
    if(out->forced_missing)out->eluded=1;
    else if(current){
        unsigned key=0,n=0;void* memory=nullptr;ServerNpcEnemyMemoryControlResult cr{};
        if(!memory_key(gate.memory,current,key)||!memory_storage(&gate.memory.storage,read<unsigned>(actor,0x984),0x40,memory,n)||
           !memory_control(memory,n,key,NpcMemoryEluded,0,0,&gate.memory.storage,&cr))return 0;
        out->eluded=cr.value!=0;
    }
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto test_interrupt=[&](int id,unsigned& value){
        ServerNpcConditionResult r{};if(!cond->test(actor,bytes,id,NpcConditionInterrupt,&b->schedule.conditions,&r))return false;
        value=r.value;return true;
    };
    auto set_condition=[&](int id,unsigned& value){
        ServerNpcConditionResult r{};if(!cond->store(actor,bytes,id,NpcConditionCurrent,true,&b->schedule.conditions,&r))return false;
        value=r.mapped;return true;
    };
    const unsigned schedule=read<unsigned>(actor,0x798);
    if(!schedule)out->new_allowed=out->lost_allowed=1;
    else{
        const void* row=nullptr;unsigned n=0;
        if(!b->schedule.storage||!b->schedule.storage(b->schedule.context,schedule,&row,&n)||!row||n<0x10)return 0;
        if(read<int>(actor,0x7a0)==read<int>(row,8))out->new_allowed=out->lost_allowed=1;
        else{
            const auto mask=read<unsigned>(row,0xc);
            out->new_allowed=(mask>>25)&1u;out->lost_allowed=(mask>>11)&1u;
            if(!out->new_allowed&&!test_interrupt(0x19,out->new_allowed))return 0;
            if(!out->lost_allowed&&!test_interrupt(0xb,out->lost_allowed))return 0;
        }
    }
    if(!out->forced_missing&&!out->new_allowed&&!(out->lost_allowed&&out->eluded)){
        write<std::uint8_t>(actor,0x84e,1);out->deferred=1;return 1;
    }
    write<std::uint8_t>(actor,0x84e,0);
    if(!grunt_enemy_choice_gate(actor,bytes,now,&gate,&out->gate))return 0;
    if(out->gate.choose){
        if(!grunt_best_enemy_impl(actor,bytes,now,&b->best,&gate.relation,&b->cache,&out->selection,senses))return 0;
        out->selected=1;candidate=out->selection.selection.enemy;
    }
    if(candidate==current&&!out->forced_missing)return 1;
    write(actor,0x988,read<unsigned>(actor,0x988)&~0x18000u);
    if(current){
        unsigned n=0;if(!gate.extent||!gate.extent(gate.context,current,&n)||n<0x90)return 0;
        if(read<int>(current,0x8c)!=0&&!set_condition(0x1d,out->dead_condition))return 0;
    }
    auto enemy=b->enemy;enemy.entries=b->best.memory.entries;enemy.count=b->best.memory.count;enemy.conditions=b->schedule.conditions;
    if(!set_enemy_impl(actor,bytes,candidate,true,now,&enemy,&out->assignment,true,speech?speech->slots:nullptr))return 0;
    out->assigned=1;out->has_enemy=candidate!=nullptr;
    if(had_flags){
        const unsigned squad=read<unsigned>(actor,0x9c4);
        if(squad){
            if(!release_slot(squad,registry.Resolve(read<unsigned>(actor,0x8a0)),read<int>(actor,0x9cc),now,enemy,speech?speech->slots:nullptr))return 0;
            write(actor,0x9cc,-1);out->extra_squad_release=1;
        }
        write(actor,0x988,read<unsigned>(actor,0x988)&~0x20000u);
    }
    if(!candidate){
        if(out->eluded){
            if(!set_condition(0xb,out->lost_condition))return 0;
            if(speech){
                ServerNpcGruntSpeechBindings cue{};cue.memory=b->best.memory;cue.original_vtable=gate.relation.original_vtable;cue.context=speech->bindings.context;cue.random=speech->bindings.lost_random;
                ++speech->out.lost_calls;
                if(!grunt_speak_request(actor,bytes,NpcGruntLost,now,&cue,&speech->bindings.sentence,&speech->bindings.request,&speech->out.lost))return 0;
            }else if(!b->lost_cue||!b->lost_cue(b->context,actor))return 0;
            out->lost_cue=1;
        }
        if(special_flag){if(!b->fire_output||!b->fire_output(b->context,actor,0xb08))return 0;++out->outputs;}
        if(!b->fire_output||!b->fire_output(b->context,actor,0xac0))return 0;++out->outputs;
    }else{
        bool special=false;
        if(!gate.relation.special_target||!gate.relation.special_target(gate.relation.context,candidate,&special))return 0;
        write(actor,0x988,read<unsigned>(actor,0x988)|(special?0x10000u:0x8000u));
    }
    out->new_flags=read<unsigned>(actor,0x988);return 1;
}
int __cdecl grunt_enemy_switch(void* actor,unsigned bytes,float now,const ServerNpcGruntEnemySwitchBindings* b,ServerNpcGruntEnemySwitchResult* out) noexcept{
    return grunt_enemy_switch_impl(actor,bytes,now,b,out,nullptr);
}
int __cdecl sentence_dispatch(void*,unsigned,float,const char*,int,unsigned,const ServerNpcGruntSentenceBindings*,ServerNpcGruntSentenceResult*) noexcept;
int __cdecl sentence_dispatch_request(void*,unsigned,float,const char*,int,unsigned,const ServerNpcGruntSentenceBindings*,const ServerNpcGruntSentenceRequestBindings*,ServerNpcGruntSentenceResult*,ServerNpcGruntSentenceRequestResult*) noexcept;
int grunt_speak_impl(void* actor,unsigned bytes,unsigned kind,float now,const ServerNpcGruntSpeechBindings* b,ServerNpcGruntSpeechResult* out,const ServerNpcGruntSentenceBindings* sentence=nullptr,ServerNpcGruntSentenceResult* sentence_out=nullptr,const ServerNpcGruntSentenceRequestBindings* request=nullptr,ServerNpcGruntSentenceRequestResult* request_out=nullptr) noexcept{
    if(!out)return 0;*out={};out->speech_result=-1;
    if(!actor||bytes<0xcc4||!b||!relation_grunt_class(b->original_vtable)||kind>NpcGruntAlert||!std::isfinite(now))return 0;
    int priority=2;
    if(kind==NpcGruntLost){
        out->deadline=read<float>(actor,0xc88);
        if(!std::isfinite(out->deadline))return 0;
        if(now<out->deadline){out->suppressed=1;return 1;}
        if(b->memory.count>sin_independent::kEntityHandleCapacity||(b->memory.count&&!b->memory.entries))return 0;
        void* target=sin_independent::EntityHandleView(b->memory.entries,b->memory.count).Resolve(read<unsigned>(actor,0x8a0));
        out->enemy_live=target!=nullptr;out->cue="GRUNT_LOST_LONG";priority=0;
        if(target){
            bool found=false;
            auto memory=b->memory;
            if(!query_last_seen(&memory,read<unsigned>(actor,0x984),target,&found,&out->last_seen)||!std::isfinite(out->last_seen))return 0;
            out->memory_found=found;
            if(static_cast<double>(now)-out->last_seen<=10.)out->cue="GRUNT_LOST_SHORT";
        }
    }else if(kind==NpcGruntRefind)out->cue="GRUNT_REFIND_ENEMY";
    else{
        out->deadline=read<float>(actor,0xc7c);
        if(!std::isfinite(out->deadline))return 0;
        if(now<=out->deadline){out->suppressed=1;return 1;}
        out->cue="GRUNT_GO_ALERT";
    }
    if(sentence){
        auto bound=*sentence;bound.speech.original_vtable=b->original_vtable;bound.entries=b->memory.entries;bound.count=b->memory.count;
        if(request){
            if(!sentence_dispatch_request(actor,bytes,now,out->cue,priority,2,&bound,request,sentence_out,request_out))return 0;
        }else if(!sentence_dispatch(actor,bytes,now,out->cue,priority,2,&bound,sentence_out))return 0;
        out->speech_result=sentence_out->request_result;
    }else if(!b->emit||!b->emit(b->context,actor,0xcc0,out->cue,priority,2,&out->speech_result))return 0;
    out->emitted=1;
    if(kind==NpcGruntLost&&out->speech_result>=0){
        float delay=0;if(!b->random||!b->random(b->context,5.f,15.f,&delay)||!std::isfinite(delay)||delay<5.f||delay>15.f)return 0;
        out->deadline=static_cast<float>(static_cast<double>(now)+delay);
        if(!std::isfinite(out->deadline))return 0;
        write(actor,0xc88,out->deadline);out->cooldown_written=1;
    }else if(kind==NpcGruntAlert){
        out->deadline=static_cast<float>(static_cast<double>(now)+10.);
        if(!std::isfinite(out->deadline))return 0;
        write(actor,0xc7c,out->deadline);out->cooldown_written=1;
    }
    return 1;
}
int __cdecl grunt_speak(void* actor,unsigned bytes,unsigned kind,float now,const ServerNpcGruntSpeechBindings* b,ServerNpcGruntSpeechResult* out) noexcept{
    return grunt_speak_impl(actor,bytes,kind,now,b,out);
}
bool sensed_vector(const void* senses,unsigned category,const ServerNpcSensedEntitiesBindings& b,void*& handles,unsigned& count) noexcept{
    void* vec=nullptr;unsigned n=0;handles=nullptr;count=0;
    if(!memory_storage(&b.storage,read<unsigned>(senses,0x54+4*category),0x10,vec,n))return false;
    const int length=read<int>(vec,0xc);if(length<0||length>32767)return false;
    count=static_cast<unsigned>(length);
    return !count||memory_storage(&b.storage,read<unsigned>(vec,0),4*count,handles,n);
}
bool sensed_input(const void* senses,unsigned bytes,const ServerNpcSensedEntitiesBindings* b) noexcept{
    return senses&&bytes>=0x60&&b&&b->count<=sin_independent::kEntityHandleCapacity&&(!b->count||b->entries);
}
int __cdecl sensed_first(const void* senses,unsigned bytes,int category,const ServerNpcSensedEntitiesBindings* b,ServerNpcSensedCursor* cursor,ServerNpcSensedResult* out) noexcept{
    if(!cursor||!out)return 0;*out={};cursor->packed=0xffffffffu;
    if(!sensed_input(senses,bytes,b)||category< -1||category>2)return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    for(unsigned k=category<0?0:static_cast<unsigned>(category);k<3;++k){
        void* handles=nullptr;unsigned count=0;if(!sensed_vector(senses,k,*b,handles,count))return 0;
        if(!count)continue;
        // First intentionally scans later buckets even for a requested category.
        cursor->packed=(static_cast<unsigned>(category)&255u)<<24|1u<<8|k;
        out->category=k;out->index=0;out->visited=1;out->entity=registry.Resolve(read<unsigned>(handles,0));
        if(!out->entity)out->stale=out->first_stale=1;
        return 1;
    }
    return 1;
}
int __cdecl sensed_next(const void* senses,unsigned bytes,const ServerNpcSensedEntitiesBindings* b,ServerNpcSensedCursor* cursor,ServerNpcSensedResult* out) noexcept{
    if(!cursor||!out)return 0;*out={};
    if(!sensed_input(senses,bytes,b))return 0;
    if(cursor->packed==0xffffffffu)return 1;
    const unsigned category=cursor->packed&255u,mode=cursor->packed>>24,index=(cursor->packed>>8)&65535u;
    if(category>=3||(mode!=255&&mode>2)||index>32767)return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    for(unsigned k=category;k<3;++k){
        void* handles=nullptr;unsigned count=0;if(!sensed_vector(senses,k,*b,handles,count))return 0;
        for(unsigned i=k==category?index:0;i<count;++i){
            ++out->visited;void* entity=registry.Resolve(read<unsigned>(handles,4*i));
            if(!entity){++out->stale;continue;}
            cursor->packed=mode<<24|(i+1)<<8|k;out->category=k;out->index=i;out->entity=entity;return 1;
        }
        if(mode!=255)break;
    }
    cursor->packed=0xffffffffu;return 1;
}
int __cdecl sensed_contains(const void* senses,unsigned bytes,void* entity,const ServerNpcSensedEntitiesBindings* b,bool* found) noexcept{
    if(!found)return 0;*found=false;ServerNpcSensedCursor cursor{};ServerNpcSensedResult result{};
    if(!sensed_first(senses,bytes,-1,b,&cursor,&result))return 0;
    while(result.entity){if(result.entity==entity){*found=true;return 1;}if(!sensed_next(senses,bytes,b,&cursor,&result))return 0;}
    return 1;
}
int grunt_visual_gather_impl(void* actor,unsigned bytes,float now,const ServerNpcGruntVisualBindings* b,ServerNpcGruntVisualResult* out,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now)||!relation_grunt_class(b->relation.original_vtable)||
       b->senses.count>sin_independent::kEntityHandleCapacity||(b->senses.count&&!b->senses.entries))return 0;
    out->special_mode=read<std::uint8_t>(actor,0xdb1)!=0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto store=[&](int id,bool value){
        ServerNpcConditionResult r{};if(!cond->store(actor,bytes,id,NpcConditionCurrent,value,&b->conditions,&r))return false;
        if(value)out->condition_sets+=r.mapped;else out->cleared+=r.changed;return true;
    };
    for(int id:{7,9,10,8,32,31})if(!store(id,false))return 0;
    void* senses=nullptr;unsigned n=0;
    if(!memory_storage(&b->senses.storage,read<unsigned>(actor,0x898),0x60,senses,n))return 0;
    auto relation=b->relation;relation.entries=b->senses.entries;relation.count=b->senses.count;
    auto report=b->report;report.original_vtable=relation.original_vtable;report.mutation.entries=b->senses.entries;report.mutation.count=b->senses.count;
    const auto registry=sin_independent::EntityHandleView(b->senses.entries,b->senses.count);
    ServerNpcSensedCursor cursor{};ServerNpcSensedResult row{};
    if(!sensed_first(senses,n,-1,&b->senses,&cursor,&row))return 0;
    out->first_stale=row.first_stale;
    while(true){
        out->stale+=row.stale;
        void* target=row.entity;if(!target)break;
        ++out->visited;bool special=false;
        if(!relation.special_target||!relation.special_target(relation.context,target,&special))return 0;
        if(special){++out->special_seen;if(!store(31,true))return 0;}
        ServerNpcGruntRelationResult rr{};
        if(!grunt_relation(actor,bytes,target,true,&relation,&rr))return 0;
        if(rr.disposition==4)++out->neutral;
        else{
            if(target==registry.Resolve(read<unsigned>(actor,0x8a0))){++out->current_seen;if(!store(10,true))return 0;}
            const bool hostile=out->special_mode||rr.disposition==1;
            const bool feared=!hostile&&rr.disposition==2;
            if(hostile||feared){
                if(hostile){
                    ++out->hostile;
                    if(!grunt_relation(actor,bytes,target,false,&relation,&rr))return 0;
                    if(!store(rr.priority<0?9:rr.priority>10?32:7,true))return 0;
                }else ++out->feared;
                unsigned extent=0;if(!b->extent||!b->extent(b->context,target,&extent)||extent<0x2c8)return 0;
                if(read<unsigned>(target,0xb8)&0x800u){
                    if(!b->refresh_transform||!b->refresh_transform(b->context,target))return 0;++out->transforms;
                }
                float position[3];std::memcpy(position,static_cast<unsigned char*>(target)+0x2bc,12);
                ServerNpcGruntMemoryReportResult mr{};
                if(!grunt_memory_report_impl(actor,bytes,target,position,nullptr,now,&report,&mr,speech))return 0;
                ++out->report_calls;out->records_created+=mr.mutation.created;
                if(feared&&!store(8,true))return 0;
            }else if(rr.disposition==3)++out->friendly;else ++out->unknown;
        }
        if(!sensed_next(senses,n,&b->senses,&cursor,&row))return 0;
    }
    return 1;
}
int __cdecl grunt_visual_gather(void* actor,unsigned bytes,float now,const ServerNpcGruntVisualBindings* b,ServerNpcGruntVisualResult* out) noexcept{
    return grunt_visual_gather_impl(actor,bytes,now,b,out);
}
unsigned __cdecl grunt_hearing_mask() noexcept{return 0x141fu;}
int __cdecl grunt_hearing_filter(void* actor,unsigned bytes,const void* sound,unsigned n,const ServerNpcGruntHearingBindings* b,ServerNpcGruntHearingFilterResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xc1c||!sound||n<0x10||!b||!relation_grunt_class(b->original_vtable))return 0;
    const unsigned type=read<unsigned>(sound,0xc);
    if(type&0x800000u){out->accepted=out->forced=1;return 1;}
    if(type&0x4000000u){out->forced=1;return 1;}
    bool yes=false;
    if((type&4u)&&read<int>(actor,0x850)==1){
        out->visible_called=1;if(!b->sound_visible||!b->sound_visible(b->context,actor,sound,0x4081,&yes))return 0;
        if(!yes)return 1;
    }
    const auto component=read<unsigned>(actor,0xc18);
    if(component){out->component_called=1;if(!b->component_ignored||!b->component_ignored(b->context,component,sound,&yes))return 0;out->accepted=!yes;}
    else out->accepted=1;
    return 1;
}
int __cdecl grunt_hearing_gather(void* actor,unsigned bytes,float now,const ServerNpcGruntHearingBindings* b,ServerNpcGruntHearingResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xc10||!b||!relation_grunt_class(b->original_vtable)||!std::isfinite(now))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto store=[&](int id,bool value){
        ServerNpcConditionResult cr{};if(!cond->store(actor,bytes,id,NpcConditionCurrent,value,&b->schedule.conditions,&cr))return false;
        if(value)out->sets+=cr.mapped;else out->cleared+=cr.changed;return true;
    };
    auto test=[&](int id,bool& yes){ServerNpcConditionResult cr{};if(!cond->test(actor,bytes,id,NpcConditionCurrent,&b->schedule.conditions,&cr))return false;yes=cr.value!=0;return true;};
    for(int id:{48,51,52,53,49,50,55,54,56,57,35})if(!store(id,false))return 0;
    void* senses=nullptr;unsigned extent=0;
    if(!memory_storage(&b->storage,read<unsigned>(actor,0x898),0x18,senses,extent))return 0;
    int index=read<int>(senses,0x14);bool seen[64]{};
    while(index!=-1){
        if(index<0||index>=64||seen[index])return 0;seen[index]=true;
        const void* sound=nullptr;unsigned n=0;
        if(!b->sound||!b->sound(b->context,index,&sound,&n)||!sound||n<0x14)return 0;
        ++out->visited;const unsigned type=read<unsigned>(sound,0xc)&0xfffffu;
        int id=-1;
        switch(type){
            case 1:id=51;break;case 2:id=52;break;case 4:id=53;break;
            case 8:{const float deadline=read<float>(actor,0xc0c);if(!std::isfinite(deadline))return 0;
                if(now>deadline)id=48;else ++out->danger_suppressed;break;}
            case 16:id=54;break;case 0x100:id=49;break;case 0x200:id=50;break;
            case 0x400:id=55;break;case 0x800:++out->ignored;break;case 0x1000:id=56;break;case 0x2000:id=53;break;
            default:id=35;++out->unknown;break;
        }
        if(id>=0&&!store(id,true))return 0;
        if(index==0)break; // next helper treats zero cursor as terminal
        index=read<int>(sound,0x10);
    }
    bool danger=false;if(!test(48,danger))return 0;
    if(!danger&&!store(57,true))return 0;
    auto output=[&](int id,unsigned offset){
        bool yes=false;if(!test(id,yes))return false;
        if(yes){if(!b->fire_output||!b->fire_output(b->context,actor,offset))return false;++out->outputs;}return true;
    };
    if(!output(52,0xb20)||!output(53,0xb38))return 0;
    bool sound51=false,sound54=false;if(!test(51,sound51))return 0;
    if(!sound51&&!test(54,sound54))return 0;
    if(sound51||sound54||danger){
        if(!b->fire_output||!b->fire_output(b->context,actor,0xb50))return 0;++out->outputs;
    }
    if(!test(48,danger))return 0;
    bool special=false;if(danger&&!test(55,special))return 0;
    if(danger&&special&&read<unsigned>(actor,0x798)){
        ServerNpcConditionResult mapped{};
        if(!cond->map(actor,48,&b->schedule.conditions,&mapped))return 0;
        if(mapped.mapped){
            const void* sched=nullptr;unsigned n=0;const unsigned off=0xc+4*(static_cast<unsigned>(mapped.bit)/32);
            if(!b->schedule.storage||!b->schedule.storage(b->schedule.context,read<unsigned>(actor,0x798),&sched,&n)||!sched||n<off+4)return 0;
            if(read<unsigned>(sched,off)&(1u<<(mapped.bit%32))){if(!store(55,false))return 0;out->special_suppressed=1;}
        }
    }
    return 1;
}
int __cdecl grunt_sound_scan(void* actor,unsigned bytes,float now,const ServerNpcGruntSoundScanBindings* b,ServerNpcGruntSoundScanResult* out) noexcept{
    if(!out)return 0;*out={};out->accepted_head=-1;
    if(!actor||bytes<0xc1c||!b||!std::isfinite(now)||!relation_grunt_class(b->hearing.original_vtable)||
       b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    void* senses=nullptr;unsigned n=0;
    if(!memory_storage(&b->hearing.storage,read<unsigned>(actor,0x898),0x18,senses,n))return 0;
    write(senses,0x14,-1);
    if(read<std::uint8_t>(actor,0xb4)&1u)out->blocked=1;
    else{
        int index=-1;if(!b->active_head||!b->active_head(b->context,&index))return 0;
        bool seen[64]{};
        while(index!=-1){
            if(index<0||index>=64||seen[index])return 0;seen[index]=true;
            void* sound=nullptr;unsigned extent=0;
            if(!b->sound_mutable||!b->sound_mutable(b->context,index,&sound,&extent)||!sound||extent<0x2c)return 0;
            ++out->visited;const unsigned type=read<unsigned>(sound,0xc);
            if(!(type&grunt_hearing_mask()))++out->mask_rejected;
            else if(registry.Resolve(read<unsigned>(sound,0))==actor)++out->self_rejected;
            else if(read<int>(actor,0x850)==4&&(type&8u))++out->state_rejected;
            else if(read<std::uint8_t>(actor,0x968))++out->disabled_rejected;
            else{
                float eye[3];if(!b->eye_position||!b->eye_position(b->context,actor,eye)||!finite_vec(eye))return 0;
                ++out->eye_calls;double distance=0;
                for(unsigned i=0;i<3;++i){const float position=read<float>(sound,0x20+4*i);if(!std::isfinite(position))return 0;
                    const double delta=static_cast<double>(position)-eye[i];distance+=delta*delta;}
                const double radius=static_cast<float>(read<int>(sound,4)); // +50c hearing scale=1.0
                if(distance>radius*radius)++out->range_rejected;
                else{
                    ServerNpcGruntHearingFilterResult filter{};
                    if(!grunt_hearing_filter(actor,bytes,sound,extent,&b->hearing,&filter))return 0;
                    if(!filter.accepted)++out->filter_rejected;
                    else{
                        write(sound,0x10,read<int>(senses,0x14));write(senses,0x14,index);
                        out->accepted_head=index;++out->accepted;
                    }
                }
            }
            index=read<std::int16_t>(sound,0x18);
        }
    }
    return grunt_hearing_gather(actor,bytes,now,&b->hearing,&out->hearing);
}
int perception_tick_impl(void* actor,unsigned bytes,float now,const ServerNpcGruntPerceptionTickBindings* b,ServerNpcGruntPerceptionTickResult* out,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;
    // Avoid deep nested aggregate initialization on MSVC x86.
    out->visual={};out->sound={};out->maintenance={};out->enemy_switch={};
    out->rebuilt=out->categories=out->completed=0;out->radius=0;
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now))return 0;
    const auto& best=b->enemy_switch.best;
    const auto vt=b->enemy_switch.gate.relation.original_vtable;
    if(!relation_grunt_class(vt)||best.memory.count>sin_independent::kEntityHandleCapacity||(best.memory.count&&!best.memory.entries))return 0;
    auto visual=b->visual;visual.relation.original_vtable=vt;
    visual.senses.entries=best.memory.entries;visual.senses.count=best.memory.count;
    visual.conditions=b->enemy_switch.schedule.conditions;
    void* senses=nullptr;unsigned extent=0;
    if(!memory_storage(&visual.senses.storage,read<unsigned>(actor,0x898),0x60,senses,extent))return 0;
    const float raw=read<float>(senses,8),previous=read<float>(senses,0xc),time=read<float>(senses,0x10);
    if(!std::isfinite(raw)||!std::isfinite(previous)||!std::isfinite(time)||raw>=2147483648.0||raw< -2147483648.0)return 0;
    out->radius=static_cast<int>(raw);
    if(time!=now||previous!=static_cast<float>(out->radius)){
        if(!b->rebuild_category)return 0;
        for(unsigned i=0;i<3;++i){if(!b->rebuild_category(b->context,senses,i,out->radius))return 0;++out->categories;}
        write(senses,0xc,static_cast<float>(out->radius));write(senses,0x10,now);out->rebuilt=1;
    }
    // Original 220a31a0 calls +4f0 even on cache hit.
    if(!grunt_visual_gather_impl(actor,bytes,now,&visual,&out->visual,speech))return 0;
    auto sound=b->sound;sound.entries=best.memory.entries;sound.count=best.memory.count;
    sound.hearing.storage=visual.senses.storage;sound.hearing.original_vtable=vt;sound.hearing.schedule=b->enemy_switch.schedule;
    if(!grunt_sound_scan(actor,bytes,now,&sound,&out->sound))return 0;
    auto maintenance=b->maintenance;maintenance.entries=best.memory.entries;maintenance.count=best.memory.count;
    void* memory=nullptr;unsigned n=0;
    if(!memory_storage(&maintenance.storage,read<unsigned>(actor,0x984),0x40,memory,n)||!maintain_memory(memory,n,now,&maintenance,&out->maintenance))return 0;
    if(!grunt_enemy_switch_impl(actor,bytes,now,&b->enemy_switch,&out->enemy_switch,&visual.senses,speech))return 0;
    out->completed=1;return 1;
}
int __cdecl perception_tick(void* actor,unsigned bytes,float now,const ServerNpcGruntPerceptionTickBindings* b,ServerNpcGruntPerceptionTickResult* out) noexcept{
    return perception_tick_impl(actor,bytes,now,b,out);
}
int combat_visibility_query_impl(void* actor,unsigned bytes,void* target,unsigned mask,const ServerNpcGruntCombatVisibilityBindings* b,ServerNpcGruntCombatVisibilityResult* out,bool want_blocker) noexcept{
    if(!out)return 0;*out={};out->blocker=0xffffffffu;
    if(!actor||bytes<0x988||!target||!b||!relation_grunt_class(b->original_vtable)||
       b->memory.count>sin_independent::kEntityHandleCapacity||(b->memory.count&&!b->memory.entries))return 0;
    if(read<std::uint8_t>(actor,0xb6)&1u){
        unsigned key=0,n=0;void* memory=nullptr;ServerNpcEnemyMemoryControlResult cr{};
        if(!memory_key(b->memory,target,key)||!memory_storage(&b->memory.storage,read<unsigned>(actor,0x984),0x1c,memory,n)||
           !memory_control(memory,n,key,NpcMemoryEluded,0,0,&b->memory.storage,&cr))return 0;
        if(cr.value){out->suppressed=1;return 1;}
    }
    bool visible=false;void* blocker=nullptr;
    if(!b->trace||!b->trace(b->context,actor,target,mask,&visible,&blocker))return 0;
    out->traced=1;out->visible=visible;
    if(want_blocker&&!visible&&blocker){
        if(!b->entity_handle||!b->entity_handle(b->context,blocker,&out->blocker)||
           sin_independent::EntityHandleView(b->memory.entries,b->memory.count).Resolve(out->blocker)!=blocker)return 0;
    }
    return 1;
}
int __cdecl combat_visibility_query(void* actor,unsigned bytes,void* target,unsigned mask,const ServerNpcGruntCombatVisibilityBindings* b,ServerNpcGruntCombatVisibilityResult* out) noexcept{
    return combat_visibility_query_impl(actor,bytes,target,mask,b,out,true);
}
int __cdecl combat_visibility_gather(void* actor,unsigned bytes,void* target,const ServerNpcGruntCombatVisibilityBindings* b,ServerNpcGruntCombatVisibilityResult* out) noexcept{
    if(!out)return 0;*out={};out->blocker=0xffffffffu;
    if(!actor||bytes<0x990||!target||!b||!relation_grunt_class(b->original_vtable))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto store=[&](int id,bool value){
        ServerNpcConditionResult cr{};if(!cond->store(actor,bytes,id,NpcConditionCurrent,value,&b->conditions,&cr))return false;
        if(value)out->sets+=cr.mapped;else out->cleared+=cr.changed;return true;
    };
    for(int id:{0x1b,0x1c,0xe,0xc})if(!store(id,false))return 0;
    write(actor,0x98c,0xffffffffu);
    ServerNpcGruntCombatVisibilityResult query{};
    if(!combat_visibility_query(actor,bytes,target,0x4081,b,&query))return 0;
    out->suppressed=query.suppressed;out->traced=query.traced;out->visible=query.visible;out->blocker=query.blocker;
    out->previous_visible=(read<unsigned>(actor,0x988)&0x20000u)!=0;
    auto emit=[&](unsigned offset,bool value,std::uint32_t handle){
        if(!b->fire_output||!b->fire_output(b->context,actor,offset,value,handle))return false;
        ++out->outputs;return true;
    };
    auto current=[&](){return static_cast<void*>(sin_independent::EntityHandleView(b->memory.entries,b->memory.count).Resolve(read<unsigned>(actor,0x8a0)));};
    if(!out->visible){
        write(actor,0x98c,out->blocker);
        if(!store(0xc,true)||!store(0xa,false))return 0;
        if(out->previous_visible){
            void* e=current();bool special=false;
            if(!e||!b->special_target||!b->special_target(b->context,e,&special))return 0;
            if(special&&!emit(0xaf0,false,0xffffffffu))return 0;
            if(!emit(0xaa8,false,0xffffffffu))return 0;
        }
        write(actor,0x988,read<unsigned>(actor,0x988)&~0x20000u);
    }else{
        if(!store(0xe,true))return 0;
        bool yes=false;if(!b->in_view||!b->in_view(b->context,actor,target,&yes))return 0;
        out->in_view=yes;if(!store(0xa,yes))return 0;
        if(!out->previous_visible){
            void* e=current();std::uint32_t handle=0xffffffffu;bool special=false;
            if(!e||!b->entity_handle||!b->entity_handle(b->context,e,&handle)||!b->special_target||!b->special_target(b->context,e,&special))return 0;
            if(special&&!emit(0xad8,true,handle))return 0;
            if(!emit(0xa90,true,handle))return 0;
        }
        write(actor,0x988,read<unsigned>(actor,0x988)|0x20000u);
    }
    unsigned extent=0;if(!b->extent||!b->extent(b->context,target,&extent)||extent<0x90)return 0;
    if(read<int>(target,0x8c)!=0){
        out->dead=1;if(!store(0x1d,true)||!store(0xa,false)||!store(0xc,false))return 0;
    }
    return 1;
}
int __cdecl combat_distance(void* actor,unsigned bytes,void* target,const ServerNpcGruntCombatRangeBindings* b,float* result) noexcept{
    if(!result)return 0;*result=0;
    if(!actor||bytes<0x11c||!target||!b||!relation_grunt_class(b->visibility.original_vtable)||!b->center)return 0;
    unsigned n=0;if(!b->visibility.extent||!b->visibility.extent(b->visibility.context,target,&n)||n<0x11c)return 0;
    float a[3]{},e[3]{};
    if(!b->center(b->context,actor,a)||!b->center(b->context,target,e)||!finite_vec(a)||!finite_vec(e))return 0;
    float d[3]{};for(unsigned i=0;i<3;++i){d[i]=static_cast<float>(static_cast<double>(e[i])-a[i]);if(!std::isfinite(d[i]))return 0;}
    const float ah=static_cast<float>(static_cast<double>(read<float>(actor,0x118))-read<float>(actor,0x10c));
    const float eh=static_cast<float>(static_cast<double>(read<float>(target,0x118))-read<float>(target,0x10c));
    if(!std::isfinite(ah)||!std::isfinite(eh)||ah<0||eh<0)return 0;
    const double half=(static_cast<double>(ah)+eh)*.5;
    const double z=d[2]>half?d[2]-half:d[2]<-half?d[2]+half:0.;
    const float squared=static_cast<float>(static_cast<double>(d[0])*d[0]+static_cast<double>(d[1])*d[1]+z*z);
    if(!std::isfinite(squared))return 0;
    *result=std::sqrt(squared);return 1;
}
int combat_range_gather_impl(void* actor,unsigned bytes,void* target,float now,const ServerNpcGruntCombatRangeBindings* b,ServerNpcGruntCombatRangeResult* out,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xa24||!target||!b||!std::isfinite(now))return 0;
    if(!combat_visibility_gather(actor,bytes,target,&b->visibility,&out->visibility))return 0;
    if(out->visibility.dead){out->dead=1;return 1;}
    if(!combat_distance(actor,bytes,target,b,&out->distance))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto test=[&](int id,bool& yes){ServerNpcConditionResult cr{};if(!cond->test(actor,bytes,id,NpcConditionCurrent,&b->visibility.conditions,&cr))return false;yes=cr.value!=0;return true;};
    auto store=[&](int id,bool yes){ServerNpcConditionResult cr{};return cond->store(actor,bytes,id,NpcConditionCurrent,yes,&b->visibility.conditions,&cr)!=0;};
    auto report=[&](void* informer){
        auto rb=b->report;rb.original_vtable=b->visibility.original_vtable;rb.mutation.entries=b->visibility.memory.entries;rb.mutation.count=b->visibility.memory.count;
        if(!grunt_memory_report_impl(actor,bytes,target,out->observation,informer,now,&rb,&out->report,speech))return false;
        out->reported=1;out->firsthand=informer==nullptr;return true;
    };
    auto origin=[&](){return b->origin&&b->origin(b->context,target,out->observation)&&finite_vec(out->observation);};
    bool in_view=false,occluded=false;
    if(!test(0xa,in_view))return 0;
    if(in_view){
        float v[3]{};if(!b->velocity||!b->velocity(b->context,target,v)||!finite_vec(v))return 0;++out->velocity_queries;
        if(v[0]!=0||v[1]!=0||v[2]!=0){
            float scale=0;if(!b->random||!b->random(b->context,-.05f,0.f,&scale)||!std::isfinite(scale)||scale<-.05f||scale>0.f)return 0;
            if(!b->velocity(b->context,target,v)||!finite_vec(v)||!origin())return 0;++out->velocity_queries;
            for(unsigned i=0;i<3;++i){
                const float delta=static_cast<float>(static_cast<double>(v[i])*scale);
                out->observation[i]=static_cast<float>(static_cast<double>(out->observation[i])-delta);
            }
            if(!finite_vec(out->observation))return 0;
        }else if(!origin())return 0;
        if(!report(nullptr))return 0;
        bool faces=false;if(!b->enemy_facing_actor||!b->enemy_facing_actor(b->context,target,actor,&faces))return 0;
        out->facing_actor=faces;
        if(!store(0x1b,faces)||!store(0x1c,!faces))return 0;
    }else{
        if(!test(0xc,occluded))return 0;
        if(!occluded&&out->distance<=256.f){if(!origin()||!report(target))return 0;}
    }
    out->threshold=read<float>(actor,0xa20);if(!std::isfinite(out->threshold))return 0;
    const auto& mb=b->visibility.memory;const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    void* weapon=registry.Resolve(read<unsigned>(actor,0x78c));
    if(weapon&&in_view){
        unsigned n=0;if(!b->visibility.extent||!b->visibility.extent(b->visibility.context,weapon,&n)||n<0x504)return 0;
        const float range=read<float>(weapon,0x500);if(!std::isfinite(range))return 0;
        if(range>=out->threshold){out->threshold=range;out->weapon_range_used=1;}
    }
    out->too_far=out->distance>=out->threshold;if(!store(0x1a,out->too_far!=0))return 0;
    void* nav=nullptr;unsigned n=0;
    if(!memory_storage(&mb.storage,read<unsigned>(actor,0x8f8),0x10,nav,n))return 0;
    const int nav_mode=read<int>(nav,0xc);
    out->attack_gate=nav_mode!=1&&nav_mode!=3&&in_view&&!out->too_far;
    return 1;
}
int __cdecl combat_range_gather(void* actor,unsigned bytes,void* target,float now,const ServerNpcGruntCombatRangeBindings* b,ServerNpcGruntCombatRangeResult* out) noexcept{
    return combat_range_gather_impl(actor,bytes,target,now,b,out);
}
int __cdecl grunt_attack_gather(void* actor,unsigned bytes,void* target,float distance,const ServerNpcGruntAttackBindings* b,ServerNpcGruntAttackResult* out) noexcept{
    if(!out)return 0;*out={};for(auto& id:out->conditions)id=-1;out->distance=distance;
    if(!actor||bytes<0x8f0||!target||!b||!relation_grunt_class(b->original_vtable)||!std::isfinite(distance)||distance<0||
       b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    auto active=[&](){return static_cast<void*>(registry.Resolve(read<unsigned>(actor,0x78c)));};
    float origin[3]{},other[3]{},forward[3]{},point[3]{};
    auto get_origin=[&](void* e,float* v){return b->origin&&b->origin(b->context,e,v)&&finite_vec(v);};
    if(!get_origin(actor,origin)||!get_origin(target,other)||!b->forward||!b->forward(b->context,actor,forward)||!finite_vec(forward))return 0;
    const float x=static_cast<float>(static_cast<double>(other[0])-origin[0]),y=static_cast<float>(static_cast<double>(other[1])-origin[1]);
    const float len=std::sqrt(static_cast<float>(static_cast<double>(x)*x+static_cast<double>(y)*y));
    if(!std::isfinite(len))return 0;
    // Original VectorNormalize leaf is not executed here; zero direction stays zero.
    out->dot=len?static_cast<float>(static_cast<double>(x/len)*forward[0]+static_cast<double>(y/len)*forward[1]):0.f;
    if(!std::isfinite(out->dot))return 0;
    auto cleanup=[&](){ServerNpcEnemyConditionResult cr{};++out->cleanup_calls;return clear_combat_conditions(actor,bytes,&b->conditions,&cr)!=0;};
    if(!cleanup())return 0;
    out->capabilities=read<unsigned>(actor,0x8ec);
    if(void* w=active()){
        if(!b->weapon_caps||!b->weapon_caps(b->context,w,&out->weapon_capabilities))return 0;out->capabilities|=out->weapon_capabilities;
    }
    bool clear=false;
    auto line=[&](){++out->line_calls;return get_origin(actor,origin)&&b->line&&b->line(b->context,actor,origin,point,&clear);};
    if(!get_origin(actor,origin)||!b->aim_point||!b->aim_point(b->context,target,origin,point)||!finite_vec(point)||!line())return 0;
    if(!clear){
        if(!cleanup()||!b->eye||!b->eye(b->context,target,point)||!finite_vec(point)||!line())return 0;
    }
    out->line_clear=clear;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto store=[&](int id,bool value){
        ServerNpcConditionResult cr{};if(!cond->store(actor,bytes,id,NpcConditionCurrent,value,&b->conditions,&cr))return false;
        if(value)out->sets+=cr.mapped;else out->range_cleared+=cr.changed;return true;
    };
    auto set=[&](unsigned i,int id,int ready){
        out->conditions[i]=id;
        if(id==ready&&!clear){++out->suppressed;return true;}
        return store(id,true);
    };
    auto weapon=[&](void* w,unsigned i,unsigned slot,int ready){
        int id=0;++out->weapon_queries;
        return b->weapon_condition&&b->weapon_condition(b->context,w,slot,out->dot,distance,&id)&&set(i,id,ready);
    };
    auto native=[&](unsigned i,int id,int ready){++out->native_queries;return set(i,id,ready);};
    const unsigned caps=out->capabilities;
    void* w=active();
    if((caps&0x2000u)&&w){
        if(!weapon(w,0,0x4ac,0x14))return 0;
    }else if(caps&0x20000u){
        const int id=distance<64?0x24:distance>784?0x25:out->dot<.5f?0x26:0x14;
        if(!native(0,id,0x14))return 0;
    }
    w=active();unsigned current_caps=0;
    if((caps&0x4000u)&&w){
        if(!b->weapon_caps||!b->weapon_caps(b->context,w,&current_caps))return 0;
    }
    if((caps&0x4000u)&&w&&(current_caps&0x4000u)){
        if(!weapon(w,1,0x4b0,0x15))return 0;
    }else if(caps&0x40000u){if(!native(1,0,0x15))return 0;}
    w=active();
    if((caps&0x8000u)&&w){if(!weapon(w,2,0x4b4,-1))return 0;}
    else if(caps&0x80000u){
        int id=0;
        if(distance<=64.f&&static_cast<double>(out->dot)>=.7){
            void* current=registry.Resolve(read<unsigned>(actor,0x8a0));
            if(!current)return 0;
            if(!get_origin(current,other)||!get_origin(actor,origin))return 0;
            if(std::abs(static_cast<double>(other[2])-origin[2])<=64.){
                void* hit=nullptr;
                if(!b->melee_trace||!b->melee_trace(b->context,actor,current,0x46004003u,&hit))return 0;
                out->melee_traced=1;id=hit==registry.Resolve(read<unsigned>(actor,0x8a0))?0x16:0;
            }
        }
        if(!native(2,id,-1))return 0;
    }
    w=active();
    if((caps&0x10000u)&&w){if(!weapon(w,3,0x4b8,-1))return 0;}
    else if(caps&0x100000u){
        if(!native(3,distance>64?0x25:static_cast<double>(out->dot)<.7?0:0x17,-1))return 0;
    }
    bool attack=false;
    for(int id:{0x15,0x14,0x17,0x16}){
        ServerNpcConditionResult cr{};
        if(!cond->test(actor,bytes,id,NpcConditionCurrent,&b->conditions,&cr))return 0;
        if(cr.value){attack=true;break;}
    }
    if(attack)for(int id:{0x24,0x25,0x28})if(!store(id,false))return 0;
    return 1;
}
int __cdecl grunt_goal_refresh(void*,unsigned,unsigned,float,const ServerNpcGruntGoalRefreshBindings*,ServerNpcGruntGoalRefreshResult*) noexcept;
int combat_conditions_impl(void* actor,unsigned bytes,void* target,float now,const ServerNpcGruntCombatConditionsBindings* b,ServerNpcGruntCombatConditionsResult* out,const ServerNpcGruntGoalRefreshBindings* goals,ServerNpcGruntGoalRefreshResult* goal_results,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;
    out->range={};out->attack={};out->cache={};
    out->completed=out->attack_called=out->cleanup_called=out->nav_calls=out->unreachable_set=out->old_sight=out->close_mark=out->point_mark=out->point_queries=0;
    out->last_seen=out->horizontal_distance=0;for(float& v:out->remembered)v=0;
    if(!actor||bytes<0xa24||!target||!b||!std::isfinite(now))return 0;
    if(!combat_range_gather_impl(actor,bytes,target,now,&b->range,&out->range,speech))return 0;
    if(out->range.dead){out->completed=1;return 1;}
    const auto& vb=b->range.visibility;
    const auto& mb=vb.memory;
    const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    auto current=[&](){return static_cast<void*>(registry.Resolve(read<unsigned>(actor,0x8a0)));};
    if(out->range.attack_gate){
        auto ab=b->attack;ab.entries=mb.entries;ab.count=mb.count;ab.conditions=vb.conditions;ab.original_vtable=vb.original_vtable;
        void* e=current();if(!e||!grunt_attack_gather(actor,bytes,e,out->range.distance,&ab,&out->attack))return 0;
        out->attack_called=1;
    }else{
        ServerNpcEnemyConditionResult cr{};if(!clear_combat_conditions(actor,bytes,&vb.conditions,&cr))return 0;
        out->cleanup_called=1;
    }
    for(unsigned phase=0;phase<2;++phase){
        if(goals){
            auto gb=*goals;gb.memory.entries=mb.entries;gb.memory.count=mb.count;gb.original_vtable=vb.original_vtable;
            if(!grunt_goal_refresh(actor,bytes,phase,now,&gb,&goal_results[phase]))return 0;
        }else if(!b->navigation_update||!b->navigation_update(b->context,actor,phase,now))return 0;
        ++out->nav_calls;
    }
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto test=[&](int id,bool& yes){ServerNpcConditionResult cr{};if(!cond->test(actor,bytes,id,NpcConditionCurrent,&vb.conditions,&cr))return false;yes=cr.value!=0;return true;};
    void* nav=nullptr;unsigned n=0;
    if(!memory_storage(&mb.storage,read<unsigned>(actor,0x8f8),0x3b,nav,n))return 0;
    if(!read<std::uint8_t>(nav,0x3a)){
        auto cb=b->cache;cb.entries=mb.entries;cb.count=mb.count;
        void* e=current();if(!e||!unreachable_enemy(actor,bytes,e,now,&cb,&out->cache))return 0;
        if(out->cache.unreachable){
            ServerNpcConditionResult cr{};if(!cond->store(actor,bytes,0x1e,NpcConditionCurrent,true,&vb.conditions,&cr))return 0;
            out->unreachable_set=cr.mapped;
        }
    }
    void* memory=nullptr;
    if(!memory_storage(&mb.storage,read<unsigned>(actor,0x984),0x40,memory,n))return 0;
    unsigned key=0;void* e=current();if(!memory_key(mb,e,key))return 0;
    ServerNpcEnemyMemoryLookup lookup{};
    if(e){if(!memory_timestamp(memory,n,key,NpcMemoryStamp1c,&mb.storage,&lookup,&out->last_seen))return 0;}
    else out->last_seen=0; // original 2206b450 null input returns zero
    if(!std::isfinite(out->last_seen))return 0;
    if(static_cast<double>(now)-out->last_seen<=8.){out->completed=1;return 1;}
    out->old_sight=1;
    if(!memory_position(memory,n,key,false,&mb.storage,&lookup,out->remembered)||!finite_vec(out->remembered))return 0;
    float origin[3]{};
    if(!b->range.origin||!b->range.origin(b->range.context,actor,origin)||!finite_vec(origin))return 0;
    float dx=static_cast<float>(static_cast<double>(out->remembered[0])-origin[0]),dy=static_cast<float>(static_cast<double>(out->remembered[1])-origin[1]);
    const float sq=static_cast<float>(static_cast<double>(dx)*dx+static_cast<double>(dy)*dy);
    if(!std::isfinite(sq))return 0;out->horizontal_distance=std::sqrt(sq);
    auto mark=[&](unsigned& wrote){
        unsigned current_key=0;if(!memory_key(mb,current(),current_key))return false;
        ServerNpcEnemyMemoryControlResult cr{};
        if(!memory_control(memory,n,current_key,NpcMemoryMarkEluded,0,0,&mb.storage,&cr))return false;
        wrote=cr.written;return true;
    };
    bool seen=false;
    if(out->horizontal_distance<48.f){
        if(!test(0xa,seen))return 0;
        if(!seen&&!mark(out->close_mark))return 0;
    }
    if(!test(0xa,seen))return 0;
    if(!seen){
        bool unreachable=false;if(!test(0x1e,unreachable))return 0;
        if(unreachable){
            bool visible=false;++out->point_queries;
            if(!b->point_visible||!b->point_visible(b->context,actor,out->remembered,0x4081,&visible))return 0;
            if(!visible&&!mark(out->point_mark))return 0;
        }
    }
    out->completed=1;return 1;
}
int __cdecl grunt_combat_conditions(void* a,unsigned n,void* e,float now,const ServerNpcGruntCombatConditionsBindings* b,ServerNpcGruntCombatConditionsResult* out) noexcept{
    return combat_conditions_impl(a,n,e,now,b,out,nullptr,nullptr);
}
int __cdecl grunt_combat_goal(void* a,unsigned n,void* e,float now,const ServerNpcGruntCombatConditionsBindings* b,const ServerNpcGruntGoalRefreshBindings* goals,ServerNpcGruntCombatConditionsResult* out,ServerNpcGruntGoalRefreshResult* results) noexcept{
    if(!goals||!results)return 0;results[0]={};results[1]={};
    return combat_conditions_impl(a,n,e,now,b,out,goals,results);
}
const ServerNpcGruntCombatGoalApi combat_goal_api{1,sizeof(ServerNpcGruntCombatGoalApi),&grunt_combat_goal};
bool goal_vector_distance(const float* a,const float* b,float& distance) noexcept{
    if(!finite_vec(a)||!finite_vec(b))return false;
    double squared=0;for(unsigned i=0;i<3;++i){const float d=static_cast<float>(static_cast<double>(a[i])-b[i]);squared+=static_cast<double>(d)*d;}
    const float sq=static_cast<float>(squared);if(!std::isfinite(sq))return false;distance=std::sqrt(sq);return true;
}
int __cdecl grunt_goal_refresh(void* actor,unsigned bytes,unsigned phase,float now,const ServerNpcGruntGoalRefreshBindings* b,ServerNpcGruntGoalRefreshResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0x988||phase>1||!b||!std::isfinite(now)||!relation_grunt_class(b->original_vtable)||
       b->memory.count>sin_independent::kEntityHandleCapacity||(b->memory.count&&!b->memory.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->memory.entries,b->memory.count);
    const auto nav_token=read<unsigned>(actor,0x8f8);void* nav=nullptr;unsigned n=0;
    if(!memory_storage(&b->memory.storage,nav_token,0x28,nav,n))return 0;
    const int mode=read<int>(nav,0xc);
    if(mode==1||mode==3){out->skipped_mode=1;return 1;}
    if(!phase){
        const float local=read<float>(actor,0x8c8);
        if(!b->global_deadline||!std::isfinite(*b->global_deadline)||!std::isfinite(local))return 0;
        if(static_cast<double>(now)-*b->global_deadline<=-.001||static_cast<double>(now)-local<=-.001){out->skipped_timer=1;return 1;}
    }
    void* path=nullptr;if(!memory_storage(&b->memory.storage,read<unsigned>(nav,0x24),0x6c,path,n))return 0;
    out->goal_type=read<unsigned>(path,0x44);out->goal_flags=read<unsigned>(path,0x48);
    if(out->goal_type!=(phase?1u:2u)){out->skipped_type=1;return 1;}
    void* target=registry.Resolve(read<unsigned>(actor,phase?0x8a8:0x8a0));
    void* goal=registry.Resolve(read<unsigned>(path,0x10));
    if(target!=goal){
        if(!b->retarget||!b->retarget(b->context,nav_token,target))return 0;out->retargeted=1;return 1;
    }
    if(phase&&!(out->goal_flags&2u))return 1;
    auto refresh=[&](){
        bool success=false;
        if(!phase){
            const float deadline=static_cast<float>(static_cast<double>(now)+.1f);
            if(!std::isfinite(deadline))return false;*b->global_deadline=deadline;out->deadline_written=1;
        }
        if(!b->refresh||!b->refresh(b->context,nav_token,&success))return false;
        out->refreshed=1;if(!success){if(!b->failure||!b->failure(b->context,actor,11))return false;out->failed=1;}return true;
    };
    if(!target)return phase?refresh():0;
    if(!phase){
        void* memory=nullptr;unsigned extent=0,key=0;ServerNpcEnemyMemoryLookup lookup{};
        if(!memory_key(b->memory,target,key)||!memory_storage(&b->memory.storage,read<unsigned>(actor,0x984),0x3c,memory,extent)||
           !memory_position(memory,extent,key,false,&b->memory.storage,&lookup,out->point))return 0;
        if(mode==2){
            float origin[3],eye[3];
            if(!b->origin||!b->eye||!b->origin(b->context,target,origin)||!b->eye(b->context,target,eye)||!finite_vec(origin)||!finite_vec(eye))return 0;
            for(unsigned i=0;i<3;++i)out->point[i]=static_cast<float>(static_cast<double>(out->point[i])+static_cast<float>(static_cast<double>(eye[i])-origin[i]));
        }
    }else if(!b->origin||!b->origin(b->context,target,out->point))return 0;
    for(unsigned i=0;i<3;++i)out->goal_delta[i]=static_cast<float>(static_cast<double>(read<float>(path,0x34+4*i))-read<float>(path,0x14+4*i));
    float actor_origin[3],velocity[3],distance=0,speed=0;const float zero[3]{};
    if(!b->origin||!b->velocity||!b->origin(b->context,actor,actor_origin)||!b->velocity(b->context,actor,velocity)||
       !goal_vector_distance(actor_origin,out->point,distance)||!goal_vector_distance(velocity,zero,speed)||
       !goal_vector_distance(out->goal_delta,out->point,out->deviation))return 0;
    const float tolerance=read<float>(path,0x68);if(!std::isfinite(tolerance))return 0;
    const float remaining=static_cast<float>(static_cast<double>(distance)-tolerance);
    double threshold=speed>0?static_cast<double>(remaining)/speed*120.:120.;
    if(threshold>120.)threshold=120.;else if(threshold<0.)threshold=0.;
    // Original stores clamp result to float before multiplying non-special target by 1.2.
    bool special=false;if(!b->special||!b->special(b->context,target,&special))return 0;
    const double exact=static_cast<double>(static_cast<float>(threshold))*(special?1.:1.2);
    out->threshold=static_cast<float>(exact);out->threshold_called=1;
    // Enemy path stores threshold in f32, target compares extended result.
    if(out->deviation>(phase?exact:out->threshold))return refresh();
    return 1;
}
bool schedule_local_id(void* actor,int local,const ServerNpcConditionMapBindings& b,int& global) noexcept{
    global=-1;const void* desc=nullptr;unsigned bytes=0;
    if(!b.descriptor||!b.descriptor(b.context,actor,&desc,&bytes)||!desc||bytes<0x18)return false;
    const void* row=static_cast<const unsigned char*>(desc)+4;unsigned seen[256]{},count=0;
    while(row){
        const int first=read<int>(row,4),last=read<int>(row,8);
        if(first!=9999&&local>=first&&local<=last){
            const auto value=static_cast<std::int64_t>(read<int>(row,0))-first+local;
            if(value<std::numeric_limits<int>::min()||value>std::numeric_limits<int>::max())return false;
            global=static_cast<int>(value);return true;
        }
        const auto next=read<unsigned>(row,0x10);if(!next)return true;
        if(count>=256)return false;for(unsigned i=0;i<count;++i)if(seen[i]==next)return false;seen[count++]=next;
        if(!b.storage||!b.storage(b.context,next,&row,&bytes)||!row||bytes<0x14)return false;
    }
    return true;
}
int __cdecl grunt_weapon_eligible(void* actor,unsigned bytes,float now,const ServerNpcGruntWeaponSearchBindings* b,ServerNpcGruntWeaponSearchResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now)||!relation_grunt_class(b->relation.original_vtable)||
       b->relation.count>sin_independent::kEntityHandleCapacity||(b->relation.count&&!b->relation.entries))return 0;
    out->deadline=read<float>(actor,0x9a4);if(!std::isfinite(out->deadline))return 0;
    if(out->deadline>now){out->cooldown_blocked=1;return 1;}
    const auto registry=sin_independent::EntityHandleView(b->relation.entries,b->relation.count);
    auto active=[&](){return static_cast<void*>(registry.Resolve(read<unsigned>(actor,0x78c)));};
    unsigned caps=read<unsigned>(actor,0x8ec);
    if(void* w=active()){unsigned wc=0;if(!b->weapon_caps||!b->weapon_caps(b->context,w,&wc))return 0;caps|=wc;}
    if(!(caps&0x200000u)){out->capability_blocked=1;return 1;}
    if(active()&&read<int>(actor,0x850)==3){out->combat_blocked=1;return 1;}
    if(read<unsigned>(actor,0x798)){
        int global=-1;if(!schedule_local_id(actor,0x3d,b->schedule.conditions,global))return 0;
        if(global==read<int>(actor,0x79c)){out->schedule_blocked=1;return 1;}
    }
    if(b->max_clients==1){
        void* player=nullptr;out->player_checked=1;
        if(!b->local_player||!b->local_player(b->context,&player))return 0;
        if(!player)out->friendly=1;
        else {ServerNpcGruntRelationResult rr{};if(!grunt_relation(actor,bytes,player,true,&b->relation,&rr))return 0;out->friendly=rr.disposition==3;}
    }
    if(!out->friendly&&active()){out->armed_blocked=1;return 1;}
    if(read<std::uint8_t>(actor,0x968)){out->disabled=1;return 1;}
    out->eligible=1;return 1;
}
int __cdecl grunt_weapon_search(void* actor,unsigned bytes,float now,const ServerNpcGruntWeaponSearchBindings* b,ServerNpcGruntWeaponSearchResult* out) noexcept{
    if(!grunt_weapon_eligible(actor,bytes,now,b,out))return 0;
    if(!out->eligible)return 1;
    const auto registry=sin_independent::EntityHandleView(b->relation.entries,b->relation.count);
    float delay=2.f;
    if(!registry.Resolve(read<unsigned>(actor,0x78c))){
        const auto squad=read<unsigned>(actor,0x9c4);
        if(squad){
            void* data=nullptr;unsigned n=0;
            if(!memory_storage(&b->storage,squad,8,data,n))return 0;
            auto name=read<unsigned>(data,4);
            if(name){void* str=nullptr;if(!memory_storage(&b->storage,name,1,str,n))return 0;if(!read<std::uint8_t>(str,0))name=0;}
            if(name==b->special_squad_name){
                bool yes=false;if(!b->squad_predicate||!b->squad_predicate(b->context,actor,&yes))return 0;
                if(!yes)delay=1.f;
            }
        }
    }
    out->deadline=static_cast<float>(static_cast<double>(now)+delay);if(!std::isfinite(out->deadline))return 0;
    write(actor,0x9a4,out->deadline);
    out->half_extents[0]=out->half_extents[1]=540.f;out->half_extents[2]=100.f;
    out->search_called=1;if(!b->find_weapon||!b->find_weapon(b->context,actor,out->half_extents,&out->weapon))return 0;
    out->found=out->weapon!=nullptr;
    if(out->found){
        const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
        if(!cond||!cond->store(actor,bytes,0x2c,NpcConditionCurrent,true,&b->schedule.conditions,&cr))return 0;
        out->condition_set=cr.mapped;
    }
    return 1;
}
bool aux_valid(void* actor,unsigned bytes,const ServerNpcGruntAuxConditionsBindings* b) noexcept{
    return actor&&bytes>=0xdb2&&b&&relation_grunt_class(b->visibility.original_vtable)&&
        b->visibility.memory.count<=sin_independent::kEntityHandleCapacity&&(!b->visibility.memory.count||b->visibility.memory.entries);
}
int __cdecl grunt_target_conditions(void* actor,unsigned bytes,float now,const ServerNpcGruntAuxConditionsBindings* b,ServerNpcGruntAuxConditionsResult* out) noexcept{
    if(!out)return 0;*out={};if(!aux_valid(actor,bytes,b)||!std::isfinite(now))return 0;
    const auto& mb=b->visibility.memory;
    void* target=sin_independent::EntityHandleView(mb.entries,mb.count).Resolve(read<unsigned>(actor,0x8a8));
    if(!target)return 1;out->target_live=1;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
    if(!cond||!cond->store(actor,bytes,0xf,NpcConditionCurrent,false,&b->visibility.conditions,&cr)||
       !cond->store(actor,bytes,0xd,NpcConditionCurrent,false,&b->visibility.conditions,&cr))return 0;
    ServerNpcGruntCombatVisibilityResult vr{};
    if(!combat_visibility_query_impl(actor,bytes,target,0x4081,&b->visibility,&vr,false))return 0;
    out->target_visible=vr.visible;const int id=vr.visible?0xf:0xd;
    if(!cond->store(actor,bytes,id,NpcConditionCurrent,true,&b->visibility.conditions,&cr))return 0;
    out->target_condition=cr.mapped?static_cast<unsigned>(id):0;
    auto gb=b->goals;gb.original_vtable=b->visibility.original_vtable;gb.memory.entries=mb.entries;gb.memory.count=mb.count;
    if(!grunt_goal_refresh(actor,bytes,1,now,&gb,&out->goal))return 0;out->target_refresh=1;return 1;
}
int __cdecl grunt_ammo_conditions(void* actor,unsigned bytes,const ServerNpcGruntAuxConditionsBindings* b,ServerNpcGruntAuxConditionsResult* out) noexcept{
    if(!out)return 0;*out={};if(!aux_valid(actor,bytes,b))return 0;
    const auto& mb=b->visibility.memory;const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    auto active=[&](){return static_cast<void*>(registry.Resolve(read<unsigned>(actor,0x78c)));};
    void* w=active();if(!w)return 1;out->weapon_live=1;
    unsigned n=0;if(!b->visibility.extent||!b->visibility.extent(b->visibility.context,w,&n)||n<0x4e8)return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
    auto set=[&](int id,unsigned& flag){if(!cond||!cond->store(actor,bytes,id,NpcConditionCurrent,true,&b->visibility.conditions,&cr))return false;flag=cr.mapped;return true;};
    auto query=[&](unsigned slot,bool& yes){void* p=active();return p&&b->weapon_predicate&&b->weapon_predicate(b->context,p,slot,&yes);};
    bool yes=false;
    if(read<int>(w,0x4e4)>=0){
        out->primary_checked=1;if(!query(0x330,yes))return 0;
        if(!yes&&!set(4,out->primary_empty))return 0;
    }
    out->secondary_checked=1;if(!query(0x334,yes))return 0;
    if(!yes){if(!query(0x43c,yes))return 0;out->secondary_uses_ammo=yes;if(yes&&!set(5,out->secondary_empty))return 0;}
    return 1;
}
int __cdecl grunt_snapshot_conditions(void* actor,unsigned bytes,const ServerNpcGruntAuxConditionsBindings* b,ServerNpcGruntAuxConditionsResult* out) noexcept{
    if(!out)return 0;*out={};if(!aux_valid(actor,bytes,b))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    if(!cond||!cond->snapshot_mask(actor,bytes))return 0;out->snapshot=1;
    if(read<int>(actor,0x850)!=4)return 1;
    const auto& mb=b->visibility.memory;const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    void* script=registry.Resolve(read<unsigned>(actor,0x970));if(!script)return 1;out->script_live=1;
    unsigned n=0;if(!b->visibility.extent||!b->visibility.extent(b->visibility.context,script,&n)||n<0x3d4)return 0;
    void* e=registry.Resolve(read<unsigned>(script,0x3d0));if(!e)return 1;
    if(read<std::uint8_t>(script,0x3cc)){
        if(!b->visibility.extent(b->visibility.context,e,&n)||n<0x90)return 0;
        if(!read<int>(e,0x8c))return 1;
    }
    void* npc=nullptr;if(!b->as_npc||!b->as_npc(b->context,e,&npc))return 0;
    if(!npc)return 1;
    if(!b->visibility.extent(b->visibility.context,npc,&n)||n<0x7ec)return 0;
    ServerNpcConditionResult cr{};
    for(int id:{0x10,0x11}){if(!cond->store(npc,n,id,NpcConditionCurrent,false,&b->visibility.conditions,&cr))return 0;out->script_clear+=cr.changed;}
    return 1;
}
bool raw_schedule_bit(void* actor,unsigned bit,const ServerNpcScheduleBindings& b,bool& yes) noexcept{
    yes=false;const auto token=read<unsigned>(actor,0x798);if(!token)return true;
    const void* s=nullptr;unsigned n=0;const unsigned off=0xc+4*(bit/32);
    if(bit>=256||!b.storage||!b.storage(b.context,token,&s,&n)||!s||n<off+4)return false;
    yes=(read<unsigned>(s,off)&(1u<<(bit%32)))!=0;return true;
}
int __cdecl grunt_flinch(void* actor,unsigned bytes,float now,const ServerNpcGruntFlinchBindings* b,ServerNpcGruntFlinchResult* out) noexcept{
    if(!out)return 0;*out={};out->activity=-1;
    if(!actor||bytes<0x9e4||!b||!std::isfinite(now))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    ServerNpcConditionResult heavy{},light{},cr{};
    if(!cond->test(actor,bytes,0x11,NpcConditionCurrent,&b->schedule.conditions,&heavy))return 0;
    out->heavy=heavy.value;bool play=false,clear_heavy=false;
    if(out->heavy){
        if(read<unsigned>(actor,0x988)&0x40u){play=true;clear_heavy=true;}
        else{
            bool declared=false;
            if(!heavy.mapped||!raw_schedule_bit(actor,static_cast<unsigned>(heavy.bit),b->schedule,declared))return 0;
            out->interrupted=declared;play=!declared;
        }
    }else{
        if(!cond->test(actor,bytes,0x10,NpcConditionCurrent,&b->schedule.conditions,&light))return 0;
        out->light=light.value;play=out->light!=0;
    }
    if(play){
        out->called=1;
        bool can=false,exists=false;
        if(!b->select_activity||!b->select_activity(b->context,actor,out->heavy!=0,1,&out->activity)||
           !b->can_animate||!b->can_animate(b->context,actor,&can))return 0;
        if(can){
            if(!b->activity_exists||!b->activity_exists(b->context,actor,out->activity,&exists))return 0;
            if(exists){if(!b->play_activity||!b->play_activity(b->context,actor,out->activity,1,1))return 0;out->played=1;}
        }
        if(!(read<unsigned>(actor,0x988)&0x40u)){
            float delay=0;write(actor,0x988,read<unsigned>(actor,0x988)|0x40u);
            if(!b->random||!b->random(b->context,.5f,1.f,&delay)||!std::isfinite(delay)||delay<.5f||delay>1.f)return 0;
            out->deadline=static_cast<float>(static_cast<double>(now)+delay);if(!std::isfinite(out->deadline))return 0;
            write(actor,0x9e0,out->deadline);out->timer_started=1;
        }
        if(clear_heavy){if(!cond->store(actor,bytes,0x11,NpcConditionCurrent,false,&b->schedule.conditions,&cr))return 0;out->heavy_cleared=cr.changed;}
    }
    if(read<unsigned>(actor,0x988)&0x40u){
        out->deadline=read<float>(actor,0x9e0);if(!std::isfinite(out->deadline))return 0;
        if(now>out->deadline){write(actor,0x988,read<unsigned>(actor,0x988)&~0x40u);out->timer_cleared=1;}
    }
    return 1;
}
int grunt_post_update_impl(void* actor,unsigned bytes,float now,bool perceived,const ServerNpcGruntPostUpdateBindings* b,ServerNpcGruntPostUpdateResult* out,int inactive_override,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;
    out->search={};out->combat={};out->combat_goals[0]={};out->combat_goals[1]={};out->target={};out->ammo={};out->snapshot={};out->flinch={};out->cleared_enemy={};
    out->completed=out->skipped_state=out->skipped_perception=out->cleared_sense_conditions=out->new_enemy_condition=out->combat_called=out->enemy_cleared=0;
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now)||!relation_grunt_class(b->aux.visibility.original_vtable))return 0;
    const auto& mb=b->aux.visibility.memory;
    if(mb.count>sin_independent::kEntityHandleCapacity||(mb.count&&!mb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    auto current=[&](){return static_cast<void*>(registry.Resolve(read<unsigned>(actor,0x8a0)));};
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto store=[&](int id,bool value,ServerNpcConditionResult& cr){return cond->store(actor,bytes,id,NpcConditionCurrent,value,&b->schedule.conditions,&cr)!=0;};
    const int state=read<int>(actor,0x850);ServerNpcConditionResult cr{};
    auto aux=b->aux;aux.visibility.conditions=b->schedule.conditions;
    if(inactive_override<0?(state==0||state==7):inactive_override!=0){
        out->skipped_state=1;if(!store(1,false,cr))return 0;
    }else{
        if(perceived){
            auto search=b->search;search.relation.entries=mb.entries;search.relation.count=mb.count;search.relation.original_vtable=aux.visibility.original_vtable;search.schedule=b->schedule;
            if(!grunt_weapon_search(actor,bytes,now,&search,&out->search))return 0;
            const int live_state=read<int>(actor,0x850);
            if(read<unsigned>(actor,0x798)&&(live_state==1||live_state==2)&&current()){
                bool declared=false;
                if(!cond->test(actor,bytes,0x19,NpcConditionCurrent,&b->schedule.conditions,&cr))return 0;
                if(!cr.value){
                    if(!raw_schedule_bit(actor,0x19,b->schedule,declared))return 0;
                    if(declared){if(!store(0x19,true,cr))return 0;out->new_enemy_condition=cr.mapped;}
                }
            }
        }else{
            out->skipped_perception=1;
            for(int id:{7,9,10,8,32,31,48,51,52,53,49,50,55,56,35}){
                if(!store(id,false,cr))return 0;out->cleared_sense_conditions+=cr.changed;
            }
        }
        if(void* e=current()){
            if(!(read<unsigned>(actor,0xb4)&0x10u)){
                auto combat=b->combat;combat.range.visibility.memory=mb;combat.range.visibility.conditions=b->schedule.conditions;combat.range.visibility.original_vtable=aux.visibility.original_vtable;
                out->combat_goals[0]={};out->combat_goals[1]={};
                if(!combat_conditions_impl(actor,bytes,e,now,&combat,&out->combat,&aux.goals,out->combat_goals,speech))return 0;
                write(actor,0x9a0,now);out->combat_called=1;
            }else{
                auto eb=b->enemy;eb.entries=mb.entries;eb.count=mb.count;eb.conditions=b->schedule.conditions;
                if(!set_enemy_impl(actor,bytes,nullptr,true,now,&eb,&out->cleared_enemy,true,speech?speech->slots:nullptr))return 0;out->enemy_cleared=1;
            }
        }
        if(!grunt_target_conditions(actor,bytes,now,&aux,&out->target)||!grunt_ammo_conditions(actor,bytes,&aux,&out->ammo))return 0;
        auto flinch=b->flinch;flinch.schedule=b->schedule;
        if(!grunt_flinch(actor,bytes,now,&flinch,&out->flinch))return 0;
    }
    if(!grunt_snapshot_conditions(actor,bytes,&aux,&out->snapshot))return 0;
    out->completed=1;return 1;
}
int __cdecl grunt_post_update(void* actor,unsigned bytes,float now,bool perceived,const ServerNpcGruntPostUpdateBindings* b,ServerNpcGruntPostUpdateResult* out) noexcept{
    return grunt_post_update_impl(actor,bytes,now,perceived,b,out,-1);
}
int grunt_update_gate_impl(void* actor,unsigned bytes,const ServerNpcGruntUpdateGateBindings* b,ServerNpcGruntUpdateGateResult* out,float now=0,const PerceptionSpeechRoute* speech=nullptr,const sin_independent::EntityHandleEntry* entries=nullptr,unsigned count=0) noexcept{
    if(!out)return 0;*out={};out->speech_result=-1;
    if(!actor||bytes<0xdb2||!b||!relation_grunt_class(b->original_vtable))return 0;
    write<std::uint8_t>(actor,0x84d,1);
    const int state=read<int>(actor,0x850);
    if(state==0||state==7){out->inactive=1;return 1;}
    void* motor=nullptr;unsigned n=0;
    if(!memory_storage(&b->storage,read<unsigned>(actor,0x908),0x18,motor,n))return 0;
    const float raw=read<float>(actor,0x370);out->desired_yaw=read<float>(motor,0x14);
    const double scaled=static_cast<double>(raw)*182.04444885253906;
    if(!std::isfinite(scaled)||!std::isfinite(out->desired_yaw)||scaled>=2147483648.||scaled< -2147483648.)return 0;
    const unsigned quantized=static_cast<unsigned>(static_cast<int>(scaled))&65535u;
    out->yaw=static_cast<float>(quantized)*0.0054931640625f;
    if(out->yaw!=out->desired_yaw){
        double d=std::fmod(static_cast<double>(out->desired_yaw)-out->yaw,360.);
        if(out->desired_yaw>out->yaw){if(d>=180.)d-=360.;}
        else if(d<=-180.)d+=360.;
        out->yaw_delta=static_cast<float>(d);
    }
    if(std::abs(static_cast<double>(out->yaw_delta))<=.006){
        const auto flags=read<unsigned>(actor,0x988);
        out->yaw_flag_cleared=(flags&0x2000u)!=0;write(actor,0x988,flags&~0x2000u);
    }
    out->forced=read<std::uint8_t>(actor,0x84c);write<std::uint8_t>(actor,0x84c,0);out->force_cleared=1;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    ServerNpcConditionResult cr{};
    if(read<unsigned>(actor,8)!=0x22044fa0u){
        bool yes=false;out->pvs_checked=1;
        if(!b->pvs_index||!b->pvs_index(b->context,read<unsigned>(actor,0x16c),&yes))return 0;
        out->pvs=yes;
        if(!cond->store(actor,bytes,1,NpcConditionCurrent,yes,&b->conditions,&cr))return 0;
    }
    if(read<unsigned>(actor,0xb4)&0x10u){out->disabled=1;return 1;}
    if(!out->forced){
        if(!cond->test(actor,bytes,1,NpcConditionCurrent,&b->conditions,&cr))return 0;
        out->condition_one=cr.value;
        if(!cr.value){
            bool gate=false;const auto token=read<unsigned>(actor,0xc18);
            if(token){
                void* component=nullptr;unsigned extent=0;
                if(!memory_storage(&b->storage,token,9,component,extent)||!b->component_gate)return 0;
                const auto prior=read<std::uint8_t>(component,8);write<std::uint8_t>(component,8,1);out->component_called=1;
                const int ok=b->component_gate(b->context,token,&gate);
                out->component_handled=read<std::uint8_t>(component,8)!=0;write(component,8,prior);
                if(!ok)return 0;
            }
            if(!out->component_handled)gate=(read<unsigned>(actor,0xb4)&0x400u)!=0;
            out->gate_value=gate;
            if(!gate&&read<int>(actor,0x850)!=3)return 1;
        }
    }
    if(!b->pre_perception||!b->pre_perception(b->context,actor))return 0;
    out->pre_called=out->perceive=1;
    const int live_state=read<int>(actor,0x850);
    if(live_state==1||live_state==2){
        int roll=-1;out->idle_roll=1;
        if(!b->random_int||!b->random_int(b->context,0,99,&roll)||roll<0||roll>99)return 0;
        if(roll==0&&!(read<unsigned>(actor,0xb4)&2u)){
            if(speech){
                if(!perception_sentence(actor,bytes,now,true,b->original_vtable,entries,count,*speech))return 0;
                out->speech_result=speech->out.idle.request_result;
            }else if(!b->idle_speech||!b->idle_speech(b->context,actor,0xcc0,"GRUNT_IDLE",0,2,&out->speech_result))return 0;
            out->idle_emitted=1;
        }
    }
    return 1;
}
int __cdecl grunt_update_gate(void* actor,unsigned bytes,const ServerNpcGruntUpdateGateBindings* b,ServerNpcGruntUpdateGateResult* out) noexcept{
    return grunt_update_gate_impl(actor,bytes,b,out);
}
int grunt_condition_update_impl(void* actor,unsigned bytes,float now,const ServerNpcGruntConditionUpdateBindings* b,ServerNpcGruntConditionUpdateResult* out,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;
    out->gate={};out->perception={};out->post={};out->completed=out->perception_called=out->post_called=0;
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now)||!relation_grunt_class(b->gate.original_vtable))return 0;
    const auto vt=b->gate.original_vtable;
    const auto& mb=b->perception.enemy_switch.best.memory;
    if(mb.count>sin_independent::kEntityHandleCapacity||(mb.count&&!mb.entries))return 0;
    auto gate=b->gate;gate.conditions=b->post.schedule.conditions;
    auto perception=b->perception;perception.enemy_switch.gate.relation.original_vtable=vt;perception.enemy_switch.schedule=b->post.schedule;
    auto post=b->post;post.aux.visibility.original_vtable=vt;post.aux.visibility.memory.entries=mb.entries;post.aux.visibility.memory.count=mb.count;
    if(!grunt_update_gate_impl(actor,bytes,&gate,&out->gate,now,speech,mb.entries,mb.count))return 0;
    if(out->gate.perceive){
        if(!perception_tick_impl(actor,bytes,now,&perception,&out->perception,speech))return 0;
        out->perception_called=1;
    }
    // Inactive branch is selected at entry, not re-evaluated after engine callbacks.
    if(!grunt_post_update_impl(actor,bytes,now,out->gate.perceive!=0,&post,&out->post,out->gate.inactive?1:0,speech))return 0;
    out->post_called=out->completed=1;return 1;
}
int __cdecl grunt_condition_update(void* actor,unsigned bytes,float now,const ServerNpcGruntConditionUpdateBindings* b,ServerNpcGruntConditionUpdateResult* out) noexcept{
    return grunt_condition_update_impl(actor,bytes,now,b,out);
}
int __cdecl grunt_point_policy(void*,unsigned,const float[3],float,const ServerNpcGruntPointPolicyBindings*,ServerNpcGruntPointPolicyResult*) noexcept;
int __cdecl grunt_object_search(void*,unsigned,int,const ServerNpcGruntObjectSearchBindings*,ServerNpcGruntObjectSearchResult*) noexcept;
int grunt_gather_conditions_impl(void* actor,unsigned bytes,float now,const ServerNpcGruntGatherConditionsBindings* b,ServerNpcGruntGatherConditionsResult* out,const ServerNpcGruntPointPolicyBindings* point_bindings,ServerNpcGruntPointPolicyResult* point_result,const ServerNpcGruntObjectSearchBindings* object_bindings=nullptr,ServerNpcGruntObjectSearchResult* object_result=nullptr,const PerceptionSpeechRoute* speech=nullptr) noexcept{
    if(!out)return 0;
    out->base={};out->completed=out->base_called=out->component_called=out->hopper_exit=out->search_called=out->search_no_enemy=out->object_live=out->object_condition=out->point_called=out->point_schedule_blocked=out->point_condition=out->player_called=out->player_condition=0;
    for(float& v:out->point)v=0;
    if(!actor||bytes<0xdb2||!b||!std::isfinite(now)||!relation_grunt_class(b->base.gate.original_vtable))return 0;
    const auto& mb=b->base.perception.enemy_switch.best.memory;
    if(mb.count>sin_independent::kEntityHandleCapacity||(mb.count&&!mb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    const auto& schedule=b->base.post.schedule;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto test=[&](int id,bool& yes){ServerNpcConditionResult cr{};if(!cond->test(actor,bytes,id,NpcConditionCurrent,&schedule.conditions,&cr))return false;yes=cr.value!=0;return true;};
    auto set=[&](int id,unsigned& flag){ServerNpcConditionResult cr{};if(!cond->store(actor,bytes,id,NpcConditionCurrent,true,&schedule.conditions,&cr))return false;flag=cr.mapped;return true;};
    auto current=[&](){return static_cast<void*>(registry.Resolve(read<unsigned>(actor,0x8a0)));};
    const auto component=read<unsigned>(actor,0xc18);
    if(component){
        if(!b->component_update||!b->component_update(b->context,component))return 0;out->component_called=1;
    }else{
        if(!grunt_condition_update_impl(actor,bytes,now,&b->base,&out->base,speech))return 0;out->base_called=1;
    }
    if(read<int>(actor,0x850)==3){
        void* e=current();
        if(e){
            unsigned n=0;const auto& vis=b->base.post.aux.visibility;
            if(!vis.extent||!vis.extent(vis.context,e,&n)||n<0x10)return 0;
            bool hopper=read<unsigned>(e,0xc)==0x22490cfcu;
            if(!hopper&&(!b->classname_matches||!b->classname_matches(b->context,e,"npc_gib_hopper",&hopper)))return 0;
            if(hopper){out->hopper_exit=out->completed=1;return 1;}
        }
        if(read<std::uint8_t>(actor,0xcb2)&&!registry.Resolve(read<unsigned>(actor,0xcb4))){
            out->search_called=1;
            if(object_bindings){
                auto ob=*object_bindings;ob.visibility.memory=mb;ob.visibility.original_vtable=b->base.gate.original_vtable;
                if(!grunt_object_search(actor,bytes,150,&ob,object_result))return 0;
                out->search_no_enemy=object_result->no_enemy;
            }else if(!current()){write(actor,0xcb4,0xffffffffu);out->search_no_enemy=1;}
            else if(!b->find_object||!b->find_object(b->context,actor,150,now))return 0;
        }
    }
    out->object_live=registry.Resolve(read<unsigned>(actor,0xcb4))!=nullptr;
    bool seen=false;
    if(out->object_live){if(!test(0xa,seen))return 0;if(seen&&!set(0x16,out->object_condition))return 0;}
    if(read<std::uint8_t>(actor,0xc6c)){
        if(!test(0xa,seen))return 0;
        if(seen){
            void* e=current();
            if(e){
                int global=-1;
                if(read<unsigned>(actor,0x798)){
                    if(!schedule_local_id(actor,0x4d,schedule.conditions,global))return 0;
                    out->point_schedule_blocked=global==read<int>(actor,0x79c);
                }
                if(!out->point_schedule_blocked){
                    void* memory=nullptr;unsigned n=0,key=0;ServerNpcEnemyMemoryLookup lookup{};
                    if(!memory_key(mb,e,key)||!memory_storage(&mb.storage,read<unsigned>(actor,0x984),0x3c,memory,n)||
                       !memory_position(memory,n,key,false,&mb.storage,&lookup,out->point)||!finite_vec(out->point))return 0;
                    bool yes=false;
                    if(point_bindings){
                        auto pb=*point_bindings;pb.entries=mb.entries;pb.count=mb.count;pb.original_vtable=b->base.gate.original_vtable;
                        if(!grunt_point_policy(actor,bytes,out->point,now,&pb,point_result))return 0;
                        yes=point_result->accepted!=0;
                    }else if(!b->point_predicate||!b->point_predicate(b->context,actor,out->point,&yes))return 0;
                    out->point_called=1;
                    if(yes&&!set(0x45,out->point_condition))return 0;
                }
            }
        }
    }
    bool blocked=false;if(!test(0x46,blocked))return 0;
    if(!blocked&&b->max_clients<=1){
        void* player=nullptr;if(!b->local_player||!b->local_player(b->context,&player))return 0;
        if(player){
            bool yes=false;if(!b->player_interaction||!b->player_interaction(b->context,player,actor,&yes))return 0;out->player_called=1;
            if(yes&&!set(0x4d,out->player_condition))return 0;
        }
    }
    out->completed=1;return 1;
}
int __cdecl grunt_point_policy(void* actor,unsigned bytes,const float point[3],float now,const ServerNpcGruntPointPolicyBindings* b,ServerNpcGruntPointPolicyResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!actor||bytes<0xdb2||!b||!finite_vec(point)||!std::isfinite(now)||!relation_grunt_class(b->original_vtable)||
       b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries)||!finite_vec(b->invalid_solution))return 0;
    if(read<int>(actor,0xc38)<1){out->no_inventory=1;return 1;}
    out->deadline=read<float>(actor,0xc80);if(!std::isfinite(out->deadline))return 0;
    if(now<out->deadline){out->cooldown_blocked=1;return 1;}
    auto deadline=[&](){
        out->deadline=static_cast<float>(static_cast<double>(now)+(read<std::uint8_t>(actor,0xc6c)?.25:1.));
        if(!std::isfinite(out->deadline))return false;write(actor,0xc80,out->deadline);out->deadline_written=1;return true;
    };
    float origin[3]{};
    if(!b->origin||!b->origin(b->context,actor,origin)||!goal_vector_distance(origin,point,out->distance))return 0;
    if(out->distance<128.f||out->distance>1024.f){out->range_blocked=1;return deadline();}
    const float scalar=read<float>(actor,0x394);if(!std::isfinite(scalar))return 0;
    if(scalar==0){out->scalar_blocked=1;return 1;}
    const auto squad=read<unsigned>(actor,0x9c4);
    if(squad){
        void* data=nullptr;unsigned n=0;
        if(!memory_storage(&b->storage,squad,0x4c,data,n))return 0;
        const int count=read<int>(data,0x48);if(count<0||count>16)return 0;
        const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
        for(int i=0;i<count;++i){
            ++out->squad_visited;void* e=registry.Resolve(read<unsigned>(data,8+4*i));
            if(!e){++out->squad_stale;continue;}
            float p[3]{};if(!b->origin(b->context,e,p)||!finite_vec(p))return 0;
            // Original squad query uses horizontal distance, including the caller if present.
            const float dx=static_cast<float>(static_cast<double>(point[0])-p[0]),dy=static_cast<float>(static_cast<double>(point[1])-p[1]);
            const float sq=static_cast<float>(static_cast<double>(dx)*dx+static_cast<double>(dy)*dy);
            if(!std::isfinite(sq))return 0;
            if(std::sqrt(sq)<=250.f){
                out->squad_blocked=1;if(!deadline())return 0;
                if(!b->notify_squad||!b->notify_squad(b->context,point,0x801000u,250,.1f))return 0;out->notified=1;return 1;
            }
        }
    }
    bool yes=false;
    if(!b->in_view||!b->in_view(b->context,actor,point,&yes))return 0;out->in_view=yes;
    if(yes){if(!b->visible||!b->visible(b->context,actor,point,0x4081,&yes))return 0;out->visible=yes;}
    out->direct=out->in_view&&out->visible;
    if(!out->direct){
        out->headroom_called=1;if(!b->headroom||!b->headroom(b->context,actor,&yes))return 0;out->headroom_clear=yes;
        if(!yes)return 1;
    }
    out->solve_called=1;
    if(!b->solve||!b->solve(b->context,actor,point,out->direct!=0,out->solution)||!finite_vec(out->solution))return 0;
    out->accepted=out->solution[0]!=b->invalid_solution[0]||out->solution[1]!=b->invalid_solution[1]||out->solution[2]!=b->invalid_solution[2];
    if(out->accepted)std::memcpy(static_cast<unsigned char*>(actor)+0xc58,out->solution,12);
    return deadline();
}
int __cdecl grunt_gather_conditions(void* a,unsigned n,float now,const ServerNpcGruntGatherConditionsBindings* b,ServerNpcGruntGatherConditionsResult* out) noexcept{
    return grunt_gather_conditions_impl(a,n,now,b,out,nullptr,nullptr);
}
int __cdecl grunt_gather_point(void* a,unsigned n,float now,const ServerNpcGruntGatherConditionsBindings* b,const ServerNpcGruntPointPolicyBindings* pb,ServerNpcGruntGatherConditionsResult* out,ServerNpcGruntPointPolicyResult* pr) noexcept{
    if(!pb||!pr)return 0;*pr={};
    return grunt_gather_conditions_impl(a,n,now,b,out,pb,pr);
}
bool normalized_object_direction(const float* from,const float* to,float* direction,float& length) noexcept{
    if(!goal_vector_distance(from,to,length))return false;
    for(unsigned i=0;i<3;++i){
        const float d=static_cast<float>(static_cast<double>(to[i])-from[i]);direction[i]=length?d/length:0.f;
    }
    direction[2]=0;return finite_vec(direction);
}
int __cdecl grunt_object_search(void* actor,unsigned bytes,int mass_limit,const ServerNpcGruntObjectSearchBindings* b,ServerNpcGruntObjectSearchResult* out) noexcept{
    if(!out)return 0;*out={};out->selected_handle=0xffffffffu;
    if(!actor||bytes<0xdb2||!b||!relation_grunt_class(b->visibility.original_vtable))return 0;
    const auto& mb=b->visibility.memory;
    if(mb.count>sin_independent::kEntityHandleCapacity||(mb.count&&!mb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(mb.entries,mb.count);
    auto current=[&](){return static_cast<void*>(registry.Resolve(read<unsigned>(actor,0x8a0)));};
    auto finish=[&](unsigned handle){
        write(actor,0xcb4,handle);out->selected_handle=handle;out->found=registry.Resolve(handle)!=nullptr;out->completed=1;return 1;
    };
    if(!current()){out->no_enemy=1;return finish(0xffffffffu);}
    auto position=[&](auto fn,void* e,float* p){return e&&fn&&fn(b->context,e,p)&&finite_vec(p);};
    float origin[3],enemy_origin[3],direction[3];
    if(!position(b->origin,actor,origin)||!position(b->origin,current(),enemy_origin)||!normalized_object_direction(origin,enemy_origin,direction,out->enemy_distance))return 0;
    if(out->enemy_distance>500.f){out->enemy_range_blocked=out->completed=1;out->selected_handle=read<unsigned>(actor,0xcb4);return 1;}
    out->search_radius=out->enemy_distance<60.f?out->enemy_distance:60.f;
    for(unsigned i=0;i<3;++i){
        const float extent=i==2?60.f:out->search_radius;
        out->mins[i]=static_cast<float>(static_cast<double>(origin[i])-extent);out->maxs[i]=static_cast<float>(static_cast<double>(origin[i])+extent);
    }
    if(!finite_vec(out->mins)||!finite_vec(out->maxs))return 0;
    const std::uint32_t* handles=nullptr;unsigned count=0;
    if(!b->enumerate||!b->enumerate(b->context,out->mins,out->maxs,&handles,&count)||count>sin_independent::kEntityHandleCapacity||(count&&!handles))return 0;
    unsigned accepted[100]{};
    for(unsigned i=0;i<count;++i){
        ++out->enumerated;void* e=registry.Resolve(handles[i]);
        if(!e){++out->stale;continue;}
        bool exists=false,movable=false,enabled=false;float mass=0;
        if(!b->object_properties||!b->object_properties(b->context,e,&exists,&mass,&movable,&enabled)||!std::isfinite(mass))return 0;
        if(!exists||static_cast<double>(mass)>mass_limit||!movable||!enabled){++out->property_rejected;continue;}
        if(out->qualified==100)break;accepted[out->qualified++]=handles[i];
    }
    if(!b->actor_reference_height||!b->actor_reference_height(b->context,actor,&out->reference_height)||!std::isfinite(out->reference_height))return 0;
    out->best_distance=out->search_radius;unsigned selected=0xffffffffu;
    for(unsigned i=0;i<out->qualified;++i){
        ++out->visited;void* e=registry.Resolve(accepted[i]);if(!e){++out->stale;continue;}
        float p[3],d[3],len=0;
        if(!position(b->center,e,p)||!position(b->origin,actor,origin))return 0;
        const double manhattan=std::abs(static_cast<double>(origin[0])-p[0])+std::abs(static_cast<double>(origin[1])-p[1]);
        if(manhattan>=out->best_distance){++out->distance_rejected;continue;}
        const float distance=static_cast<float>(manhattan);
        if(!position(b->center,e,p)||!normalized_object_direction(origin,p,d,len))return 0;
        const double dot=static_cast<double>(d[0])*direction[0]+static_cast<double>(d[1])*direction[1];
        if(dot<.8){++out->direction_rejected;continue;}
        if(!position(b->origin,current(),enemy_origin))return 0;
        const double enemy_manhattan=std::abs(static_cast<double>(p[0])-enemy_origin[0])+std::abs(static_cast<double>(p[1])-enemy_origin[1]);
        if(enemy_manhattan<=distance){++out->enemy_distance_rejected;continue;}
        unsigned n=0;const auto& vis=b->visibility;
        if(!vis.extent||!vis.extent(vis.context,e,&n)||n<0x12c)return 0;
        const float offset=read<float>(e,0x128);float eye[3];
        if(!std::isfinite(offset)||!position(b->eye,actor,eye))return 0;
        if(static_cast<double>(p[2])+offset<out->reference_height||p[2]>eye[2]){++out->height_rejected;continue;}
        bool has_model=false;float top=0;
        if(!b->world_model_top||!b->world_model_top(b->context,e,&has_model,&top)||!std::isfinite(top))return 0;
        if(!has_model||top<out->reference_height){++out->model_rejected;continue;}
        ServerNpcGruntCombatVisibilityResult vr{};
        if(!combat_visibility_query_impl(actor,bytes,e,0x4081,&vis,&vr,false))return 0;
        if(!vr.visible){++out->visibility_rejected;continue;}
        bool rejected=false;
        if(!b->attached_object||!b->attached_object(b->context,e,&rejected))return 0;
        if(rejected){++out->attachment_rejected;continue;}
        float target[3];bool yes=false;
        if(!position(b->center,current(),target)||!b->point_test||!b->point_test(b->context,e,target,0x200400bu,&yes))return 0;
        if(!yes){++out->point_rejected;continue;}
        for(const char* pattern:{"weapon_*","item_ammo*","item_health*"}){
            if(!b->classname_matches||!b->classname_matches(b->context,e,pattern,&yes))return 0;
            if(yes){rejected=true;break;}
        }
        if(rejected){++out->class_rejected;continue;}
        void* prop=nullptr;if(!b->as_physics_prop||!b->as_physics_prop(b->context,e,&prop))return 0;
        if(prop){
            if(!vis.extent(vis.context,prop,&n)||n<0xb8)return 0;
            if(read<unsigned>(prop,0xb4)&0x2000u){++out->prop_rejected;continue;}
        }
        selected=accepted[i];out->best_distance=distance;++out->replaced;
    }
    return finish(selected);
}
int __cdecl grunt_gather_native(void* a,unsigned n,float now,const ServerNpcGruntGatherNativeBindings* b,ServerNpcGruntGatherNativeResult* out) noexcept{
    if(!out)return 0;out->gather={};out->point={};out->object={};out->object.selected_handle=0xffffffffu;
    if(!b)return 0;
    return grunt_gather_conditions_impl(a,n,now,&b->gather,&out->gather,&b->point,&out->point,&b->object,&out->object);
}
void reset_perception_speech(ServerNpcGruntPerceptionSpeechResult& out) noexcept{
    out={};
    out.lost.speech.cue.speech_result=out.lost.speech.sentence.request_result=out.idle.request_result=out.refind.request_result=-1;
    for(auto* r:{&out.lost.request,&out.idle_request,&out.refind_request})r->group_id=r->pick_result=r->sentence_id=r->request_result=-1;
}
int __cdecl perception_speech_report(void* a,unsigned n,void* target,const float* pos,void* informer,float now,const ServerNpcGruntMemoryReportBindings* b,const ServerNpcGruntPerceptionSpeechBindings* speech,ServerNpcGruntMemoryReportResult* out,ServerNpcGruntPerceptionSpeechResult* result) noexcept{
    if(!speech||!result)return 0;reset_perception_speech(*result);const PerceptionSpeechRoute route{*speech,*result};
    return grunt_memory_report_impl(a,n,target,pos,informer,now,b,out,&route);
}
int __cdecl perception_speech_switch(void* a,unsigned n,float now,const ServerNpcGruntEnemySwitchBindings* b,const ServerNpcGruntPerceptionSpeechBindings* speech,ServerNpcGruntEnemySwitchResult* out,ServerNpcGruntPerceptionSpeechResult* result) noexcept{
    if(!speech||!result)return 0;reset_perception_speech(*result);const PerceptionSpeechRoute route{*speech,*result};
    return grunt_enemy_switch_impl(a,n,now,b,out,nullptr,&route);
}
int __cdecl perception_speech_prepare(void* a,unsigned n,float now,const ServerNpcGruntUpdateGateBindings* b,const ServerNpcGruntPerceptionSpeechBindings* speech,ServerNpcGruntUpdateGateResult* out,ServerNpcGruntPerceptionSpeechResult* result) noexcept{
    if(!speech||!result||!std::isfinite(now))return 0;reset_perception_speech(*result);const PerceptionSpeechRoute route{*speech,*result};
    return grunt_update_gate_impl(a,n,b,out,now,&route,speech->sentence.entries,speech->sentence.count);
}
int __cdecl perception_speech_update(void* a,unsigned n,float now,const ServerNpcGruntConditionUpdateBindings* b,const ServerNpcGruntPerceptionSpeechBindings* speech,ServerNpcGruntConditionUpdateResult* out,ServerNpcGruntPerceptionSpeechResult* result) noexcept{
    if(!speech||!result)return 0;reset_perception_speech(*result);const PerceptionSpeechRoute route{*speech,*result};
    return grunt_condition_update_impl(a,n,now,b,out,&route);
}
int __cdecl perception_speech_gather(void* a,unsigned n,float now,const ServerNpcGruntGatherNativeBindings* b,const ServerNpcGruntPerceptionSpeechBindings* speech,ServerNpcGruntGatherNativeResult* out,ServerNpcGruntPerceptionSpeechResult* result) noexcept{
    if(!speech||!result||!out||!b)return 0;reset_perception_speech(*result);out->gather={};out->point={};out->object={};out->object.selected_handle=0xffffffffu;
    const PerceptionSpeechRoute route{*speech,*result};
    return grunt_gather_conditions_impl(a,n,now,&b->gather,&out->gather,&b->point,&out->point,&b->object,&out->object,&route);
}
const ServerNpcGruntPerceptionSpeechApi perception_speech_api{1,sizeof(ServerNpcGruntPerceptionSpeechApi),&perception_speech_report,&perception_speech_switch,&perception_speech_prepare,&perception_speech_update,&perception_speech_gather};
bool preschedule_init(void* a,unsigned bytes,float now,const ServerNpcGruntPrescheduleBindings* b,ServerNpcGruntPrescheduleResult* out) noexcept{
    if(!out)return false;*out={};out->target_handle=0xffffffffu;
    return a&&bytes>=0xdb2&&b&&std::isfinite(now)&&relation_grunt_class(b->original_vtable)&&
        b->count<=sin_independent::kEntityHandleCapacity&&(!b->count||b->entries);
}
bool pending_policy(void* a,float now,const ServerNpcGruntPrescheduleBindings& b,ServerNpcGruntPrescheduleResult& out) noexcept{
    const auto registry=sin_independent::EntityHandleView(b.entries,b.count);
    void* object=registry.Resolve(read<unsigned>(a,0xcb8));out.pending_live=object!=nullptr;
    if(!object)return true;
    void* enemy=registry.Resolve(read<unsigned>(a,0x8a0));out.enemy_live=enemy!=nullptr;
    if(!enemy)return true;
    auto pos=[&](auto fn,void* e,float* p){return fn&&fn(b.context,e,p)&&finite_vec(p);};
    float origin[3],object_origin[3],enemy_origin[3],center[3],od[3],ed[3],len=0;
    if(!pos(b.origin,a,origin)||!pos(b.origin,object,object_origin))return false;
    out.height_delta=static_cast<float>(std::abs(static_cast<double>(origin[2])-object_origin[2]));
    if(out.height_delta>12.f){out.height_rejected=1;return true;}
    if(!goal_vector_distance(origin,object_origin,out.distance)||!std::isfinite(b.pending_range))return false;
    if(static_cast<double>(out.distance)>=static_cast<double>(b.pending_range)*.5){out.range_rejected=1;return true;}
    if(!pos(b.center,object,center)||!pos(b.origin,enemy,enemy_origin)||
       !normalized_object_direction(origin,center,od,len)||!normalized_object_direction(origin,enemy_origin,ed,len))return false;
    const double dot=static_cast<double>(od[0])*ed[0]+static_cast<double>(od[1])*ed[1];
    out.dot=static_cast<float>(dot);
    if(dot<static_cast<double>(.8f)){out.direction_rejected=1;return true;}
    if(!b.trace_fraction||!b.trace_fraction(b.context,a,origin,center,0x202400bu,&out.trace_fraction)||!std::isfinite(out.trace_fraction))return false;
    if(out.trace_fraction!=1.f){out.trace_rejected=1;return true;}
    const void* prop=nullptr;unsigned n=0;
    if(!b.as_pending_prop||!b.as_pending_prop(b.context,object,&prop,&n))return false;
    if(!prop){out.cast_rejected=1;return true;}
    if(n<0x7a0)return false;
    if(read<int>(prop,0x8c)!=0){out.state_rejected=1;return true;}
    const float deadline=read<float>(prop,0x79c);
    if(!std::isfinite(deadline))return false;
    if(static_cast<double>(deadline)-now<=.5){out.deadline_rejected=1;return true;}
    out.accepted=1;return true;
}
int __cdecl pending_object(void* a,unsigned bytes,float now,const ServerNpcGruntPrescheduleBindings* b,ServerNpcGruntPrescheduleResult* out) noexcept{
    if(!preschedule_init(a,bytes,now,b,out)||!pending_policy(a,now,*b,*out))return 0;
    out->completed=1;return 1;
}
bool speech_squad(const ServerNpcGruntSpeechQueueBindings& b,unsigned token,void*& data) noexcept{
    unsigned n=0;data=nullptr;return b.squad_storage&&b.squad_storage(b.context,token,&data,&n)&&data&&n>=0x58;
}
bool speech_admission(void* a,float now,int priority,const ServerNpcGruntSpeechQueueBindings& b,ServerNpcGruntSpeechQueueResult& out) noexcept{
    const auto squad=read<unsigned>(a,0x9c4);
    void* squad_data=nullptr;
    float deadline=read<float>(a,0xc04);int prior=read<int>(a,0xc08);
    if(squad){
        out.squad_used=1;if(!speech_squad(b,squad,squad_data))return 0;
        deadline=read<float>(squad_data,0x50);prior=read<int>(squad_data,0x54);
    }
    if(!std::isfinite(deadline))return 0;
    // Owner +584 -> 22037960 / squad 220a7cd0: TEST AH,41; JP.
    // For finite values, both less and equal still require greater priority.
    if(now<=deadline&&priority<=prior){out.priority_blocked=1;return true;}
    if((read<unsigned>(a,0xb4)&2u)&&read<int>(a,0x850)!=3){out.state_blocked=1;return true;}
    return true;
}
bool speech_mark(void* a,float now,int priority,float extra,const ServerNpcGruntSpeechQueueBindings& b,ServerNpcGruntSpeechQueueResult& out) noexcept{
    void* squad_data=nullptr;
    auto next=[&](float& stamp){
        float sample=0;if(!b.random_float||!b.random_float(b.context,1.5f,2.f,&sample)||!std::isfinite(sample)||sample<1.5f||sample>2.f)return false;
        stamp=static_cast<float>(static_cast<double>(now)+sample+extra);return std::isfinite(stamp);
    };
    if(!next(out.owner_deadline))return 0;
    write(a,0xc08,priority);write(a,0xc04,out.owner_deadline);out.owner_written=1;
    if(const auto token=read<unsigned>(a,0x9c4)){
        if(!next(out.squad_deadline)||!speech_squad(b,token,squad_data))return 0;
        write(squad_data,0x50,out.squad_deadline);write(squad_data,0x54,priority);out.squad_written=1;
    }
    return true;
}
int __cdecl speech_queue_tick(void* a,unsigned bytes,float now,const ServerNpcGruntSpeechQueueBindings* b,ServerNpcGruntSpeechQueueResult* out) noexcept{
    if(!out)return 0;*out={};out->index=-1;
    if(!a||bytes<0xcd8||!b||!relation_grunt_class(b->original_vtable)||!std::isfinite(now))return 0;
    out->index=read<int>(a,0xccc);
    auto done=[&](){out->completed=1;return 1;};
    auto clear=[&](){write(a,0xccc,-1);out->cleared=1;return done();};
    if(out->index==-1){out->empty=1;return done();}
    // Finite game-clock values; strict expiry, equality remains eligible.
    const float expiration=read<float>(a,0xcd0);
    if(!std::isfinite(expiration))return 0;
    if(expiration<now){out->expired=1;return clear();}
    out->priority=read<int>(a,0xcd4);
    if(!speech_admission(a,now,out->priority,*b,*out))return 0;
    if(out->priority_blocked||out->state_blocked)return done();
    // 221b37c0 itself skips negative indexes. Diagnostic lookups are omitted.
    if(out->index>=0){
        if(!std::isfinite(b->volume)||!b->deliver||!b->deliver(b->context,read<unsigned>(a,0x16c),out->index,b->volume,b->level,0,read<unsigned>(a,0xcc8)))return 0;
        out->delivered=1;
    }
    // Owner +588 -> 220319d0 with zero additional seconds, after delivery.
    if(!speech_mark(a,now,read<int>(a,0xcd4),0.f,*b,*out))return 0;
    return clear();
}
bool sentence_init(void* a,unsigned n,const ServerNpcGruntSentenceBindings* b,ServerNpcGruntSentenceResult* out) noexcept{
    if(!out)return false;*out={};out->request_result=-1;out->cooldown.index=-1;
    return a&&n>=0xcd8&&b&&relation_grunt_class(b->speech.original_vtable)&&
        b->count<=sin_independent::kEntityHandleCapacity&&(!b->count||b->entries);
}
bool sentence_eligible(void* e,const ServerNpcGruntSentenceBindings& b,bool& yes) noexcept{
    yes=false;if(!e)return true;
    unsigned n=0;
    if(!b.extent||!b.extent(b.speech.context,e,&n)||n<0x208)return false;
    bool enabled=read<int>(e,0x204)!=0;
    if(!enabled&&(!b.component_eligible||!b.component_eligible(b.speech.context,e,&enabled)))return false;
    if(!enabled)return true;
    bool rejected=false;
    if(!b.entity_rejected||!b.entity_rejected(b.speech.context,e,&rejected))return false;
    yes=!rejected;return true;
}
bool sentence_scan(void* group,const ServerNpcGruntSentenceBindings& b,ServerNpcGruntSentenceResult& out) noexcept{
    ++out.group_scans;out.eligible_count=0;out.first_eligible=nullptr;
    const int n=read<int>(group,0x48);if(n<0||n>16)return false;
    const auto registry=sin_independent::EntityHandleView(b.entries,b.count);
    for(int i=0;i<n;++i){
        void* e=registry.Resolve(read<unsigned>(group,8+4*i));bool yes=false;
        if(!sentence_eligible(e,b,yes))return false;
        if(yes){if(!out.eligible_count)out.first_eligible=e;++out.eligible_count;}
    }
    return true;
}
bool sentence_mode(void* a,unsigned mode,const ServerNpcGruntSentenceBindings& b,ServerNpcGruntSentenceResult& out) noexcept{
    if(mode==0||mode>3){out.mode_allowed=1;return true;}
    if(read<int>(a,0x850)!=3&&(read<unsigned>(a,0xb4)&2u))return true;
    if(mode==1){out.mode_allowed=1;return true;}
    const auto token=read<unsigned>(a,0x9c4);if(!token)return true;
    void* group=nullptr;if(!speech_squad(b.speech,token,group)||!sentence_scan(group,b,out))return false;
    if(out.eligible_count<=1)return true;
    if(mode==2){out.mode_allowed=1;return true;}
    bool eligible=false;if(!sentence_eligible(a,b,eligible))return false;
    out.candidate_eligible=eligible;if(!eligible)return true;
    // 220aa2d0 calls 220a9210: rescan, do not reuse earlier filtered count.
    if(!sentence_scan(group,b,out))return false;
    out.leader_match=out.eligible_count>1&&out.first_eligible==a;
    out.mode_allowed=out.leader_match;return true;
}
int __cdecl sentence_gate(void* a,unsigned n,unsigned mode,const ServerNpcGruntSentenceBindings* b,ServerNpcGruntSentenceResult* out) noexcept{
    if(!sentence_init(a,n,b,out)||!sentence_mode(a,mode,*b,*out))return 0;
    out->completed=1;return 1;
}
int __cdecl sentence_request(void* a,unsigned n,const char* cue,const ServerNpcGruntSpeechQueueBindings* speech,const ServerNpcGruntSentenceRequestBindings* b,ServerNpcGruntSentenceRequestResult* out) noexcept{
    if(!out)return 0;*out={};out->group_id=out->pick_result=out->sentence_id=out->request_result=-1;
    if(!a||n<0xcd8||!cue||!speech||!b||!relation_grunt_class(speech->original_vtable))return 0;
    auto done=[&](){out->completed=1;return 1;};
    unsigned used=0;while(used<128&&cue[used]){out->group[used]=cue[used];++used;}
    if(used==128)return 0;
    // Original +5b4 is a pure getter of +8e8 or shared empty string, all eight classes.
    if(const auto token=read<unsigned>(a,0x8e8)){
        const char* suffix=nullptr;unsigned extent=0;
        if(!b->suffix||!b->suffix(b->context,token,&suffix,&extent)||!suffix||!extent)return 0;
        unsigned i=0;while(i<extent&&used<128&&suffix[i])out->group[used++]=suffix[i++];
        if(i==extent||used==128)return 0;
        out->suffix_used=1;
    }
    out->group[used]=0;
    // 221b3840: manager gate, group lookup, pick a name, then resolve name+1.
    if(!b->manager_ready){out->manager_missing=1;return done();}
    if(!std::isfinite(speech->volume)||!b->group_index)return 0;
    out->group_called=1;if(!b->group_index(b->context,out->group,&out->group_id))return 0;
    if(out->group_id<0)return done();
    out->pick_called=1;
    if(!b->pick_name||!b->pick_name(b->context,out->group_id,out->selected,sizeof(out->selected),&out->pick_result))return 0;
    if(out->pick_result<0||!out->selected[0])return done();
    unsigned end=0;while(end<sizeof(out->selected)&&out->selected[end])++end;
    if(end==sizeof(out->selected))return 0;
    out->lookup_called=1;
    if(!b->name_index||!b->name_index(b->context,out->selected+1,&out->sentence_id))return 0;
    // Native helper submits even negative lookup result; 220a34f0 normalizes it to -1.
    out->submitted=1;
    if(!b->submit||!b->submit(b->context,read<unsigned>(a,0x16c),out->sentence_id,speech->volume,speech->level,0,read<unsigned>(a,0xcc8)))return 0;
    out->request_result=out->sentence_id<0?-1:out->sentence_id;
    return done();
}
int sentence_dispatch_impl(void* a,unsigned n,float now,const char* cue,int priority,unsigned mode,const ServerNpcGruntSentenceBindings* b,ServerNpcGruntSentenceResult* out,const ServerNpcGruntSentenceRequestBindings* request=nullptr,ServerNpcGruntSentenceRequestResult* request_out=nullptr) noexcept{
    if(!sentence_init(a,n,b,out)||!std::isfinite(now)||!sentence_mode(a,mode,*b,*out))return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(!out->mode_allowed)return done();
    write(a,0xccc,-1);out->queue_reset=1;
    out->cooldown.priority=priority;
    if(priority!=-1){
        if(!speech_admission(a,now,priority,b->speech,out->cooldown))return 0;
        if(out->cooldown.priority_blocked||out->cooldown.state_blocked)return done();
    }
    // Do not reproduce original unbounded 128-byte stack copy.
    if(request){
        if(!sentence_request(a,n,cue,&b->speech,request,request_out))return 0;
        out->request_result=request_out->request_result;
    }else{
        if(!cue||!b->enqueue)return 0;
        unsigned length=0;while(length<128&&cue[length])++length;
        if(length==128)return 0;
        if(!b->enqueue(b->speech.context,a,cue,&out->request_result))return 0;
    }
    out->queued=1;
    // Original push 40000000 is float 2.0f, not a bitmask. Even -1 result marks cooldown.
    if(priority!=-1&&!speech_mark(a,now,priority,2.f,b->speech,out->cooldown))return 0;
    return done();
}
int __cdecl sentence_dispatch(void* a,unsigned n,float now,const char* cue,int priority,unsigned mode,const ServerNpcGruntSentenceBindings* b,ServerNpcGruntSentenceResult* out) noexcept{
    return sentence_dispatch_impl(a,n,now,cue,priority,mode,b,out);
}
int __cdecl sentence_dispatch_request(void* a,unsigned n,float now,const char* cue,int priority,unsigned mode,const ServerNpcGruntSentenceBindings* b,const ServerNpcGruntSentenceRequestBindings* request,ServerNpcGruntSentenceResult* out,ServerNpcGruntSentenceRequestResult* request_out) noexcept{
    if(!request||!request_out)return 0;*request_out={};request_out->group_id=request_out->pick_result=request_out->sentence_id=request_out->request_result=-1;
    return sentence_dispatch_impl(a,n,now,cue,priority,mode,b,out,request,request_out);
}
const ServerNpcGruntSentenceRequestApi sentence_request_api{1,sizeof(ServerNpcGruntSentenceRequestApi),&sentence_request,&sentence_dispatch_request};
int __cdecl grunt_speak_native(void* a,unsigned n,unsigned kind,float now,const ServerNpcGruntSpeechBindings* b,const ServerNpcGruntSentenceBindings* sentence,ServerNpcGruntNativeSpeechResult* out) noexcept{
    if(!out)return 0;*out={};out->cue.speech_result=-1;out->sentence.request_result=-1;
    if(!sentence)return 0;
    return grunt_speak_impl(a,n,kind,now,b,&out->cue,sentence,&out->sentence);
}
int grunt_survivor_impl(void* a,unsigned n,void* removed,unsigned rn,float now,const ServerNpcGruntSentenceBindings* b,ServerNpcGruntSurvivorResult* out,const ServerNpcGruntSentenceRequestBindings* request=nullptr,ServerNpcGruntSentenceRequestResult* request_out=nullptr) noexcept{
    if(!out)return 0;*out={};out->sentence.request_result=-1;
    if(!sentence_init(a,n,b,&out->sentence)||!std::isfinite(now))return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(read<std::uint8_t>(a,0xbf)&8){out->flag_blocked=1;return done();}
    if(!removed||rn<0xa0)return 0;
    if(read<int>(removed,0x9c)>0){out->removed_positive=1;return done();}
    const auto token=read<unsigned>(a,0x9c4);void* group=nullptr;
    if(!token||!speech_squad(b->speech,token,group)||!sentence_scan(group,*b,out->sentence))return 0;
    out->eligible_count=out->sentence.eligible_count;
    auto dispatch=[&](int priority,unsigned mode){
        return request?sentence_dispatch_request(a,n,now,out->cue,priority,mode,b,request,&out->sentence,request_out):
            sentence_dispatch(a,n,now,out->cue,priority,mode,b,&out->sentence);
    };
    if(out->eligible_count<2){
        out->last_member=1;out->cue="GRUNT_LAST_OF_SQUAD";
        if(!dispatch(-1,1))return 0;
        // Native marks even after a mode rejection or signed enqueue failure.
        if(!speech_mark(a,now,0,0.f,b->speech,out->final_mark))return 0;
    }else{
        out->cue="GRUNT_MAN_DOWN";
        if(!dispatch(0,2))return 0;
    }
    return done();
}
int __cdecl grunt_survivor(void* a,unsigned n,void* removed,unsigned rn,float now,const ServerNpcGruntSentenceBindings* b,ServerNpcGruntSurvivorResult* out) noexcept{
    return grunt_survivor_impl(a,n,removed,rn,now,b,out);
}
int __cdecl grunt_speak_request(void* a,unsigned n,unsigned kind,float now,const ServerNpcGruntSpeechBindings* b,const ServerNpcGruntSentenceBindings* sentence,const ServerNpcGruntSentenceRequestBindings* request,ServerNpcGruntRequestSpeechResult* out) noexcept{
    if(!out)return 0;*out={};out->speech.cue.speech_result=out->speech.sentence.request_result=-1;
    out->request.group_id=out->request.pick_result=out->request.sentence_id=out->request.request_result=-1;
    if(!sentence||!request)return 0;
    return grunt_speak_impl(a,n,kind,now,b,&out->speech.cue,sentence,&out->speech.sentence,request,&out->request);
}
int __cdecl grunt_survivor_request(void* a,unsigned n,void* removed,unsigned rn,float now,const ServerNpcGruntSentenceBindings* b,const ServerNpcGruntSentenceRequestBindings* request,ServerNpcGruntRequestSurvivorResult* out) noexcept{
    if(!out)return 0;*out={};out->survivor.sentence.request_result=-1;
    out->request.group_id=out->request.pick_result=out->request.sentence_id=out->request.request_result=-1;
    if(!request)return 0;
    return grunt_survivor_impl(a,n,removed,rn,now,b,&out->survivor,request,&out->request);
}
int __cdecl grunt_squad_leave(void* a,unsigned n,std::uint32_t group_token,std::uint32_t handle,bool notify,float now,const ServerNpcGruntSquadLeaveBindings* b,ServerNpcGruntSquadLeaveResult* out) noexcept{
    if(!out)return 0;*out={};out->index=-1;
    if(!b||!std::isfinite(now))return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(!a)return done();
    const auto& sb=b->sentence;
    if(n<0x9cc||!group_token||!relation_grunt_class(sb.speech.original_vtable)||
       sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    // Host requires +8 handle getter to identify a live departing actor.
    if(registry.Resolve(handle)!=a)return 0;
    void* group=nullptr;if(!speech_squad(sb.speech,group_token,group))return 0;
    int count=read<int>(group,0x48);if(count<0||count>16)return 0;
    for(int i=0;i<count;++i)if(registry.Resolve(read<unsigned>(group,8+4*i))==a){out->index=i;break;}
    if(out->index<0){out->missing=1;out->remaining=count;return done();}
    // 220aa116 memmove, then decrement. Native leaves unused trailing slot untouched.
    auto* rows=static_cast<unsigned char*>(group)+8;
    if(out->index+1<count)std::memmove(rows+4*out->index,rows+4*(out->index+1),4*(count-out->index-1));
    write(group,0x48,--count);out->removed=1;out->remaining=count;
    if(notify){
        for(int i=0;;++i){
            const int live_count=read<int>(group,0x48);if(live_count<0||live_count>16)return 0;
            if(i>=live_count)break;
            void* member=registry.Resolve(read<unsigned>(group,8+4*i));
            if(!member){++out->stale;continue;}
            unsigned extent=0,vt=0;
            if(!b->member_class||!b->member_class(b->context,member,&vt,&extent))return 0;
            ++out->notifications;
            if(relation_grunt_class(vt)){
                auto bound=sb;bound.speech.original_vtable=vt;
                ++out->native_notifications;
                if(!grunt_survivor_request(member,extent,a,n,now,&bound,&b->request,&out->last))return 0;
            }else{
                ++out->external_notifications;
                if(!b->other_notify||!b->other_notify(b->context,member,a))return 0;
            }
        }
    }
    // All eight known Grunt +690 methods store actor+9c4 directly.
    write(a,0x9c4,0u);write(a,0x9c8,0u);out->cleared=1;out->remaining=read<int>(group,0x48);
    return done();
}
int __cdecl grunt_squad_join(void* a,unsigned n,std::uint32_t group_token,std::uint32_t array_token,std::uint32_t handle,float now,const ServerNpcGruntSquadJoinBindings* b,ServerNpcGruntSquadJoinResult* out) noexcept{
    if(!out)return 0;*out={};out->index=-1;out->evicted_handle=0xffffffffu;
    if(!b||!std::isfinite(now))return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(!a)return done();
    const auto& sb=b->group.sentence;
    if(n<0xdb2||!relation_grunt_class(sb.speech.original_vtable)||!group_token||!array_token||
       sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    if(read<int>(a,0x8c)){out->blocked=1;return done();}
    const auto previous=read<unsigned>(a,0x9c4);
    if(previous==group_token){out->already_member=1;return done();}
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    if(registry.Resolve(handle)!=a)return 0;
    if(previous){
        out->detached=1;
        if(!grunt_squad_leave(a,n,previous,handle,false,now,&b->group,&out->previous))return 0;
    }
    void* group=nullptr;if(!speech_squad(sb.speech,group_token,group))return 0;
    int count=read<int>(group,0x48);if(count<0||count>16)return 0;
    // At capacity only the inline array count changes: evicted owner is NOT detached.
    if(count==16){out->evicted=1;out->evicted_handle=read<unsigned>(group,8+15*4);write(group,0x48,--count);}
    out->index=count;write(group,0x48,count+1);write(group,0x4c,array_token);write(group,8+4*count,handle);
    out->appended=1;out->count=count+1;
    write(a,0x9c4,group_token);write(a,0x9c8,read<unsigned>(group,4));out->owner_written=1;
    if(out->count<=1)return done();
    // 22060cc0: first raw slot, not first eligible; invalid slot is a host failure.
    void* first=registry.Resolve(read<unsigned>(group,8));if(!first)return 0;
    out->first_resolved=1;
    unsigned vt=0,extent=0,token=0;
    if(!b->group.member_class||!b->group.member_class(b->group.context,first,&vt,&extent))return 0;
    if(relation_grunt_class(vt)){if(extent<0x988)return 0;token=read<unsigned>(first,0x984);}
    else if(!b->other_memory||!b->other_memory(b->group.context,first,&token))return 0;
    // Original assumes valid memory. Do not traverse a null/aliased mutable owner tree.
    if(!token||token==read<unsigned>(a,0x984))return 0;
    void* memory=nullptr;unsigned mn=0;MemoryTree tree;unsigned height=0;
    if(!memory_storage(&b->source_memory,token,0x40,memory,mn)||!tree.open(memory,mn,&b->source_memory)||!tree.check(height))return 0;
    unsigned stack[255]{},depth=0,i=tree.root;
    auto report=b->report;report.original_vtable=sb.speech.original_vtable;report.mutation.entries=sb.entries;report.mutation.count=sb.count;
    ServerNpcGruntPerceptionSpeechBindings dialogue{};dialogue.sentence=sb;dialogue.request=b->group.request;
    reset_perception_speech(out->dialogue);const PerceptionSpeechRoute route{dialogue,out->dialogue};
    // Valid stable tree: 2206b7b0/2206b130 visit in-order, skip unresolved handles.
    while(i!=MemoryTree::nil||depth){
        while(i!=MemoryTree::nil){stack[depth++]=i;i=tree.edge(i,0);}
        i=stack[--depth];++out->records;
        void* record=nullptr;unsigned rn=0;
        if(!memory_storage(&b->source_memory,tree.record(i),0x10,record,rn))return 0;
        void* target=registry.Resolve(read<unsigned>(record,0));
        if(!target)++out->stale;
        else{
            float pos[3];std::memcpy(pos,static_cast<unsigned char*>(record)+4,12);
            ++out->reports;
            if(!grunt_memory_report_impl(a,n,target,pos,first,now,&report,&out->last_report,&route))return 0;
            out->created+=out->last_report.mutation.created;out->updated+=out->last_report.mutation.updated;
        }
        i=tree.edge(i,1);
    }
    return done();
}
int __cdecl grunt_squad_detach(void* a,unsigned n,std::uint32_t handle,float now,const ServerNpcGruntSquadLeaveBindings* b,ServerNpcGruntSquadLeaveResult* out) noexcept{
    if(!out)return 0;*out={};out->index=-1;
    if(!a||n<0x9cc||!b||!relation_grunt_class(b->sentence.speech.original_vtable)||!std::isfinite(now))return 0;
    const auto token=read<unsigned>(a,0x9c4);
    if(token){
        if(!grunt_squad_leave(a,n,token,handle,false,now,b,out))return 0;
        write(a,0x9c4,0u); // wrapper writes even after leave's missing return
    }
    out->completed=1;return 1;
}
constexpr unsigned squad_name_limit=1024;
bool squad_name_size(const char* name,unsigned extent,unsigned& length) noexcept{
    length=0;if(!name)return true;
    const unsigned limit=extent<squad_name_limit?extent:squad_name_limit;
    while(length<limit&&name[length])++length;
    return length<limit;
}
unsigned squad_fold(unsigned char c) noexcept{return c>='A'&&c<='Z'?c+32u:c;}
bool squad_name_equal(const char* a,unsigned an,const char* b,unsigned bn) noexcept{
    if(an!=bn)return false;
    for(unsigned i=0;i<an;++i)if(squad_fold(static_cast<unsigned char>(a[i]))!=squad_fold(static_cast<unsigned char>(b[i])))return false;
    return true;
}
bool squad_registry_lookup(std::uint32_t head,const char* name,const ServerNpcGruntSquadRegistryBindings& b,ServerNpcGruntSquadRegistryResult& out) noexcept{
    unsigned length=0;if(!squad_name_size(name,squad_name_limit,length))return false;
    // 220a8050 walks group+0 and compares name strings, treating null as empty.
    // Bounded cycle guard; original assumes a finite linked list.
    for(auto token=head;token;){
        if(++out.visited>4096)return false;
        void* group=nullptr;unsigned n=0;if(!memory_storage(&b.storage,token,0x78,group,n))return false;
        const auto key=read<unsigned>(group,4);const char* stored=nullptr;unsigned size=0,stored_length=0;
        if(key&&(!b.name||!b.name(b.context,key,&stored,&size)||!stored||!squad_name_size(stored,size,stored_length)))return false;
        if(squad_name_equal(name,length,stored,stored_length)){
            out.found=1;out.token=token;out.array_token=read<unsigned>(group,0x4c);out.name_token=key;return true;
        }
        token=read<unsigned>(group,0);
    }
    return true;
}
bool squad_registry_create(std::uint32_t& head,const char* name,const ServerNpcGruntSquadRegistryBindings& b,ServerNpcGruntSquadRegistryResult& out) noexcept{
    unsigned len=0;if(!squad_name_size(name,squad_name_limit,len)||!b.allocate||!b.release)return false;
    void* group=nullptr;unsigned n=0;std::uint32_t token=0,array_token=0;
    if(!b.allocate(b.context,0x78,&token,&array_token,&group,&n))return false;
    out.allocated=1;out.token=token;out.array_token=array_token;
    auto rollback=[&](){if(token&&b.release(b.context,token))out.released=1;return false;};
    if(!token||!array_token||!group||n<0x78)return rollback();
    void* verified=nullptr;unsigned extent=0;
    if(!memory_storage(&b.storage,token,0x78,verified,extent)||verified!=group)return rollback();
    // 220aa6a0 initializes fields only, not the unused array or +54.
    write(group,0x48,0u);write(group,0x4c,array_token);write(group,0x58,0xffffffffu);
    for(unsigned off=0x60;off<=0x70;off+=4)write(group,off,0u);
    std::uint32_t key=0;
    // 221f3de0 returns zero for empty names without calling the intern pool.
    if(len&&(!b.intern||!b.intern(b.context,name,&key)))return rollback();
    write(group,0,0u);write(group,4,key);write(group,0x48,0u);write(group,0x50,0u);write(group,0x58,0xffffffffu);
    write(group,0x74,0u);write(group,0x5c,0u);out.name_token=key;out.constructed=1;
    // A non-null empty string still links, whereas a null input does not.
    if(name){write(group,0,head);head=token;out.linked=1;}
    return true;
}
int __cdecl squad_lookup(std::uint32_t head,const char* name,const ServerNpcGruntSquadRegistryBindings* b,ServerNpcGruntSquadRegistryResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!b||!squad_registry_lookup(head,name,*b,*out))return 0;
    out->completed=1;return 1;
}
int __cdecl squad_create(std::uint32_t* head,const char* name,const ServerNpcGruntSquadRegistryBindings* b,ServerNpcGruntSquadRegistryResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!head||!b||!squad_registry_create(*head,name,*b,*out))return 0;
    out->completed=1;return 1;
}
int __cdecl squad_attach(void* a,unsigned n,std::uint32_t handle,float now,std::uint32_t* head,const char* name,const ServerNpcGruntSquadRegistryBindings* b,const ServerNpcGruntSquadJoinBindings* join,ServerNpcGruntSquadRegistryResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!head||!b||!join||!std::isfinite(now)||!squad_registry_lookup(*head,name,*b,*out))return 0;
    if(!out->found&&!squad_registry_create(*head,name,*b,*out))return 0;
    void* group=nullptr;unsigned extent=0;void* other=nullptr;
    if(!memory_storage(&b->storage,out->token,0x78,group,extent)||!speech_squad(join->group.sentence.speech,out->token,other)||group!=other)return 0;
    if(!grunt_squad_join(a,n,out->token,out->array_token,handle,now,join,&out->join))return 0;
    out->completed=1;return 1;
}
int __cdecl squad_input(void* a,unsigned n,std::uint32_t handle,float now,std::uint32_t* head,unsigned type,std::uint32_t token,const ServerNpcGruntSquadInputBindings* input,ServerNpcGruntSquadInputResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!input)return 0;const auto* b=&input->registry;const auto* join=&input->join;
    if(!a||n<0xdb2||!head||!std::isfinite(now)||!relation_grunt_class(join->group.sentence.speech.original_vtable))return 0;
    auto done=[&](){out->completed=1;return 1;};
    // All eight +5c0 = 22030360: owner capabilities OR live weapon +4a0.
    const auto& sb=join->group.sentence;
    if(sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    unsigned caps=read<unsigned>(a,0x8ec),weapon_caps=0;
    if(void* weapon=sin_independent::EntityHandleView(sb.entries,sb.count).Resolve(read<unsigned>(a,0x78c))){
        ++out->weapon_queried;
        if(!input->weapon_capabilities||!input->weapon_capabilities(input->context,weapon,&weapon_caps))return 0;
        caps|=weapon_caps;
    }
    if(!(caps&0x04000000u)){out->capability_blocked=1;return done();}
    if(type!=2){token=0;out->nonstring=1;}
    write(a,0x9c8,token);out->name_token=token;
    if(!token){
        out->clearing=1;
        const auto old=read<unsigned>(a,0x9c4);
        if(old){
            if(!grunt_squad_leave(a,n,old,handle,true,now,&join->group,&out->departure))return 0;
            write(a,0x9c4,0u);out->owner_written=1;
        }
        return done();
    }
    const char* name=nullptr;unsigned extent=0,len=0;
    if(!b->name||!b->name(b->context,token,&name,&extent)||!name||!squad_name_size(name,extent,len))return 0;
    if(!squad_attach(a,n,handle,now,head,name,b,join,&out->attachment))return 0;
    // Input explicitly assigns the returned group even if join declined (e.g. +8c).
    write(a,0x9c4,out->attachment.token);out->owner_written=1;
    return done();
}
int __cdecl grunt_squad_initialize(void* a,unsigned n,std::uint32_t handle,float now,std::uint32_t* head,const ServerNpcGruntSquadInitBindings* b,ServerNpcGruntSquadInitResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xdb2||!head||!b||!std::isfinite(now)||!relation_grunt_class(b->join.group.sentence.speech.original_vtable))return 0;
    auto done=[&](){out->completed=1;return 1;};
    write(a,0x8ec,read<unsigned>(a,0x8ec)|0x04000000u);out->capability_set=1;
    if(!b->base_initialize||!b->base_initialize(b->context,a))return 0;
    out->base_called=1;
    if(read<unsigned>(a,0xc14)){
        if(!b->optional_initialize||!b->optional_initialize(b->context,a))return 0;
        out->optional_called=1;
    }
    if(read<unsigned>(a,0x9c4)){out->existing_group=1;return done();}
    out->name_token=read<unsigned>(a,0x9c8);
    const char* name=nullptr;unsigned extent=0,len=0;
    if(out->name_token&&(!b->registry.name||!b->registry.name(b->registry.context,out->name_token,&name,&extent)||!name||!squad_name_size(name,extent,len)))return 0;
    if(!squad_attach(a,n,handle,now,head,name,&b->registry,&b->join,&out->attachment))return 0;
    write(a,0x9c4,out->attachment.token);out->owner_written=1;
    return done();
}
const ServerNpcGruntSquadInitApi squad_init_api{1,sizeof(ServerNpcGruntSquadInitApi),&grunt_squad_initialize};
const ServerNpcGruntSquadInputApi squad_input_api{1,sizeof(ServerNpcGruntSquadInputApi),&squad_input};
int __cdecl squad_destroy(std::uint32_t* head,std::uint32_t token,const ServerNpcGruntSquadDestroyBindings* b,ServerNpcGruntSquadDestroyResult* out) noexcept{
    if(!out)return 0;*out={};out->token=token;
    if(!head||!b)return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(!token)return done();
    void* target=nullptr;unsigned n=0;
    if(!memory_storage(&b->registry.storage,token,0x78,target,n)||!b->registry.release)return 0;
    // Host ownership guard: original raw destructor assumes no surviving references.
    if(read<int>(target,0x48)!=0){out->empty_required=1;return done();}
    void* predecessor=nullptr;
    for(auto current=*head;current;){
        if(++out->visited>4096)return 0;
        void* group=nullptr;if(!memory_storage(&b->registry.storage,current,0x78,group,n))return 0;
        if(current==token){
            if(group!=target)return 0;
            const auto successor=read<unsigned>(group,0);out->successor=successor;
            if(successor==token)return 0;
            if(predecessor)write(predecessor,0,successor);else *head=successor;
            out->unlinked=1;break;
        }
        predecessor=group;out->predecessor=current;current=read<unsigned>(group,0);
    }
    // 220a98b0 on group+60. Grow-size -1 denotes externally owned storage.
    write(target,0x6c,0u);
    if(read<int>(target,0x68)!=-1){
        if(const auto records=read<unsigned>(target,0x60)){
            if(!b->release_records)return 0;++out->record_release_calls;
            if(!b->release_records(b->context,records))return 0;
            write(target,0x60,0u);
        }
        write(target,0x64,0u);
    }else out->external_records=1;
    write(target,0x70,read<unsigned>(target,0x60));
    // Second original storage cleanup sees zero on the stable owned-storage path.
    if(read<int>(target,0x68)!=-1){
        if(const auto records=read<unsigned>(target,0x60)){
            if(!b->release_records)return 0;++out->record_release_calls;
            if(!b->release_records(b->context,records))return 0;
            write(target,0x60,0u);
        }
        write(target,0x64,0u);
    }
    write(target,0x48,0u); // inline marker already canonical in the bounded representation
    out->inline_reset=1;
    if(!b->registry.release(b->registry.context,token))return 0;
    out->group_released=1;return done();
}
bool slots_init(float now,const ServerNpcGruntSquadSlotsBindings* b,ServerNpcGruntSquadSlotsResult* out) noexcept{
    if(!out)return false;*out={};out->selected=out->index=-1;
    return b&&std::isfinite(now)&&b->count<=sin_independent::kEntityHandleCapacity&&(!b->count||b->entries);
}
bool slots_get(std::uint32_t token,void* target,float now,const ServerNpcGruntSquadSlotsBindings& b,ServerNpcGruntSquadSlotsResult& out,void*& selected) noexcept{
    selected=nullptr;++out.accesses;
    void* group=nullptr;unsigned n=0;
    if(!memory_storage(&b.storage,token,0x78,group,n))return false;
    int count=read<int>(group,0x6c),capacity=read<int>(group,0x64);
    auto base=read<unsigned>(group,0x60);void* storage=nullptr;
    if(count<0||capacity<0||count>capacity||capacity>4096)return false;
    if(capacity&&(!base||base>0xffffffffu-8u*static_cast<unsigned>(capacity)||!memory_storage(&b.storage,base,8*capacity,storage,n)))return false;
    auto row=[&](int i){return static_cast<unsigned char*>(storage)+8*i;};
    const auto registry=sin_independent::EntityHandleView(b.entries,b.count);
    auto choose=[&](int i){
        selected=row(i);out.index=i;out.record_token=base+8u*static_cast<unsigned>(i);out.bitmap=read<unsigned>(selected,4);
        write(group,0x5c,out.record_token);return true;
    };
    out.deadline=read<float>(group,0x74);if(!std::isfinite(out.deadline))return false;
    if(now>out.deadline){
        ++out.refreshed;
        if(count){
            const int members=read<int>(group,0x48);if(members<0||members>16)return false;
            write(group,0x5c,0u);
            void* active[16]{};unsigned active_count=0;
            // Native temporary pointer-key tree only needs membership here.
            for(int i=0;i<members;++i){
                void* member=registry.Resolve(read<unsigned>(group,8+4*i));if(!member)return false;
                ++out.refresh_members;
                unsigned vt=0,extent=0;void* enemy=nullptr;
                if(!b.member_class||!b.member_class(b.context,member,&vt,&extent))return false;
                if(relation_grunt_class(vt)){if(extent<0x8a4)return false;enemy=registry.Resolve(read<unsigned>(member,0x8a0));}
                else if(!b.other_enemy||!b.other_enemy(b.context,member,&enemy))return false;
                if(enemy)active[active_count++]=enemy;
            }
            for(int i=count-1;i>=0;--i){
                void* e=registry.Resolve(read<unsigned>(row(i),0));bool keep=false;
                if(e)for(unsigned j=0;j<active_count;++j)if(active[j]==e){keep=true;break;}
                if(!keep){
                    --count;if(i!=count)std::memcpy(row(i),row(count),8);
                    write(group,0x6c,count);++out.pruned;
                }
            }
        }
        out.deadline=static_cast<float>(static_cast<double>(now)+30.0);if(!std::isfinite(out.deadline))return false;
        write(group,0x74,out.deadline);
    }
    const auto cached=read<unsigned>(group,0x5c);
    if(cached){
        if(!storage||cached<base||(cached-base)%8||((cached-base)/8)>=static_cast<unsigned>(count))return false;
        const int i=static_cast<int>((cached-base)/8);
        if(registry.Resolve(read<unsigned>(row(i),0))==target){++out.cached;return choose(i);}
    }
    for(int i=0;i<count;++i){++out.scanned;if(registry.Resolve(read<unsigned>(row(i),0))==target)return choose(i);}
    write(group,0x5c,0u);
    if(count==capacity){
        const int growth=read<int>(group,0x68);if(growth<0||!b.resize)return false;
        int next=capacity?capacity:4;
        if(capacity){if(growth){if(growth>4096-capacity)return false;next=capacity+growth;}else next=capacity*2;}
        if(next<=capacity||next>4096)return false;
        std::uint32_t new_base=0;void* new_storage=nullptr;unsigned extent=0;
        if(!b.resize(b.context,base,static_cast<unsigned>(next),&new_base,&new_storage,&extent)||!new_base||!new_storage||extent<8u*static_cast<unsigned>(next)||new_base>0xffffffffu-8u*static_cast<unsigned>(next))return false;
        void* verified=nullptr;
        if(!memory_storage(&b.storage,new_base,8*next,verified,extent)||verified!=new_storage)return false;
        base=new_base;storage=new_storage;capacity=next;write(group,0x60,base);write(group,0x64,capacity);++out.grown;
    }
    unsigned handle=0xffffffffu;
    if(target&&(!b.entity_handle||!b.entity_handle(b.context,target,&handle)||registry.Resolve(handle)!=target))return false;
    write(group,0x6c,count+1);write(group,0x70,base);write(row(count),0,handle);write(row(count),4,0u);++out.appended;
    return choose(count);
}
int __cdecl slots_read(std::uint32_t token,void* target,float now,const ServerNpcGruntSquadSlotsBindings* b,ServerNpcGruntSquadSlotsResult* out) noexcept{
    if(!slots_init(now,b,out))return 0;void* row=nullptr;
    if(!slots_get(token,target,now,*b,*out,row))return 0;out->completed=1;return 1;
}
bool slots_valid_bit(int bit) noexcept{return bit>=0&&bit<32;}
int __cdecl slots_bit(std::uint32_t token,void* target,int bit,unsigned op,float now,const ServerNpcGruntSquadSlotsBindings* b,ServerNpcGruntSquadSlotsResult* out) noexcept{
    if(!slots_init(now,b,out)||!slots_valid_bit(bit)||op>NpcSquadBitClear)return 0;void* row=nullptr;
    if(!slots_get(token,target,now,*b,*out,row))return 0;
    const auto mask=1u<<bit;out->bit_value=(out->bitmap&mask)!=0;
    if(op==NpcSquadBitSet){out->bitmap|=mask;write(row,4,out->bitmap);}
    else if(op==NpcSquadBitClear){out->bitmap&=~mask;write(row,4,out->bitmap);}
    out->completed=1;return 1;
}
int __cdecl slots_reserve(void* a,unsigned n,int first,int last,float now,const ServerNpcGruntSquadSlotsBindings* b,ServerNpcGruntSquadSlotsResult* out) noexcept{
    if(!slots_init(now,b,out)||!a||n<0x9d0)return 0;
    auto done=[&](bool yes){out->allowed=yes;out->completed=1;return 1;};
    const auto token=read<unsigned>(a,0x9c4);out->selected=read<int>(a,0x9cc);
    if(!token){out->ungrouped=1;return done(true);}
    if(first>last)return done(false);
    if(!slots_valid_bit(first)||!slots_valid_bit(last))return 0;
    if(out->selected>=first&&out->selected<=last){out->retained=1;return done(true);}
    if(out->selected!=-1&&!slots_valid_bit(out->selected))return 0;
    void* target=sin_independent::EntityHandleView(b->entries,b->count).Resolve(read<unsigned>(a,0x8a0));void* row=nullptr;
    for(int bit=first;bit<=last;++bit){
        if(!slots_get(token,target,now,*b,*out,row))return 0;
        const auto mask=1u<<bit;if(out->bitmap&mask)continue;
        const int old=read<int>(a,0x9cc);
        if(old!=-1){
            if(!slots_get(token,target,now,*b,*out,row))return 0;
            out->old_unmarked=(out->bitmap&(1u<<old))==0;
            if(!slots_get(token,target,now,*b,*out,row))return 0;
            write(row,4,out->bitmap&~(1u<<old));out->old_cleared=1;
        }
        if(!slots_get(token,target,now,*b,*out,row))return 0;
        out->bitmap|=mask;write(row,4,out->bitmap);write(a,0x9cc,bit);out->selected=bit;out->new_set=1;
        return done(true);
    }
    return done(false);
}
int __cdecl slots_occupied(std::uint32_t token,void* target,int first,int last,float now,const ServerNpcGruntSquadSlotsBindings* b,ServerNpcGruntSquadSlotsResult* out) noexcept{
    if(!slots_init(now,b,out))return 0;
    if(first<=last&&(!slots_valid_bit(first)||!slots_valid_bit(last)))return 0;
    out->allowed=1;void* row=nullptr;
    for(int bit=first;bit<=last;++bit){
        if(!slots_get(token,target,now,*b,*out,row))return 0;
        if(!(out->bitmap&(1u<<bit))){out->allowed=0;break;}
    }
    out->completed=1;return 1;
}
int __cdecl slots_release(std::uint32_t token,void* target,int bit,float now,const ServerNpcGruntSquadSlotsBindings* b,ServerNpcGruntSquadSlotsResult* out) noexcept{
    if(!slots_init(now,b,out))return 0;
    if(bit!=-1){
        if(!slots_valid_bit(bit))return 0;void* row=nullptr;
        if(!slots_get(token,target,now,*b,*out,row))return 0;
        const auto mask=1u<<bit;out->old_unmarked=(out->bitmap&mask)==0;
        if(!slots_get(token,target,now,*b,*out,row))return 0;
        out->bitmap&=~mask;write(row,4,out->bitmap);out->old_cleared=1;
    }
    out->completed=1;return 1;
}
void reset_perception_slots(ServerNpcGruntPerceptionSlotsResult& out) noexcept{
    reset_perception_speech(out.dialogue);out.last_release={};out.last_release.index=out.last_release.selected=-1;out.release_calls=0;
}
int __cdecl slots_set_enemy(void* a,unsigned n,void* target,bool notify,float now,const ServerNpcEnemyStateBindings* b,const ServerNpcGruntSquadSlotsBindings* slots,ServerNpcEnemyStateResult* out,ServerNpcGruntPerceptionSlotsResult* result) noexcept{
    if(!slots||!result)return 0;reset_perception_slots(*result);const SquadReleaseRoute route{*slots,*result};
    return set_enemy_impl(a,n,target,notify,now,b,out,true,&route);
}
int __cdecl slots_enemy_switch(void* a,unsigned n,float now,const ServerNpcGruntEnemySwitchBindings* b,const ServerNpcGruntPerceptionSlotsBindings* slots,ServerNpcGruntEnemySwitchResult* out,ServerNpcGruntPerceptionSlotsResult* result) noexcept{
    if(!slots||!result)return 0;reset_perception_slots(*result);const SquadReleaseRoute release{slots->slots,*result};
    const PerceptionSpeechRoute route{slots->dialogue,result->dialogue,&release};
    return grunt_enemy_switch_impl(a,n,now,b,out,nullptr,&route);
}
int __cdecl slots_perception_update(void* a,unsigned n,float now,const ServerNpcGruntConditionUpdateBindings* b,const ServerNpcGruntPerceptionSlotsBindings* slots,ServerNpcGruntConditionUpdateResult* out,ServerNpcGruntPerceptionSlotsResult* result) noexcept{
    if(!slots||!result)return 0;reset_perception_slots(*result);const SquadReleaseRoute release{slots->slots,*result};
    const PerceptionSpeechRoute route{slots->dialogue,result->dialogue,&release};
    return grunt_condition_update_impl(a,n,now,b,out,&route);
}
int __cdecl slots_perception_gather(void* a,unsigned n,float now,const ServerNpcGruntGatherNativeBindings* b,const ServerNpcGruntPerceptionSlotsBindings* slots,ServerNpcGruntGatherNativeResult* out,ServerNpcGruntPerceptionSlotsResult* result) noexcept{
    if(!slots||!result||!out||!b)return 0;reset_perception_slots(*result);out->gather={};out->point={};out->object={};out->object.selected_handle=0xffffffffu;
    const SquadReleaseRoute release{slots->slots,*result};const PerceptionSpeechRoute route{slots->dialogue,result->dialogue,&release};
    return grunt_gather_conditions_impl(a,n,now,&b->gather,&out->gather,&b->point,&out->point,&b->object,&out->object,&route);
}
void reset_squad_report(ServerNpcGruntSquadReportResult& out) noexcept{
    out={};reset_perception_slots(out.perception);reset_perception_speech(out.last_dialogue);
}
int __cdecl squad_report(void* a,unsigned n,void* target,const float* pos,void* informer,float now,const ServerNpcGruntMemoryReportBindings* b,const ServerNpcGruntSquadReportBindings* reports,ServerNpcGruntMemoryReportResult* out,ServerNpcGruntSquadReportResult* result) noexcept{
    if(!reports||!result)return 0;reset_squad_report(*result);
    const SquadReportRoute forward{*reports,*result};const PerceptionSpeechRoute route{reports->perception.dialogue,result->perception.dialogue,nullptr,&forward};
    return grunt_memory_report_impl(a,n,target,pos,informer,now,b,out,&route);
}
int __cdecl squad_report_update(void* a,unsigned n,float now,const ServerNpcGruntConditionUpdateBindings* b,const ServerNpcGruntSquadReportBindings* reports,ServerNpcGruntConditionUpdateResult* out,ServerNpcGruntSquadReportResult* result) noexcept{
    if(!reports||!result)return 0;reset_squad_report(*result);
    const SquadReleaseRoute release{reports->perception.slots,result->perception};const SquadReportRoute forward{*reports,*result};
    const PerceptionSpeechRoute route{reports->perception.dialogue,result->perception.dialogue,&release,&forward};
    return grunt_condition_update_impl(a,n,now,b,out,&route);
}
int __cdecl squad_report_gather(void* a,unsigned n,float now,const ServerNpcGruntGatherNativeBindings* b,const ServerNpcGruntSquadReportBindings* reports,ServerNpcGruntGatherNativeResult* out,ServerNpcGruntSquadReportResult* result) noexcept{
    if(!reports||!result||!out||!b)return 0;reset_squad_report(*result);out->gather={};out->point={};out->object={};out->object.selected_handle=0xffffffffu;
    const SquadReleaseRoute release{reports->perception.slots,result->perception};const SquadReportRoute forward{*reports,*result};
    const PerceptionSpeechRoute route{reports->perception.dialogue,result->perception.dialogue,&release,&forward};
    return grunt_gather_conditions_impl(a,n,now,&b->gather,&out->gather,&b->point,&out->point,&b->object,&out->object,&route);
}
int __cdecl squad_enemy_broadcast(std::uint32_t token,void* target,float now,const ServerNpcGruntSquadEnemyBindings* b,ServerNpcGruntSquadEnemyResult* out) noexcept{
    if(!out)return 0;*out={};out->last_seen=-std::numeric_limits<float>::max();
    if(!b||!std::isfinite(now)||b->slots.count>sin_independent::kEntityHandleCapacity||(b->slots.count&&!b->slots.entries))return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(!target){out->null_target=1;return done();}
    const auto& sb=b->slots;void* group=nullptr;unsigned gn=0;
    if(!memory_storage(&sb.storage,token,0x4c,group,gn))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    for(int i=0;;++i){
        const int count=read<int>(group,0x48);if(count<0||count>16)return 0;
        if(i>=count)break;
        ++out->visited;void* actor=registry.Resolve(read<unsigned>(group,8+4*i));
        if(!actor){++out->stale;continue;}
        unsigned vt=0,n=0;ServerNpcEnemyStateBindings enemy{};ServerNpcEnemyMemoryQueryBindings memory{};
        if(!b->member||!b->member(b->context,actor,&vt,&n,&enemy,&memory))return 0;
        if(!relation_grunt_class(vt)){
            ++out->external_members;
            if(!b->other_member||!b->other_member(b->context,actor,target,now))return 0;
            continue;
        }
        ++out->native_members;
        if(n<0xc04)return 0;
        void* current=registry.Resolve(read<unsigned>(actor,0x8a0));
        if(current){
            // +154 and +150 are pure handle resolvers for all eight classes.
            if(current==target){++out->same;continue;}
            ServerNpcConditionResult cr{};
            if(!cond->test(actor,n,0xa,NpcConditionCurrent,&enemy.conditions,&cr))return 0;
            if(cr.value){++out->condition_blocked;continue;}
            unsigned key=0;void* owner=nullptr;unsigned mn=0;
            if(!memory_key(memory,current,key)||!memory_storage(&memory.storage,read<unsigned>(actor,0x984),0x40,owner,mn)||
                !memory_timestamp(owner,mn,key,NpcMemoryStamp1c,&memory.storage,&out->last_lookup,&out->last_seen)||!std::isfinite(out->last_seen))return 0;
            // Original x87 difference must be strictly greater than 3.0.
            if(static_cast<double>(now)-out->last_seen<=3.0){++out->recent;continue;}
        }else ++out->unassigned;
        enemy.entries=sb.entries;enemy.count=sb.count;
        reset_perception_slots(out->last_slots);const SquadReleaseRoute release{sb,out->last_slots};
        if(!set_enemy_impl(actor,n,target,true,now,&enemy,&out->last_assignment,true,&release))return 0;
        ++out->assigned;
    }
    return done();
}
bool native_state_kill(void* a,unsigned n,void* e,const ServerNpcStateBindings& b,const NativeStateRoute& route) noexcept{
    if(!e)return true;
    const auto& s=route.bindings;auto& out=route.out;
    if(!s.target_classification||!s.target_classification(s.context,e,&out.classification))return false;
    out.cue=out.classification==1?"GRUNT_PLAYER_DEAD":out.classification==10?"GRUNT_KILLMUTANT":"GRUNT_KILL_MONST";
    auto sentence=s.sentence;sentence.speech.original_vtable=s.original_vtable;sentence.entries=b.entities.entries;sentence.count=b.entities.count;
    ++out.cue_calls;
    return sentence_dispatch_request(a,n,route.now,out.cue,2,2,&sentence,&s.request,&out.sentence,&out.request)!=0;
}
bool native_state_broadcast(std::uint32_t token,void* target,const ServerNpcStateBindings& b,const NativeStateRoute& route) noexcept{
    auto bound=route.bindings.squad;bound.slots.entries=b.entities.entries;bound.slots.count=b.entities.count;
    ++route.out.broadcast_calls;
    return squad_enemy_broadcast(token,target,route.now,&bound,&route.out.broadcast)!=0;
}
bool native_state_clear(void* a,unsigned n,const ServerNpcStateBindings& b,const NativeStateRoute& route) noexcept{
    const auto& squad=route.bindings.squad;ServerNpcEnemyStateBindings bound{};ServerNpcEnemyMemoryQueryBindings memory{};
    unsigned vt=0,extent=0;
    if(!squad.member||!squad.member(squad.context,a,&vt,&extent,&bound,&memory)||vt!=route.bindings.original_vtable||extent<n)return false;
    bound.entries=b.entities.entries;bound.count=b.entities.count;bound.conditions=b.schedule.conditions;
    ++route.out.clear_calls;
    reset_perception_slots(route.out.clear_slots);
    const SquadReleaseRoute release{squad.slots,route.out.clear_slots};
    return set_enemy_impl(a,n,nullptr,true,route.now,&bound,&route.out.clear,true,&release)!=0;
}
bool state_route_init(float now,const ServerNpcGruntNativeStateBindings* b,ServerNpcGruntNativeStateResult* out) noexcept{
    if(!out)return false;*out={};out->classification=-1;out->sentence.request_result=-1;
    out->request.group_id=out->request.pick_result=out->request.sentence_id=out->request.request_result=-1;
    return b&&relation_grunt_class(b->original_vtable)&&std::isfinite(now);
}
int __cdecl native_state_choose(void* a,unsigned n,float now,const ServerNpcStateBindings* b,const ServerNpcGruntNativeStateBindings* native,ServerNpcGruntNativeStateResult* out) noexcept{
    if(!state_route_init(now,native,out))return 0;const NativeStateRoute route{*native,*out,now};
    return choose_ideal_impl(a,n,b,&out->state,&route);
}
int __cdecl native_state_apply(void* a,unsigned n,int desired,float now,const ServerNpcStateBindings* b,const ServerNpcGruntNativeStateBindings* native,ServerNpcGruntNativeStateResult* out) noexcept{
    if(!state_route_init(now,native,out))return 0;const NativeStateRoute route{*native,*out,now};
    return apply_impl(a,n,desired,now,b,&out->state,&route);
}
int __cdecl grunt_removal(void* a,unsigned n,std::uint32_t handle,void* attacker,bool event,float now,const ServerNpcGruntRemovalBindings* b,ServerNpcGruntRemovalResult* out) noexcept{
    if(!out)return 0;*out={};out->release.index=out->release.selected=out->departure.index=-1;
    if(!a||n<0xdb2||!b||!std::isfinite(now))return 0;
    const auto& sb=b->leave.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(read<std::uint8_t>(a,0xa44)){out->repeated=1;return done();}
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    if(registry.Resolve(handle)!=a)return 0;
    write<std::uint8_t>(a,0xa44,1);out->latched=1;
    if(read<int>(a,0x850)==4){
        if(void* link=registry.Resolve(read<unsigned>(a,0x970))){
            if(!b->state4_link||!b->state4_link(b->context,a,link))return 0;
            out->state4_link=1;
        }
    }
    if(void* link=registry.Resolve(read<unsigned>(a,0xa18))){
        if(!b->clear_link||!b->clear_link(b->context,a,link)||!b->clear_owner||!b->clear_owner(b->context,a))return 0;
        out->cleared_link=1;
    }
    if(event){if(!b->event||!b->event(b->context,a,attacker))return 0;out->event=1;}
    auto slots=b->reports.perception.slots;slots.entries=sb.entries;slots.count=sb.count;
    if(const auto token=read<unsigned>(a,0x9c4)){
        if(!slots_release(token,registry.Resolve(read<unsigned>(a,0x8a0)),read<int>(a,0x9cc),now,&slots,&out->release))return 0;
        write(a,0x9cc,-1);out->slot_released=1;
    }
    if(read<unsigned>(a,0x9c4)){
        if(read<int>(a,0x850)==1&&attacker){
            void* memory=nullptr;unsigned extent=0;
            if(!memory_storage(&b->memory.mutation.storage,read<unsigned>(a,0x984),0x1c,memory,extent)||
                !memory_lookup(memory,extent,0,false,&b->memory.mutation.storage,&out->zero_key))return 0;
            out->report_checked=1;
            if(!out->zero_key.record){
                float origin[3]{};
                if(!b->origin||!b->origin(b->context,a,origin)||!finite_vec(origin))return 0;
                auto report=b->memory;report.original_vtable=sb.speech.original_vtable;report.mutation.entries=sb.entries;report.mutation.count=sb.count;
                if(!squad_report(a,n,attacker,origin,nullptr,now,&report,&b->reports,&out->report,&out->forwarding))return 0;
                out->report_called=1;
            }
        }
        // Report callback may have changed the current group, matching original reload.
        const auto token=read<unsigned>(a,0x9c4);
        if(!token||!grunt_squad_leave(a,n,token,handle,true,now,&b->leave,&out->departure))return 0;
        write(a,0x9c4,0u);out->owner_cleared=1;
    }
    if(!b->finalize||!b->finalize(b->context,a))return 0;
    out->finalized=1;return done();
}
bool reaction_init(void* a,unsigned n,float now,const ServerNpcGruntReactionSpeechBindings* b,ServerNpcGruntReactionSpeechResult* out) noexcept{
    if(!out)return false;*out={};out->sentence.request_result=-1;out->request.group_id=out->request.pick_result=out->request.sentence_id=out->request.request_result=-1;
    return a&&n>=0xdb2&&b&&std::isfinite(now)&&relation_grunt_class(b->sentence.speech.original_vtable);
}
int __cdecl grunt_pain_speech(void* a,unsigned n,float now,const ServerNpcGruntReactionSpeechBindings* b,ServerNpcGruntReactionSpeechResult* out) noexcept{
    if(!reaction_init(a,n,now,b,out))return 0;
    auto done=[&](){out->completed=1;return 1;};
    const auto flags=read<unsigned>(a,0xbc);
    if(flags&0x10000000u){out->blocked_flags=1;return done();}
    out->deadline=read<float>(a,0xc78);
    if(!std::isfinite(out->deadline))return 0;
    if(now<=out->deadline){out->blocked_deadline=1;return done();}
    if(flags&0x08000000u){out->blocked_flags=1;return done();}
    const int denom=read<int>(a,0x98);if(!denom)return 0;
    out->ratio=static_cast<double>(read<int>(a,0x9c))/denom;
    const auto marks=read<unsigned>(a,0x988);
    out->cue="GRUNT_PAIN";
    if(!(marks&0x80000000u)&&out->ratio>0.9){
        write(a,0x988,marks|0x80000000u);out->taunt_marked=1;out->cue="GRUNT_TAUNT";
    }else if(!(marks&0x40000000u)&&out->ratio<0.25){
        write(a,0x988,marks|0x40000000u);out->pain_marked=1;
    }
    if(!sentence_dispatch_request(a,n,now,out->cue,-1,0,&b->sentence,&b->request,&out->sentence,&out->request))return 0;
    out->deadline=static_cast<float>(static_cast<double>(now)+1.0);if(!std::isfinite(out->deadline))return 0;
    write(a,0xc78,out->deadline);out->deadline_written=1;return done();
}
int __cdecl grunt_death_speech(void* a,unsigned n,float now,const ServerNpcGruntReactionSpeechBindings* b,ServerNpcGruntReactionSpeechResult* out) noexcept{
    if(!reaction_init(a,n,now,b,out))return 0;
    auto done=[&](){out->completed=1;return 1;};
    const auto vt=b->sentence.speech.original_vtable;
    const bool jet=vt==0x2248c514||vt==0x2248e83c||vt==0x2249964c||vt==0x224a4a4c;
    if(jet){
        if(n<0x1008)return 0;
        if(read<int>(a,0x1004)==5){out->blocked_state=1;return done();}
        if(read<std::uint8_t>(a,0xc44)){out->blocked_damage=1;return done();}
    }
    if(read<std::uint8_t>(a,0xbf)&0x10){out->blocked_flags=1;return done();}
    if(!read<std::uint8_t>(a,0xcb1)){out->blocked_ready=1;return done();}
    if(read<std::uint8_t>(a,0xc44)){out->blocked_damage=1;return done();}
    out->cue=read<int>(a,0xc68)==10?"GRUNT_MUTANTDEATH":"GRUNT_DIE";
    if(!sentence_dispatch_request(a,n,now,out->cue,-1,0,&b->sentence,&b->request,&out->sentence,&out->request))return 0;
    return done();
}
int __cdecl grunt_event_speech(void* a,unsigned n,unsigned event,float now,const ServerNpcGruntEventSpeechBindings* b,ServerNpcGruntEventSpeechResult* out) noexcept{
    if(!out)return 0;*out={};out->roll=-1;out->sentence.request_result=-1;out->request.group_id=out->request.pick_result=out->request.sentence_id=out->request.request_result=-1;
    if(!a||n<0xdb2||!b||!std::isfinite(now))return 0;
    const auto& sb=b->speech.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    auto done=[&](){out->completed=1;return 1;};
    if(event==0){
        if(!b->random_int||!b->random_int(b->context,0,5,&out->roll)||out->roll<0||out->roll>5)return 0;
        if(out->roll>1){out->roll_blocked=1;return done();}
        void* enemy=sin_independent::EntityHandleView(sb.entries,sb.count).Resolve(read<unsigned>(a,0x8a0));
        void* target=nullptr;
        if(enemy&&(!b->enemy_target||!b->enemy_target(b->context,enemy,&target)))return 0;
        if(!target){out->target_missing=1;return done();}
        if(!speech_admission(a,now,0,sb.speech,out->admission))return 0;
        if(out->admission.priority_blocked||out->admission.state_blocked){out->admission_blocked=1;return done();}
        float p[3]{},q[3]{};
        if(!b->position||!b->position(b->context,target,p)||!b->position(b->context,a,q)||!finite_vec(p)||!finite_vec(q))return 0;
        for(unsigned i=0;i<3;++i){const double d=static_cast<double>(q[i])-p[i];out->distance_squared+=d*d;}
        if(out->distance_squared>4000000.0){out->distance_blocked=1;return done();}
        bool yes=false;
        if(!b->target_accept||!b->target_accept(b->context,target,a,&yes))return 0;
        if(!yes){out->target_blocked=1;return done();}
        if(!b->visible||!b->visible(b->context,a,target,0x4081,0,&yes))return 0;
        if(!yes){out->visibility_blocked=1;return done();}
        out->cue="GRUNT_ASSAULT";
    }else if(event==1){
        const auto token=read<unsigned>(a,0x8f8);
        if(!token||!b->flank_metric||!b->flank_metric(b->context,token,&out->metric)||!std::isfinite(out->metric))return 0;
        if(out->metric<=240.f){out->metric_blocked=1;return done();}
        out->cue="GRUNT_FLANK";
    }else{out->ignored=1;return done();}
    if(!sentence_dispatch_request(a,n,now,out->cue,0,2,&sb,&b->speech.request,&out->sentence,&out->request))return 0;
    return done();
}
const ServerNpcGruntEventSpeechApi event_speech_api{1,sizeof(ServerNpcGruntEventSpeechApi),&grunt_event_speech};
const ServerNpcGruntReactionSpeechApi reaction_speech_api{1,sizeof(ServerNpcGruntReactionSpeechApi),&grunt_pain_speech,&grunt_death_speech};


int __cdecl grunt_state_signal(void*,unsigned,int,const ServerNpcGruntStateSignalBindings*,ServerNpcGruntStateSignalResult*) noexcept;
struct IgnitionRoute {const ServerNpcGruntIgnitionBindings* bindings;ServerNpcGruntIgnitionResult* result;};
int grunt_life_damage_impl(void*,unsigned,void*,unsigned,unsigned,const ServerNpcGruntLifeDamageBindings*,ServerNpcGruntLifeDamageResult*,const IgnitionRoute*) noexcept;
int __cdecl grunt_life_damage(void*,unsigned,void*,unsigned,unsigned,const ServerNpcGruntLifeDamageBindings*,ServerNpcGruntLifeDamageResult*) noexcept;
struct LifeDamageRoute {
    const ServerNpcGruntLifeDamageBindings* bindings;
    ServerNpcGruntLifeDamageResult* result;
    const IgnitionRoute* ignition=nullptr;
};
struct StateSignalRoute {
    const ServerNpcGruntStateSignalBindings* bindings;
    ServerNpcGruntStateSignalResult* result;
    const LifeDamageRoute* life=nullptr;
};
int bound_state_signal(void* a,unsigned n,int value,const ServerNpcGruntSentenceBindings& source,const StateSignalRoute& route) noexcept{
    auto b=*route.bindings;
    b.members.entries=source.entries;b.members.count=source.count;
    b.members.speech.original_vtable=source.speech.original_vtable;
    return grunt_state_signal(a,n,value,&b,route.result);
}
int grunt_damage_pain_prefix_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamagePainPrefixBindings* b,ServerNpcGruntDamagePainPrefixResult* out,const StateSignalRoute* route) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xdb2||!damage||dn<0x48||!b||!std::isfinite(now)||(!(route&&route->life)&&!b->base_damage))return 0;
    const auto& sb=b->reaction.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    write(a,0x988,read<unsigned>(a,0x988)&~2u);
    if(read<unsigned>(a,0x874)){write(damage,0x2c,0u);out->damage_zeroed=1;}
    if(route&&route->life){
        auto bound=*route->life->bindings;
        bound.entries=sb.entries;bound.count=sb.count;bound.original_vtable=sb.speech.original_vtable;
        if(!grunt_life_damage_impl(a,n,damage,dn,NpcDamageAlive,&bound,route->life->result,route->life->ignition))return 0;
        out->base_result=route->life->result->native_result;
    }else if(!b->base_damage(b->context,a,damage,&out->base_result))return 0;
    out->base_called=1;
    if(!out->base_result){out->completed=1;return 1;}
    if(read<int>(a,0x868)==1){
        if(route){
            if(!bound_state_signal(a,n,1,sb,*route))return 0;
        }else if(!b->state_signal||!b->state_signal(b->context,a,1))return 0;
        out->signal_called=1;
    }
    // Reload group and serial-checked attacker after both preceding calls.
    if(const auto group=read<unsigned>(a,0x9c4)){
        void* attacker=sin_independent::EntityHandleView(sb.entries,sb.count).Resolve(read<unsigned>(damage,0x28));
        // Original cast helper dereferences its argument. Null/stale is outside the supported domain.
        if(!attacker||!b->as_npc)return 0;
        bool npc=false;unsigned extent=0;
        if(!b->as_npc(b->context,attacker,&npc,&extent))return 0;
        out->attacker_checked=1;
        if(npc){
            if(extent<0x9c8)return 0;
            out->same_squad=read<unsigned>(attacker,0x9c4)==group;
        }
    }
    if(!out->same_squad){
        out->pain_called=1;
        if(!grunt_pain_speech(a,n,now,&b->reaction,&out->pain))return 0;
    }
    out->tail_required=1;out->completed=1;return 1;
}
int __cdecl grunt_damage_pain_prefix(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamagePainPrefixBindings* b,ServerNpcGruntDamagePainPrefixResult* out) noexcept{
    return grunt_damage_pain_prefix_impl(a,n,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntDamagePainPrefixApi damage_pain_prefix_api{1,sizeof(ServerNpcGruntDamagePainPrefixApi),&grunt_damage_pain_prefix};


int __cdecl grunt_behavior_switch(void* a,unsigned n,std::uint32_t next,float now,const ServerNpcGruntBehaviorSwitchBindings* b,ServerNpcGruntBehaviorSwitchResult* out) noexcept{
    if(!out)return 0;*out={};out->release.index=out->release.selected=-1;
    if(!a||n<0xc1c||!b||!std::isfinite(now)||!relation_grunt_class(b->original_vtable)||
       b->slots.count>sin_independent::kEntityHandleCapacity||(b->slots.count&&!b->slots.entries))return 0;
    out->previous=read<unsigned>(a,0xc18);out->requested=next;
    const bool changed=out->previous!=next;
    if(changed&&next&&(!b->begin||!b->activate||!b->activation_guard))return 0;
    if(changed&&out->previous&&!b->end)return 0;
    write(a,0xc18,next);
    if(!changed){out->completed=1;return 1;}
    out->changed=1;
    if(next){
        if(!b->begin(b->context,next))return 0;
        out->began=1;*b->activation_guard=1;
        // Native reloads the current pointer after +1c, not the requested token.
        out->activated=read<unsigned>(a,0xc18);
        const bool ok=out->activated&&b->activate(b->context,out->activated);
        *b->activation_guard=0; // also unwind adapter failure, unlike an unchecked native dereference
        if(!ok)return 0;
        out->activated_called=1;
    }
    if(out->previous){
        if(!b->end(b->context,out->previous))return 0;
        out->ended=1;
        if(const auto group=read<unsigned>(a,0x9c4)){
            const auto reg=sin_independent::EntityHandleView(b->slots.entries,b->slots.count);
            if(!slots_release(group,reg.Resolve(read<unsigned>(a,0x8a0)),read<int>(a,0x9cc),now,&b->slots,&out->release))return 0;
            write(a,0x9cc,-1);out->slot_released=1;
        }
    }
    // +778=2216e310 ret8 for all eight supported original vtables.
    out->completed=1;return 1;
}
const ServerNpcGruntBehaviorSwitchApi behavior_switch_api{1,sizeof(ServerNpcGruntBehaviorSwitchApi),&grunt_behavior_switch};

int __cdecl grunt_behavior_removal(void* a,unsigned n,std::uint32_t handle,void* attacker,bool event,float now,const ServerNpcGruntBehaviorRemovalBindings* b,ServerNpcGruntBehaviorRemovalResult* out) noexcept{
    if(!out)return 0;*out={};out->active_release.index=out->active_release.selected=-1;
    // Preflight the complete composed cleanup extent before component side effects.
    if(!a||n<0xdb2||!b||!std::isfinite(now))return 0;
    const auto& sb=b->removal.leave.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    if(registry.Resolve(handle)!=a)return 0;
    // Original loop re-reads count and array after each callback.
    for(int i=0;;++i){
        const int count=read<int>(a,0xc28);if(count>4096)return 0;
        if(i>=count)break;
        void* array=nullptr;unsigned extent=0;
        if(!memory_storage(&b->component_storage,read<unsigned>(a,0xc1c),4u*static_cast<unsigned>(count),array,extent))return 0;
        const auto component=read<unsigned>(array,4*i);
        if(!component||!b->component_removed||!b->component_removed(b->context,component,attacker,event))return 0;
        ++out->components;
    }
    ServerNpcGruntBehaviorSwitchBindings sw{};
    sw.slots=b->removal.reports.perception.slots;sw.slots.entries=sb.entries;sw.slots.count=sb.count;
    sw.original_vtable=sb.speech.original_vtable;sw.context=b->context;sw.end=b->component_end;
    ServerNpcGruntBehaviorSwitchResult sr{};
    const int switched=grunt_behavior_switch(a,n,0,now,&sw,&sr);
    out->previous_active=sr.previous;out->active_cleared=sr.changed;out->active_ended=sr.ended;
    out->slot_released=sr.slot_released;out->active_release=sr.release;
    if(!switched)return 0;
    if(!grunt_removal(a,n,handle,attacker,event,now,&b->removal,&out->removal))return 0;
    out->completed=1;return 1;
}
const ServerNpcGruntBehaviorRemovalApi behavior_removal_api{1,sizeof(ServerNpcGruntBehaviorRemovalApi),&grunt_behavior_removal};


bool death_dirty(void* a,const ServerNpcEnemyMemoryBindings& storage,unsigned& local,unsigned& external) noexcept{
    if(read<std::uint8_t>(a,0x1f0)){write<std::uint8_t>(a,0x200,1);++local;return true;}
    if(const auto token=read<unsigned>(a,0x16c)){
        void* p=nullptr;unsigned extent=0;
        if(!memory_storage(&storage,token,4,p,extent))return false;
        write(p,0,read<unsigned>(p,0)|1u);++external;
    }
    return true;
}

int __cdecl grunt_state_signal(void* a,unsigned n,int value,const ServerNpcGruntStateSignalBindings* b,ServerNpcGruntStateSignalResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xdb2||!b)return 0;
    const auto& sb=b->members;const auto vt=sb.speech.original_vtable;
    if(!relation_grunt_class(vt)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const bool jet=vt==0x2248c514||vt==0x2248e83c||vt==0x2249964c||vt==0x224a4a4c;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    if(jet&&read<unsigned>(a,0x868)){
        if(n<0x1028||!b->jet_prepare||!b->jet_prepare(b->context,a))return 0;
        out->jet_prepared=1;
        out->jet_handle=read<unsigned>(a,0x1024);
        void* linked=registry.Resolve(out->jet_handle);
        if(!linked||!b->member_signal||!b->member_signal(b->context,linked,1))return 0;
        out->jet_linked=1;
        const auto object=read<unsigned>(a,0x218);
        if(!object||!b->jet_object||!b->jet_object(b->context,object))return 0;
        out->jet_object_called=1;
    }
    // Jet override tail calls the base, which re-reads the state.
    if(!read<unsigned>(a,0x868)){out->completed=1;return 1;}
    write(a,0x868,0u);out->state_cleared=1;
    const auto flags=read<unsigned>(a,0x78);
    if(flags&0x20){
        if(!death_dirty(a,b->dirty_storage,out->local_dirty,out->external_dirty))return 0;
        write(a,0x78,flags&~0x20u);out->flag_cleared=1;
    }
    if(!b->refresh||!b->refresh(b->context,a))return 0;
    out->refreshed=1;
    if(static_cast<std::uint8_t>(value)){
        if(!b->output||!b->output(b->context,a,0xbc8))return 0;
        out->output_called=1;
    }
    if(!read<std::uint8_t>(a,0x870)||!read<unsigned>(a,0x9c4)){out->completed=1;return 1;}
    // First/next original iterators 220a9300/220a93c0 use the same filtered group order.
    // Re-fetch the group for each next call, but not within a single iterator scan.
    int i=0;
    for(;;){
        void* group=nullptr;
        const auto token=read<unsigned>(a,0x9c4);
        if(!token||!speech_squad(sb.speech,token,group))return 0;
        void* member=nullptr;
        for(;;++i){
            const int count=read<int>(group,0x48);
            if(count<0||count>16)return 0;
            if(i>=count)break;
            ++out->scans;
            void* e=registry.Resolve(read<unsigned>(group,8+4*i));
            bool yes=false;if(!sentence_eligible(e,sb,yes))return 0;
            if(yes){
                ++out->eligible;
                // Both original iterator methods resolve the current row again after eligibility.
                member=registry.Resolve(read<unsigned>(group,8+4*i));
                break;
            }
        }
        if(!member)break;
        unsigned extent=0;
        if(!sb.extent||!sb.extent(sb.speech.context,member,&extent)||extent<0x871)return 0;
        if(read<int>(member,0x8c)!=0)++out->dead;
        else if(member==a)++out->self;
        else{
            write<std::uint8_t>(member,0x870,0);
            if(!b->member_signal||!b->member_signal(b->context,member,1))return 0;
            ++out->propagated;
        }
        ++i;
        if(i>16)return 0;
    }
    out->completed=1;return 1;
}
const ServerNpcGruntStateSignalApi state_signal_api{1,sizeof(ServerNpcGruntStateSignalApi),&grunt_state_signal};


int __cdecl grunt_death_select(void* a,unsigned n,const void* damage,unsigned dn,std::uint32_t hitgroup,const ServerNpcGruntDeathSelectorBindings* b,ServerNpcGruntDeathSelectorResult* out) noexcept{
    if(!out)return 0;*out={};out->selection=-1;out->hitgroup=hitgroup;
    if(!a||n<0x298||!damage||dn<12||!b||!b->model_available)return 0;
    bool available=false;if(!b->model_available(b->context,a,&available))return 0;
    out->model_available=available;
    if(!available){out->completed=1;return 1;}
    if(read<unsigned>(a,0xb8)&0x800u){
        if(!b->refresh_origin||!b->refresh_origin(b->context,a))return 0;
        out->refreshed=1;
    }
    // All supported +1dc slots point to 22141910: matrix column 0,
    // negated column 1; the unused up-vector output is null.
    for(unsigned i=0;i<3;++i){
        out->forward[i]=read<float>(a,0x268+16*i);
        out->right[i]=-read<float>(a,0x26c+16*i);
        out->direction[i]=-read<float>(damage,4*i);
    }
    if(!finite_vec(out->forward)||!finite_vec(out->right)||!finite_vec(out->direction)||!b->normalize||!b->normalize(b->context,out->direction)||!finite_vec(out->direction))return 0;
    out->normalized=1;
    auto dot=[&](const float* v){return static_cast<double>(v[2])*out->direction[2]+static_cast<double>(v[1])*out->direction[1]+static_cast<double>(v[0])*out->direction[0];};
    out->forward_dot=dot(out->forward);out->right_dot=dot(out->right);
    if(!std::isfinite(out->forward_dot)||!std::isfinite(out->right_dot))return 0;
    out->right_dominant=std::abs(out->right_dot)>std::abs(out->forward_dot);
    out->selection=out->right_dominant?(out->right_dot<0?0x1a7:0x1a5):(out->forward_dot<0?0x1a6:0x1a4);
    constexpr unsigned groups[]{1,2,2,3,4,5,6};
    out->auxiliary=hitgroup>=1&&hitgroup<=7?groups[hitgroup-1]:1;
    out->completed=1;return 1;
}
struct DeathSelectorRoute {const ServerNpcGruntDeathSelectorBindings* bindings;ServerNpcGruntDeathSelectorResult* result;};

int grunt_death_prepare_impl(void* a,unsigned n,void* damage,unsigned dn,const ServerNpcGruntDeathPrepareBindings* b,ServerNpcGruntDeathPrepareResult* out,const DeathSelectorRoute* selector) noexcept{
    if(!out)return 0;*out={};out->selection=out->activity=-1;
    if(!a||n<0x898||!damage||dn<0x3c||!b||!b->model_available)return 0;
    bool available=false;if(!b->model_available(b->context,a,&available))return 0;
    out->model_available=available;
    if(!available){out->completed=1;return 1;}
    if(selector){
        if(!grunt_death_select(a,n,damage,dn,read<unsigned>(a,0x630),selector->bindings,selector->result))return 0;
        out->selection=selector->result->selection;out->auxiliary=selector->result->auxiliary;
    }else if(!b->select||!b->select(b->context,a,damage,read<unsigned>(a,0x630),&out->selection,&out->auxiliary))return 0;
    out->selected=1;
    if(out->selection!=-1){
        if(!b->activity_lookup||!b->activity_lookup(b->context,a,out->selection,&out->activity))return 0;
        out->looked_up=1;
    }else out->auxiliary=0;
    if(read<int>(a,0x890)!=out->activity){
        if(!death_dirty(a,b->dirty_storage,out->local_dirty,out->external_dirty))return 0;
        write(a,0x890,out->activity);out->activity_written=1;
    }
    if(read<unsigned>(a,0x894)!=out->auxiliary){
        if(!death_dirty(a,b->dirty_storage,out->local_dirty,out->external_dirty))return 0;
        write(a,0x894,out->auxiliary);out->aux_written=1;
    }
    out->completed=1;return 1;
}
int __cdecl grunt_death_prepare(void* a,unsigned n,void* damage,unsigned dn,const ServerNpcGruntDeathPrepareBindings* b,ServerNpcGruntDeathPrepareResult* out) noexcept{
    return grunt_death_prepare_impl(a,n,damage,dn,b,out,nullptr);
}
struct DeathPrepareRoute {
    const ServerNpcGruntDeathPrepareBindings* bindings;
    ServerNpcGruntDeathPrepareResult* result;
    unsigned* life_local;
    unsigned* life_external;
    const StateSignalRoute* signal=nullptr;
    const DeathSelectorRoute* selector=nullptr;
};

int grunt_death_prefix_impl(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntDeathPrefixBindings* b,ServerNpcGruntDeathPrefixResult* out,const DeathPrepareRoute* route) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xdb2||!damage||dn<0x3c||!b||!std::isfinite(now))return 0;
    const auto& sb=b->removal.removal.leave.sentence;
    const auto vt=sb.speech.original_vtable;
    if(!relation_grunt_class(vt)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const bool elite=vt==0x2248df04;
    const bool jet=vt==0x2248c514||vt==0x2248e83c||vt==0x2249964c||vt==0x224a4a4c;
    if((elite&&n<0xfe0)||(jet&&n<0x100c))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    if(registry.Resolve(handle)!=a)return 0;
    // 2205eec0(0x47,1): compare mapped schedule against +79c only if schedule exists.
    if(read<unsigned>(a,0x798)){
        int global=-1;if(!schedule_local_id(a,0x47,b->schedules,global))return 0;
        if(global==read<int>(a,0x79c)){out->schedule_blocked=out->completed=1;return 1;}
    }
    if(route&&route->signal){
        if(!bound_state_signal(a,n,0,sb,*route->signal))return 0;
    }else if(!b->state_signal||!b->state_signal(b->context,a,0))return 0;
    out->signal_called=1;
    if(route){
        if(!grunt_death_prepare_impl(a,n,damage,dn,route->bindings,route->result,route->selector))return 0;
    }else if(!b->prepare||!b->prepare(b->context,a,damage))return 0;
    out->prepared=1;
    if(read<int>(a,0x8c)!=1){
        if(route){
            if(!death_dirty(a,route->bindings->dirty_storage,*route->life_local,*route->life_external))return 0;
        }else if(!b->life_change||!b->life_change(b->context,a))return 0;
        write(a,0x8c,1);out->life_changed=1;
    }
    out->attacker=read<unsigned>(damage,0x28);
    if(!grunt_behavior_removal(a,n,handle,registry.Resolve(out->attacker),true,now,&b->removal,&out->removal))return 0;
    out->removed=1;
    // +180 dispatch: three ground tables no-op, Elite serial handle, four Jetpack manager release.
    if(elite){
        if(void* link=registry.Resolve(read<unsigned>(a,0xfdc))){
            if(!b->elite_link||!b->elite_link(b->context,link))return 0;
            out->subclass_called=1;
        }
    }else if(jet){
        out->jet_token=read<unsigned>(a,0x1008);
        if(!b->jet_release||!b->jet_release(b->context,out->jet_token))return 0;
        out->subclass_called=1;write(a,0x1008,0u);out->jet_cleared=1;
    }
    auto reaction=b->reaction;
    // Share the class/serial registry without discarding the caller's speech callbacks.
    reaction.sentence.entries=sb.entries;reaction.sentence.count=sb.count;
    reaction.sentence.speech.original_vtable=vt;
    if(!grunt_death_speech(a,n,now,&reaction,&out->speech))return 0;
    out->speech_called=1;out->tail_required=out->completed=1;return 1;
}
int __cdecl grunt_death_prefix(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntDeathPrefixBindings* b,ServerNpcGruntDeathPrefixResult* out) noexcept{
    return grunt_death_prefix_impl(a,n,handle,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntDeathPrefixApi death_prefix_api{1,sizeof(ServerNpcGruntDeathPrefixApi),&grunt_death_prefix};

int grunt_death_event_impl(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntDeathEventBindings* b,ServerNpcGruntDeathEventResult* out,const DeathPrepareRoute* route) noexcept{
    if(!out)return 0;
    out->prefix={};out->condition={};out->condition.global=out->condition.bit=-1;
    out->activity=-1;out->previous_state=-1;
    out->completed=out->field_cleared=out->base_called=out->activity_called=out->ideal_written=out->state_written=out->time_written=out->remove_called=0;
    if(!b||!grunt_death_prefix_impl(a,n,handle,damage,dn,now,&b->prefix,&out->prefix,route))return 0;
    if(!out->prefix.tail_required){out->completed=1;return 1;}
    // All eight +398 slots return false (22044730), no opaque override.
    if(read<std::uint8_t>(a,0xbd)&0x20){write(a,0x7c,0u);out->field_cleared=1;}
    if(!b->base_death||!b->base_death(b->context,a,damage))return 0;
    out->base_called=1;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    if(!cond||!cond->store(a,n,0x10,NpcConditionCurrent,true,&b->prefix.schedules,&out->condition))return 0;
    if(read<int>(a,0x858)!=7){write(a,0x858,7);out->ideal_written=1;}
    // +2a8 performs activity lookup even if the subsequent flag blocks its result.
    if(!b->activity_lookup||!b->activity_lookup(b->context,a,0x18,&out->activity))return 0;
    out->activity_called=1;
    const bool first=out->activity!=-1&&!(read<unsigned>(a,0xbc)&0x20000000u);
    if(first||read<int>(a,0x1c)==0x17){
        out->previous_state=read<int>(a,0x850);
        if(out->previous_state!=7){write(a,0x854,now);out->time_written=1;}
        write(a,0x850,7);out->state_written=1;
        if(read<int>(a,0x858)!=7)write(a,0x858,7);
        // +4d0=22027d90 ret8 for all eight vtables.
    }
    if((read<std::uint8_t>(damage,0x3a)&0x80)&&!(read<unsigned>(a,0xb8)&0x100000u)){
        if(!b->final_remove||!b->final_remove(b->context,a))return 0;
        out->remove_called=1;
    }
    out->prefix.tail_required=0;out->completed=1;return 1;
}
int __cdecl grunt_death_event(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntDeathEventBindings* b,ServerNpcGruntDeathEventResult* out) noexcept{
    return grunt_death_event_impl(a,n,handle,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntDeathEventApi death_event_api{1,sizeof(ServerNpcGruntDeathEventApi),&grunt_death_event};
int __cdecl grunt_death_prepared_event(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntDeathPreparedEventBindings* b,ServerNpcGruntDeathPreparedEventResult* out) noexcept{
    if(!out)return 0;
    out->prepare={};out->prepare.selection=out->prepare.activity=-1;
    out->life_local_dirty=out->life_external_dirty=0;
    if(!b)return 0;
    const DeathPrepareRoute route{&b->prepare,&out->prepare,&out->life_local_dirty,&out->life_external_dirty};
    return grunt_death_event_impl(a,n,handle,damage,dn,now,&b->event,&out->event,&route);
}
const ServerNpcGruntDeathPreparedEventApi death_prepared_event_api{1,sizeof(ServerNpcGruntDeathPreparedEventApi),&grunt_death_prepare,&grunt_death_prepared_event};
int grunt_combat_damage_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntCombatDamageBindings* b,ServerNpcGruntCombatDamageResult* out,const LifeDamageRoute* life) noexcept{
    if(!out)return 0;
    out->damage={};out->signal={};
    if(!b)return 0;
    const StateSignalRoute route{&b->signal,&out->signal,life};
    return grunt_damage_pain_prefix_impl(a,n,damage,dn,now,&b->damage,&out->damage,&route);
}
int __cdecl grunt_combat_damage(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntCombatDamageBindings* b,ServerNpcGruntCombatDamageResult* out) noexcept{
    return grunt_combat_damage_impl(a,n,damage,dn,now,b,out,nullptr);
}
int __cdecl grunt_combat_death(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntCombatDeathBindings* b,ServerNpcGruntCombatDeathResult* out) noexcept{
    if(!out)return 0;
    auto& d=out->death;
    d.prepare={};d.prepare.selection=d.prepare.activity=-1;
    d.life_local_dirty=d.life_external_dirty=0;out->signal={};
    if(!b)return 0;
    const StateSignalRoute signal{&b->signal,&out->signal};
    const DeathPrepareRoute route{&b->death.prepare,&d.prepare,&d.life_local_dirty,&d.life_external_dirty,&signal};
    return grunt_death_event_impl(a,n,handle,damage,dn,now,&b->death.event,&d.event,&route);
}
const ServerNpcGruntCombatReactionsApi combat_reactions_api{1,sizeof(ServerNpcGruntCombatReactionsApi),&grunt_combat_damage,&grunt_combat_death};
int __cdecl grunt_death_selected(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntDeathSelectedBindings* b,ServerNpcGruntDeathSelectedResult* out) noexcept{
    if(!out)return 0;
    out->selector={};out->selector.selection=-1;
    auto& d=out->combat.death;
    d.prepare={};d.prepare.selection=d.prepare.activity=-1;
    d.life_local_dirty=d.life_external_dirty=0;out->combat.signal={};
    if(!b)return 0;
    const StateSignalRoute signal{&b->combat.signal,&out->combat.signal};
    const DeathSelectorRoute selector{&b->selector,&out->selector};
    const DeathPrepareRoute route{&b->combat.death.prepare,&d.prepare,&d.life_local_dirty,&d.life_external_dirty,&signal,&selector};
    return grunt_death_event_impl(a,n,handle,damage,dn,now,&b->combat.death.event,&d.event,&route);
}
const ServerNpcGruntDeathSelectorApi death_selector_api{1,sizeof(ServerNpcGruntDeathSelectorApi),&grunt_death_select,&grunt_death_selected};

bool damage_group_eligible(void* npc,unsigned n,const ServerNpcGruntDamageOutputBindings& b,bool& yes) noexcept{
    yes=false;
    if(!npc||n<0x9c8)return false;
    const auto token=read<unsigned>(npc,0x9c4);
    if(!token)return true;
    void* group=nullptr;if(!speech_squad(b.entities.speech,token,group))return false;
    auto name=read<unsigned>(group,4);
    if(name){
        void* ptr=nullptr;unsigned bytes=0;
        if(!memory_storage(&b.names,name,1,ptr,bytes))return false;
        if(!read<std::uint8_t>(ptr,0))name=0;
    }
    if(name!=b.player_group_name)return true;
    return sentence_eligible(npc,b.entities,yes);
}
int __cdecl grunt_damage_outputs(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageOutputBindings* b,ServerNpcGruntDamageOutputResult* out) noexcept{
    if(!out)return 0;*out={};out->condition.global=out->condition.bit=-1;
    if(!a||n<0xdb2||!damage||dn<0x3c||!b||!std::isfinite(now))return 0;
    const auto& sb=b->entities;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    const auto stamp=read<float>(a,0x994);
    if(!std::isfinite(stamp))return 0;
    out->stamp_equal=stamp==now;
    auto emit=[&](unsigned offset,unsigned& flag){if(!b->output||!b->output(b->context,a,offset))return false;flag=1;return true;};
    if(!out->stamp_equal){
        if(!emit(0xa48,out->generic_output))return 0;
        void* attacker=registry.Resolve(read<unsigned>(damage,0x28));
        bool player=false;
        if(!attacker||!b->is_player||!b->is_player(b->context,attacker,&player))return 0;
        bool qualify=player;
        if(player){
            if(!emit(0xb68,out->player_output))return 0;
        }else{
            // Original RTTI is called after reloading damage attacker. Local-player lookup
            // occurs before testing cast result, even for a null cast.
            void* npc=nullptr;unsigned extent=0;
            if(!b->cast_npc||!b->cast_npc(b->context,registry.Resolve(read<unsigned>(damage,0x28)),&npc,&extent))return 0;
            out->cast_called=1;
            void* local=nullptr;
            if(b->max_clients<=1){
                if(!b->local_player||!b->local_player(b->context,&local))return 0;
                out->local_player_called=1;
            }
            if(npc){
                if(extent<0x9c8)return 0;
                if(!read<int>(npc,0x8c)&&local&&read<unsigned>(npc,0x9c4)){
                    if(!damage_group_eligible(npc,extent,*b,qualify))return 0;
                    out->eligible=qualify;
                }
            }
        }
        if(qualify&&!emit(0xb80,out->qualified_output))return 0;
    }
    const auto flags=read<unsigned>(damage,0x38);
    if((flags&1u)&&!(flags&0x1000000u)){
        const auto amount=read<float>(damage,0x2c);
        if(!std::isfinite(amount))return 0;
        if(amount>=5.f){
            const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
            if(!cond||!cond->store(a,n,0x12,NpcConditionCurrent,true,&b->conditions,&out->condition))return 0;
        }
    }
    if(read<int>(a,0x9c)<=read<int>(a,0x98)/2&&!emit(0xa78,out->half_output))return 0;
    out->tail_required=out->completed=1;return 1;
}
int grunt_damage_output_chain_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageOutputChainBindings* b,ServerNpcGruntDamageOutputChainResult* out,const LifeDamageRoute* life) noexcept{
    if(!out)return 0;out->outputs={};
    if(!b||!grunt_combat_damage_impl(a,n,damage,dn,now,&b->prefix,&out->prefix,life))return 0;
    if(!out->prefix.damage.tail_required)return 1;
    auto tail=b->outputs;
    const auto& source=b->prefix.damage.reaction.sentence;
    tail.entities.entries=source.entries;tail.entities.count=source.count;
    tail.entities.speech.original_vtable=source.speech.original_vtable;
    return grunt_damage_outputs(a,n,damage,dn,now,&tail,&out->outputs);
}
int __cdecl grunt_damage_output_chain(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageOutputChainBindings* b,ServerNpcGruntDamageOutputChainResult* out) noexcept{
    return grunt_damage_output_chain_impl(a,n,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntDamageOutputsApi damage_outputs_api{1,sizeof(ServerNpcGruntDamageOutputsApi),&grunt_damage_outputs,&grunt_damage_output_chain};

bool damage_notify_refresh(void* a,const ServerNpcGruntDamageNotificationBindings& b,unsigned& refreshed) noexcept{
    if(!(read<unsigned>(a,0xb8)&0x800u))return true;
    if(!b.refresh_origin||!b.refresh_origin(b.context,a))return false;
    ++refreshed;return true;
}
int __cdecl grunt_damage_receive(void* a,unsigned n,void* source,unsigned sn,const ServerNpcGruntDamageNotificationBindings* b,ServerNpcGruntDamageReceiveResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0x86c||!source||sn<0x2c8||!b||!relation_grunt_class(b->original_vtable))return 0;
    if(read<int>(a,0x868)==3){out->state_blocked=out->completed=1;return 1;}
    if(!damage_notify_refresh(source,*b,out->source_refreshed)||!damage_notify_refresh(a,*b,out->receiver_refreshed))return 0;
    const bool active=read<int>(a,0x868)!=0;
    float d[3];
    for(unsigned i=0;i<3;++i){d[i]=static_cast<float>(static_cast<double>(read<float>(a,0x2bc+4*i))-read<float>(source,0x2bc+4*i));if(!std::isfinite(d[i]))return 0;}
    out->distance_squared=static_cast<float>(static_cast<double>(d[2])*d[2]+static_cast<double>(d[1])*d[1]+static_cast<double>(d[0])*d[0]);
    if(!std::isfinite(out->distance_squared))return 0;
    if(active&&out->distance_squared<57600.f){
        if(!b->state_signal||!b->state_signal(b->context,a,1))return 0;
        out->signal_called=1;
    }
    if(out->distance_squared<360000.f){write<std::uint8_t>(a,0x84c,1);write(a,0x85c,0u);out->marked=1;}
    out->completed=1;return 1;
}
int __cdecl grunt_damage_notify(void* a,unsigned n,void* attacker,const ServerNpcGruntDamageNotificationBindings* b,ServerNpcGruntDamageNotificationResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0x9c8||!b||!relation_grunt_class(b->original_vtable)||!attacker||!b->as_npc)return 0;
    void* npc=nullptr;if(!b->as_npc(b->context,attacker,&npc))return 0;out->cast_called=1;
    if(!npc){out->completed=1;return 1;}
    if(!damage_notify_refresh(a,*b,out->source_refreshed)||!b->count||!b->member)return 0;
    for(int i=0;;++i){
        int count=0;if(!b->count(b->context,&count)||count>4096||i>4096)return 0;
        if(i>=count)break;
        void* m=nullptr;unsigned extent=0;std::uint32_t vt=0;
        if(!b->member(b->context,i,&m,&extent,&vt))return 0;++out->visited;
        if(!m){++out->null_member;continue;}
        if(m==a){++out->self;continue;}
        if(extent<0x9c8)return 0;
        unsigned refreshed=0;if(!damage_notify_refresh(m,*b,refreshed))return 0;
        double d[3];for(unsigned j=0;j<3;++j){d[j]=static_cast<double>(read<float>(m,0x2bc+4*j))-read<float>(a,0x2bc+4*j);if(!std::isfinite(d[j]))return 0;}
        if(std::abs(d[2])>=120.0){++out->vertical_blocked;continue;}
        if(d[0]*d[0]+d[1]*d[1]>=360000.0){++out->horizontal_blocked;continue;}
        if(read<unsigned>(m,0x9c4)!=read<unsigned>(a,0x9c4)){
            int disposition=0;if(!b->disposition||!b->disposition(b->context,a,m,&disposition))return 0;
            ++out->disposition_checked;if(disposition!=3){++out->disposition_blocked;continue;}
        }
        if(relation_grunt_class(vt)){
            auto bound=*b;bound.original_vtable=vt;
            if(!grunt_damage_receive(m,extent,a,n,&bound,&out->last_receive))return 0;
            ++out->native_received;
        }else{
            if(!b->other_receive||!b->other_receive(b->context,m,a,npc))return 0;
            ++out->hosted_received;
        }
    }
    out->completed=1;return 1;
}
struct DamageReportRoute {
    const ServerNpcGruntMemoryReportBindings* bindings;
    const ServerNpcGruntSquadReportBindings* squad;
    ServerNpcGruntMemoryReportResult* result;
    ServerNpcGruntSquadReportResult* stats;
    unsigned* called;
};
struct DamageNotifyRoute {
    const ServerNpcGruntDamageNotificationBindings* bindings;
    ServerNpcGruntDamageNotificationResult* result;
    const DamageReportRoute* report=nullptr;
    const LifeDamageRoute* life=nullptr;
};

int grunt_damage_tail_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageTailBindings* b,ServerNpcGruntDamageTailResult* out,const DamageNotifyRoute* route) noexcept{
    if(!out)return 0;*out={};out->lookup.index=out->touch.index=255;
    if(!a||n<0xdb2||!damage||dn<0x3c||!b||!std::isfinite(now))return 0;
    const auto& sb=b->entities;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    auto attacker=[&](){return registry.Resolve(read<unsigned>(damage,0x28));};
    auto current=[&](){return registry.Resolve(read<unsigned>(a,0x8a0));};
    auto origin=[&](void* e,float* p){
        unsigned extent=n;
        if(!e||(e!=a&&(!sb.extent||!sb.extent(sb.speech.context,e,&extent)))||extent<0x2c8)return false;
        if((read<unsigned>(e,0xb8)&0x800u)&&(!b->refresh_origin||!b->refresh_origin(b->context,e)))return false;
        for(unsigned i=0;i<3;++i)p[i]=read<float>(e,0x2bc+4*i);
        return std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]);
    };
    auto lookup=[&](bool touch){
        void* memory=nullptr;unsigned extent=0;std::uint32_t key=0;
        if(!memory_key(b->memory,attacker(),key)||!memory_storage(&b->memory.storage,read<unsigned>(a,0x984),0x1c,memory,extent))return false;
        if(touch){
            if(!memory_touch(memory,extent,key,now,&b->memory.storage,&out->touch))return false;
            out->memory_touched=out->touch.record!=0;return true;
        }
        return memory_lookup(memory,extent,key,false,&b->memory.storage,&out->lookup)!=0;
    };
    out->enabled=(read<std::uint8_t>(a,0xbd)&0x20)!=0;
    if(!out->enabled||!attacker()){out->native_result=out->completed=1;return 1;}
    out->attacker_valid=1;
    unsigned an=0;
    if(!sb.extent||!sb.extent(sb.speech.context,attacker(),&an)||an<0xc0)return 0;
    out->attacker_flags=(read<unsigned>(attacker(),0xbc)&0x2080u)!=0;
    if(out->attacker_flags){
        const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
        bool yes=false;
        if(!b->accept||!b->accept(b->context,a,attacker(),&yes))return 0;
        if(yes){
            if(!b->visible||!b->visible(b->context,a,attacker(),0x4081,&yes))return 0;
            out->visible=yes;
        }
        if(!out->visible){
            if(void* inflictor=registry.Resolve(read<unsigned>(damage,0x24))){
                if(!origin(inflictor,out->position))return 0;
            }else{
                if(!origin(a,out->position))return 0;
                for(unsigned i=0;i<3;++i){
                    const float direction=route&&route->life?route->life->bindings->shared_direction[i]:b->fallback_vector[i];
                    const float scaled=static_cast<float>(static_cast<double>(direction)*64.0);
                    out->position[i]=static_cast<float>(static_cast<double>(out->position[i])+scaled);
                    if(!std::isfinite(out->position[i]))return 0;
                }
            }
            bool existing=false;
            if(current()){
                if(!lookup(false))return 0;
                if(!out->lookup.record){
                    ServerNpcConditionResult c{};
                    if(!cond->test(a,n,0xa,NpcConditionCurrent,&b->conditions,&c))return 0;
                    existing=!c.value;
                }
            }
            void* target=nullptr;void* source=nullptr;
            if(existing){source=current();target=current();out->report_existing=1;}
            else{
                if(!lookup(false))return 0;
                target=out->lookup.record?attacker():nullptr;
            }
            if(route&&route->report){
                const auto& r=*route->report;
                auto bound=*r.bindings;bound.original_vtable=sb.speech.original_vtable;
                bound.mutation.entries=sb.entries;bound.mutation.count=sb.count;
                ++*r.called;
                // +678 returns a creation flag, not an adapter-success flag; retail ignores it here.
                if(!squad_report(a,n,target,out->position,source,now,&bound,r.squad,r.result,r.stats))return 0;
            }else if(!b->report||!b->report(b->context,a,target,out->position,source))return 0;
            out->reported=1;
        }
        out->amount=read<float>(damage,0x2c);if(!std::isfinite(out->amount))return 0;
        if(out->amount>0.f&&!cond->store(a,n,0x10,NpcConditionCurrent,true,&b->conditions,&out->light))return 0;
        if(((read<std::uint8_t>(damage,0x3b)&0x40)&&out->amount>=4.f)||out->amount>20.f){
            if(!cond->store(a,n,0x11,NpcConditionCurrent,true,&b->conditions,&out->heavy))return 0;
        }
        write<std::uint8_t>(a,0x84c,1);write(a,0x85c,0u);
        const float last=read<float>(a,0x994);if(!std::isfinite(last))return 0;
        out->accumulated_prior=static_cast<double>(now)-last<1.0;
        if(out->accumulated_prior){
            const float prior=read<float>(a,0x990);if(!std::isfinite(prior))return 0;
            out->total=static_cast<float>(static_cast<double>(prior)+out->amount);
        }else out->total=out->amount;
        if(!std::isfinite(out->total))return 0;
        write(a,0x990,out->total);write(a,0x994,now);
        if(void* target=attacker()){
            bool player=false;
            if(!b->is_player||!b->is_player(b->context,target,&player))return 0;
            if(player){write(a,0x998,now);out->player_stamp=1;}
        }
        if(!lookup(true))return 0;
        if(static_cast<double>(read<int>(a,0x98))*0.3<static_cast<double>(read<float>(a,0x990))&&
            !cond->store(a,n,0x13,NpcConditionCurrent,true,&b->conditions,&out->accumulated))return 0;
        if(route){
            auto bound=*route->bindings;bound.original_vtable=sb.speech.original_vtable;
            if(!grunt_damage_notify(a,n,attacker(),&bound,route->result))return 0;
        }else if(!b->notify_attacker||!b->notify_attacker(b->context,a,attacker()))return 0;
        out->notified=1;
    }
    float p[3]{};
    if(!origin(a,p)||!b->final_notice||!b->final_notice(b->context,a,p,1,0x400,0.5f,4))return 0;
    out->final_notice=1;out->native_result=out->completed=1;return 1;
}
int __cdecl grunt_damage_tail(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageTailBindings* b,ServerNpcGruntDamageTailResult* out) noexcept{
    return grunt_damage_tail_impl(a,n,damage,dn,now,b,out,nullptr);
}
int grunt_damage_event_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageEventBindings* b,ServerNpcGruntDamageEventResult* out,const DamageNotifyRoute* route) noexcept{
    if(!out)return 0;
    out->completed=0;out->native_result=0;out->tail={};
    if(!b||!grunt_damage_output_chain_impl(a,n,damage,dn,now,&b->chain,&out->chain,route?route->life:nullptr))return 0;
    if(!out->chain.outputs.tail_required){out->completed=1;return 1;}
    auto tail=b->tail;
    const auto& source=b->chain.prefix.damage.reaction.sentence;
    tail.entities.entries=source.entries;tail.entities.count=source.count;
    tail.entities.speech.original_vtable=source.speech.original_vtable;
    if(!grunt_damage_tail_impl(a,n,damage,dn,now,&tail,&out->tail,route))return 0;
    out->chain.outputs.tail_required=out->chain.prefix.damage.tail_required=0;
    out->native_result=out->tail.native_result;out->completed=1;return 1;
}
int __cdecl grunt_damage_event(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageEventBindings* b,ServerNpcGruntDamageEventResult* out) noexcept{
    return grunt_damage_event_impl(a,n,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntDamageEventApi damage_event_api{1,sizeof(ServerNpcGruntDamageEventApi),&grunt_damage_tail,&grunt_damage_event};
int __cdecl grunt_damage_notified_event(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageNotifiedEventBindings* b,ServerNpcGruntDamageNotifiedEventResult* out) noexcept{
    if(!out)return 0;out->notify={};
    if(!b)return 0;
    const DamageNotifyRoute route{&b->notify,&out->notify};
    return grunt_damage_event_impl(a,n,damage,dn,now,&b->event,&out->event,&route);
}
const ServerNpcGruntDamageNotificationApi damage_notification_api{1,sizeof(ServerNpcGruntDamageNotificationApi),&grunt_damage_receive,&grunt_damage_notify,&grunt_damage_notified_event};


int __cdecl grunt_bone_hit(void* a,const char* name,const float* pos,float range,const ServerNpcGruntBoneHitBindings* b,ServerNpcGruntBoneHitResult* out) noexcept{
    if(!out)return 0;*out={};out->index=-1;
    if(!a||!name||!pos||!std::isfinite(range)||!b||!b->index)return 0;
    if(!b->index(b->context,a,name,&out->index))return 0;
    if(out->index==-1){out->completed=1;return 1;}
    if(out->index<0||!b->matrix)return 0;
    float matrix[12]{};
    if(!b->matrix(b->context,a,out->index,matrix))return 0;
    out->found=1;double d[3];
    for(unsigned i=0;i<3;++i){
        out->origin[i]=matrix[4*i+3];d[i]=static_cast<double>(pos[i])-out->origin[i];
        if(!std::isfinite(d[i]))return 0;
    }
    out->squared=static_cast<float>(d[2]*d[2]+d[0]*d[0]+d[1]*d[1]);
    if(!std::isfinite(out->squared))return 0;
    out->distance=std::sqrt(static_cast<double>(out->squared));
    out->hit=out->distance<range;out->completed=1;return 1;
}

int __cdecl special_vector(void* a,unsigned n,const float* v,const ServerNpcEnemyMemoryBindings* storage,unsigned* changed,unsigned* local,unsigned* external) noexcept{
    if(!a||n<0xc54||!v||!storage||!changed||!local||!external||!finite_vec(v))return 0;
    *changed=*local=*external=0;
    bool differs=false;
    for(unsigned i=0;i<3;++i){
        const float old=read<float>(a,0xc48+4*i);
        if(!std::isfinite(old))return 0;
        differs=differs||old!=v[i];
    }
    if(!differs)return 1;
    if(!death_dirty(a,*storage,*local,*external))return 0;
    std::memmove(static_cast<unsigned char*>(a)+0xc48,v,12);*changed=1;return 1;
}
int __cdecl grunt_special_impact(void* a,unsigned n,void* damage,unsigned dn,const ServerNpcGruntSpecialImpactBindings* b,ServerNpcGruntSpecialImpactResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xdb2||!damage||dn<0x3c||!b)return 0;
    const auto& sb=b->entities;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    if(sb.speech.original_vtable==0x2248df04){out->elite_noop=out->completed=1;return 1;}
    if(read<std::uint8_t>(a,0xc44)!=1){
        if(!death_dirty(a,b->dirty_storage,out->local_dirty,out->external_dirty))return 0;
        write<std::uint8_t>(a,0xc44,1);out->latch_written=1;
    }
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    void* attacker=registry.Resolve(read<unsigned>(damage,0x28));
    unsigned extent=0;
    if(!attacker||!sb.extent||!sb.extent(sb.speech.context,attacker,&extent)||extent<0x2c8)return 0;
    if(read<unsigned>(attacker,0xb8)&0x800u){
        if(!b->refresh_origin||!b->refresh_origin(b->context,attacker))return 0;
        out->attacker_refreshed=1;
    }
    for(unsigned i=0;i<3;++i)out->initial[i]=static_cast<float>(static_cast<double>(read<float>(damage,0xc+4*i))-read<float>(attacker,0x2bc+4*i));
    auto set=[&](const float* v){
        unsigned changed=0,l=0,e=0;
        if(!special_vector(a,n,v,&b->dirty_storage,&changed,&l,&e))return false;
        out->vector_writes+=changed;out->local_dirty+=l;out->external_dirty+=e;return true;
    };
    if(!set(out->initial)||!b->normalize)return 0;
    // Original normalize mutates the stored vector directly, without a second dirty call.
    std::memcpy(out->normalized,static_cast<unsigned char*>(a)+0xc48,12);
    if(!b->normalize(b->context,out->normalized)||!finite_vec(out->normalized))return 0;
    std::memcpy(static_cast<unsigned char*>(a)+0xc48,out->normalized,12);out->normalized_called=1;
    const float amount=read<float>(damage,0x2c);if(!std::isfinite(amount))return 0;
    out->scale_bound=amount>250.f?amount:250.f;
    if(!b->scale||!b->scale(b->context,150.f,out->scale_bound,&out->scale)||!std::isfinite(out->scale))return 0;
    for(unsigned i=0;i<3;++i)out->scaled[i]=static_cast<float>(static_cast<double>(read<float>(a,0xc48+4*i))*out->scale);
    if(!set(out->scaled))return 0;
    // Shared +1c4=22358a60, select height before optional origin refresh.
    const float height=read<std::uint8_t>(a,0xc64)?66.f:40.f;
    if(read<unsigned>(a,0xb8)&0x800u){
        if(!b->refresh_origin||!b->refresh_origin(b->context,a))return 0;
        out->actor_refreshed=1;
    }
    for(unsigned i=0;i<3;++i)out->origin[i]=read<float>(a,0x2bc+4*i);
    out->origin[2]=static_cast<float>(static_cast<double>(out->origin[2])+height);
    if(read<unsigned>(a,0x798)){
        int global=-1;if(!schedule_local_id(a,0x6b,b->schedules,global))return 0;
        if(global==read<int>(a,0x79c)){out->origin[2]=static_cast<float>(static_cast<double>(out->origin[2])-35.0);out->schedule_adjusted=1;}
    }
    if(!finite_vec(out->origin))return 0;
    out->tail_required=out->completed=1;return 1;
}
struct SpecialImpactRoute {const ServerNpcGruntSpecialImpactBindings* bindings;ServerNpcGruntSpecialImpactResult* result;};

int grunt_impact_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntImpactBindings* b,ServerNpcGruntImpactResult* out,const SpecialImpactRoute* special) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xdb2||!damage||dn<0x3c||!b||!std::isfinite(now))return 0;
    const auto& sb=b->reaction.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    auto emit=[&](int id){
        const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
        if(!b->event||!b->event(b->context,id,registry.Resolve(read<unsigned>(damage,0x28)),a))return false;
        out->event_id=static_cast<unsigned>(id);return true;
    };
    out->head_present=read<std::uint8_t>(a,0xc74)!=0;
    if(out->head_present){
        bool allowed=false;if(!b->rule||!b->rule(b->context,&allowed))return 0;
        out->rule_called=1;
        if(allowed||(read<unsigned>(damage,0x38)&0x80u)){
            if(!b->detach||!b->detach(b->context,a))return 0;
            out->detached=1;if(!emit(7))return 0;
        }
    }else{
        const float amount=read<float>(damage,0x2c);if(!std::isfinite(amount))return 0;
        out->threshold=static_cast<float>(static_cast<double>(amount)*static_cast<double>(0.005f));
        if(!std::isfinite(out->threshold)||!b->random_float||!b->random_float(b->context,0.f,1.f,&out->sample)||
           !std::isfinite(out->sample)||out->sample<0.f||out->sample>1.f)return 0;
        out->random_called=1;
        const float live=read<float>(damage,0x2c);if(!std::isfinite(live))return 0;
        if(out->sample<out->threshold&&static_cast<double>(read<int>(a,0x9c))<live){
            if(special){
                auto bound=*special->bindings;
                bound.entities.entries=sb.entries;bound.entities.count=sb.count;bound.entities.speech.original_vtable=sb.speech.original_vtable;
                if(!grunt_special_impact(a,n,damage,dn,&bound,special->result))return 0;
                if(special->result->tail_required){
                    if(!bound.effect_tail||!bound.effect_tail(bound.context,a,special->result->origin))return 0;
                    special->result->tail_called=1;
                }
            }else if(!b->lethal||!b->lethal(b->context,a,damage))return 0;
            out->lethal_called=1;if(!emit(8))return 0;
            out->completed=1;return 1;
        }
    }
    out->deadline=static_cast<float>(static_cast<double>(now)+1.0);if(!std::isfinite(out->deadline))return 0;
    write(a,0xc78,out->deadline);
    if(!sentence_dispatch_request(a,n,now,"GRUNT_HEAD",2,0,&sb,&b->reaction.request,&out->speech,&out->request))return 0;
    out->speech_called=1;out->completed=1;return 1;
}
int __cdecl grunt_impact(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntImpactBindings* b,ServerNpcGruntImpactResult* out) noexcept{
    return grunt_impact_impl(a,n,damage,dn,now,b,out,nullptr);
}
struct DamageDetailRoute {const ServerNpcGruntDamageDetailBindings* bindings;ServerNpcGruntDamageDetailResult* result;const DamageNotifyRoute* notify=nullptr;const SpecialImpactRoute* special=nullptr;};

int grunt_damage_override_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageOverrideBindings* b,ServerNpcGruntDamageOverrideResult* out,const DamageDetailRoute* route) noexcept{
    if(!out)return 0;
    // Large composed result: avoid deep aggregate initialization on MSVC x86.
    out->base.completed=0;out->base.native_result=0;
    out->berserk={};out->cough={};out->knee={};out->berserk_request={};out->cough_request={};out->knee_request={};
    out->bit11={};out->bit47={};out->knee_total=out->random_sample=out->copy_amount=0;out->component=0;
    out->completed=out->cast_called=out->berserk_called=out->impact_called=out->rejected_flags=out->cough_blocked=out->cough_called=0;
    out->copy_used=out->knee_checked=out->knee_added=out->knee_reset=out->knee_called=out->random_called=out->component_called=out->base_called=0;
    out->native_result=0;
    if(!a||n<0xdb2||!damage||dn<0x48||!b||!std::isfinite(now))return 0;
    const auto& reaction=b->base.chain.prefix.damage.reaction;
    const auto& sb=reaction.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    auto speak=[&](const char* cue,ServerNpcGruntSentenceResult& sr,ServerNpcGruntSentenceRequestResult& rr,unsigned& called){
        called=1;return sentence_dispatch_request(a,n,now,cue,2,0,&sb,&reaction.request,&sr,&rr)!=0;
    };
    auto done=[&](){out->completed=1;return 1;};
    void* cast=nullptr;unsigned extent=0;
    if(!b->cast_grunt||!b->cast_grunt(b->context,registry.Resolve(read<unsigned>(damage,0x28)),&cast,&extent))return 0;
    out->cast_called=1;
    if(cast){
        if(extent<0xdb2)return 0;
        if(read<std::uint8_t>(cast,0xdb1)&&!speak("GRUNT_BERSERK_SQUADMATE",out->berserk,out->berserk_request,out->berserk_called))return 0;
    }
    auto flags=read<unsigned>(damage,0x38);
    if((flags&0x400000u)&&!(flags&2u)){out->rejected_flags=1;return done();}
    if(read<int>(damage,0x44)==1){
        if(route){
            auto bound=route->bindings->impact;bound.reaction=reaction;
            if(!grunt_impact_impl(a,n,damage,dn,now,&bound,&route->result->impact,route->special))return 0;
        }else if(!b->impact||!b->impact(b->context,a,damage))return 0;
        out->impact_called=1;
    }
    flags=read<unsigned>(damage,0x38);
    if((flags&0x3200008u)&&(read<std::uint8_t>(a,0xbf)&8u)){out->rejected_flags=1;return done();}
    if(flags&0x10000u){
        const float deadline=read<float>(a,0xc94);
        if(!std::isfinite(deadline))return 0;
        if(now<=deadline){out->cough_blocked=1;return done();}
        const float next=static_cast<float>(static_cast<double>(now)+5.0);if(!std::isfinite(next))return 0;
        write(a,0xc94,next);
        if(!speak("GRUNT_COUGH",out->cough,out->cough_request,out->cough_called))return 0;
        return done();
    }
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    const auto& conditions=b->base.chain.outputs.conditions;
    if(flags&0x80u){
        if(!cond->store(a,n,0x11,NpcConditionCurrent,true,&conditions,&out->bit11))return 0;
        write(a,0x988,read<unsigned>(a,0x988)&~0x40u);
    }
    alignas(4) unsigned char copied[0x48];
    void* forwarded=damage;
    if(read<std::uint8_t>(a,0xb6)&8u){
        std::memcpy(copied,damage,sizeof(copied));
        out->copy_amount=static_cast<float>(read<int>(a,0x9c));
        write(copied,0x2c,out->copy_amount);forwarded=copied;out->copy_used=1;
    }else{
        float pos[3];for(unsigned i=0;i<3;++i){pos[i]=read<float>(damage,0xc+4*i);if(!std::isfinite(pos[i]))return 0;}
        bool hit=false;
        if(route){
            auto& br=route->result->right;
            if(!grunt_bone_hit(a,"ValveBiped.Bip01_R_Calf",pos,10.f,&route->bindings->bone,&br))return 0;
            hit=br.hit!=0;
        }else if(!b->bone_hit||!b->bone_hit(b->context,a,"ValveBiped.Bip01_R_Calf",pos,10.f,&hit))return 0;
        ++out->knee_checked;
        if(!hit){
            // Original reloads damage position for the second call.
            for(unsigned i=0;i<3;++i){pos[i]=read<float>(damage,0xc+4*i);if(!std::isfinite(pos[i]))return 0;}
            if(route){
            auto& br=route->result->left;
            if(!grunt_bone_hit(a,"ValveBiped.Bip01_L_Calf",pos,10.f,&route->bindings->bone,&br))return 0;
            hit=br.hit!=0;
        }else if(!b->bone_hit(b->context,a,"ValveBiped.Bip01_L_Calf",pos,10.f,&hit))return 0;
            ++out->knee_checked;
        }
        out->knee_total=read<float>(a,0xcac);if(!std::isfinite(out->knee_total))return 0;
        if(hit){
            const float amount=read<float>(damage,0x2c);if(!std::isfinite(amount))return 0;
            out->knee_total=static_cast<float>(static_cast<double>(out->knee_total)+amount);
            if(!std::isfinite(out->knee_total))return 0;
            write(a,0xcac,out->knee_total);out->knee_added=1;
        }
        if(out->knee_total>12.f){
            bool admit=false;
            if(sb.speech.original_vtable!=0x2248df04){
                if(!b->random_float||!b->random_float(b->context,0.f,1.f,&out->random_sample)||
                    !std::isfinite(out->random_sample)||out->random_sample<0.f||out->random_sample>1.f)return 0;
                out->random_called=1;admit=out->random_sample>0.5f;
            }
            if(admit){
                if(!cond->store(a,n,0x47,NpcConditionCurrent,true,&conditions,&out->bit47))return 0;
                const float next=static_cast<float>(static_cast<double>(now)+1.0);if(!std::isfinite(next))return 0;
                write(a,0xc78,next);
                if(!speak("GRUNT_KNEE",out->knee,out->knee_request,out->knee_called))return 0;
            }
            write(a,0xcac,0.f);out->knee_reset=1;
        }
    }
    out->component=read<unsigned>(a,0xc18);
    if(out->component){
        if(!b->component_damage||!b->component_damage(b->context,out->component,forwarded,&out->native_result))return 0;
        out->component_called=1;return done();
    }
    if(!grunt_damage_event_impl(a,n,forwarded,0x48,now,&b->base,&out->base,route?route->notify:nullptr))return 0;
    out->native_result=out->base.native_result;out->base_called=1;return done();
}
int __cdecl grunt_damage_override(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageOverrideBindings* b,ServerNpcGruntDamageOverrideResult* out) noexcept{
    return grunt_damage_override_impl(a,n,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntDamageOverrideApi damage_override_api{1,sizeof(ServerNpcGruntDamageOverrideApi),&grunt_damage_override};
int __cdecl grunt_damage_detail(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageDetailBindings* b,ServerNpcGruntDamageDetailResult* out) noexcept{
    if(!out)return 0;out->impact={};out->right={};out->left={};
    if(!b)return 0;
    const DamageDetailRoute route{b,out};
    return grunt_damage_override_impl(a,n,damage,dn,now,&b->damage,&out->damage,&route);
}
const ServerNpcGruntDamageDetailApi damage_detail_api{1,sizeof(ServerNpcGruntDamageDetailApi),&grunt_bone_hit,&grunt_impact,&grunt_damage_detail};
int __cdecl grunt_special_damage(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntSpecialDamageBindings* b,ServerNpcGruntSpecialDamageResult* out) noexcept{
    if(!out)return 0;out->special={};out->detail.impact={};out->detail.right={};out->detail.left={};
    if(!b)return 0;
    const SpecialImpactRoute special{&b->special,&out->special};
    const DamageDetailRoute route{&b->detail,&out->detail,nullptr,&special};
    return grunt_damage_override_impl(a,n,damage,dn,now,&b->detail.damage,&out->detail.damage,&route);
}
const ServerNpcGruntSpecialImpactApi special_impact_api{1,sizeof(ServerNpcGruntSpecialImpactApi),&special_vector,&grunt_special_impact,&grunt_special_damage};

int __cdecl grunt_damage_native_tail(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageNativeTailBindings* b,ServerNpcGruntDamageNativeTailResult* out) noexcept{
    if(!out)return 0;
    out->tail={};out->notify={};out->report={};reset_squad_report(out->squad);out->report_called=0;
    if(!b)return 0;
    const DamageReportRoute report{&b->report,&b->squad,&out->report,&out->squad,&out->report_called};
    const DamageNotifyRoute notify{&b->notify,&out->notify,&report};
    return grunt_damage_tail_impl(a,n,damage,dn,now,&b->tail,&out->tail,&notify);
}
int __cdecl grunt_damage_native_chain(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntDamageNativeChainBindings* b,ServerNpcGruntDamageNativeChainResult* out) noexcept{
    if(!out)return 0;
    out->detail.impact={};out->detail.right={};out->detail.left={};
    out->notify={};out->report={};reset_squad_report(out->squad);out->report_called=0;
    if(!b)return 0;
    const DamageReportRoute report{&b->report,&b->squad,&out->report,&out->squad,&out->report_called};
    const DamageNotifyRoute notify{&b->notify,&out->notify,&report};
    const DamageDetailRoute detail{&b->detail,&out->detail,&notify};
    return grunt_damage_override_impl(a,n,damage,dn,now,&b->detail.damage,&out->detail.damage,&detail);
}
const ServerNpcGruntDamageNativeChainApi damage_native_chain_api{1,sizeof(ServerNpcGruntDamageNativeChainApi),&grunt_damage_native_tail,&grunt_damage_native_chain};

int __cdecl grunt_ignition(void* a,unsigned n,void* attacker,void* inflictor,const ServerNpcGruntIgnitionBindings* b,ServerNpcGruntIgnitionResult* out) noexcept{
    if(!out)return 0;*out={};out->attacker_handle=0xffffffffu;
    if(!a||n<0xc44||!b||!relation_grunt_class(b->original_vtable))return 0;
    auto done=[&](){out->completed=1;return 1;};
    // The specific +2d4 variant checks the sixth argument (zero in 222a7b20).
    if(b->original_vtable==0x2248c514){out->class_blocked=1;return done();}
    if(attacker&&(!b->attacker_handle||!b->attacker_handle(b->context,attacker,&out->attacker_handle)))return 0;
    write(a,0xc3c,out->attacker_handle);
    if(inflictor){
        if(!b->inflictor_name||!b->inflictor_name(b->context,inflictor,&out->inflictor_name))return 0;
        if(out->inflictor_name){
            void* name=nullptr;unsigned extent=0;
            if(!memory_storage(&b->name_storage,out->inflictor_name,1,name,extent))return 0;
            if(!read<std::uint8_t>(name,0))out->inflictor_name=0;
        }
    }
    write(a,0xc40,out->inflictor_name);out->attribution_written=1;
    if(read<std::uint8_t>(a,0xbf)&8u){out->flag_blocked=1;return done();}
    // +f4 and +530 both return true for all eight supported original tables.
    void* flame=nullptr;
    if(!b->create_flame||!b->create_flame(b->context,a,attacker,inflictor,&flame))return 0;
    if(flame){
        out->created=1;
        if(!b->lifetime||!b->lifetime(b->context,flame,12.f))return 0;
        out->lifetime_called=1;
        out->flags_before=read<unsigned>(a,0xbc);out->flags_after=out->flags_before|0x8000000u;
        // +268 ret for all eight; 221639f0 tracks bit16 only, unchanged here.
        if(out->flags_before!=out->flags_after){write(a,0xbc,out->flags_after);out->flag_written=1;}
        if(!b->attach||!b->attach(b->context,a,flame))return 0;
        out->attached=1;
    }
    if(attacker){
        bool player=false;if(!b->is_player||!b->is_player(b->context,attacker,&player))return 0;
        out->player_checked=1;
        if(player){if(!b->event||!b->event(b->context,9,attacker,a))return 0;out->event_called=1;}
    }
    // The original sends output even if flame creation returned null.
    if(!b->output||!b->output(b->context,a,0x47c))return 0;
    out->output_called=1;return done();
}

int grunt_life_damage_impl(void* a,unsigned n,void* damage,unsigned dn,unsigned route,const ServerNpcGruntLifeDamageBindings* b,ServerNpcGruntLifeDamageResult* out,const IgnitionRoute* ignition) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0x638||!damage||dn<0x3c||!b||route>NpcDamageDead||!relation_grunt_class(b->original_vtable)||
       b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    auto finish=[&](int value){out->native_result=value;out->health_after=read<int>(a,0x9c);out->completed=1;return 1;};
    out->health_before=read<int>(a,0x9c);
    auto decrement=[&](double amount){
        if(!std::isfinite(amount))return false;
        const double integer=std::trunc(amount);
        const double next=static_cast<double>(read<int>(a,0x9c))-integer;
        if(integer< -2147483648.0||integer>2147483647.0||next< -2147483648.0||next>2147483647.0)return false;
        if(integer!=0){
            // All eight +1b4=220018f0 ret; no callback or dirty flag.
            write(a,0x9c,static_cast<int>(next));++out->health_writes;
        }
        return true;
    };
    auto deep_remove=[&](){
        if(read<int>(a,0x9c)>=-500)return true;
        if(!b->remove||!b->remove(b->context,a))return false;
        ++out->removed;return true;
    };
    if(route!=NpcDamageDying){
        if(void* inflictor=registry.Resolve(read<unsigned>(damage,0x24))){
            if(!b->center||!b->normalize||!b->shared_direction)return 0;
            out->inflictor_valid=1;float p[3]{},q[3]{};
            if(!b->center(b->context,inflictor,p)||!finite_vec(p))return 0;
            ++out->centers;p[2]=static_cast<float>(static_cast<double>(p[2])-10.0);
            if(!b->center(b->context,a,q)||!finite_vec(q))return 0;++out->centers;
            for(unsigned i=0;i<3;++i)out->direction[i]=static_cast<float>(static_cast<double>(p[i])-q[i]);
            if(!finite_vec(out->direction)||!b->normalize(b->context,out->direction)||!finite_vec(out->direction))return 0;
            out->normalized=1;
        }else if(route==NpcDamageAlive){
            if(!finite_vec(b->default_direction)||!b->shared_direction)return 0;
            std::memcpy(out->direction,b->default_direction,sizeof(out->direction));
        }
        if(out->inflictor_valid||route==NpcDamageAlive){
            std::memcpy(b->shared_direction,out->direction,sizeof(out->direction));out->direction_written=1;
        }
    }
    if(route==NpcDamageAlive){
        if(read<int>(a,0x94)!=1){
            const float amount=read<float>(damage,0x2c),prior=read<float>(a,0x634);
            if(!std::isfinite(amount)||!std::isfinite(prior))return 0;
            const float fraction=static_cast<float>(static_cast<double>(amount)-std::floor(static_cast<double>(amount)));
            double integral=static_cast<double>(amount)-fraction;
            const double accumulated=static_cast<double>(prior)+fraction;
            out->fraction_before=prior;
            out->fraction_after=static_cast<float>(accumulated);if(!std::isfinite(out->fraction_after))return 0;
            write(a,0x634,out->fraction_after);out->fraction_written=1;
            if(accumulated>=1.0){
                integral+=1.0;out->fraction_after=static_cast<float>(accumulated-1.0);
                write(a,0x634,out->fraction_after);
            }
            if(integral<=0.0)return finish(0);
            if(!decrement(integral))return 0;
        }
        if(read<std::uint8_t>(damage,0x3b)&1){
            const float amount=read<float>(damage,0x2c);if(!std::isfinite(amount))return 0;
            // 2248c514 +2d4=22353da0 rejects the zero final argument.
            if(amount>12.f&&b->original_vtable!=0x2248c514){
                void* attacker=registry.Resolve(read<unsigned>(damage,0x28));void* inflictor=registry.Resolve(read<unsigned>(damage,0x24));
                if(ignition){
                    auto bound=*ignition->bindings;bound.original_vtable=b->original_vtable;
                    if(!grunt_ignition(a,n,attacker,inflictor,&bound,ignition->result))return 0;
                }else if(!b->reaction||!b->reaction(b->context,a,attacker,inflictor))return 0;
                out->reaction_called=1;
            }
        }
    }else{
        if(route==NpcDamageDead&&(read<unsigned>(damage,0x38)&0x2e1u)&&read<int>(a,0x94)!=1){
            if(!decrement(static_cast<double>(read<float>(damage,0x2c))*0.1))return 0;
        }
        if((read<std::uint8_t>(damage,0x3b)&2)&&read<int>(a,0x94)!=1){
            if(!decrement(read<float>(damage,0x2c))||!deep_remove())return 0;
        }
        // Dying base 222a45b0 returns 1 unconditionally; dead also returns 1.
    }
    return finish(1);
}
int __cdecl grunt_life_damage(void* a,unsigned n,void* damage,unsigned dn,unsigned route,const ServerNpcGruntLifeDamageBindings* b,ServerNpcGruntLifeDamageResult* out) noexcept{
    return grunt_life_damage_impl(a,n,damage,dn,route,b,out,nullptr);
}
const ServerNpcGruntLifeDamageApi life_damage_api{1,sizeof(ServerNpcGruntLifeDamageApi),&grunt_life_damage};
int grunt_alive_damage_chain_impl(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntAliveDamageBindings* b,ServerNpcGruntAliveDamageResult* out,const IgnitionRoute* ignition) noexcept{
    if(!out)return 0;
    out->life={};
    auto& c=out->chain;
    c.detail.impact={};c.detail.right={};c.detail.left={};
    c.notify={};c.report={};reset_squad_report(c.squad);c.report_called=0;
    if(!b||!b->life.shared_direction)return 0;
    const LifeDamageRoute life{&b->life,&out->life,ignition};
    const DamageReportRoute report{&b->chain.report,&b->chain.squad,&c.report,&c.squad,&c.report_called};
    const DamageNotifyRoute notify{&b->chain.notify,&c.notify,&report,&life};
    const DamageDetailRoute detail{&b->chain.detail,&c.detail,&notify};
    return grunt_damage_override_impl(a,n,damage,dn,now,&b->chain.detail.damage,&c.detail.damage,&detail);
}
int __cdecl grunt_alive_damage_chain(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntAliveDamageBindings* b,ServerNpcGruntAliveDamageResult* out) noexcept{
    return grunt_alive_damage_chain_impl(a,n,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntAliveDamageApi alive_damage_api{1,sizeof(ServerNpcGruntAliveDamageApi),&grunt_alive_damage_chain};
int __cdecl grunt_ignition_alive(void* a,unsigned n,void* damage,unsigned dn,float now,const ServerNpcGruntIgnitionAliveBindings* b,ServerNpcGruntIgnitionAliveResult* out) noexcept{
    if(!out)return 0;out->ignition={};out->ignition.attacker_handle=0xffffffffu;
    if(!b)return 0;
    const IgnitionRoute route{&b->ignition,&out->ignition};
    return grunt_alive_damage_chain_impl(a,n,damage,dn,now,&b->alive,&out->alive,&route);
}
const ServerNpcGruntIgnitionApi ignition_api{1,sizeof(ServerNpcGruntIgnitionApi),&grunt_ignition,&grunt_ignition_alive};

bool jetpack_death_class(std::uint32_t vt) noexcept{
    return vt==0x2248c514||vt==0x2248e83c||vt==0x2249964c||vt==0x224a4a4c;
}
int __cdecl jetpack_death_mode(void* a,unsigned n,void* damage,unsigned dn,const ServerNpcJetpackDeathModeBindings* b,ServerNpcJetpackDeathModeResult* out) noexcept{
    if(!out)return 0;*out={};out->sample=out->initial_mode=out->final_mode=-1;
    if(!a||n<0x1074||!damage||dn<0x48||!b||!jetpack_death_class(b->original_vtable))return 0;
    std::memmove(static_cast<unsigned char*>(a)+0x102c,damage,0x48);out->copied=1;
    out->on_fire=(read<std::uint8_t>(a,0xbf)&8u)!=0;
    if(out->on_fire)out->initial_mode=5;
    else{
        if(!b->random_int||!b->random_int(b->context,0,100,&out->sample)||out->sample<0||out->sample>100)return 0;
        out->random_called=1;
        out->initial_mode=out->sample<27?1:out->sample<54?4:out->sample<81?0:3;
    }
    write(a,0x1004,out->initial_mode);
    bool yes=false;if(!b->effect_rule||!b->effect_rule(b->context,a,&yes))return 0;
    out->effect_checked=1;
    if(yes){if(!b->effect||!b->effect(b->context,a))return 0;out->effect_called=1;}
    if(!b->force_mode3||!b->force_mode3(b->context,&yes))return 0;
    out->force_checked=1;
    if(yes){write(a,0x1004,3);out->forced=1;}
    out->final_mode=read<int>(a,0x1004);
    constexpr std::uint32_t branches[]{0x22366826,0x22366826,0x223669fb,0x223668ec,0x2236681f,0x223668ec};
    if(out->final_mode>=0&&out->final_mode<6)out->branch=branches[out->final_mode];
    else out->branch=0x223669fb;
    out->tail_required=out->branch!=0x223669fb;
    out->completed=1;return 1;
}
struct JetpackBranchRoute {const ServerNpcJetpackDeathBranchBindings* bindings;ServerNpcJetpackDeathBranchResult* result;};
struct JetpackDeathRoute {const ServerNpcJetpackDeathModeBindings* bindings;ServerNpcJetpackDeathModeResult* result;const JetpackBranchRoute* branch=nullptr;};
int __cdecl grunt_ground_death(void*,unsigned,std::uint32_t,void*,unsigned,float,const ServerNpcGruntGroundDeathBindings*,ServerNpcGruntGroundDeathResult*) noexcept;
struct GroundDeathRoute {
    const ServerNpcGruntGroundDeathBindings* bindings;
    ServerNpcGruntGroundDeathResult* result;
    std::uint32_t handle;
    unsigned* called;
};

int jetpack_branch(void* a,unsigned n,void* damage,unsigned dn,float now,unsigned mode,const ServerNpcGruntSentenceBindings& sb,const GroundDeathRoute& ground,const JetpackBranchRoute& branch) noexcept{
    auto& out=*branch.result;out={};out.mode=mode;
    const auto& b=*branch.bindings;
    if(!a||n<0x1074||!damage||dn<0x48||!jetpack_death_class(sb.speech.original_vtable))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    const bool delayed=mode==0||mode==1||mode==4,immediate=mode==3||mode==5;
    if(!delayed&&!immediate){out.completed=1;return 1;}
    if(mode==4||immediate){
        // 22363f30 removes only serial-valid entities, in +fe8 then +fe4 order.
        for(unsigned offset:{0xfe8u,0xfe4u}){
            if(void* e=registry.Resolve(read<unsigned>(a,offset))){
                if(!b.remove||!b.remove(b.context,e))return 0;
                ++out.removed;write(a,offset,0xffffffffu);++out.cleared;
            }
        }
        out.cleanup=1;
    }
    auto sentence=[&](){
        if(read<std::uint8_t>(a,0xc44))return true;
        if(!b.sentence||!b.sentence(b.context,a,"SINTEK_JETPACK_DEATH"))return false;
        out.sentence=1;return true;
    };
    if(delayed){
        if(!b.powerdown||!b.powerdown(b.context,a,"NPC_Jetpack.Powerdown"))return 0;
        out.powerdown=1;
        if(!sentence())return 0;
        if(!b.ignite||!b.ignite(b.context,a,1.f,1,nullptr,nullptr,25.f,1))return 0;
        out.ignited=1;
        if(read<int>(a,0x8c)!=1){
            if(!death_dirty(a,b.dirty_storage,out.local_dirty,out.external_dirty))return 0;
            write(a,0x8c,1);out.life_changed=1;
        }
        for(unsigned i=0;i<3;++i)out.force[i]=read<float>(damage,4*i);
        if(!finite_vec(out.force)||!b.ragdoll||!b.ragdoll(b.context,a,damage,out.force))return 0;
        out.ragdoll=1;
        if(!b.time||!b.time(b.context,&out.stamp)||!std::isfinite(out.stamp))return 0;
        write(a,0xffc,out.stamp);
        if(!b.install_think||!b.install_think(b.context,a,0x22365760u))return 0;
        out.installed=1;
        if(!b.time(b.context,&out.scheduled)||!std::isfinite(out.scheduled)||!b.schedule||!b.schedule(b.context,a,out.scheduled))return 0;
        out.scheduled_called=out.completed=1;return 1;
    }
    // 22366922 calls removal even if the +1028 handle is stale or null.
    if(!b.remove||!b.remove(b.context,registry.Resolve(read<unsigned>(a,0x1028))))return 0;
    out.aux_removed=1;write(a,0x1028,0xffffffffu);
    if(!b.effect_begin||!b.effect_begin(b.context,a))return 0;
    out.effect_scope=1;
    // Always close the temporary scope once opened, including adapter failures.
    bool ok=sentence();
    if(ok&&(read<unsigned>(a,0xb8)&0x800u)){
        ok=b.refresh_origin&&b.refresh_origin(b.context,a);out.refreshed=ok;
    }
    if(ok){ok=b.immediate_effect&&b.immediate_effect(b.context,a);out.effect_called=ok;}
    if(ok){
        auto bound=*ground.bindings;
        auto& identity=bound.base.combat.death.event.prefix.removal.removal.leave.sentence;
        identity.entries=sb.entries;identity.count=sb.count;identity.speech.original_vtable=sb.speech.original_vtable;
        ok=grunt_ground_death(a,n,ground.handle,damage,dn,now,&bound,ground.result);
        out.base_called=ok;if(ok)*ground.called=1;
    }
    const bool ended=b.effect_end&&b.effect_end(b.context,a);out.effect_ended=ended;
    if(!ok||!ended)return 0;
    out.completed=1;return 1;
}
bool ground_death_class(std::uint32_t vt) noexcept{
    return vt==0x224954a4||vt==0x2249aaa4||vt==0x224a416c;
}

int grunt_life_dispatch_impl(void* a,unsigned n,const void* damage,unsigned dn,float now,const ServerNpcGruntLifeDispatchBindings* b,ServerNpcGruntLifeDispatchResult* out,const JetpackDeathRoute* jet,const GroundDeathRoute* ground=nullptr) noexcept{
    if(!out)return 0;
    out->alive.alive.chain.detail.damage.completed=0;out->alive.alive.life={};out->alive.ignition={};out->other={};
    out->scaled_amount=0;out->scale=1;out->initial_state=-1;out->native_result=0;
    out->completed=out->route=out->mode_blocked=out->owner_checked=out->owner_scaled=out->scale_blocked=out->effect_called=out->object_called=out->death_called=out->post_death_noop=0;
    if(!a||n<0xdb2||!damage||dn<0x48||!b||!std::isfinite(now))return 0;
    const auto& sb=b->combat.alive.chain.detail.damage.base.chain.prefix.damage.reaction.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    auto done=[&](){out->completed=1;return 1;};
    alignas(4) unsigned char copy[0x48];std::memcpy(copy,damage,sizeof(copy));
    out->scaled_amount=read<float>(copy,0x2c);
    if(!read<int>(a,0x94)){out->mode_blocked=1;return done();}
    if(!std::isfinite(out->scaled_amount))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    if(void* owner=registry.Resolve(read<unsigned>(a,0xe8))){
        bool yes=false;if(!b->owner_type||!b->owner_type(b->context,owner,&yes))return 0;
        out->owner_checked=1;
        if(yes){
            if(!b->owner_scale||!b->owner_scale(b->context,owner,a,copy,&out->scale)||!std::isfinite(out->scale))return 0;
            out->owner_scaled=1;
            out->scaled_amount=static_cast<float>(static_cast<double>(read<float>(copy,0x2c))*out->scale);
            if(!std::isfinite(out->scaled_amount))return 0;
            write(copy,0x2c,out->scaled_amount);
            if(out->scaled_amount<=0){out->scale_blocked=1;return done();}
        }
    }
    if(read<std::uint8_t>(copy,0x39)&1u){
        float pos[3];for(unsigned i=0;i<3;++i)pos[i]=read<float>(copy,0xc+4*i);
        if(!finite_vec(pos)||!b->pre_effect||!b->pre_effect(b->context,a,pos))return 0;
        out->effect_called=1;
    }
    out->initial_state=read<int>(a,0x8c);
    if(out->initial_state==0){
        out->route=NpcDamageAlive;
        if(!grunt_ignition_alive(a,n,copy,sizeof(copy),now,&b->combat,&out->alive))return 0;
        out->native_result=out->alive.alive.chain.detail.damage.native_result;
        // Retail tests live health even when the alive handler returned zero.
        if(read<int>(a,0x9c)<=0){
            if(const auto token=read<unsigned>(a,0x218)){
                if(!b->object_death||!b->object_death(b->context,token,0))return 0;
                out->object_called=1;
            }
            if(jet&&jetpack_death_class(sb.speech.original_vtable)){
                auto bound=*jet->bindings;bound.original_vtable=sb.speech.original_vtable;
                if(!jetpack_death_mode(a,n,copy,sizeof(copy),&bound,jet->result))return 0;
                if(jet->result->tail_required){
                    if(jet->branch){
                        if(!ground||!jetpack_branch(a,n,copy,sizeof(copy),now,static_cast<unsigned>(jet->result->final_mode),sb,*ground,*jet->branch))return 0;
                    }else if(!bound.tail||!bound.tail(bound.context,a,copy,static_cast<unsigned>(jet->result->final_mode),jet->result->branch))return 0;
                    jet->result->tail_called=1;
                }
            }else if(ground&&ground_death_class(sb.speech.original_vtable)){
                // Bind the same class/registry as the accepted damage entry, preserving
                // the death-specific engine service contexts.
                auto bound=*ground->bindings;
                auto& identity=bound.base.combat.death.event.prefix.removal.removal.leave.sentence;
                identity.entries=sb.entries;identity.count=sb.count;identity.speech.original_vtable=sb.speech.original_vtable;
                if(!grunt_ground_death(a,n,ground->handle,copy,sizeof(copy),now,&bound,ground->result))return 0;
                *ground->called=1;
            }else if(!b->death||!b->death(b->context,a,copy,sb.speech.original_vtable))return 0;
            out->death_called=1;
            // All eight +398=22044730 false, +3a8=222a4490 ret.
            out->post_death_noop=1;
        }
    }else{
        out->route=out->initial_state==1?NpcDamageDying:NpcDamageDead;
        auto life=b->combat.alive.life;life.entries=sb.entries;life.count=sb.count;life.original_vtable=sb.speech.original_vtable;
        if(!grunt_life_damage(a,n,copy,sizeof(copy),out->route,&life,&out->other))return 0;
        out->native_result=out->other.native_result;
        // Dead branch's +398 is false, so +3a4 is unreachable for these classes.
    }
    return done();
}
int __cdecl grunt_life_dispatch(void* a,unsigned n,const void* damage,unsigned dn,float now,const ServerNpcGruntLifeDispatchBindings* b,ServerNpcGruntLifeDispatchResult* out) noexcept{
    return grunt_life_dispatch_impl(a,n,damage,dn,now,b,out,nullptr);
}
const ServerNpcGruntLifeDispatchApi life_dispatch_api{1,sizeof(ServerNpcGruntLifeDispatchApi),&grunt_life_dispatch};
int __cdecl jetpack_death_dispatch(void* a,unsigned n,const void* damage,unsigned dn,float now,const ServerNpcJetpackDeathDispatchBindings* b,ServerNpcJetpackDeathDispatchResult* out) noexcept{
    if(!out)return 0;out->jet={};out->jet.sample=out->jet.initial_mode=out->jet.final_mode=-1;
    if(!b)return 0;
    const JetpackDeathRoute route{&b->jet,&out->jet};
    return grunt_life_dispatch_impl(a,n,damage,dn,now,&b->life,&out->life,&route);
}
const ServerNpcJetpackDeathModeApi jetpack_death_mode_api{1,sizeof(ServerNpcJetpackDeathModeApi),&jetpack_death_mode,&jetpack_death_dispatch};
int __cdecl grunt_ground_death_prepare(void* a,unsigned n,void* damage,unsigned dn,const ServerNpcGruntGroundDeathBindings* b,ServerNpcGruntGroundDeathResult* out) noexcept{
    if(!out)return 0;
    out->base.combat.death.event.completed=0;
    out->ammo=nullptr;out->attacker_class=0;out->grenade_sample=-1;
    out->completed=out->attacker_valid=out->loot_enabled=out->weapon_checked=out->rule_checked=out->matches=out->ammo_dropped=out->kick_called=out->grenade_dropped=out->copy_used=out->components=out->base_called=0;
    if(!a||n<0xdb2||!damage||dn<0x48||!b||!b->loot_enabled)return 0;
    const auto& sb=b->base.combat.death.event.prefix.removal.removal.leave.sentence;
    if(!relation_grunt_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(sb.entries,sb.count);
    if(void* attacker=registry.Resolve(read<unsigned>(damage,0x28))){
        out->attacker_valid=1;
        if(!b->classification||!b->classification(b->context,attacker,&out->attacker_class))return 0;
    }
    write(a,0xc68,out->attacker_class);
    bool enabled=false;if(!b->loot_enabled(b->context,&enabled))return 0;
    out->loot_enabled=enabled;
    if(!enabled){out->completed=1;return 1;}
    if(!(read<std::uint8_t>(a,0xb6)&0x10u)){
        void* weapon=nullptr;if(!b->weapon||!b->weapon(b->context,a,&weapon))return 0;
        out->weapon_checked=1;
        if(weapon){
            bool yes=false;if(!b->loot_rule||!b->loot_rule(b->context,&yes))return 0;
            out->rule_checked=1;
            if(yes){
                constexpr const char* names[]{"weapon_assault_rifle","weapon_scattergun","weapon_npc_pistol"};
                constexpr const char* ammo[]{"item_ammo_ar_grenade","item_ammo_flak","item_ammo_duag"};
                for(unsigned i=0;i<3;++i){
                    if(!b->matches||!b->matches(b->context,weapon,names[i],&yes))return 0;
                    ++out->matches;
                    if(yes){
                        out->ammo=ammo[i];void* item=nullptr;
                        if(!b->drop||!b->drop(b->context,a,out->ammo,&item))return 0;
                        out->ammo_dropped=item!=nullptr;
                        if(item){if(!b->kick||!b->kick(b->context,item))return 0;out->kick_called=1;}
                        break;
                    }
                }
            }
        }
    }
    if(!b->random_int||!b->random_int(b->context,0,1,&out->grenade_sample)||out->grenade_sample<0||out->grenade_sample>1)return 0;
    if(out->grenade_sample==1&&read<int>(a,0xc38)>0){
        void* item=nullptr;if(!b->drop||!b->drop(b->context,a,"weapon_grenade",&item))return 0;
        out->grenade_dropped=item!=nullptr;
    }
    if(read<std::uint8_t>(a,0xb6)&8u){
        std::memcpy(out->copied,damage,0x48);out->copy_used=1;
        float scaled[3];
        for(unsigned i=0;i<3;++i){scaled[i]=static_cast<float>(static_cast<double>(read<float>(damage,4*i))*10.0);if(!std::isfinite(scaled[i]))return 0;}
        for(unsigned i=0;i<3;++i){
            float v=0;if(!b->random_float||!b->random_float(b->context,10.f,50.f,&v)||!std::isfinite(v)||v<10.f||v>50.f)return 0;
            const float next=static_cast<float>(static_cast<double>(scaled[i])+v);if(!std::isfinite(next))return 0;
            write(out->copied,4*i,next);
        }
    }
    out->completed=1;return 1;
}
int __cdecl grunt_ground_death(void* a,unsigned n,std::uint32_t handle,void* damage,unsigned dn,float now,const ServerNpcGruntGroundDeathBindings* b,ServerNpcGruntGroundDeathResult* out) noexcept{
    if(!out||!b||!std::isfinite(now))return 0;
    const auto& rb=b->base.combat.death.event.prefix.removal;
    const auto& sb=rb.removal.leave.sentence;
    if(!a||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries)||sin_independent::EntityHandleView(sb.entries,sb.count).Resolve(handle)!=a)return 0;
    if(!grunt_ground_death_prepare(a,n,damage,dn,b,out))return 0;
    out->completed=0;
    void* forwarded=out->copy_used?static_cast<void*>(out->copied):damage;
    // Original 2208b3b0 calls every live component +44 before base death.
    for(int i=0;;++i){
        const int count=read<int>(a,0xc28);if(count>4096||i>4096)return 0;
        if(i>=count)break;
        void* array=nullptr;unsigned extent=0;
        if(!memory_storage(&rb.component_storage,read<unsigned>(a,0xc1c),4u*static_cast<unsigned>(count),array,extent))return 0;
        const auto token=read<unsigned>(array,4*i);
        if(!token||!b->component_death||!b->component_death(b->context,token,forwarded))return 0;
        ++out->components;
    }
    if(!grunt_death_selected(a,n,handle,forwarded,0x48,now,&b->base,&out->base))return 0;
    out->base_called=out->completed=1;return 1;
}
const ServerNpcGruntGroundDeathApi ground_death_api{1,sizeof(ServerNpcGruntGroundDeathApi),&grunt_ground_death_prepare,&grunt_ground_death};
int __cdecl grunt_ground_life(void* a,unsigned n,std::uint32_t handle,const void* damage,unsigned dn,float now,const ServerNpcGruntGroundLifeBindings* b,ServerNpcGruntGroundLifeResult* out) noexcept{
    if(!out)return 0;out->ground.completed=out->ground.base_called=0;out->ground_called=0;
    out->jet={};out->jet.sample=out->jet.initial_mode=out->jet.final_mode=-1;
    if(!b||!a)return 0;
    const auto& sb=b->life.combat.alive.chain.detail.damage.base.chain.prefix.damage.reaction.sentence;
    if(sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries)||
        sin_independent::EntityHandleView(sb.entries,sb.count).Resolve(handle)!=a)return 0;
    const JetpackDeathRoute jet{&b->jet,&out->jet};
    const GroundDeathRoute ground{&b->ground,&out->ground,handle,&out->ground_called};
    return grunt_life_dispatch_impl(a,n,damage,dn,now,&b->life,&out->life,&jet,&ground);
}
const ServerNpcGruntGroundLifeApi ground_life_api{1,sizeof(ServerNpcGruntGroundLifeApi),&grunt_ground_life};
int __cdecl jetpack_death_branches(void* a,unsigned n,std::uint32_t handle,const void* damage,unsigned dn,float now,const ServerNpcJetpackDeathBranchesBindings* b,ServerNpcJetpackDeathBranchesResult* out) noexcept{
    if(!out)return 0;out->branch={};
    auto& c=out->combat;
    c.ground.completed=c.ground.base_called=0;c.ground_called=0;c.jet={};c.jet.sample=c.jet.initial_mode=c.jet.final_mode=-1;
    if(!b||!a)return 0;
    const auto& sb=b->combat.life.combat.alive.chain.detail.damage.base.chain.prefix.damage.reaction.sentence;
    if(!jetpack_death_class(sb.speech.original_vtable)||sb.count>sin_independent::kEntityHandleCapacity||(sb.count&&!sb.entries)||
        sin_independent::EntityHandleView(sb.entries,sb.count).Resolve(handle)!=a)return 0;
    const JetpackBranchRoute branch{&b->branch,&out->branch};
    const JetpackDeathRoute jet{&b->combat.jet,&c.jet,&branch};
    const GroundDeathRoute ground{&b->combat.ground,&c.ground,handle,&c.ground_called};
    return grunt_life_dispatch_impl(a,n,damage,dn,now,&b->combat.life,&c.life,&jet,&ground);
}
const ServerNpcJetpackDeathBranchesApi jetpack_death_branches_api{1,sizeof(ServerNpcJetpackDeathBranchesApi),&jetpack_death_branches};

















const ServerNpcGruntRemovalApi removal_api{1,sizeof(ServerNpcGruntRemovalApi),&grunt_removal};
const ServerNpcGruntNativeStateApi native_state_api{1,sizeof(ServerNpcGruntNativeStateApi),&native_state_choose,&native_state_apply};
const ServerNpcGruntSquadEnemyApi squad_enemy_api{1,sizeof(ServerNpcGruntSquadEnemyApi),&squad_enemy_broadcast};
const ServerNpcGruntSquadReportApi squad_report_api{1,sizeof(ServerNpcGruntSquadReportApi),&squad_report,&squad_report_update,&squad_report_gather};
const ServerNpcGruntPerceptionSlotsApi perception_slots_api{1,sizeof(ServerNpcGruntPerceptionSlotsApi),&slots_set_enemy,&slots_enemy_switch,&slots_perception_update,&slots_perception_gather};
const ServerNpcGruntSquadSlotsApi squad_slots_api{1,sizeof(ServerNpcGruntSquadSlotsApi),&slots_read,&slots_bit,&slots_reserve,&slots_occupied,&slots_release};
const ServerNpcGruntSquadDestroyApi squad_destroy_api{1,sizeof(ServerNpcGruntSquadDestroyApi),&squad_destroy};
const ServerNpcGruntSquadRegistryApi squad_registry_api{1,sizeof(ServerNpcGruntSquadRegistryApi),&squad_lookup,&squad_create,&squad_attach};
const ServerNpcGruntSquadJoinApi squad_join_api{1,sizeof(ServerNpcGruntSquadJoinApi),&grunt_squad_join,&grunt_squad_detach};
const ServerNpcGruntSquadLeaveApi squad_leave_api{1,sizeof(ServerNpcGruntSquadLeaveApi),&grunt_squad_leave};
const ServerNpcGruntRequestSpeechApi request_speech_api{1,sizeof(ServerNpcGruntRequestSpeechApi),&grunt_speak_request,&grunt_survivor_request};
const ServerNpcGruntNativeSpeechApi native_speech_api{1,sizeof(ServerNpcGruntNativeSpeechApi),&grunt_speak_native,&grunt_survivor};
const ServerNpcGruntSentenceApi sentence_api{1,sizeof(ServerNpcGruntSentenceApi),&sentence_gate,&sentence_dispatch};
int grunt_preschedule_impl(void* a,unsigned bytes,float now,const ServerNpcGruntPrescheduleBindings* b,ServerNpcGruntPrescheduleResult* out,const ServerNpcGruntSpeechQueueBindings* speech=nullptr,ServerNpcGruntSpeechQueueResult* speech_out=nullptr) noexcept{
    if(!preschedule_init(a,bytes,now,b,out))return 0;
    const auto registry=sin_independent::EntityHandleView(b->entries,b->count);
    const float threshold=read<float>(a,0xc70);
    if(!std::isfinite(threshold))return 0;
    if(threshold>1.f){
        void* enemy=registry.Resolve(read<unsigned>(a,0x8a0));
        if(enemy){
            float p[3],e[3];
            if(!b->origin||!b->origin(b->context,a,p)||!b->origin(b->context,enemy,e)||!finite_vec(p)||!finite_vec(e))return 0;
            write<std::uint8_t>(a,0xc6c,std::abs(static_cast<double>(p[2])-e[2])>=threshold?1:0);out->height_written=1;
        }
    }
    const auto component=read<unsigned>(a,0xc18);
    if(component){out->component_called=1;if(!b->component_tick||!b->component_tick(b->context,component))return 0;}
    out->speech_called=1;
    if(speech){
        auto bound=*speech;bound.original_vtable=b->original_vtable;
        if(!speech_queue_tick(a,bytes,now,&bound,speech_out))return 0;
    }else if(!b->speech_queue_tick||!b->speech_queue_tick(b->context,a,now))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!cond)return 0;
    auto store=[&](int id,bool value,unsigned& flag){ServerNpcConditionResult cr{};if(!cond->store(a,bytes,id,NpcConditionCurrent,value,&b->conditions,&cr))return false;flag=cr.mapped;return true;};
    if(read<std::uint8_t>(a,0xbf)&8u){
        bool yes=false;out->rule_called=1;if(!b->rule_query||!b->rule_query(b->context,&yes))return 0;
        if(yes){
            if(read<int>(a,0x9c)){out->death_called=1;if(!b->death||!b->death(b->context,a))return 0;write(a,0x9c,0);}
        }else if(!store(0x46,true,out->condition46_written))return 0;
    }else if(!store(0x46,false,out->condition46_written))return 0;
    if(!pending_policy(a,now,*b,*out))return 0;
    if(out->accepted){
        const auto handle=read<unsigned>(a,0xcb8);void* object=registry.Resolve(handle);
        if(!object||!b->entity_handle||!b->entity_handle(b->context,object,&out->target_handle)||registry.Resolve(out->target_handle)!=object)return 0;
        write(a,0x8a8,out->target_handle);write(a,0xcb4,handle);write(a,0xcb8,0xffffffffu);
    }
    if(!store(0x4c,out->accepted!=0,out->condition4c_written))return 0;
    out->completed=1;return 1;
}
int __cdecl grunt_preschedule(void* a,unsigned bytes,float now,const ServerNpcGruntPrescheduleBindings* b,ServerNpcGruntPrescheduleResult* out) noexcept{
    return grunt_preschedule_impl(a,bytes,now,b,out);
}
int __cdecl grunt_preschedule_speech(void* a,unsigned bytes,float now,const ServerNpcGruntPrescheduleBindings* b,const ServerNpcGruntSpeechQueueBindings* speech,ServerNpcGruntPrescheduleResult* out,ServerNpcGruntSpeechQueueResult* speech_out) noexcept{
    if(!speech||!speech_out)return 0;*speech_out={};speech_out->index=-1;
    return grunt_preschedule_impl(a,bytes,now,b,out,speech,speech_out);
}
const ServerNpcGruntSpeechQueueApi speech_queue_api{1,sizeof(ServerNpcGruntSpeechQueueApi),&speech_queue_tick,&grunt_preschedule_speech};
const ServerNpcGruntPrescheduleApi preschedule_api{1,sizeof(ServerNpcGruntPrescheduleApi),&grunt_preschedule,&pending_object};
const ServerNpcGruntGatherNativeApi gather_native_api{1,sizeof(ServerNpcGruntGatherNativeApi),&grunt_gather_native};
const ServerNpcGruntObjectSearchApi object_search_api{1,sizeof(ServerNpcGruntObjectSearchApi),&grunt_object_search};
const ServerNpcGruntGatherPointApi gather_point_api{1,sizeof(ServerNpcGruntGatherPointApi),&grunt_gather_point};
const ServerNpcGruntPointPolicyApi point_policy_api{1,sizeof(ServerNpcGruntPointPolicyApi),&grunt_point_policy};
const ServerNpcGruntGatherConditionsApi gather_conditions_api{1,sizeof(ServerNpcGruntGatherConditionsApi),&grunt_gather_conditions};
const ServerNpcGruntConditionUpdateApi condition_update_api{1,sizeof(ServerNpcGruntConditionUpdateApi),&grunt_condition_update};
const ServerNpcGruntUpdateGateApi update_gate_api{1,sizeof(ServerNpcGruntUpdateGateApi),&grunt_update_gate};
const ServerNpcGruntPostUpdateApi post_update_api{1,sizeof(ServerNpcGruntPostUpdateApi),&grunt_flinch,&grunt_post_update};
const ServerNpcGruntAuxConditionsApi aux_conditions_api{1,sizeof(ServerNpcGruntAuxConditionsApi),&grunt_target_conditions,&grunt_ammo_conditions,&grunt_snapshot_conditions};
const ServerNpcGruntWeaponSearchApi weapon_search_api{1,sizeof(ServerNpcGruntWeaponSearchApi),&grunt_weapon_eligible,&grunt_weapon_search};
const ServerNpcGruntGoalRefreshApi goal_refresh_api{1,sizeof(ServerNpcGruntGoalRefreshApi),&grunt_goal_refresh};
const ServerNpcGruntCombatConditionsApi combat_conditions_api{1,sizeof(ServerNpcGruntCombatConditionsApi),&grunt_combat_conditions};
const ServerNpcGruntAttackApi attack_api{1,sizeof(ServerNpcGruntAttackApi),&grunt_attack_gather};
const ServerNpcGruntCombatRangeApi combat_range_api{1,sizeof(ServerNpcGruntCombatRangeApi),&combat_distance,&combat_range_gather};
const ServerNpcGruntCombatVisibilityApi combat_visibility_api{1,sizeof(ServerNpcGruntCombatVisibilityApi),&combat_visibility_query,&combat_visibility_gather};
const ServerNpcGruntPerceptionTickApi tick_api{1,sizeof(ServerNpcGruntPerceptionTickApi),&perception_tick};
const ServerNpcGruntSoundScanApi scan_api{1,sizeof(ServerNpcGruntSoundScanApi),&grunt_sound_scan};
const ServerNpcGruntHearingApi hearing_api{1,sizeof(ServerNpcGruntHearingApi),&grunt_hearing_mask,&grunt_hearing_filter,&grunt_hearing_gather};
const ServerNpcGruntVisualApi visual_api{1,sizeof(ServerNpcGruntVisualApi),&grunt_visual_gather};
const ServerNpcSensedEntitiesApi sensed_api{1,sizeof(ServerNpcSensedEntitiesApi),&sensed_first,&sensed_next,&sensed_contains};
const ServerNpcGruntSpeechApi speech_api{1,sizeof(ServerNpcGruntSpeechApi),&grunt_speak};
const ServerNpcGruntEnemySwitchApi switch_api{1,sizeof(ServerNpcGruntEnemySwitchApi),&grunt_enemy_switch};
const ServerNpcGruntEnemyChoiceGateApi choice_gate_api{1,sizeof(ServerNpcGruntEnemyChoiceGateApi),&grunt_enemy_choice_gate};
const ServerNpcGruntMemoryReportApi grunt_memory_report_api{1,sizeof(ServerNpcGruntMemoryReportApi),&grunt_memory_report};
const ServerNpcGruntBestEnemyApi grunt_best_api{1,sizeof(ServerNpcGruntBestEnemyApi),&grunt_best_enemy};
const ServerNpcGruntRelationApi relation_api{1,sizeof(ServerNpcGruntRelationApi),&grunt_relation,&grunt_valid};
const ServerNpcEnemyMemoryControlApi control_api{1,sizeof(ServerNpcEnemyMemoryControlApi),&initialize_memory_owner,&memory_control};
const ServerNpcEnemyMemoryMutationApi mutation_api{1,sizeof(ServerNpcEnemyMemoryMutationApi),&upsert_memory,&remove_memory,&erase_memory_record};
const ServerNpcEnemyMemoryTreeApi tree_api{1,sizeof(ServerNpcEnemyMemoryTreeApi),&tree_validate,&tree_insert,&tree_erase};
const ServerNpcEnemyMemoryMaintenanceApi maintenance_api{1,sizeof(ServerNpcEnemyMemoryMaintenanceApi),&maintain_memory};
const ServerNpcEnemyMemoryRecordApi record_api{1,sizeof(ServerNpcEnemyMemoryRecordApi),&initialize_memory_record,&update_memory_record,&memory_expiry};
const ServerNpcBestEnemyQueryApi best_query_api{1,sizeof(ServerNpcBestEnemyQueryApi),&query_best_enemy};
const ServerNpcUnreachableEnemyApi unreachable_api{1,sizeof(ServerNpcUnreachableEnemyApi),&unreachable_enemy};
const ServerNpcBestEnemyApi best_enemy_api{1,sizeof(ServerNpcBestEnemyApi),&best_enemy};
const ServerNpcEnemyMemoryQueryApi query_api{1,sizeof(ServerNpcEnemyMemoryQueryApi),&query_last_seen,&query_aim_vector};
const ServerNpcEnemyMemoryApi memory_api{1,sizeof(ServerNpcEnemyMemoryApi),&memory_lookup,&memory_timestamp,&memory_position,&memory_touch,&memory_recent};
int __cdecl notify_enemy_change(void* context,void* actor) noexcept{
    if(!context)return 0;auto& b=*static_cast<ServerNpcEnemyNotifyContext*>(context);
    switch(b.original_vtable){
        case 0x2248df04:case 0x224954a4:case 0x2249aaa4:case 0x224a416c:
        case 0x2248c514:case 0x2248e83c:case 0x2249964c:case 0x224a4a4c:break;
        default:return 0;
    }
    ++b.calls;return clear_combat_conditions(actor,b.actor_bytes,&b.conditions,&b.last);
}
const ServerNpcEnemyNotifyApi notify_api{1,sizeof(ServerNpcEnemyNotifyApi),&notify_enemy_change};
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

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyMemoryApi=_SiN_GetServerNpcEnemyMemoryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryApi* __cdecl
SiN_GetServerNpcEnemyMemoryApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyMemoryApi)?&memory_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyMemoryQueryApi=_SiN_GetServerNpcEnemyMemoryQueryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryQueryApi* __cdecl
SiN_GetServerNpcEnemyMemoryQueryApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyMemoryQueryApi)?&query_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcBestEnemyApi=_SiN_GetServerNpcBestEnemyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcBestEnemyApi* __cdecl
SiN_GetServerNpcBestEnemyApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcBestEnemyApi)?&best_enemy_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcUnreachableEnemyApi=_SiN_GetServerNpcUnreachableEnemyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcUnreachableEnemyApi* __cdecl
SiN_GetServerNpcUnreachableEnemyApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcUnreachableEnemyApi)?&unreachable_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcBestEnemyQueryApi=_SiN_GetServerNpcBestEnemyQueryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcBestEnemyQueryApi* __cdecl
SiN_GetServerNpcBestEnemyQueryApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcBestEnemyQueryApi)?&best_query_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyMemoryRecordApi=_SiN_GetServerNpcEnemyMemoryRecordApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryRecordApi* __cdecl
SiN_GetServerNpcEnemyMemoryRecordApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyMemoryRecordApi)?&record_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyMemoryMaintenanceApi=_SiN_GetServerNpcEnemyMemoryMaintenanceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryMaintenanceApi* __cdecl
SiN_GetServerNpcEnemyMemoryMaintenanceApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyMemoryMaintenanceApi)?&maintenance_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyMemoryTreeApi=_SiN_GetServerNpcEnemyMemoryTreeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryTreeApi* __cdecl
SiN_GetServerNpcEnemyMemoryTreeApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyMemoryTreeApi)?&tree_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyMemoryMutationApi=_SiN_GetServerNpcEnemyMemoryMutationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryMutationApi* __cdecl
SiN_GetServerNpcEnemyMemoryMutationApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyMemoryMutationApi)?&mutation_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyNotifyApi=_SiN_GetServerNpcEnemyNotifyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyNotifyApi* __cdecl
SiN_GetServerNpcEnemyNotifyApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyNotifyApi)?&notify_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcEnemyMemoryControlApi=_SiN_GetServerNpcEnemyMemoryControlApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryControlApi* __cdecl
SiN_GetServerNpcEnemyMemoryControlApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcEnemyMemoryControlApi)?&control_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntRelationApi=_SiN_GetServerNpcGruntRelationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntRelationApi* __cdecl
SiN_GetServerNpcGruntRelationApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntRelationApi)?&relation_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcRelationMatrixApi=_SiN_GetServerNpcRelationMatrixApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcRelationMatrixApi* __cdecl
SiN_GetServerNpcRelationMatrixApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcRelationMatrixApi)?&matrix_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntBestEnemyApi=_SiN_GetServerNpcGruntBestEnemyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntBestEnemyApi* __cdecl
SiN_GetServerNpcGruntBestEnemyApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntBestEnemyApi)?&grunt_best_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntMemoryReportApi=_SiN_GetServerNpcGruntMemoryReportApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntMemoryReportApi* __cdecl
SiN_GetServerNpcGruntMemoryReportApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntMemoryReportApi)?&grunt_memory_report_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntEnemyChoiceGateApi=_SiN_GetServerNpcGruntEnemyChoiceGateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntEnemyChoiceGateApi* __cdecl
SiN_GetServerNpcGruntEnemyChoiceGateApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntEnemyChoiceGateApi)?&choice_gate_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntEnemySwitchApi=_SiN_GetServerNpcGruntEnemySwitchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntEnemySwitchApi* __cdecl
SiN_GetServerNpcGruntEnemySwitchApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntEnemySwitchApi)?&switch_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSpeechApi=_SiN_GetServerNpcGruntSpeechApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSpeechApi* __cdecl
SiN_GetServerNpcGruntSpeechApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSpeechApi)?&speech_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSensedEntitiesApi=_SiN_GetServerNpcSensedEntitiesApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSensedEntitiesApi* __cdecl
SiN_GetServerNpcSensedEntitiesApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSensedEntitiesApi)?&sensed_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntVisualConditionsApi=_SiN_GetServerNpcGruntVisualConditionsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntVisualApi* __cdecl
SiN_GetServerNpcGruntVisualConditionsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntVisualApi)?&visual_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSoundScanApi=_SiN_GetServerNpcGruntSoundScanApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSoundScanApi* __cdecl
SiN_GetServerNpcGruntSoundScanApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSoundScanApi)?&scan_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntPerceptionTickApi=_SiN_GetServerNpcGruntPerceptionTickApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPerceptionTickApi* __cdecl
SiN_GetServerNpcGruntPerceptionTickApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntPerceptionTickApi)?&tick_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntHearingApi=_SiN_GetServerNpcGruntHearingApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntHearingApi* __cdecl
SiN_GetServerNpcGruntHearingApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntHearingApi)?&hearing_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntCombatVisibilityApi=_SiN_GetServerNpcGruntCombatVisibilityApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntCombatVisibilityApi* __cdecl
SiN_GetServerNpcGruntCombatVisibilityApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntCombatVisibilityApi)?&combat_visibility_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntCombatRangeApi=_SiN_GetServerNpcGruntCombatRangeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntCombatRangeApi* __cdecl
SiN_GetServerNpcGruntCombatRangeApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntCombatRangeApi)?&combat_range_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntAttackConditionsApi=_SiN_GetServerNpcGruntAttackConditionsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntAttackApi* __cdecl
SiN_GetServerNpcGruntAttackConditionsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntAttackApi)?&attack_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntCombatConditionsApi=_SiN_GetServerNpcGruntCombatConditionsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntCombatConditionsApi* __cdecl
SiN_GetServerNpcGruntCombatConditionsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntCombatConditionsApi)?&combat_conditions_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntGoalRefreshApi=_SiN_GetServerNpcGruntGoalRefreshApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGoalRefreshApi* __cdecl
SiN_GetServerNpcGruntGoalRefreshApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntGoalRefreshApi)?&goal_refresh_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntCombatGoalApi=_SiN_GetServerNpcGruntCombatGoalApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntCombatGoalApi* __cdecl
SiN_GetServerNpcGruntCombatGoalApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntCombatGoalApi)?&combat_goal_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntWeaponSearchApi=_SiN_GetServerNpcGruntWeaponSearchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntWeaponSearchApi* __cdecl
SiN_GetServerNpcGruntWeaponSearchApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntWeaponSearchApi)?&weapon_search_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntAuxConditionsApi=_SiN_GetServerNpcGruntAuxConditionsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntAuxConditionsApi* __cdecl
SiN_GetServerNpcGruntAuxConditionsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntAuxConditionsApi)?&aux_conditions_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntPostUpdateApi=_SiN_GetServerNpcGruntPostUpdateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPostUpdateApi* __cdecl
SiN_GetServerNpcGruntPostUpdateApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntPostUpdateApi)?&post_update_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntUpdateGateApi=_SiN_GetServerNpcGruntUpdateGateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntUpdateGateApi* __cdecl
SiN_GetServerNpcGruntUpdateGateApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntUpdateGateApi)?&update_gate_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntConditionUpdateApi=_SiN_GetServerNpcGruntConditionUpdateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntConditionUpdateApi* __cdecl
SiN_GetServerNpcGruntConditionUpdateApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntConditionUpdateApi)?&condition_update_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntGatherConditionsApi=_SiN_GetServerNpcGruntGatherConditionsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGatherConditionsApi* __cdecl
SiN_GetServerNpcGruntGatherConditionsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntGatherConditionsApi)?&gather_conditions_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntPointPolicyApi=_SiN_GetServerNpcGruntPointPolicyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPointPolicyApi* __cdecl
SiN_GetServerNpcGruntPointPolicyApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntPointPolicyApi)?&point_policy_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntGatherPointApi=_SiN_GetServerNpcGruntGatherPointApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGatherPointApi* __cdecl
SiN_GetServerNpcGruntGatherPointApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntGatherPointApi)?&gather_point_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntObjectSearchApi=_SiN_GetServerNpcGruntObjectSearchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntObjectSearchApi* __cdecl
SiN_GetServerNpcGruntObjectSearchApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntObjectSearchApi)?&object_search_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntGatherNativeApi=_SiN_GetServerNpcGruntGatherNativeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGatherNativeApi* __cdecl
SiN_GetServerNpcGruntGatherNativeApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntGatherNativeApi)?&gather_native_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntPrescheduleApi=_SiN_GetServerNpcGruntPrescheduleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPrescheduleApi* __cdecl SiN_GetServerNpcGruntPrescheduleApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntPrescheduleApi)?&preschedule_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSpeechQueueApi=_SiN_GetServerNpcGruntSpeechQueueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSpeechQueueApi* __cdecl SiN_GetServerNpcGruntSpeechQueueApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSpeechQueueApi)?&speech_queue_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSentenceApi=_SiN_GetServerNpcGruntSentenceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSentenceApi* __cdecl SiN_GetServerNpcGruntSentenceApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSentenceApi)?&sentence_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntNativeSpeechApi=_SiN_GetServerNpcGruntNativeSpeechApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntNativeSpeechApi* __cdecl SiN_GetServerNpcGruntNativeSpeechApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntNativeSpeechApi)?&native_speech_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSentenceRequestApi=_SiN_GetServerNpcGruntSentenceRequestApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSentenceRequestApi* __cdecl SiN_GetServerNpcGruntSentenceRequestApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSentenceRequestApi)?&sentence_request_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntRequestSpeechApi=_SiN_GetServerNpcGruntRequestSpeechApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntRequestSpeechApi* __cdecl SiN_GetServerNpcGruntRequestSpeechApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntRequestSpeechApi)?&request_speech_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntPerceptionSpeechApi=_SiN_GetServerNpcGruntPerceptionSpeechApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPerceptionSpeechApi* __cdecl SiN_GetServerNpcGruntPerceptionSpeechApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntPerceptionSpeechApi)?&perception_speech_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadLeaveApi=_SiN_GetServerNpcGruntSquadLeaveApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadLeaveApi* __cdecl SiN_GetServerNpcGruntSquadLeaveApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadLeaveApi)?&squad_leave_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadJoinApi=_SiN_GetServerNpcGruntSquadJoinApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadJoinApi* __cdecl SiN_GetServerNpcGruntSquadJoinApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadJoinApi)?&squad_join_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadRegistryApi=_SiN_GetServerNpcGruntSquadRegistryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadRegistryApi* __cdecl SiN_GetServerNpcGruntSquadRegistryApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadRegistryApi)?&squad_registry_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadDestroyApi=_SiN_GetServerNpcGruntSquadDestroyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadDestroyApi* __cdecl SiN_GetServerNpcGruntSquadDestroyApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadDestroyApi)?&squad_destroy_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadSlotsApi=_SiN_GetServerNpcGruntSquadSlotsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadSlotsApi* __cdecl SiN_GetServerNpcGruntSquadSlotsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadSlotsApi)?&squad_slots_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntPerceptionSlotsApi=_SiN_GetServerNpcGruntPerceptionSlotsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPerceptionSlotsApi* __cdecl SiN_GetServerNpcGruntPerceptionSlotsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntPerceptionSlotsApi)?&perception_slots_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadReportApi=_SiN_GetServerNpcGruntSquadReportApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadReportApi* __cdecl SiN_GetServerNpcGruntSquadReportApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadReportApi)?&squad_report_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadEnemyApi=_SiN_GetServerNpcGruntSquadEnemyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadEnemyApi* __cdecl SiN_GetServerNpcGruntSquadEnemyApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadEnemyApi)?&squad_enemy_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntNativeStateApi=_SiN_GetServerNpcGruntNativeStateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntNativeStateApi* __cdecl SiN_GetServerNpcGruntNativeStateApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntNativeStateApi)?&native_state_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadInputApi=_SiN_GetServerNpcGruntSquadInputApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadInputApi* __cdecl SiN_GetServerNpcGruntSquadInputApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadInputApi)?&squad_input_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntRemovalApi=_SiN_GetServerNpcGruntRemovalApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntRemovalApi* __cdecl SiN_GetServerNpcGruntRemovalApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntRemovalApi)?&removal_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntReactionSpeechApi=_SiN_GetServerNpcGruntReactionSpeechApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntReactionSpeechApi* __cdecl SiN_GetServerNpcGruntReactionSpeechApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntReactionSpeechApi)?&reaction_speech_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntEventSpeechApi=_SiN_GetServerNpcGruntEventSpeechApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntEventSpeechApi* __cdecl SiN_GetServerNpcGruntEventSpeechApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntEventSpeechApi)?&event_speech_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSquadInitApi=_SiN_GetServerNpcGruntSquadInitApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadInitApi* __cdecl SiN_GetServerNpcGruntSquadInitApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSquadInitApi)?&squad_init_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntBehaviorRemovalApi=_SiN_GetServerNpcGruntBehaviorRemovalApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntBehaviorRemovalApi* __cdecl SiN_GetServerNpcGruntBehaviorRemovalApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntBehaviorRemovalApi)?&behavior_removal_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDamagePainPrefixApi=_SiN_GetServerNpcGruntDamagePainPrefixApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDamagePainPrefixApi* __cdecl SiN_GetServerNpcGruntDamagePainPrefixApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDamagePainPrefixApi)?&damage_pain_prefix_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDeathPrefixApi=_SiN_GetServerNpcGruntDeathPrefixApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDeathPrefixApi* __cdecl SiN_GetServerNpcGruntDeathPrefixApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDeathPrefixApi)?&death_prefix_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDeathEventApi=_SiN_GetServerNpcGruntDeathEventApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDeathEventApi* __cdecl SiN_GetServerNpcGruntDeathEventApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDeathEventApi)?&death_event_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntBehaviorSwitchApi=_SiN_GetServerNpcGruntBehaviorSwitchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntBehaviorSwitchApi* __cdecl SiN_GetServerNpcGruntBehaviorSwitchApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntBehaviorSwitchApi)?&behavior_switch_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDeathPreparedEventApi=_SiN_GetServerNpcGruntDeathPreparedEventApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDeathPreparedEventApi* __cdecl SiN_GetServerNpcGruntDeathPreparedEventApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDeathPreparedEventApi)?&death_prepared_event_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntStateSignalApi=_SiN_GetServerNpcGruntStateSignalApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntStateSignalApi* __cdecl SiN_GetServerNpcGruntStateSignalApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntStateSignalApi)?&state_signal_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntCombatReactionsApi=_SiN_GetServerNpcGruntCombatReactionsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntCombatReactionsApi* __cdecl SiN_GetServerNpcGruntCombatReactionsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntCombatReactionsApi)?&combat_reactions_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDamageOutputsApi=_SiN_GetServerNpcGruntDamageOutputsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDamageOutputsApi* __cdecl SiN_GetServerNpcGruntDamageOutputsApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDamageOutputsApi)?&damage_outputs_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDamageEventApi=_SiN_GetServerNpcGruntDamageEventApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDamageEventApi* __cdecl SiN_GetServerNpcGruntDamageEventApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDamageEventApi)?&damage_event_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDamageOverrideApi=_SiN_GetServerNpcGruntDamageOverrideApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDamageOverrideApi* __cdecl SiN_GetServerNpcGruntDamageOverrideApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDamageOverrideApi)?&damage_override_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDamageNotificationApi=_SiN_GetServerNpcGruntDamageNotificationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDamageNotificationApi* __cdecl SiN_GetServerNpcGruntDamageNotificationApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDamageNotificationApi)?&damage_notification_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDamageDetailApi=_SiN_GetServerNpcGruntDamageDetailApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDamageDetailApi* __cdecl SiN_GetServerNpcGruntDamageDetailApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDamageDetailApi)?&damage_detail_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDamageNativeChainApi=_SiN_GetServerNpcGruntDamageNativeChainApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDamageNativeChainApi* __cdecl SiN_GetServerNpcGruntDamageNativeChainApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDamageNativeChainApi)?&damage_native_chain_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntDeathSelectorApi=_SiN_GetServerNpcGruntDeathSelectorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntDeathSelectorApi* __cdecl SiN_GetServerNpcGruntDeathSelectorApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntDeathSelectorApi)?&death_selector_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntLifeDamageApi=_SiN_GetServerNpcGruntLifeDamageApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntLifeDamageApi* __cdecl SiN_GetServerNpcGruntLifeDamageApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntLifeDamageApi)?&life_damage_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntAliveDamageApi=_SiN_GetServerNpcGruntAliveDamageApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntAliveDamageApi* __cdecl SiN_GetServerNpcGruntAliveDamageApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntAliveDamageApi)?&alive_damage_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntIgnitionApi=_SiN_GetServerNpcGruntIgnitionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntIgnitionApi* __cdecl SiN_GetServerNpcGruntIgnitionApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntIgnitionApi)?&ignition_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntLifeDispatchApi=_SiN_GetServerNpcGruntLifeDispatchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntLifeDispatchApi* __cdecl SiN_GetServerNpcGruntLifeDispatchApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntLifeDispatchApi)?&life_dispatch_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcJetpackDeathModeApi=_SiN_GetServerNpcJetpackDeathModeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcJetpackDeathModeApi* __cdecl SiN_GetServerNpcJetpackDeathModeApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcJetpackDeathModeApi)?&jetpack_death_mode_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntGroundDeathApi=_SiN_GetServerNpcGruntGroundDeathApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGroundDeathApi* __cdecl SiN_GetServerNpcGruntGroundDeathApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntGroundDeathApi)?&ground_death_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntGroundLifeApi=_SiN_GetServerNpcGruntGroundLifeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGroundLifeApi* __cdecl SiN_GetServerNpcGruntGroundLifeApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntGroundLifeApi)?&ground_life_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcGruntSpecialImpactApi=_SiN_GetServerNpcGruntSpecialImpactApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSpecialImpactApi* __cdecl SiN_GetServerNpcGruntSpecialImpactApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcGruntSpecialImpactApi)?&special_impact_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcJetpackDeathBranchesApi=_SiN_GetServerNpcJetpackDeathBranchesApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcJetpackDeathBranchesApi* __cdecl SiN_GetServerNpcJetpackDeathBranchesApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcJetpackDeathBranchesApi)?&jetpack_death_branches_api:nullptr;
}
