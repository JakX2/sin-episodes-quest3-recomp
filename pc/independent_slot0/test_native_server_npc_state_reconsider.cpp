#include "native_server_npc_state_reconsider.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <limits>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xdb2]{},desc[0x48]{},schedule[0x30]{},enemy[0x86c]{},replacement[0x86c]{};
    unsigned char memory[0x40]{},nodes[384]{},record[0x38]{},fallback[0x38]{},unreachable_rows[60]{};
    const ServerNpcUnreachableEnemyApi* unreachable_api=nullptr;ServerNpcUnreachableEnemyBindings unreachable_binding{};
    const ServerNpcEnemyMemoryMutationApi* mutation_api=nullptr;ServerNpcEnemyMemoryMutationBindings mutation{};
    bool allocated[2]{};unsigned releases=0;
    unsigned char relation_rows[48]{},relation_matrix[240]{};bool rel_player=false,rel_special=false,rel_npc=false;
    sin_independent::EntityHandleEntry entries[3]{};
    unsigned clears=0,changes=0,lost=0,squad=0,removed_index=255,removed_token=0;int query=-1;
    int rank[2]{0,0},disp[2]{1,1};bool valid[2]{true,true},unreachable[2]{false,false},visible[2]{true,true};float distance[2]{10,20};
    unsigned idx(void* e){assert(e==enemy||e==replacement);return e==enemy?0u:1u;}
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    void condition(int id){unsigned off=0x7cc+4*(id/32);put(actor,off,get<unsigned>(off)|(1u<<(id%32)));}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcStateApi>(GetProcAddress(s,server_npc_state_reconsider_export));
    assert(g&&!GetProcAddress(c,server_npc_state_reconsider_export)&&!g(0,sizeof(ServerNpcStateApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcStateApi));assert(api);
    F f;f.entries[1].serial=7;f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.actor,0x798,1u);f.put(f.actor,0x850,3);f.put(f.actor,0x858,3);
    f.put(f.actor,0x8a0,0x7001u);
    ServerNpcStateBindings b{};b.context=&f;b.entities={f.entries,2,nullptr,nullptr,nullptr};
    b.schedule.context=&f;b.schedule.conditions.context=&f;
    b.schedule.storage=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(token!=1)return 0;*out=f.schedule;*n=sizeof(f.schedule);return 1;};
    b.schedule.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.clear_enemy=[](void* p,void*) noexcept -> int{auto& f=F::self(p);++f.clears;f.put(f.actor,0x8a0,0xffffffffu);return 1;};
    b.state_changed=[](void* p,void*,int old,int next) noexcept -> int{auto& f=F::self(p);assert(old!=next&&f.get<int>(0x850)==next);++f.changes;return 1;};
    b.lost_enemy=[](void* p,void*,void* e) noexcept -> int{auto& f=F::self(p);assert(e==f.enemy);++f.lost;return 1;};
    b.squad_enemy=[](void* p,std::uint32_t s,void* e) noexcept -> int{auto& f=F::self(p);assert(s==1&&e==f.enemy);++f.squad;return 1;};
    b.state_query=[](void* p,void*,int,int* out) noexcept -> int{*out=F::self(p).query;return 1;};
    ServerNpcStateResult out{};auto gate=[&](bool yes){assert(api->should_reconsider(f.actor,sizeof(f.actor),&b,&out)&&out.reconsider==yes);};
    gate(true);f.condition(0x22);gate(false);f.put(f.schedule,0x10,4u);gate(true);f.put(f.schedule,0x10,0u);
    f.put(f.actor,0x858,7);gate(false);f.put(f.actor,0x858,4);gate(false);f.put(f.actor,0x858,3);
    f.put(f.actor,0x8a0,0x8001u);gate(true);
    assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.ideal==2&&out.patrol_set&&(f.get<unsigned>(0x7d4)&16u));
    f.condition(0x1d);assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.ideal==2&&!out.patrol_set);
    f.put(f.actor,0x8a0,0x7001u);assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.lost_enemy_called&&f.lost==1);
    assert(api->apply(f.actor,sizeof(f.actor),1,12.f,&b,&out)&&out.changed&&out.enemy_cleared&&f.clears==1&&f.changes==1&&f.get<float>(0x854)==12.f);
    assert(api->apply(f.actor,sizeof(f.actor),1,13.f,&b,&out)&&!out.changed&&f.get<float>(0x854)==12.f);
    f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x9c4,1u);f.condition(0x19);f.query=3;
    assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.ideal==3&&out.squad_called&&f.squad==1);
    gate(true);
    auto ge=reinterpret_cast<GetServerNpcEnemyStateApi>(GetProcAddress(s,server_npc_enemy_state_export));
    assert(ge&&!GetProcAddress(c,server_npc_enemy_state_export)&&!ge(0,sizeof(ServerNpcEnemyStateApi))&&!ge(1,0));
    const auto* es=ge(1,sizeof(ServerNpcEnemyStateApi));assert(es);
    ServerNpcEnemyStateBindings eb{};eb.context=&f;eb.entries=f.entries;eb.count=3;eb.conditions=b.schedule.conditions;
    f.entries[2].serial=8;f.entries[2].entity=reinterpret_cast<CBaseEntity*>(f.replacement);
    f.put(f.actor,0x9cc,5);f.put(f.actor,0x8b0,std::uint8_t(1));
    eb.enemy_changing=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);++f.clears;return 1;};
    eb.release_squad_slot=[](void* p,std::uint32_t token,void* old,int slot) noexcept -> int{auto& f=F::self(p);assert(token==1&&old==f.enemy&&slot==5&&f.get<unsigned>(0x8a0)==0x7001u);++f.squad;return 1;};
    eb.entity_handle=[](void* p,void* e,std::uint32_t* out) noexcept -> int{auto& f=F::self(p);if(e==f.enemy)*out=0x7001u;else if(e==f.replacement)*out=0x8002u;else return 0;return 1;};
    ServerNpcEnemyStateResult er{};
    f.put(f.actor,0x7cc,0u);
    assert(es->set(f.actor,sizeof(f.actor),f.replacement,true,20,&eb,&er)&&er.changed&&er.notified&&er.squad_released&&er.new_condition_set&&er.reset_fields);
    assert(er.old_handle==0x7001u&&er.new_handle==0x8002u&&f.get<unsigned>(0x8a0)==0x8002u&&f.get<int>(0x9cc)==-1&&f.get<std::uint8_t>(0x8b0)==0&&f.get<float>(0x8a4)==20.f);
    assert(f.get<float>(0xbf8)==-1.f&&f.get<unsigned>(0xbfc)==0&&f.get<unsigned>(0xc00)==0&&(f.get<unsigned>(0x7cc)&(1u<<25)));
    f.put(f.actor,0x7cc,0u);f.put(f.actor,0x8b0,std::uint8_t(7));f.put(f.actor,0xbf8,7.f);
    assert(es->set(f.actor,sizeof(f.actor),f.replacement,true,21,&eb,&er)&&!er.changed&&!er.notified&&!er.new_condition_set&&f.get<std::uint8_t>(0x8b0)==7&&f.get<float>(0x8a4)==21.f&&f.get<float>(0xbf8)==-1.f);
    f.put(f.actor,0x9c4,0u);
    assert(es->set(f.actor,sizeof(f.actor),f.enemy,false,22,&eb,&er)&&er.changed&&!er.new_condition_set&&!er.squad_released&&!(f.get<unsigned>(0x7cc)&(1u<<25)));
    f.condition(0x19);assert(es->set(f.actor,sizeof(f.actor),nullptr,true,23,&eb,&er)&&er.changed&&er.new_condition_cleared&&f.get<unsigned>(0x8a0)==0xffffffffu&&!(f.get<unsigned>(0x7cc)&(1u<<25)));
    f.put(f.actor,0x8a0,0x9002u);f.condition(0x19);
    assert(es->set(f.actor,sizeof(f.actor),nullptr,true,24,&eb,&er)&&!er.changed&&er.new_condition_cleared&&f.get<unsigned>(0x8a0)==0xffffffffu&&f.get<float>(0x8a4)==24.f);
    assert(!es->set(f.actor,0xc03,nullptr,true,25,&eb,&er));
    ServerNpcEnemyConditionResult ecr{};
    for(int id: {0x14,0x15,0x16,0x17,0x27,0x28,0x29,0x2a,0x2b,0x19,0x2c})f.condition(id);
    assert(es->clear_combat_conditions(f.actor,sizeof(f.actor),&eb.conditions,&ecr)&&ecr.prior_preserved&&ecr.cleared==9&&ecr.restored);
    assert((f.get<unsigned>(0x7cc)&0x00f00000u)==(1u<<21)&&(f.get<unsigned>(0x7cc)&(1u<<25))&&(f.get<unsigned>(0x7d0)&0xf80u)==0&&(f.get<unsigned>(0x7d0)&(1u<<12)));
    f.put(f.actor,0x7cc,0u);f.condition(0x14);
    assert(es->clear_combat_conditions(f.actor,sizeof(f.actor),&eb.conditions,&ecr)&&!ecr.prior_preserved&&ecr.cleared==1&&!ecr.restored);
    auto gm=reinterpret_cast<GetServerNpcEnemyMemoryApi>(GetProcAddress(s,server_npc_enemy_memory_export));
    assert(gm&&!GetProcAddress(c,server_npc_enemy_memory_export)&&!gm(0,sizeof(ServerNpcEnemyMemoryApi))&&!gm(1,0));
    const auto* mem=gm(1,sizeof(ServerNpcEnemyMemoryApi));assert(mem);
    ServerNpcEnemyMemoryBindings mb{};mb.context=&f;
    mb.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);switch(token){case 100:*out=f.nodes;*n=sizeof(f.nodes);return 1;case 101:*out=f.record;*n=sizeof(f.record);return 1;case 102:*out=f.fallback;*n=sizeof(f.fallback);return 1;default:return 0;}
    };
    f.put(f.memory,0,0x22068d70u);f.put(f.memory,4,100u);f.put(f.memory,8,4u);f.put(f.memory,0x18,0x22068b40u);f.put(f.memory,0x1c,5.f);
    f.memory[0x10]=0;
    auto node=[&](unsigned i,unsigned left,unsigned right,std::uint32_t key,std::uint32_t record){
        f.nodes[12*i]=static_cast<unsigned char>(left);f.nodes[12*i+1]=static_cast<unsigned char>(right);
        f.put(f.nodes,12*i+4,key);f.put(f.nodes,12*i+8,record);
    };
    node(0,1,2,0x80000000u,101);node(1,255,255,0,102);node(2,255,255,0xf0000000u,0);
    f.put(f.record,4,1.f);f.put(f.record,8,2.f);f.put(f.record,12,3.f);
    f.put(f.record,0x10,4.f);f.put(f.record,0x14,5.f);f.put(f.record,0x18,6.f);
    for(unsigned i=0;i<5;++i){const unsigned offsets[]={0x1c,0x20,0x24,0x2c,0x30};f.put(f.record,offsets[i],10.f+i);}
    f.put(f.fallback,0x1c,3.f);
    ServerNpcEnemyMemoryLookup ml{};float stamp=0,v[3]{};
    assert(mem->lookup(f.memory,sizeof(f.memory),0x80000000u,true,&mb,&ml)&&ml.record==101&&ml.index==0&&ml.visited==1&&!ml.fallback_attempted);
    assert(mem->lookup(f.memory,sizeof(f.memory),0x394u,false,&mb,&ml)&&ml.key==0&&ml.record==102&&ml.index==1&&!ml.fallback_attempted);
    assert(mem->lookup(f.memory,sizeof(f.memory),0x90000000u,true,&mb,&ml)&&ml.fallback_attempted&&ml.fallback_used&&ml.record==102&&ml.visited==4);
    assert(mem->lookup(f.memory,sizeof(f.memory),0xf0000000u,true,&mb,&ml)&&ml.matched&&!ml.record&&!ml.fallback_attempted);
    assert(mem->lookup(f.memory,sizeof(f.memory),0x90000000u,false,&mb,&ml)&&!ml.matched&&!ml.fallback_attempted&&ml.index==255);
    for(unsigned i=0;i<5;++i)assert(mem->timestamp(f.memory,sizeof(f.memory),0x80000000u,i,&mb,&ml,&stamp)&&stamp==10.f+i);
    assert(mem->timestamp(f.memory,sizeof(f.memory),0,NpcMemoryStamp1c,&mb,&ml,&stamp)&&stamp==0&&!ml.visited);
    assert(mem->timestamp(f.memory,sizeof(f.memory),0x394,NpcMemoryStamp1c,&mb,&ml,&stamp)&&stamp==3);
    assert(mem->timestamp(f.memory,sizeof(f.memory),0xf0000000u,NpcMemoryStamp1c,&mb,&ml,&stamp)&&stamp==-std::numeric_limits<float>::max());
    assert(mem->position(f.memory,sizeof(f.memory),0x80000000u,false,&mb,&ml,v)&&v[0]==1&&v[1]==2&&v[2]==3);
    assert(mem->position(f.memory,sizeof(f.memory),0x80000000u,true,&mb,&ml,v)&&v[0]==4&&v[1]==5&&v[2]==6);
    assert(mem->position(f.memory,sizeof(f.memory),0xf0000000u,false,&mb,&ml,v)&&v[0]==1&&v[2]==3);
    assert(mem->touch(f.memory,sizeof(f.memory),0x90000000u,19,&mb,&ml)&&ml.fallback_used);
    assert(mem->timestamp(f.memory,sizeof(f.memory),0x394,NpcMemoryStamp2c,&mb,&ml,&stamp)&&stamp==19);
    bool recent=false;
    assert(mem->recent(f.memory,sizeof(f.memory),0x80000000u,14.99f,&mb,&ml,&recent)&&recent);
    assert(mem->recent(f.memory,sizeof(f.memory),0x80000000u,15,&mb,&ml,&recent)&&!recent);
    assert(mem->recent(f.memory,sizeof(f.memory),0xf0000000u,15,&mb,&ml,&recent)&&recent);
    assert(mem->recent(f.memory,sizeof(f.memory),0,15,&mb,&ml,&recent)&&!recent);
    f.nodes[0]=0;assert(!mem->lookup(f.memory,sizeof(f.memory),1,true,&mb,&ml));f.nodes[0]=1;
    f.nodes[1]=4;assert(!mem->lookup(f.memory,sizeof(f.memory),0x90000000u,true,&mb,&ml));f.nodes[1]=2;
    f.memory[0x10]=255;assert(mem->lookup(f.memory,sizeof(f.memory),1,true,&mb,&ml)&&!ml.record&&ml.fallback_attempted&&!ml.visited);
    f.put(f.memory,0x18,0u);assert(!mem->lookup(f.memory,sizeof(f.memory),1,true,&mb,&ml));
    auto gb=reinterpret_cast<GetServerNpcBestEnemyApi>(GetProcAddress(s,server_npc_best_enemy_export));
    assert(gb&&!GetProcAddress(c,server_npc_best_enemy_export)&&!gb(0,sizeof(ServerNpcBestEnemyApi))&&!gb(1,0));
    const auto* best=gb(1,sizeof(ServerNpcBestEnemyApi));assert(best);
    ServerNpcBestEnemyBindings bb{};bb.context=&f;bb.memory.context=&f;bb.memory.entries=f.entries;bb.memory.count=3;
    bb.memory.storage=mb;bb.memory.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);switch(token){case 100:*out=f.nodes;*n=sizeof(f.nodes);return 1;case 101:*out=f.record;*n=sizeof(f.record);return 1;case 102:*out=f.fallback;*n=sizeof(f.fallback);return 1;case 103:*out=f.memory;*n=sizeof(f.memory);return 1;default:return 0;}
    };
    bb.memory.entity_key=[](void* p,void* e,std::uint32_t* key) noexcept -> int{*key=F::self(p).idx(e)+1000;return 1;};
    bb.extent=[](void*,void*,unsigned* n) noexcept -> int{*n=0xc0;return 1;};
    bb.disposition=[](void* p,void*,void* e,int* v) noexcept -> int{auto& f=F::self(p);*v=f.disp[f.idx(e)];return 1;};
    bb.valid=[](void* p,void*,void* e,bool* v) noexcept -> int{auto& f=F::self(p);*v=f.valid[f.idx(e)];return 1;};
    bb.unreachable=[](void* p,void*,void* e,bool* v) noexcept -> int{auto& f=F::self(p);*v=f.unreachable[f.idx(e)];return 1;};
    bb.priority=[](void* p,void*,void* e,int* v) noexcept -> int{auto& f=F::self(p);*v=f.rank[f.idx(e)];return 1;};
    bb.origin=[](void* p,void* e,float* v) noexcept -> int{auto& f=F::self(p);v[0]=e==f.actor?0:f.distance[f.idx(e)];v[1]=v[2]=0;return 1;};
    bb.sensed=[](void*,std::uint32_t,void*,bool* v) noexcept -> int{*v=false;return 1;};
    bb.visible=[](void* p,void*,void* e,unsigned mask,bool* v) noexcept -> int{auto& f=F::self(p);assert(mask==0x4081);*v=f.visible[f.idx(e)];return 1;};
    bb.component_best=[](void* p,std::uint32_t token,void** v) noexcept -> int{assert(token==77);*v=F::self(p).replacement;return 1;};
    f.put(f.memory,0x18,0x22068b40u);f.memory[0x10]=0;node(0,255,1,1000,101);node(1,255,255,1001,102);
    std::memset(f.record,0,sizeof(f.record));std::memset(f.fallback,0,sizeof(f.fallback));
    f.put(f.record,0,0x7001u);f.put(f.fallback,0,0x8002u);f.put(f.actor,0x984,103u);f.put(f.actor,0x8c4,0.f);f.put(f.actor,0xc18,0u);
    ServerNpcBestEnemyResult br{};auto pick=[&](){assert(best->select(f.actor,sizeof(f.actor),40,&bb,&br));};
    pick();assert(br.enemy==f.enemy&&br.visited==2&&br.distance_squared==100&&br.visible);
    f.rank[1]=5;pick();assert(br.enemy==f.replacement&&br.priority==5&&br.distance_squared==400&&br.replaced==2);
    f.unreachable[1]=true;pick();assert(br.enemy==f.enemy&&br.unreachable_rejected==1);
    f.unreachable[0]=true;f.unreachable[1]=false;f.rank[0]=9;pick();assert(br.enemy==f.replacement&&!br.unreachable);
    f.unreachable[0]=false;f.rank[0]=f.rank[1]=0;f.visible[0]=false;pick();assert(br.enemy==f.replacement&&br.visible);
    f.visible[0]=true;f.visible[1]=false;f.distance[1]=5;pick();assert(br.enemy==f.enemy);
    f.visible[1]=true;pick();assert(br.enemy==f.replacement&&br.distance_squared==25);
    f.put(f.actor,0x8c4,10.f);f.put(f.record,0x1c,9.f);f.put(f.fallback,0x1c,10.f);pick();assert(br.enemy==f.replacement&&br.old==1);
    f.put(f.fallback,0x28,41.f);pick();assert(!br.enemy&&br.not_yet_valid==1);
    f.put(f.fallback,0x28,40.f);f.fallback[0x35]=1;pick();assert(!br.enemy&&br.eluded==1);f.fallback[0x35]=0;
    f.put(f.fallback,0,0x9002u);pick();assert(!br.enemy&&br.stale==1);f.put(f.fallback,0,0x8002u);
    f.put(f.replacement,0x8c,1);pick();assert(!br.enemy&&br.dead==1);f.put(f.replacement,0x8c,0);
    f.valid[1]=false;pick();assert(!br.enemy&&br.invalid==1);f.valid[1]=true;
    f.disp[1]=3;pick();assert(!br.enemy&&br.disposition_rejected==1);f.disp[1]=1;
    f.put(f.actor,0xc18,77u);pick();assert(br.component_called&&br.enemy==f.replacement&&!br.visited);f.put(f.actor,0xc18,0u);
    f.nodes[13]=0;assert(!best->select(f.actor,sizeof(f.actor),40,&bb,&br));f.nodes[13]=255;
    auto gu=reinterpret_cast<GetServerNpcUnreachableEnemyApi>(GetProcAddress(s,server_npc_unreachable_enemy_export));
    assert(gu&&!GetProcAddress(c,server_npc_unreachable_enemy_export)&&!gu(0,sizeof(ServerNpcUnreachableEnemyApi))&&!gu(1,0));
    f.unreachable_api=gu(1,sizeof(ServerNpcUnreachableEnemyApi));assert(f.unreachable_api);
    auto& ub=f.unreachable_binding;ub.context=&f;ub.entries=f.entries;ub.count=3;ub.origin=bb.origin;
    ub.storage.context=&f;ub.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(token!=104)return 0;*out=f.unreachable_rows;*n=sizeof(f.unreachable_rows);return 1;};
    auto unreachable_row=[&](unsigned i,unsigned handle,float expiry,float x){
        f.put(f.unreachable_rows+20*i,0,handle);f.put(f.unreachable_rows+20*i,4,expiry);f.put(f.unreachable_rows+20*i,8,x);
        f.put(f.unreachable_rows+20*i,12,0.f);f.put(f.unreachable_rows+20*i,16,0.f);
    };
    f.put(f.actor,0x914,104u);f.put(f.actor,0x920,3);
    f.distance[0]=120;f.distance[1]=5;
    unreachable_row(0,0x7001u,40,0);unreachable_row(1,0x9002u,99,0);unreachable_row(2,0x8002u,0,0);
    ServerNpcUnreachableEnemyResult ur{};
    auto query_unreachable=[&](void* e,float now){assert(f.unreachable_api->query(f.actor,sizeof(f.actor),e,now,&ub,&ur));};
    query_unreachable(f.enemy,40);assert(ur.unreachable&&ur.stale_removed==1&&ur.visited==3&&ur.final_count==2&&ur.distance_squared==14400);
    unsigned moved=0;std::memcpy(&moved,f.unreachable_rows+20,4);assert(moved==0x8002u);
    query_unreachable(f.enemy,40.01f);assert(!ur.unreachable&&ur.expired_removed==1&&!ur.origin_called&&ur.final_count==1);
    query_unreachable(f.replacement,40);assert(ur.expired_removed==1&&ur.final_count==0);
    f.put(f.actor,0x920,1);unreachable_row(0,0x7001u,50,0);f.distance[0]=120.01f;
    query_unreachable(f.enemy,40);assert(!ur.unreachable&&ur.moved_removed==1&&ur.final_count==0);
    // Actual native cache supplies the previously explicit BestEnemy predicate.
    f.put(f.actor,0x920,1);unreachable_row(0,0x7001u,50,120);f.distance[0]=120;
    f.put(f.actor,0x8c4,0.f);f.rank[0]=99;f.rank[1]=0;
    bb.unreachable=[](void* p,void* actor,void* e,bool* v) noexcept -> int{
        auto& f=F::self(p);ServerNpcUnreachableEnemyResult result{};
        if(!f.unreachable_api->query(actor,sizeof(f.actor),e,40,&f.unreachable_binding,&result))return 0;*v=result.unreachable!=0;return 1;
    };
    pick();assert(br.enemy==f.replacement&&!br.unreachable&&br.visited==2);
    f.put(f.actor,0x920,4);assert(!f.unreachable_api->query(f.actor,sizeof(f.actor),f.enemy,40,&ub,&ur));
    auto grmem=reinterpret_cast<GetServerNpcEnemyMemoryRecordApi>(GetProcAddress(s,server_npc_enemy_memory_record_export));
    assert(grmem&&!GetProcAddress(c,server_npc_enemy_memory_record_export)&&!grmem(0,sizeof(ServerNpcEnemyMemoryRecordApi))&&!grmem(1,0));
    const auto* rec=grmem(1,sizeof(ServerNpcEnemyMemoryRecordApi));assert(rec);
    unsigned char raw[0x38];std::memset(raw,0x5a,sizeof(raw));float initial[3]{100,200,300},position[3]{0,0,0};
    ServerNpcEnemyMemoryRecordResult rr{};ServerNpcEnemyMemoryExpiryResult expiry{};
    auto rf=[&](unsigned off){float v=0;std::memcpy(&v,raw+off,4);return v;};
    assert(rec->initialize(raw,sizeof(raw),0x7001u,initial,position,20,5,2,true,&rr)&&rr.initialized&&rr.first_seen_written);
    assert(rf(4)==0&&rf(0x10)==100&&rf(0x14)==200&&rf(0x18)==300&&rf(0x1c)==20&&rf(0x20)==20&&rf(0x24)==20&&rf(0x28)==22&&rf(0x2c)==0&&rf(0x30)==20&&raw[0x37]==0x5a);
    position[0]=12;raw[0x35]=1;assert(rec->update(raw,sizeof(raw),position,24,false,&rr)&&!rr.position_written&&!rr.changed_stamp_written&&rr.eluded_cleared&&!raw[0x35]);
    position[0]=48;assert(rec->update(raw,sizeof(raw),position,25,false,&rr)&&rr.position_written&&!rr.changed_stamp_written&&rf(0x24)==20);
    position[0]=97;assert(rec->update(raw,sizeof(raw),position,25,false,&rr)&&rr.position_written&&rr.changed_stamp_written&&rf(0x24)==25);
    position[0]=337;assert(rec->update(raw,sizeof(raw),position,26,true,&rr)&&rr.position_written&&!rr.changed_stamp_written&&rf(0x1c)==26&&rf(0x24)==25);
    position[0]=578;assert(rec->update(raw,sizeof(raw),position,27,true,&rr)&&rr.changed_stamp_written&&rf(0x24)==27&&rf(0x30)==20);
    assert(rec->expiry(raw,sizeof(raw),true,true,3,32,5,&expiry)&&!expiry.remove);
    assert(rec->expiry(raw,sizeof(raw),true,true,3,32.01f,5,&expiry)&&expiry.remove&&expiry.expired);
    raw[0x36]=1;assert(rec->expiry(raw,sizeof(raw),true,true,3,99,5,&expiry)&&expiry.protected_record&&!expiry.remove);
    assert(rec->expiry(raw,sizeof(raw),true,true,7,27,5,&expiry)&&expiry.dead&&expiry.remove);
    assert(rec->expiry(raw,sizeof(raw),false,false,0,27,5,&expiry)&&expiry.stale&&expiry.remove);
    assert(rec->initialize(raw,sizeof(raw),0xffffffffu,initial,position,20,5,-1,false,&rr)&&raw[0x34]==1&&rf(0x1c)<15&&rf(0x28)==0&&rf(0x30)==-std::numeric_limits<float>::max());
    assert(rec->expiry(raw,sizeof(raw),false,false,0,20,5,&expiry)&&expiry.expired&&!expiry.stale);
    assert(rec->update(raw,sizeof(raw),position,21,true,&rr)&&rr.first_seen_written&&rf(0x30)==21&&rf(0x1c)==21);
    assert(!rec->update(raw,0x37,position,22,true,&rr));
    auto gmaint=reinterpret_cast<GetServerNpcEnemyMemoryMaintenanceApi>(GetProcAddress(s,server_npc_enemy_memory_maintenance_export));
    assert(gmaint&&!GetProcAddress(c,server_npc_enemy_memory_maintenance_export)&&!gmaint(0,sizeof(ServerNpcEnemyMemoryMaintenanceApi))&&!gmaint(1,0));
    const auto* maint=gmaint(1,sizeof(ServerNpcEnemyMemoryMaintenanceApi));assert(maint);
    ServerNpcEnemyMemoryMaintenanceBindings maintenance{};maintenance.context=&f;maintenance.storage=bb.memory.storage;maintenance.entries=f.entries;maintenance.count=3;
    maintenance.npc_state=[](void*,void*,bool* npc,int* state) noexcept -> int{*npc=true;*state=3;return 1;};
    maintenance.extent=bb.extent;maintenance.origin=bb.origin;
    maintenance.erase=[](void* p,void* memory,unsigned bytes,unsigned index,std::uint32_t token) noexcept -> int{
        auto& f=F::self(p);assert(memory==f.memory&&bytes==sizeof(f.memory)&&index==1&&token==102);
        f.removed_index=index;f.removed_token=token;f.nodes[1]=255;return 1;
    };
    f.memory[0x10]=0;node(0,255,1,1000,101);node(1,255,255,1001,102);
    f.put(f.memory,0x1c,1.75f);f.put(f.memory,0x20,60.f);
    f.put(f.record,0,0x7001u);f.put(f.record,0x1c,100.f);f.put(f.fallback,0,0x8002u);f.put(f.fallback,0x1c,39.f);
    f.record[0x34]=f.record[0x36]=f.fallback[0x34]=f.fallback[0x36]=0;
    f.distance[0]=33;f.distance[1]=44;
    ServerNpcEnemyMemoryMaintenanceResult mr{};
    assert(maint->maintain(f.memory,sizeof(f.memory),100,&maintenance,&mr)&&mr.visited==2&&mr.removed==1&&mr.expired==1&&mr.position_refreshed==1&&mr.secondary_refreshed==1&&mr.origin_calls==2&&f.removed_index==1);
    float latest=0,secondary=0;std::memcpy(&latest,f.record+4,4);std::memcpy(&secondary,f.record+0x10,4);assert(latest==33&&secondary==33);
    f.distance[0]=44;
    assert(maint->maintain(f.memory,sizeof(f.memory),101.75f,&maintenance,&mr)&&mr.position_refreshed==1&&!mr.secondary_refreshed);
    std::memcpy(&latest,f.record+4,4);assert(latest==44);
    f.distance[0]=55;
    assert(maint->maintain(f.memory,sizeof(f.memory),101.76f,&maintenance,&mr)&&!mr.origin_calls&&!mr.removed);
    f.put(f.enemy,0xbc,0x8000u);
    assert(maint->maintain(f.memory,sizeof(f.memory),100,&maintenance,&mr)&&!mr.position_refreshed&&mr.secondary_refreshed&&mr.origin_calls==1);
    std::memcpy(&latest,f.record+4,4);std::memcpy(&secondary,f.record+0x10,4);assert(latest==44&&secondary==55);
    f.put(f.enemy,0xbc,0u);
    f.nodes[0]=0;assert(!maint->maintain(f.memory,sizeof(f.memory),100,&maintenance,&mr)&&!mr.visited);f.nodes[0]=255;
    auto gtree=reinterpret_cast<GetServerNpcEnemyMemoryTreeApi>(GetProcAddress(s,server_npc_enemy_memory_tree_export));
    assert(gtree&&!GetProcAddress(c,server_npc_enemy_memory_tree_export)&&!gtree(0,sizeof(ServerNpcEnemyMemoryTreeApi))&&!gtree(1,0));
    const auto* tree=gtree(1,sizeof(ServerNpcEnemyMemoryTreeApi));assert(tree);
    f.memory[0x10]=f.memory[0x12]=255;f.memory[0x11]=f.memory[0x13]=0;f.put(f.memory,8,32u);
    ServerNpcEnemyMemoryTreeResult tr{};
    assert(tree->validate(f.memory,sizeof(f.memory),&mb,&tr)&&tr.count==0&&tr.root==255);
    unsigned indices[32]{};
    // Permuted keys force both rotations and recoloring, stable original node indices.
    for(unsigned j=0;j<32;++j){
        unsigned key=(j*13)%32+1;
        assert(tree->insert(f.memory,sizeof(f.memory),key,1000+key,&mb,&tr)&&tr.index==j&&tr.count==j+1);
        indices[key-1]=tr.index;
        assert(tree->validate(f.memory,sizeof(f.memory),&mb,&tr));
    }
    assert(!tree->insert(f.memory,sizeof(f.memory),99,1099,&mb,&tr));
    assert(!tree->insert(f.memory,sizeof(f.memory),1,999,&mb,&tr));
    for(unsigned j=0;j<32;++j){
        assert(mem->lookup(f.memory,sizeof(f.memory),j+1,false,&mb,&ml)&&ml.record==1001+j&&ml.index==indices[j]);
    }
    // Deletions include two-child, root and black-leaf cases; survivors retain indices.
    bool removed[32]{};
    for(unsigned j=0;j<32;++j){
        const unsigned key=(j*7)%32;assert(tree->erase_index(f.memory,sizeof(f.memory),indices[key],&mb,&tr)&&tr.count==31-j&&tr.key==key+1&&tr.record==1001+key);
        removed[key]=true;assert(tree->validate(f.memory,sizeof(f.memory),&mb,&tr));
        for(unsigned k=0;k<32;++k){
            assert(mem->lookup(f.memory,sizeof(f.memory),k+1,false,&mb,&ml));
            if(removed[k])assert(!ml.matched);else assert(ml.index==indices[k]&&ml.record==1001+k);
        }
    }
    const auto free_index=tr.free_head;
    assert(tree->insert(f.memory,sizeof(f.memory),0,102,&mb,&tr)&&tr.reused&&tr.index==free_index&&tr.high_water==32);
    assert(tree->erase_index(f.memory,sizeof(f.memory),free_index,&mb,&tr)&&tr.count==0);
    assert(!tree->erase_index(f.memory,sizeof(f.memory),free_index,&mb,&tr));
    f.nodes[free_index*12+1]=static_cast<unsigned char>(free_index);
    assert(!tree->validate(f.memory,sizeof(f.memory),&mb,&tr));
    auto gmut=reinterpret_cast<GetServerNpcEnemyMemoryMutationApi>(GetProcAddress(s,server_npc_enemy_memory_mutation_export));
    assert(gmut&&!GetProcAddress(c,server_npc_enemy_memory_mutation_export)&&!gmut(0,sizeof(ServerNpcEnemyMemoryMutationApi))&&!gmut(1,0));
    f.mutation_api=gmut(1,sizeof(ServerNpcEnemyMemoryMutationApi));assert(f.mutation_api);
    auto& mutation=f.mutation;mutation.context=&f;mutation.entries=f.entries;mutation.count=3;mutation.storage=bb.memory.storage;
    mutation.entity_key=bb.memory.entity_key;mutation.entity_handle=eb.entity_handle;
    mutation.constructor_vector[0]=100;mutation.constructor_vector[1]=200;mutation.constructor_vector[2]=300;
    mutation.allocate=[](void* p,unsigned size,std::uint32_t* token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);assert(size==0x38);
        for(unsigned i=0;i<2;++i)if(!f.allocated[i]){f.allocated[i]=true;*token=101+i;*out=i?f.fallback:f.record;*n=0x38;return 1;}return 0;
    };
    mutation.release=[](void* p,std::uint32_t token) noexcept -> int{
        auto& f=F::self(p);assert(token>=101&&token<=102&&f.allocated[token-101]);f.allocated[token-101]=false;++f.releases;return 1;
    };
    f.memory[0x10]=f.memory[0x12]=255;f.memory[0x11]=f.memory[0x13]=0;f.put(f.memory,0x3c,0u);
    f.put(f.memory,0x1c,1.75f);f.put(f.memory,0x20,60.f);
    ServerNpcEnemyMemoryMutationResult mut{};float new_pos[3]{10,20,30};
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,100,0,true,&mutation,&mut)&&mut.created&&mut.allocated&&mut.creation_counter==1&&mut.token==101);
    new_pos[0]=30;assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,101,0,true,&mutation,&mut)&&mut.updated&&!mut.allocated&&mut.creation_counter==1&&mut.record.position_written);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.replacement,new_pos,101,2,false,&mutation,&mut)&&mut.created&&mut.creation_counter==2&&mut.token==102);
    assert(!f.mutation_api->upsert(f.memory,sizeof(f.memory),nullptr,new_pos,101,0,true,&mutation,&mut));
    assert(tree->validate(f.memory,sizeof(f.memory),&mutation.storage,&tr)&&tr.count==2);
    assert(f.mutation_api->remove(f.memory,sizeof(f.memory),1001,&mutation,&mut)&&mut.removed&&mut.released&&mut.creation_counter==2&&f.releases==1);
    assert(f.mutation_api->remove(f.memory,sizeof(f.memory),1001,&mutation,&mut)&&!mut.removed&&f.releases==1);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),nullptr,new_pos,101,0,true,&mutation,&mut)&&mut.created&&mut.tree.reused&&mut.creation_counter==3&&mut.key==0);
    assert(f.mutation_api->remove(f.memory,sizeof(f.memory),0x394u,&mutation,&mut)&&!mut.removed);
    // Sweep both expired records through the native tree/pool adapter, not a stub unlink.
    maintenance.erase=[](void* p,void* memory,unsigned size,unsigned index,std::uint32_t token) noexcept -> int{
        auto& f=F::self(p);return f.mutation_api->erase_record(&f.mutation,memory,size,index,token);
    };
    assert(maint->maintain(f.memory,sizeof(f.memory),162,&maintenance,&mr)&&mr.visited==2&&mr.removed==2&&mr.expired==2&&f.releases==3);
    assert(tree->validate(f.memory,sizeof(f.memory),&mutation.storage,&tr)&&tr.count==0&&tr.high_water==2);
    assert(mem->lookup(f.memory,sizeof(f.memory),1000,true,&mutation.storage,&ml)&&!ml.matched);
    auto gn=reinterpret_cast<GetServerNpcEnemyNotifyApi>(GetProcAddress(s,server_npc_enemy_notify_export));
    assert(gn&&!GetProcAddress(c,server_npc_enemy_notify_export)&&!gn(0,sizeof(ServerNpcEnemyNotifyApi))&&!gn(1,0));
    const auto* notify=gn(1,sizeof(ServerNpcEnemyNotifyApi));assert(notify);
    ServerNpcEnemyNotifyContext nc{};nc.actor_bytes=sizeof(f.actor);nc.conditions=eb.conditions;
    for(unsigned vt:{0x2248df04u,0x224954a4u,0x2249aaa4u,0x224a416cu,0x2248c514u,0x2248e83cu,0x2249964cu,0x224a4a4cu}){
        nc.original_vtable=vt;f.condition(0x14);f.condition(0x15);
        assert(notify->notify(&nc,f.actor)&&nc.last.prior_preserved&&nc.last.restored);
        assert((f.get<unsigned>(0x7cc)&0x00300000u)==(1u<<21));
    }
    nc.original_vtable=0;assert(!notify->notify(&nc,f.actor)&&nc.calls==8);
    auto gcontrol=reinterpret_cast<GetServerNpcEnemyMemoryControlApi>(GetProcAddress(s,server_npc_enemy_memory_control_export));
    assert(gcontrol&&!GetProcAddress(c,server_npc_enemy_memory_control_export)&&!gcontrol(0,sizeof(ServerNpcEnemyMemoryControlApi))&&!gcontrol(1,0));
    const auto* control=gcontrol(1,sizeof(ServerNpcEnemyMemoryControlApi));assert(control);
    std::memset(f.memory,0x5a,sizeof(f.memory));float owner_vector[3]{-1,-2,-3};
    assert(control->initialize(f.memory,sizeof(f.memory),0,0,owner_vector,&mb));
    assert(tree->validate(f.memory,sizeof(f.memory),&mb,&tr)&&tr.count==0&&tr.high_water==0&&tr.root==255);
    assert(mem->position(f.memory,sizeof(f.memory),123,false,&mb,&ml,v)&&v[0]==-1&&v[2]==-3);
    assert(!control->initialize(f.memory,sizeof(f.memory),100,33,owner_vector,&mb));
    assert(control->initialize(f.memory,sizeof(f.memory),100,32,owner_vector,&mb));
    float recent_default=0,lifetime_default=0;std::memcpy(&recent_default,f.memory+0x1c,4);std::memcpy(&lifetime_default,f.memory+0x20,4);
    assert(recent_default==1.75f&&lifetime_default==60.f&&tree->validate(f.memory,sizeof(f.memory),&mb,&tr));
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),nullptr,new_pos,200,0,true,&mutation,&mut)&&mut.created);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,200,0,true,&mutation,&mut)&&mut.created);
    ServerNpcEnemyMemoryControlResult ctr{};
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryExists,0,0,&mb,&ctr)&&ctr.value);
    assert(control->control(f.memory,sizeof(f.memory),2000,NpcMemoryExists,0,0,&mb,&ctr)&&!ctr.value&&!ctr.lookup.fallback_attempted);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryMarkEluded,0,0,&mb,&ctr)&&ctr.written);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryEluded,0,0,&mb,&ctr)&&ctr.value==1);
    assert(control->control(f.memory,sizeof(f.memory),0x394u,NpcMemoryEluded,0,0,&mb,&ctr)&&ctr.value==0);
    assert(control->control(f.memory,sizeof(f.memory),2000,NpcMemorySetProtected,0,1,&mb,&ctr)&&!ctr.written);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemorySetProtected,0,1,&mb,&ctr)&&ctr.written);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemorySetEligibility,205,0,&mb,&ctr)&&ctr.written);
    f.put(f.actor,0x984,103u);f.put(f.actor,0xc18,0u);f.put(f.actor,0x8c4,0.f);f.put(f.actor,0x920,0);
    assert(best->select(f.actor,sizeof(f.actor),204,&bb,&br)&&!br.enemy&&br.not_yet_valid==1);
    assert(best->select(f.actor,sizeof(f.actor),205,&bb,&br)&&!br.enemy&&br.eluded==1);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,205,0,true,&mutation,&mut)&&mut.updated&&mut.record.eluded_cleared);
    assert(best->select(f.actor,sizeof(f.actor),205,&bb,&br)&&br.enemy==f.enemy);
    assert(maint->maintain(f.memory,sizeof(f.memory),266,&maintenance,&mr)&&mr.removed==1&&mr.expired==1);
    assert(tree->validate(f.memory,sizeof(f.memory),&mb,&tr)&&tr.count==1);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemorySetProtected,0,0,&mb,&ctr)&&ctr.written);
    assert(maint->maintain(f.memory,sizeof(f.memory),266,&maintenance,&mr)&&mr.removed==1&&mr.expired==1);
    assert(tree->validate(f.memory,sizeof(f.memory),&mb,&tr)&&tr.count==0);
    auto grel=reinterpret_cast<GetServerNpcGruntRelationApi>(GetProcAddress(s,server_npc_grunt_relation_export));
    assert(grel&&!GetProcAddress(c,server_npc_grunt_relation_export)&&!grel(0,sizeof(ServerNpcGruntRelationApi))&&!grel(1,0));
    const auto* rel=grel(1,sizeof(ServerNpcGruntRelationApi));assert(rel);
    ServerNpcGruntRelationBindings rb{};rb.context=&f;rb.entries=f.entries;rb.count=3;rb.original_vtable=0x2249aaa4;
    rb.storage.context=&f;rb.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{if(token!=500)return 0;auto& f=F::self(p);*out=f.relation_rows;*n=sizeof(f.relation_rows);return 1;};
    rb.classify=[](void* p,void* e,int* out) noexcept -> int{auto& f=F::self(p);*out=e==f.actor?3:e==f.enemy?5:6;return 1;};
    rb.matrix_row=[](void* p,int cls,const void** out,unsigned* n) noexcept -> int{assert(cls==3);auto& f=F::self(p);*out=f.relation_matrix;*n=sizeof(f.relation_matrix);return 1;};
    rb.is_player=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).rel_player;return 1;};
    rb.special_target=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).rel_special;return 1;};
    rb.as_npc=[](void* p,void* e,void** out) noexcept -> int{*out=F::self(p).rel_npc?e:nullptr;return 1;};
    rb.npc_identity=[](void*,void*,std::uint32_t* vt,unsigned* n) noexcept -> int{*vt=0x2249aaa4;*n=0x86c;return 1;};
    rb.component_valid=[](void*,std::uint32_t token,void*,bool* out) noexcept -> int{assert(token==77);*out=false;return 1;};
    f.put(f.actor,0x638,500u);f.put(f.actor,0x644,3);
    auto relation_row=[&](unsigned i,unsigned handle,int cls,int disp,int rank){
        auto* r=f.relation_rows+16*i;f.put(r,0,handle);f.put(r,4,cls);f.put(r,8,disp);f.put(r,12,rank);
    };
    relation_row(0,0xffffffffu,5,2,7);relation_row(1,0x9002u,0,4,0);relation_row(2,0x7001u,0,1,11);
    f.put(f.relation_matrix,16*6+8,2);f.put(f.relation_matrix,16*6+12,9);
    ServerNpcGruntRelationResult relation_result{};ServerNpcGruntValidityResult validity{};
    assert(rel->relation(f.actor,sizeof(f.actor),f.enemy,true,&rb,&relation_result)&&relation_result.source==1&&relation_result.index==2&&relation_result.disposition==1&&relation_result.priority==11&&!relation_result.class_queries);
    f.put(f.relation_rows+32,0,0xffffffffu);
    assert(rel->relation(f.actor,sizeof(f.actor),f.enemy,true,&rb,&relation_result)&&relation_result.source==2&&relation_result.index==0&&relation_result.disposition==2&&relation_result.priority==7&&relation_result.class_queries==2);
    assert(rel->relation(f.actor,sizeof(f.actor),f.replacement,true,&rb,&relation_result)&&relation_result.source==3&&relation_result.index==6&&relation_result.disposition==2&&relation_result.priority==9);
    f.actor[0xdb1]=1;
    assert(rel->relation(f.actor,sizeof(f.actor),f.replacement,true,&rb,&relation_result)&&relation_result.overridden&&relation_result.disposition==4&&relation_result.special_called);
    f.rel_special=true;assert(rel->relation(f.actor,sizeof(f.actor),f.replacement,true,&rb,&relation_result)&&relation_result.disposition==1);
    f.rel_player=true;assert(rel->relation(f.actor,sizeof(f.actor),f.replacement,true,&rb,&relation_result)&&relation_result.disposition==1&&!relation_result.special_called);
    assert(rel->relation(f.actor,sizeof(f.actor),f.replacement,false,&rb,&relation_result)&&!relation_result.overridden&&relation_result.priority==9);
    f.actor[0xdb1]=0;assert(rel->relation(f.actor,sizeof(f.actor),nullptr,true,&rb,&relation_result)&&relation_result.disposition==4);
    assert(rel->valid(f.actor,sizeof(f.actor),f.enemy,&rb,&validity)&&validity.valid&&!validity.npc_found);
    f.rel_npc=true;f.put(f.enemy,0x868,1);
    assert(rel->valid(f.actor,sizeof(f.actor),f.enemy,&rb,&validity)&&validity.valid&&validity.native_npc_gate);
    f.put(f.enemy,0x868,2);assert(rel->valid(f.actor,sizeof(f.actor),f.enemy,&rb,&validity)&&!validity.valid);
    f.put(f.actor,0xc18,77u);assert(rel->valid(f.actor,sizeof(f.actor),f.enemy,&rb,&validity)&&validity.component_called&&!validity.valid);
    auto gmatrix=reinterpret_cast<GetServerNpcRelationMatrixApi>(GetProcAddress(s,server_npc_relation_matrix_export));
    assert(gmatrix&&!GetProcAddress(c,server_npc_relation_matrix_export)&&!gmatrix(0,sizeof(ServerNpcRelationMatrixApi))&&!gmatrix(1,0));
    const auto* matrix=gmatrix(1,sizeof(ServerNpcRelationMatrixApi));assert(matrix);
    ServerNpcRelationCell cells[225]{};
    assert(!matrix->initialize(cells,224)&&matrix->initialize(cells,225));
    unsigned priorities=0;int totals[5]{};
    for(int i=0;i<15;++i){
        const void* row=nullptr;unsigned bytes=0;
        assert(matrix->row(cells,i,&row,&bytes)&&row==cells+15*i&&bytes==240);
        for(int j=0;j<15;++j){const auto& cell=cells[15*i+j];
            assert(cell.handle==0xffffffffu&&cell.classification==0&&cell.disposition>=1&&cell.disposition<=4);
            assert(cell.priority==(j==1&&(i==6||i==7||i==10)?100:0));priorities+=cell.priority!=0;++totals[cell.disposition];
        }
    }
    assert(priorities==3&&cells[3*15+5].disposition==1&&cells[4*15+6].disposition==2&&cells[1*15+1].disposition==3&&cells[0].disposition==4);
    assert(!matrix->set(cells,225,15,0,1,99)&&!matrix->set(cells,224,0,0,1,99));
    assert(matrix->set(cells,225,3,6,1,17)&&cells[3*15+6].disposition==1&&cells[3*15+6].priority==17);
    std::memcpy(f.relation_matrix,cells+3*15,240);
    f.put(f.actor,0xc18,0u);f.actor[0xdb1]=0;
    assert(rel->relation(f.actor,sizeof(f.actor),f.replacement,true,&rb,&relation_result)&&relation_result.source==3&&relation_result.disposition==1&&relation_result.priority==17);
    assert(matrix->initialize(cells,225)&&cells[3*15+6].priority==0);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native NPC state reconsideration and Grunt ideal-state transitions");
}
