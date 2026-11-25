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
int best_enemy_impl(void* actor,unsigned bytes,float now,const ServerNpcBestEnemyBindings* b,ServerNpcBestEnemyResult* out,const ServerNpcGruntRelationBindings* rel,const ServerNpcUnreachableEnemyBindings* cache,ServerNpcGruntBestEnemyResult* stats) noexcept{
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
        if(!b->sensed||!b->sensed(b->context,read<unsigned>(actor,0x898),entity,&value))return false;
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
int __cdecl grunt_best_enemy(void* actor,unsigned bytes,float now,const ServerNpcBestEnemyBindings* b,const ServerNpcGruntRelationBindings* rel,const ServerNpcUnreachableEnemyBindings* cache,ServerNpcGruntBestEnemyResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!b||!rel||!cache||bytes<0xdb2||!relation_grunt_class(rel->original_vtable))return 0;
    auto rb=*rel;auto cb=*cache;
    rb.entries=cb.entries=b->memory.entries;rb.count=cb.count=b->memory.count;
    return best_enemy_impl(actor,bytes,now,b,&out->selection,&rb,&cb,out);
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
int __cdecl grunt_enemy_switch(void* actor,unsigned bytes,float now,const ServerNpcGruntEnemySwitchBindings* b,ServerNpcGruntEnemySwitchResult* out) noexcept{
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
        if(!grunt_best_enemy(actor,bytes,now,&b->best,&gate.relation,&b->cache,&out->selection))return 0;
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
