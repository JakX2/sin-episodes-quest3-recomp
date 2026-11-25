#include "native_server_npc_state_reconsider.h"
#include <cstring>
#include <cmath>
#include <limits>
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
int __cdecl best_enemy(void* actor,unsigned bytes,float now,const ServerNpcBestEnemyBindings* b,ServerNpcBestEnemyResult* out) noexcept{
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
    auto priority=[&](void* entity,int& value){return b->priority&&b->priority(b->context,actor,entity,&value);};
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
        int disp=0;if(!b->disposition||!b->disposition(b->context,actor,entity,&disp))return 0;
        if(disp!=1){if(!b->disposition(b->context,actor,entity,&disp))return 0;if(disp!=2){++out->disposition_rejected;continue;}}
        bool yes=false;if(!query_bool(b->valid,entity,yes))return 0;if(!yes){++out->invalid;continue;}
        const float cutoff=read<float>(actor,0x8c4),stamp=read<float>(record,0x1c),valid_after=read<float>(record,0x28);
        if(!std::isfinite(cutoff)||!std::isfinite(stamp)||!std::isfinite(valid_after))return 0;
        if(cutoff>0&&stamp<cutoff){++out->old;continue;}
        if(valid_after>now){++out->not_yet_valid;continue;}
        unsigned key=0;if(!memory_key(b->memory,entity,key))return 0;
        ServerNpcEnemyMemoryLookup lookup{};
        if(!memory_lookup(memory,memory_bytes,key,false,&b->memory.storage,&lookup))return 0;
        if(lookup.record){void* rec=nullptr;if(!memory_record(&b->memory.storage,lookup,0x36,rec))return 0;
            if(read<std::uint8_t>(rec,0x35)){++out->eluded;continue;}}
        if(!b->disposition(b->context,actor,entity,&disp))return 0;
        if(disp==2&&!read<std::uint8_t>(record,0x36)){
            void* current=registry.Resolve(read<unsigned>(actor,0x8a0));unsigned current_key=0;float last=0;
            if(!memory_key(b->memory,current,current_key)||!memory_timestamp(memory,memory_bytes,current_key,NpcMemoryStamp20,&b->memory.storage,&lookup,&last))return 0;
            if(last==-std::numeric_limits<float>::max()){++out->feared;continue;}
        }
        bool unreachable=false;if(!query_bool(b->unreachable,entity,unreachable))return 0;
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
const ServerNpcUnreachableEnemyApi unreachable_api{1,sizeof(ServerNpcUnreachableEnemyApi),&unreachable_enemy};
const ServerNpcBestEnemyApi best_enemy_api{1,sizeof(ServerNpcBestEnemyApi),&best_enemy};
const ServerNpcEnemyMemoryQueryApi query_api{1,sizeof(ServerNpcEnemyMemoryQueryApi),&query_last_seen,&query_aim_vector};
const ServerNpcEnemyMemoryApi memory_api{1,sizeof(ServerNpcEnemyMemoryApi),&memory_lookup,&memory_timestamp,&memory_position,&memory_touch,&memory_recent};
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
