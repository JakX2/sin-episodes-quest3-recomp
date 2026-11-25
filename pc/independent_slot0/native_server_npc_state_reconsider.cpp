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
int __cdecl clear_combat_conditions(void*,unsigned,const ServerNpcConditionMapBindings*,ServerNpcEnemyConditionResult*) noexcept;
int set_enemy_impl(void* a,unsigned bytes,void* target,bool notify,float now,const ServerNpcEnemyStateBindings* b,ServerNpcEnemyStateResult* out,bool native_notify) noexcept {
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
        int rank=0,dist=0;
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
                out->visible=yes;
                if(!priority(entity,rank))return 0;
            }
            out->unreachable=unreachable;
        }
        out->enemy=entity;out->priority=rank;out->distance_squared=dist;++out->replaced;
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
int __cdecl grunt_memory_report(void* actor,unsigned bytes,void* target,const float position[3],void* informer,float now,const ServerNpcGruntMemoryReportBindings* b,ServerNpcGruntMemoryReportResult* out) noexcept{
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
        if(!b->cue||!b->cue(b->context,actor,"GRUNT_REFIND_ENEMY",2,2))return 0;
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
        if(!member||!b->forward||!b->forward(b->context,member,target,position,actor))return 0;
        ++out->forwarded;
    }
    return 1;
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
int grunt_enemy_switch_impl(void* actor,unsigned bytes,float now,const ServerNpcGruntEnemySwitchBindings* b,ServerNpcGruntEnemySwitchResult* out,const ServerNpcSensedEntitiesBindings* senses) noexcept{
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
    if(!set_enemy_impl(actor,bytes,candidate,true,now,&enemy,&out->assignment,true))return 0;
    out->assigned=1;out->has_enemy=candidate!=nullptr;
    if(had_flags){
        const unsigned squad=read<unsigned>(actor,0x9c4);
        if(squad){
            if(!enemy.release_squad_slot||!enemy.release_squad_slot(enemy.context,squad,registry.Resolve(read<unsigned>(actor,0x8a0)),read<int>(actor,0x9cc)))return 0;
            write(actor,0x9cc,-1);out->extra_squad_release=1;
        }
        write(actor,0x988,read<unsigned>(actor,0x988)&~0x20000u);
    }
    if(!candidate){
        if(out->eluded){
            if(!set_condition(0xb,out->lost_condition)||!b->lost_cue||!b->lost_cue(b->context,actor))return 0;
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
int __cdecl grunt_speak(void* actor,unsigned bytes,unsigned kind,float now,const ServerNpcGruntSpeechBindings* b,ServerNpcGruntSpeechResult* out) noexcept{
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
    if(!b->emit||!b->emit(b->context,actor,0xcc0,out->cue,priority,2,&out->speech_result))return 0;
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
int __cdecl grunt_visual_gather(void* actor,unsigned bytes,float now,const ServerNpcGruntVisualBindings* b,ServerNpcGruntVisualResult* out) noexcept{
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
                if(!grunt_memory_report(actor,bytes,target,position,nullptr,now,&report,&mr))return 0;
                ++out->report_calls;out->records_created+=mr.mutation.created;
                if(feared&&!store(8,true))return 0;
            }else if(rr.disposition==3)++out->friendly;else ++out->unknown;
        }
        if(!sensed_next(senses,n,&b->senses,&cursor,&row))return 0;
    }
    return 1;
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
int __cdecl perception_tick(void* actor,unsigned bytes,float now,const ServerNpcGruntPerceptionTickBindings* b,ServerNpcGruntPerceptionTickResult* out) noexcept{
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
    if(!grunt_visual_gather(actor,bytes,now,&visual,&out->visual))return 0;
    auto sound=b->sound;sound.entries=best.memory.entries;sound.count=best.memory.count;
    sound.hearing.storage=visual.senses.storage;sound.hearing.original_vtable=vt;sound.hearing.schedule=b->enemy_switch.schedule;
    if(!grunt_sound_scan(actor,bytes,now,&sound,&out->sound))return 0;
    auto maintenance=b->maintenance;maintenance.entries=best.memory.entries;maintenance.count=best.memory.count;
    void* memory=nullptr;unsigned n=0;
    if(!memory_storage(&maintenance.storage,read<unsigned>(actor,0x984),0x40,memory,n)||!maintain_memory(memory,n,now,&maintenance,&out->maintenance))return 0;
    if(!grunt_enemy_switch_impl(actor,bytes,now,&b->enemy_switch,&out->enemy_switch,&visual.senses))return 0;
    out->completed=1;return 1;
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
int __cdecl combat_range_gather(void* actor,unsigned bytes,void* target,float now,const ServerNpcGruntCombatRangeBindings* b,ServerNpcGruntCombatRangeResult* out) noexcept{
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
        if(!grunt_memory_report(actor,bytes,target,out->observation,informer,now,&rb,&out->report))return false;
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
int combat_conditions_impl(void* actor,unsigned bytes,void* target,float now,const ServerNpcGruntCombatConditionsBindings* b,ServerNpcGruntCombatConditionsResult* out,const ServerNpcGruntGoalRefreshBindings* goals,ServerNpcGruntGoalRefreshResult* goal_results) noexcept{
    if(!out)return 0;
    out->range={};out->attack={};out->cache={};
    out->completed=out->attack_called=out->cleanup_called=out->nav_calls=out->unreachable_set=out->old_sight=out->close_mark=out->point_mark=out->point_queries=0;
    out->last_seen=out->horizontal_distance=0;for(float& v:out->remembered)v=0;
    if(!actor||bytes<0xa24||!target||!b||!std::isfinite(now))return 0;
    if(!combat_range_gather(actor,bytes,target,now,&b->range,&out->range))return 0;
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
int __cdecl grunt_post_update(void* actor,unsigned bytes,float now,bool perceived,const ServerNpcGruntPostUpdateBindings* b,ServerNpcGruntPostUpdateResult* out) noexcept{
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
    if(state==0||state==7){
        out->skipped_state=1;if(!store(1,false,cr))return 0;
    }else{
        if(perceived){
            auto search=b->search;search.relation.entries=mb.entries;search.relation.count=mb.count;search.relation.original_vtable=aux.visibility.original_vtable;search.schedule=b->schedule;
            if(!grunt_weapon_search(actor,bytes,now,&search,&out->search))return 0;
            if(read<unsigned>(actor,0x798)&&(state==1||state==2)&&current()){
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
                if(!grunt_combat_goal(actor,bytes,e,now,&combat,&aux.goals,&out->combat,out->combat_goals))return 0;
                write(actor,0x9a0,now);out->combat_called=1;
            }else{
                auto eb=b->enemy;eb.entries=mb.entries;eb.count=mb.count;eb.conditions=b->schedule.conditions;
                if(!set_enemy_impl(actor,bytes,nullptr,true,now,&eb,&out->cleared_enemy,true))return 0;out->enemy_cleared=1;
            }
        }
        if(!grunt_target_conditions(actor,bytes,now,&aux,&out->target)||!grunt_ammo_conditions(actor,bytes,&aux,&out->ammo))return 0;
        auto flinch=b->flinch;flinch.schedule=b->schedule;
        if(!grunt_flinch(actor,bytes,now,&flinch,&out->flinch))return 0;
    }
    if(!grunt_snapshot_conditions(actor,bytes,&aux,&out->snapshot))return 0;
    out->completed=1;return 1;
}
int __cdecl grunt_update_gate(void* actor,unsigned bytes,const ServerNpcGruntUpdateGateBindings* b,ServerNpcGruntUpdateGateResult* out) noexcept{
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
            if(!b->idle_speech||!b->idle_speech(b->context,actor,0xcc0,"GRUNT_IDLE",0,2,&out->speech_result))return 0;
            out->idle_emitted=1;
        }
    }
    return 1;
}
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
