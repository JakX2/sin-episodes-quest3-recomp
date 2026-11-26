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
    unsigned char memory[0x40]{},nodes[384]{},record[0x38]{},fallback[0x38]{},unreachable_rows[60]{},third[0xc0]{},third_record[0x38]{};
    const ServerNpcUnreachableEnemyApi* unreachable_api=nullptr;ServerNpcUnreachableEnemyBindings unreachable_binding{};
    const ServerNpcEnemyMemoryMutationApi* mutation_api=nullptr;ServerNpcEnemyMemoryMutationBindings mutation{};
    bool allocated[2]{};unsigned releases=0;
    unsigned char relation_rows[48]{},relation_matrix[240]{},squad_data[0x4c]{};bool rel_player=false,rel_special=false,rel_npc=false;
    unsigned char senses[0x60]{},sense_vectors[3][16]{},sense_handles[3][16]{};
    unsigned char heard[8][0x30]{};int active_sound_head=1;unsigned rebuild_calls=0;int rebuild_radius=100;unsigned hearing_outputs=0,hearing_offsets[3]{};bool sound_visible=true,component_ignored=false;
    unsigned attack_weapon_caps=0,attack_line_calls=0,attack_line_mode=0;float attack_forward[3]{1,0,0};bool melee_hits=true;
    unsigned combat_nav_calls=0;bool remembered_visible=false;
    unsigned char update_motor[0x18]{},update_component[9]{};bool update_pvs=false,update_component_handled=false,update_component_value=false;int update_roll=0;
    unsigned char combat_nav[0x3c]{},goal_path[0x6c]{};unsigned goal_retargets=0,goal_refreshes=0,goal_failures=0;bool goal_success=true;void* goal_target=nullptr;float combat_center[3]{},combat_velocity[3]{},combat_origin[3]{70,0,0};bool combat_facing=true;
    bool combat_trace=true,combat_fov=true;unsigned combat_output_count=0,combat_output_offsets[8]{};bool combat_output_value[8]{};std::uint32_t combat_output_handles[8]{};
    int speech_code=0;unsigned speech_calls=0,random_calls=0;const char* speech_name=nullptr;
    unsigned char speech_squad[0x58]{};unsigned queue_deliveries=0,queue_randoms=0;
    unsigned report_cues=0,report_forwarded=0,lost_cues=0,outputs=0;unsigned output_offsets[2]{};
    sin_independent::EntityHandleEntry entries[4]{};
    unsigned clears=0,changes=0,lost=0,squad=0,removed_index=255,removed_token=0;int query=-1;
    int rank[3]{0,0,0},disp[3]{1,1,1};bool valid[3]{true,true,true},unreachable[3]{false,false,false},visible[3]{true,true,true};float distance[3]{10,20,8};
    unsigned idx(void* e){assert(e==enemy||e==replacement||e==third);return e==enemy?0u:e==replacement?1u:2u;}
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
        auto& f=F::self(p);switch(token){case 100:*out=f.nodes;*n=sizeof(f.nodes);return 1;case 101:*out=f.record;*n=sizeof(f.record);return 1;case 102:*out=f.fallback;*n=sizeof(f.fallback);return 1;case 103:*out=f.memory;*n=sizeof(f.memory);return 1;case 104:*out=f.third_record;*n=sizeof(f.third_record);return 1;default:return 0;}
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
    f.unreachable[0]=false;f.rank[0]=f.rank[1]=0;f.visible[0]=false;pick();assert(br.enemy==f.replacement&&!br.visible&&br.distance_squared==100);
    f.visible[0]=true;f.visible[1]=false;f.distance[1]=5;pick();assert(br.enemy==f.enemy);
    f.visible[1]=true;pick();assert(br.enemy==f.replacement&&br.distance_squared==100);
    // Third candidate exposes the stale distance cache, not just result telemetry.
    f.entries[3].serial=9;f.entries[3].entity=reinterpret_cast<CBaseEntity*>(f.third);bb.memory.count=4;
    f.put(f.third_record,0,0x9003u);node(1,255,2,1001,102);node(2,255,255,1002,104);
    pick();assert(br.enemy==f.third&&br.distance_squared==100&&br.visible&&br.replaced==3);
    // A farther visible second candidate does not refresh visibility either.
    f.visible[0]=false;f.visible[2]=false;f.distance[1]=20;f.distance[2]=5;
    pick();assert(br.enemy==f.third&&!br.visible&&br.distance_squared==100&&br.replaced==3);
    f.visible[0]=f.visible[2]=true;f.distance[1]=5;bb.memory.count=3;f.entries[3]={};node(1,255,255,1001,102);
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
    auto ggruntbest=reinterpret_cast<GetServerNpcGruntBestEnemyApi>(GetProcAddress(s,server_npc_grunt_best_enemy_export));
    assert(ggruntbest&&!GetProcAddress(c,server_npc_grunt_best_enemy_export)&&!ggruntbest(0,sizeof(ServerNpcGruntBestEnemyApi))&&!ggruntbest(1,0));
    const auto* gruntbest=ggruntbest(1,sizeof(ServerNpcGruntBestEnemyApi));assert(gruntbest);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,300,0,true,&mutation,&mut));
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.replacement,new_pos,300,0,true,&mutation,&mut));
    f.put(f.actor,0x644,0);f.put(f.actor,0xc18,0u);f.put(f.actor,0x920,0);
    f.rel_npc=true;f.put(f.enemy,0x868,1);f.put(f.replacement,0x868,1);
    std::memcpy(f.relation_matrix,cells+3*15,240);
    auto native_best=bb;native_best.disposition=nullptr;native_best.priority=nullptr;native_best.valid=nullptr;native_best.unreachable=nullptr;
    ServerNpcGruntBestEnemyResult gbr{};
    assert(gruntbest->select(f.actor,sizeof(f.actor),300,&native_best,&rb,&ub,&gbr)&&gbr.selection.enemy==f.replacement&&gbr.selection.disposition_rejected==0);
    f.put(f.replacement,0x868,2);
    assert(gruntbest->select(f.actor,sizeof(f.actor),300,&native_best,&rb,&ub,&gbr)&&gbr.selection.enemy==f.enemy&&gbr.selection.invalid==1);
    f.put(f.replacement,0x868,1);f.distance[0]=10;f.distance[1]=20;
    f.put(f.actor,0x920,1);unreachable_row(0,0x7001u,310,10);
    assert(gruntbest->select(f.actor,sizeof(f.actor),300,&native_best,&rb,&ub,&gbr)&&gbr.selection.enemy==f.replacement&&gbr.cache_queries==2&&gbr.validity_queries==2&&gbr.disposition_queries>=4);
    f.put(f.actor,0x920,0);f.actor[0xdb1]=1;f.rel_player=f.rel_special=false;
    assert(gruntbest->select(f.actor,sizeof(f.actor),300,&native_best,&rb,&ub,&gbr)&&!gbr.selection.enemy&&gbr.selection.disposition_rejected==2&&!gbr.cache_queries);
    f.actor[0xdb1]=0;
    auto greport=reinterpret_cast<GetServerNpcGruntMemoryReportApi>(GetProcAddress(s,server_npc_grunt_memory_report_export));
    assert(greport&&!GetProcAddress(c,server_npc_grunt_memory_report_export)&&!greport(0,sizeof(ServerNpcGruntMemoryReportApi))&&!greport(1,0));
    const auto* report=greport(1,sizeof(ServerNpcGruntMemoryReportApi));assert(report);
    assert(f.mutation_api->remove(f.memory,sizeof(f.memory),1000,&mutation,&mut)&&mut.removed);
    assert(f.mutation_api->remove(f.memory,sizeof(f.memory),1001,&mutation,&mut)&&mut.removed);
    ServerNpcGruntMemoryReportBindings report_binding{};report_binding.mutation=mutation;report_binding.context=&f;report_binding.original_vtable=0x2249aaa4;
    report_binding.idle_delay=.5f;report_binding.alert_delay=.2f;
    f.entries[3].serial=9;f.entries[3].entity=reinterpret_cast<CBaseEntity*>(f.actor);report_binding.mutation.count=4;
    report_binding.mutation.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);switch(token){case 100:*out=f.nodes;*n=sizeof(f.nodes);return 1;case 101:*out=f.record;*n=sizeof(f.record);return 1;case 102:*out=f.fallback;*n=sizeof(f.fallback);return 1;case 103:*out=f.memory;*n=sizeof(f.memory);return 1;case 105:*out=f.squad_data;*n=sizeof(f.squad_data);return 1;default:return 0;}
    };
    report_binding.cue=[](void* p,void* a,const char* name,int priority,int type) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&!std::strcmp(name,"GRUNT_REFIND_ENEMY")&&priority==2&&type==2);++f.report_cues;return 1;};
    report_binding.forward=[](void* p,void* member,void* target,const float* pos,void* source) noexcept -> int{auto& f=F::self(p);assert(member==f.replacement&&target==f.enemy&&source==f.actor&&pos[0]==30);++f.report_forwarded;return 1;};
    f.put(f.actor,0x9c4,105u);f.put(f.squad_data,8,0x9003u);f.put(f.squad_data,12,0x8002u);f.put(f.squad_data,0x48,2);
    f.put(f.actor,0x850,1);f.put(f.actor,0x984,103u);f.put(f.actor,0x85c,9u);f.put(f.actor,0x864,10u);
    ServerNpcGruntMemoryReportResult report_result{};
    assert(report->report(f.actor,sizeof(f.actor),f.enemy,new_pos,nullptr,400,&report_binding,&report_result)&&report_result.firsthand&&report_result.retail_return&&report_result.delay==.5f&&report_result.forwarded==1&&!report_result.idle_reset);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryMarkEluded,0,0,&mb,&ctr));
    f.put(f.actor,0x850,3);
    assert(report->report(f.actor,sizeof(f.actor),f.enemy,new_pos,f.actor,401,&report_binding,&report_result)&&report_result.mutation.updated&&!report_result.retail_return&&report_result.refind_cue&&report_result.delay==.2f&&report_result.forwarded==1&&f.report_cues==1&&f.report_forwarded==2);
    f.put(f.actor,0x850,1);
    assert(report->report(f.actor,sizeof(f.actor),f.replacement,new_pos,f.enemy,402,&report_binding,&report_result)&&!report_result.firsthand&&report_result.retail_return&&report_result.idle_reset&&!report_result.forwarded&&report_result.delay==0&&f.get<unsigned>(0x864)==0&&f.get<unsigned>(0x85c)==0);
    assert(mem->timestamp(f.memory,sizeof(f.memory),1001,NpcMemoryStamp1c,&mb,&ml,&stamp)&&stamp<402);
    f.actor[0xb6]=1;assert(report->report(f.actor,sizeof(f.actor),f.enemy,new_pos,nullptr,403,&report_binding,&report_result)&&report_result.blocked&&!report_result.retail_return);f.actor[0xb6]=0;
    f.put(f.actor,0x984,0u);assert(report->report(f.actor,sizeof(f.actor),f.enemy,new_pos,nullptr,403,&report_binding,&report_result)&&report_result.no_memory&&report_result.retail_return);
    auto gchoice=reinterpret_cast<GetServerNpcGruntEnemyChoiceGateApi>(GetProcAddress(s,server_npc_grunt_enemy_choice_gate_export));
    assert(gchoice&&!GetProcAddress(c,server_npc_grunt_enemy_choice_gate_export)&&!gchoice(0,sizeof(ServerNpcGruntEnemyChoiceGateApi))&&!gchoice(1,0));
    const auto* choice=gchoice(1,sizeof(ServerNpcGruntEnemyChoiceGateApi));assert(choice);
    ServerNpcGruntEnemyChoiceGateBindings choice_binding{};choice_binding.context=&f;choice_binding.relation=rb;
    choice_binding.memory=bb.memory;choice_binding.conditions=eb.conditions;choice_binding.extent=bb.extent;
    choice_binding.random=[](void*,float lo,float hi,float* result) noexcept -> int{assert(lo==1&&hi==3);*result=2;return 1;};
    f.put(f.actor,0x984,103u);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0xc18,0u);f.actor[0xdb1]=0;
    f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);f.put(f.actor,0x8bc,500.f);f.put(f.actor,0x644,0);
    ServerNpcGruntEnemyChoiceGateResult choice_result{};
    auto choose=[&](float now,unsigned reason){assert(choice->query(f.actor,sizeof(f.actor),now,&choice_binding,&choice_result)&&choice_result.reason==reason&&choice_result.choose==(reason!=NpcChoiceRetain));};
    choose(403,NpcChoiceNoEnemy);assert(choice_result.counter_written);
    f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x8c0,0xffffffffu);
    choose(403,NpcChoiceMemoryChanged);assert(!choice_result.counter_written&&f.get<unsigned>(0x8c0)==0xffffffffu);
    f.put(f.actor,0x8c0,choice_result.counter);
    choose(403,NpcChoiceRetain);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryMarkEluded,0,0,&mb,&ctr));choose(403,NpcChoiceEluded);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,403,0,true,&mutation,&mut));
    f.condition(7);choose(403,NpcChoiceCondition);assert(choice_result.condition==7);f.put(f.actor,0x7cc,0u);
    f.put(f.enemy,0x868,2);choose(403,NpcChoiceInvalid);f.put(f.enemy,0x868,1);
    f.actor[0xdb1]=1;choose(403,NpcChoiceDisposition);f.actor[0xdb1]=0;
    f.put(f.enemy,0x8c,1);f.actor[0x8b0]=0;f.put(f.actor,0x8b4,1.f);f.put(f.actor,0x8b8,3.f);
    choose(403,NpcChoiceRetain);assert(choice_result.death_timer_started&&f.get<float>(0x8ac)==405);
    choose(404.9f,NpcChoiceRetain);choose(405,NpcChoiceDeathTimer);f.put(f.enemy,0x8c,0);
    f.put(f.actor,0x8bc,403.f);f.condition(0x21);choose(403,NpcChoicePeriodicCondition);assert(choice_result.periodic_timer_written&&f.get<float>(0x8bc)==404.5f);f.put(f.actor,0x7d0,0u);
    choose(414,NpcChoiceOldSight);
    f.put(f.actor,0x8bc,500.f);assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemorySetEligibility,420,0,&mb,&ctr));
    choose(420,NpcChoiceEligibility);choose(420.01f,NpcChoiceRetain);
    assert(f.mutation_api->remove(f.memory,sizeof(f.memory),1000,&mutation,&mut)&&mut.removed);choose(421,NpcChoiceMissingRecord);
    auto gswitch=reinterpret_cast<GetServerNpcGruntEnemySwitchApi>(GetProcAddress(s,server_npc_grunt_enemy_switch_export));
    assert(gswitch&&!GetProcAddress(c,server_npc_grunt_enemy_switch_export)&&!gswitch(0,sizeof(ServerNpcGruntEnemySwitchApi))&&!gswitch(1,0));
    const auto* switcher=gswitch(1,sizeof(ServerNpcGruntEnemySwitchApi));assert(switcher);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,422,0,true,&mutation,&mut));
    ServerNpcGruntEnemySwitchBindings swb{};swb.gate=choice_binding;swb.best=native_best;swb.cache=ub;swb.enemy=eb;swb.schedule=b.schedule;swb.context=&f;
    swb.enemy.enemy_changing=nullptr; // internal native notification replaces callback
    swb.lost_cue=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.get<unsigned>(0x8a0)==0xffffffffu);++f.lost_cues;return 1;};
    swb.fire_output=[](void* p,void* a,unsigned offset) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.outputs<2);f.output_offsets[f.outputs++]=offset;return 1;};
    f.put(f.actor,0x798,1u);f.put(f.actor,0x7a0,0);f.put(f.schedule,8,1);f.put(f.schedule,0xc,0u);
    f.put(f.actor,0x7ec,0u);f.put(f.actor,0x7f0,0u);f.put(f.actor,0x988,0x28000u);f.put(f.actor,0x9c4,0u);
    f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x8c0,0xffffffffu);
    f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);f.distance[0]=20;f.distance[1]=5;f.rel_special=true;
    ServerNpcGruntEnemySwitchResult switch_result;
    assert(switcher->update(f.actor,sizeof(f.actor),422,&swb,&switch_result)&&switch_result.deferred&&switch_result.has_enemy&&!switch_result.selected&&f.actor[0x84e]);
    f.put(f.schedule,0xc,1u<<25);f.condition(0x14);f.condition(0x15);
    assert(switcher->update(f.actor,sizeof(f.actor),422,&swb,&switch_result)&&!switch_result.deferred&&switch_result.selected&&switch_result.assigned&&switch_result.assignment.new_handle==0x8002u&&switch_result.new_flags==0x10000u&&!f.actor[0x84e]);
    assert((f.get<unsigned>(0x7cc)&0x00300000u)==(1u<<21)&&(f.get<unsigned>(0x7cc)&(1u<<25)));
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryMarkEluded,0,0,&mb,&ctr));
    assert(control->control(f.memory,sizeof(f.memory),1001,NpcMemoryMarkEluded,0,0,&mb,&ctr));
    f.put(f.actor,0x8a0,0x9002u);f.put(f.schedule,0xc,0u);
    assert(switcher->update(f.actor,sizeof(f.actor),423,&swb,&switch_result)&&switch_result.forced_missing&&switch_result.assigned&&!switch_result.has_enemy&&switch_result.lost_condition&&switch_result.lost_cue&&switch_result.outputs==2&&switch_result.new_flags==0);
    assert(f.lost_cues==1&&f.outputs==2&&f.output_offsets[0]==0xb08&&f.output_offsets[1]==0xac0&&(f.get<unsigned>(0x7cc)&(1u<<11))&&!(f.get<unsigned>(0x7cc)&(1u<<25)));
    auto gpspeech=reinterpret_cast<GetServerNpcGruntPerceptionSpeechApi>(GetProcAddress(s,server_npc_grunt_perception_speech_export));
    assert(gpspeech&&!GetProcAddress(c,server_npc_grunt_perception_speech_export)&&!gpspeech(0,sizeof(ServerNpcGruntPerceptionSpeechApi))&&!gpspeech(1,0));
    const auto* pspeech=gpspeech(1,sizeof(ServerNpcGruntPerceptionSpeechApi));assert(pspeech);
    ServerNpcGruntPerceptionSpeechBindings psb{};ServerNpcGruntPerceptionSpeechResult psr{};
    // Same lost-target transition, native mode 2 rejects absent squad without calling legacy lost_cue.
    auto native_switch=swb;native_switch.lost_cue=nullptr;
    f.put(f.actor,0x988,0x10000u);f.put(f.actor,0x8a0,0x9002u);f.put(f.actor,0xc88,0.f);f.put(f.actor,0xccc,41);f.outputs=0;
    assert(pspeech->enemy_switch(f.actor,sizeof(f.actor),423,&native_switch,&psb,&switch_result,&psr)&&switch_result.lost_cue&&psr.lost_calls==1&&psr.lost.speech.cue.emitted&&!psr.lost.speech.sentence.mode_allowed&&f.get<int>(0xccc)==41&&f.lost_cues==1&&f.outputs==2);
    auto gspeech=reinterpret_cast<GetServerNpcGruntSpeechApi>(GetProcAddress(s,server_npc_grunt_speech_export));
    assert(gspeech&&!GetProcAddress(c,server_npc_grunt_speech_export)&&!gspeech(0,sizeof(ServerNpcGruntSpeechApi))&&!gspeech(1,0));
    const auto* speech=gspeech(1,sizeof(ServerNpcGruntSpeechApi));assert(speech);
    ServerNpcGruntSpeechBindings spb{};spb.context=&f;spb.original_vtable=0x2249aaa4;spb.memory=bb.memory;
    spb.emit=[](void* p,void* a,unsigned member,const char* cue,int priority,int type,int* code) noexcept -> int{
        auto& f=F::self(p);assert(a==f.actor&&member==0xcc0&&type==2&&priority==(!std::strncmp(cue,"GRUNT_LOST_",11)?0:2));
        f.speech_name=cue;++f.speech_calls;*code=f.speech_code;return 1;
    };
    spb.random=[](void* p,float lo,float hi,float* value) noexcept -> int{auto& f=F::self(p);assert(lo==5&&hi==15);++f.random_calls;*value=7;return 1;};
    f.put(f.actor,0xc88,432.f);f.put(f.actor,0xc7c,432.f);f.put(f.actor,0x8a0,0x7001u);
    ServerNpcGruntSpeechResult sr{};
    assert(speech->speak(f.actor,sizeof(f.actor),NpcGruntLost,431,&spb,&sr)&&sr.suppressed&&!f.speech_calls);
    f.speech_code=-1;
    assert(speech->speak(f.actor,sizeof(f.actor),NpcGruntLost,432,&spb,&sr)&&sr.emitted&&sr.enemy_live&&sr.memory_found&&sr.last_seen==422&&!std::strcmp(sr.cue,"GRUNT_LOST_SHORT")&&!sr.cooldown_written&&!f.random_calls);
    f.speech_code=0;
    assert(speech->speak(f.actor,sizeof(f.actor),NpcGruntLost,432.01f,&spb,&sr)&&!std::strcmp(sr.cue,"GRUNT_LOST_LONG")&&sr.cooldown_written&&f.random_calls==1&&f.get<float>(0xc88)==439.01f);
    f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0xc88,0.f);
    assert(speech->speak(f.actor,sizeof(f.actor),NpcGruntLost,433,&spb,&sr)&&!sr.enemy_live&&!std::strcmp(sr.cue,"GRUNT_LOST_LONG"));
    assert(speech->speak(f.actor,sizeof(f.actor),NpcGruntRefind,433,&spb,&sr)&&sr.emitted&&!sr.cooldown_written&&!std::strcmp(sr.cue,"GRUNT_REFIND_ENEMY"));
    assert(speech->speak(f.actor,sizeof(f.actor),NpcGruntAlert,432,&spb,&sr)&&sr.suppressed);
    f.speech_code=-1;
    assert(speech->speak(f.actor,sizeof(f.actor),NpcGruntAlert,433,&spb,&sr)&&sr.emitted&&sr.cooldown_written&&sr.deadline==443&&f.get<float>(0xc7c)==443&&!std::strcmp(sr.cue,"GRUNT_GO_ALERT"));
    assert(!speech->speak(f.actor,0xcc3,NpcGruntLost,444,&spb,&sr));
    auto gsense=reinterpret_cast<GetServerNpcSensedEntitiesApi>(GetProcAddress(s,server_npc_sensed_entities_export));
    assert(gsense&&!GetProcAddress(c,server_npc_sensed_entities_export)&&!gsense(0,sizeof(ServerNpcSensedEntitiesApi))&&!gsense(1,0));
    const auto* sensed=gsense(1,sizeof(ServerNpcSensedEntitiesApi));assert(sensed);
    ServerNpcSensedEntitiesBindings seb{};seb.entries=f.entries;seb.count=4;seb.storage.context=&f;
    seb.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);if(token>=600&&token<603){*out=f.sense_vectors[token-600];*n=16;return 1;}
        if(token>=610&&token<613){*out=f.sense_handles[token-610];*n=16;return 1;}if(token==620){*out=f.senses;*n=sizeof(f.senses);return 1;}return 0;
    };
    for(unsigned i=0;i<3;++i){f.put(f.senses,0x54+4*i,600+i);f.put(f.sense_vectors[i],0,610+i);}
    f.put(f.sense_vectors[1],0xc,3);f.put(f.sense_vectors[2],0xc,1);
    f.put(f.sense_handles[1],0,0x7001u);f.put(f.sense_handles[1],4,0x9002u);f.put(f.sense_handles[1],8,0x8002u);f.put(f.sense_handles[2],0,0x9003u);
    ServerNpcSensedCursor sc{};ServerNpcSensedResult sensed_result{};bool found=false;
    assert(sensed->first(f.senses,sizeof(f.senses),-1,&seb,&sc,&sensed_result)&&sensed_result.entity==f.enemy&&sc.packed==0xff000101u);
    assert(sensed->next(f.senses,sizeof(f.senses),&seb,&sc,&sensed_result)&&sensed_result.entity==f.replacement&&sensed_result.stale==1&&sensed_result.index==2);
    assert(sensed->next(f.senses,sizeof(f.senses),&seb,&sc,&sensed_result)&&sensed_result.entity==f.actor&&sensed_result.category==2);
    assert(sensed->next(f.senses,sizeof(f.senses),&seb,&sc,&sensed_result)&&!sensed_result.entity&&sc.packed==0xffffffffu);
    assert(sensed->contains(f.senses,sizeof(f.senses),f.replacement,&seb,&found)&&found);
    assert(sensed->first(f.senses,sizeof(f.senses),0,&seb,&sc,&sensed_result)&&sensed_result.category==1);
    assert(sensed->next(f.senses,sizeof(f.senses),&seb,&sc,&sensed_result)&&sensed_result.entity==f.replacement);
    assert(sensed->next(f.senses,sizeof(f.senses),&seb,&sc,&sensed_result)&&!sensed_result.entity);
    f.put(f.sense_handles[1],0,0x9001u);
    assert(sensed->first(f.senses,sizeof(f.senses),-1,&seb,&sc,&sensed_result)&&!sensed_result.entity&&sensed_result.first_stale);
    assert(sensed->contains(f.senses,sizeof(f.senses),f.replacement,&seb,&found)&&!found);
    assert(sensed->next(f.senses,sizeof(f.senses),&seb,&sc,&sensed_result)&&sensed_result.entity==f.replacement);
    f.put(f.sense_handles[1],0,0x7001u);
    f.put(f.sense_vectors[1],0xc,5);assert(!sensed->first(f.senses,sizeof(f.senses),-1,&seb,&sc,&sensed_result));
    f.put(f.sense_vectors[1],0xc,3);
    auto gvisual=reinterpret_cast<GetServerNpcGruntVisualApi>(GetProcAddress(s,server_npc_grunt_visual_conditions_export));
    assert(gvisual&&!GetProcAddress(c,server_npc_grunt_visual_conditions_export)&&!gvisual(0,sizeof(ServerNpcGruntVisualApi))&&!gvisual(1,0));
    const auto* visual=gvisual(1,sizeof(ServerNpcGruntVisualApi));assert(visual);
    ServerNpcGruntVisualBindings vb{};vb.senses=seb;vb.relation=rb;vb.report=report_binding;vb.conditions=eb.conditions;vb.context=&f;
    vb.extent=[](void* p,void* e,unsigned* n) noexcept -> int{auto& f=F::self(p);if(e==f.enemy)*n=sizeof(f.enemy);else if(e==f.replacement)*n=sizeof(f.replacement);else return 0;return 1;};
    vb.refresh_transform=[](void* p,void* e) noexcept -> int{auto& f=F::self(p);assert(e==f.enemy);f.put(e,0xb8,0u);f.put(e,0x2bc,70.f);return 1;};
    f.put(f.actor,0x898,620u);f.put(f.actor,0x984,103u);f.put(f.actor,0x9c4,0u);f.put(f.actor,0x8a0,0x7001u);
    f.put(f.sense_vectors[2],0xc,0);
    f.put(f.actor,0x644,2);relation_row(0,0x7001u,0,1,-2);relation_row(1,0x8002u,0,2,11);
    f.put(f.enemy,0xb8,0x800u);f.put(f.replacement,0x2bc,90.f);
    f.rel_special=false;f.rel_player=false;f.actor[0xdb1]=0;
    f.put(f.actor,0x7cc,(1u<<7)|(1u<<31)|(1u<<14));f.put(f.actor,0x7d0,1u);
    ServerNpcGruntVisualResult vr{};
    assert(visual->gather(f.actor,sizeof(f.actor),450,&vb,&vr)&&vr.visited==2&&vr.stale==1&&vr.hostile==1&&vr.feared==1&&vr.current_seen==1&&vr.transforms==1&&vr.report_calls==2&&vr.cleared==3);
    assert((f.get<unsigned>(0x7cc)&((1u<<7)|(1u<<8)|(1u<<9)|(1u<<10)|(1u<<31)))==((1u<<8)|(1u<<9)|(1u<<10))&&!(f.get<unsigned>(0x7d0)&1u)&&(f.get<unsigned>(0x7cc)&(1u<<14)));
    assert(mem->position(f.memory,sizeof(f.memory),1000,false,&mb,&ml,v)&&v[0]==70);
    f.actor[0xdb1]=1;f.rel_special=true;
    assert(visual->gather(f.actor,sizeof(f.actor),451,&vb,&vr)&&vr.special_mode&&vr.hostile==2&&!vr.feared&&vr.special_seen==2&&vr.report_calls==2);
    assert((f.get<unsigned>(0x7cc)&((1u<<8)|(1u<<9)|(1u<<31)))==((1u<<9)|(1u<<31))&&(f.get<unsigned>(0x7d0)&1u));
    f.rel_special=false;
    assert(visual->gather(f.actor,sizeof(f.actor),452,&vb,&vr)&&vr.neutral==2&&!vr.report_calls&&!vr.current_seen);
    f.actor[0xdb1]=0;relation_row(0,0x7001u,0,3,0);relation_row(1,0x8002u,0,4,0);
    assert(visual->gather(f.actor,sizeof(f.actor),453,&vb,&vr)&&vr.friendly==1&&vr.neutral==1&&vr.current_seen==1&&!vr.report_calls);
    f.put(f.sense_handles[1],0,0x9001u);
    assert(visual->gather(f.actor,sizeof(f.actor),454,&vb,&vr)&&vr.first_stale&&!vr.visited&&!vr.report_calls&&!(f.get<unsigned>(0x7cc)&(1u<<10)));
    auto ghearing=reinterpret_cast<GetServerNpcGruntHearingApi>(GetProcAddress(s,server_npc_grunt_hearing_export));
    assert(ghearing&&!GetProcAddress(c,server_npc_grunt_hearing_export)&&!ghearing(0,sizeof(ServerNpcGruntHearingApi))&&!ghearing(1,0));
    const auto* hearing=ghearing(1,sizeof(ServerNpcGruntHearingApi));assert(hearing&&hearing->mask()==0x141f);
    ServerNpcGruntHearingBindings hb{};hb.storage=seb.storage;hb.schedule=b.schedule;hb.context=&f;hb.original_vtable=0x2249aaa4;
    hb.sound=[](void* p,int index,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(index<0||index>=8)return 0;*out=f.heard[index];*n=sizeof(f.heard[index]);return 1;};
    hb.fire_output=[](void* p,void* a,unsigned off) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.hearing_outputs<3);f.hearing_offsets[f.hearing_outputs++]=off;return 1;};
    hb.sound_visible=[](void* p,void*,const void*,unsigned mask,bool* yes) noexcept -> int{assert(mask==0x4081);*yes=F::self(p).sound_visible;return 1;};
    hb.component_ignored=[](void* p,std::uint32_t token,const void*,bool* yes) noexcept -> int{assert(token==77);*yes=F::self(p).component_ignored;return 1;};
    ServerNpcGruntHearingFilterResult hf{};
    f.put(f.heard[0],0xc,0x4800000u);assert(hearing->filter(f.actor,sizeof(f.actor),f.heard[0],sizeof(f.heard[0]),&hb,&hf)&&hf.accepted&&hf.forced);
    f.put(f.heard[0],0xc,0x4000000u);assert(hearing->filter(f.actor,sizeof(f.actor),f.heard[0],sizeof(f.heard[0]),&hb,&hf)&&!hf.accepted&&hf.forced);
    f.put(f.heard[0],0xc,4u);f.put(f.actor,0x850,1);f.put(f.actor,0xc18,77u);f.sound_visible=false;
    assert(hearing->filter(f.actor,sizeof(f.actor),f.heard[0],sizeof(f.heard[0]),&hb,&hf)&&!hf.accepted&&hf.visible_called&&!hf.component_called);
    f.sound_visible=true;f.component_ignored=true;
    assert(hearing->filter(f.actor,sizeof(f.actor),f.heard[0],sizeof(f.heard[0]),&hb,&hf)&&!hf.accepted&&hf.component_called);
    f.put(f.actor,0xc18,0u);assert(hearing->filter(f.actor,sizeof(f.actor),f.heard[0],sizeof(f.heard[0]),&hb,&hf)&&hf.accepted);
    f.put(f.senses,0x14,1);const unsigned heard_types[]={8,0x400,2,4,0x800,0x80000};
    for(int i=1;i<=6;++i){f.put(f.heard[i],0xc,heard_types[i-1]);f.put(f.heard[i],0x10,i==6?-1:i+1);}
    f.put(f.actor,0xc0c,500.f);f.put(f.actor,0x798,1u);f.put(f.schedule,0x10,1u<<16);
    f.put(f.actor,0x7d0,0u);ServerNpcGruntHearingResult hr{};
    assert(hearing->gather(f.actor,sizeof(f.actor),500,&hb,&hr)&&hr.visited==6&&hr.danger_suppressed==1&&hr.unknown==1&&hr.ignored==1&&!hr.special_suppressed&&hr.outputs==2);
    assert(f.hearing_offsets[0]==0xb20&&f.hearing_offsets[1]==0xb38&&(f.get<unsigned>(0x7d0)&(1u<<25))&&(f.get<unsigned>(0x7d0)&(1u<<23)));
    f.hearing_outputs=0;
    assert(hearing->gather(f.actor,sizeof(f.actor),500.01f,&hb,&hr)&&hr.special_suppressed&&hr.outputs==3&&f.hearing_offsets[2]==0xb50);
    assert((f.get<unsigned>(0x7d0)&(1u<<16))&&!(f.get<unsigned>(0x7d0)&((1u<<23)|(1u<<25))));
    f.hearing_outputs=0;f.put(f.senses,0x14,0);f.put(f.heard[0],0xc,1u);f.put(f.heard[0],0x10,1);
    assert(hearing->gather(f.actor,sizeof(f.actor),501,&hb,&hr)&&hr.visited==1&&hr.outputs==1);
    f.put(f.senses,0x14,1);f.put(f.heard[1],0x10,1);assert(!hearing->gather(f.actor,sizeof(f.actor),501,&hb,&hr));
    auto gscan=reinterpret_cast<GetServerNpcGruntSoundScanApi>(GetProcAddress(s,server_npc_grunt_sound_scan_export));
    assert(gscan&&!GetProcAddress(c,server_npc_grunt_sound_scan_export)&&!gscan(0,sizeof(ServerNpcGruntSoundScanApi))&&!gscan(1,0));
    const auto* scan=gscan(1,sizeof(ServerNpcGruntSoundScanApi));assert(scan);
    ServerNpcGruntSoundScanBindings sb{};sb.hearing=hb;sb.entries=f.entries;sb.count=4;sb.context=&f;
    sb.active_head=[](void* p,int* head) noexcept -> int{*head=F::self(p).active_sound_head;return 1;};
    sb.sound_mutable=[](void* p,int index,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(index<0||index>=8)return 0;*out=f.heard[index];*n=sizeof(f.heard[index]);return 1;};
    sb.eye_position=[](void*,void*,float* pos) noexcept -> int{pos[0]=pos[1]=pos[2]=0;return 1;};
    auto sound_row=[&](int i,unsigned type,unsigned owner,float x,int radius,int next){
        std::memset(f.heard[i],0,sizeof(f.heard[i]));f.put(f.heard[i],0,owner);f.put(f.heard[i],4,radius);
        f.put(f.heard[i],0xc,type);f.put(f.heard[i],0x18,static_cast<std::int16_t>(next));f.put(f.heard[i],0x20,x);
    };
    sound_row(1,1,0x7001u,100,100,2);sound_row(2,8,0x8002u,20,100,3);
    sound_row(3,2,0x9003u,0,100,4);sound_row(4,4,0xffffffffu,101,100,5);
    sound_row(5,0x200,0xffffffffu,0,100,6);sound_row(6,0x4000004,0xffffffffu,0,100,7);
    sound_row(7,0x400,0xffffffffu,0,100,-1);
    f.put(f.actor,0x850,3);f.put(f.actor,0xb4,0u);f.actor[0x968]=0;f.hearing_outputs=0;
    ServerNpcGruntSoundScanResult ssr{};
    assert(scan->scan(f.actor,sizeof(f.actor),510,&sb,&ssr)&&ssr.visited==7&&ssr.accepted==3&&ssr.accepted_head==7&&ssr.self_rejected==1&&ssr.mask_rejected==1&&ssr.range_rejected==1&&ssr.filter_rejected==1);
    int link=0;std::memcpy(&link,f.heard[7]+0x10,4);assert(link==2);std::memcpy(&link,f.heard[2]+0x10,4);assert(link==1);std::memcpy(&link,f.heard[1]+0x10,4);assert(link==-1);
    assert(ssr.hearing.visited==3&&ssr.hearing.special_suppressed);
    f.hearing_outputs=0;f.put(f.actor,0x850,4);
    assert(scan->scan(f.actor,sizeof(f.actor),511,&sb,&ssr)&&ssr.state_rejected==1&&ssr.accepted==2&&!ssr.hearing.special_suppressed);
    f.hearing_outputs=0;f.actor[0x968]=1;
    assert(scan->scan(f.actor,sizeof(f.actor),512,&sb,&ssr)&&!ssr.accepted&&ssr.disabled_rejected==4&&!ssr.hearing.visited);
    f.actor[0x968]=0;f.put(f.actor,0xb4,1u);
    assert(scan->scan(f.actor,sizeof(f.actor),513,&sb,&ssr)&&ssr.blocked&&!ssr.visited&&!ssr.hearing.visited);
    f.put(f.actor,0xb4,0u);f.put(f.heard[7],0x18,std::int16_t(1));
    assert(!scan->scan(f.actor,sizeof(f.actor),514,&sb,&ssr));
    f.put(f.heard[7],0x18,std::int16_t(-1));
    auto gtick=reinterpret_cast<GetServerNpcGruntPerceptionTickApi>(GetProcAddress(s,server_npc_grunt_perception_tick_export));
    assert(gtick&&!GetProcAddress(c,server_npc_grunt_perception_tick_export)&&!gtick(0,sizeof(ServerNpcGruntPerceptionTickApi))&&!gtick(1,0));
    const auto* tick=gtick(1,sizeof(ServerNpcGruntPerceptionTickApi));assert(tick);
    ServerNpcGruntPerceptionTickBindings ptb;ptb.visual=vb;ptb.sound=sb;ptb.maintenance=maintenance;ptb.enemy_switch=swb;ptb.context=&f;
    ptb.enemy_switch.best.sensed=nullptr;ptb.enemy_switch.best.visible=nullptr; // actual sensed vectors drive visibility
    ptb.rebuild_category=[](void* p,void* senses,unsigned category,int radius) noexcept -> int{
        auto& f=F::self(p);assert(senses==f.senses&&category==f.rebuild_calls%3&&radius==f.rebuild_radius);++f.rebuild_calls;return 1;
    };
    f.put(f.sense_handles[1],0,0x7001u);f.put(f.sense_vectors[2],0xc,0);f.put(f.senses,8,100.75f);
    relation_row(0,0x7001u,0,1,1);relation_row(1,0x8002u,0,1,5);
    f.actor[0xdb1]=0;f.rel_special=false;f.put(f.actor,0x850,3);f.put(f.actor,0x798,0u);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x988,0u);
    f.active_sound_head=-1;f.hearing_outputs=0;f.put(f.actor,0x920,0);f.put(f.actor,0xc18,0u);
    ServerNpcGruntPerceptionTickResult ptr;
    assert(tick->tick(f.actor,sizeof(f.actor),520,&ptb,&ptr)&&ptr.completed&&ptr.rebuilt&&ptr.categories==3&&ptr.radius==100&&ptr.visual.report_calls==2&&ptr.maintenance.visited==2&&!ptr.maintenance.removed&&ptr.enemy_switch.assigned&&ptr.enemy_switch.assignment.new_handle==0x8002u&&f.rebuild_calls==3);
    assert(ptr.enemy_switch.selection.selection.visible&&f.get<unsigned>(0x8a0)==0x8002u);
    assert(tick->tick(f.actor,sizeof(f.actor),520,&ptb,&ptr)&&ptr.completed&&!ptr.rebuilt&&!ptr.categories&&ptr.visual.report_calls==2&&f.rebuild_calls==3);
    f.put(f.senses,8,101.5f);f.rebuild_radius=101;
    assert(tick->tick(f.actor,sizeof(f.actor),520,&ptb,&ptr)&&ptr.completed&&ptr.rebuilt&&ptr.radius==101&&f.rebuild_calls==6);
    assert(tick->tick(f.actor,sizeof(f.actor),521,&ptb,&ptr)&&ptr.completed&&ptr.rebuilt&&f.rebuild_calls==9);
    auto gcv=reinterpret_cast<GetServerNpcGruntCombatVisibilityApi>(GetProcAddress(s,server_npc_grunt_combat_visibility_export));
    assert(gcv&&!GetProcAddress(c,server_npc_grunt_combat_visibility_export)&&!gcv(0,sizeof(ServerNpcGruntCombatVisibilityApi))&&!gcv(1,0));
    const auto* cv=gcv(1,sizeof(ServerNpcGruntCombatVisibilityApi));assert(cv);
    ServerNpcGruntCombatVisibilityBindings cvb{};cvb.context=&f;cvb.original_vtable=0x2249aaa4;cvb.memory=bb.memory;cvb.conditions=eb.conditions;cvb.extent=bb.extent;cvb.entity_handle=eb.entity_handle;cvb.special_target=rb.special_target;
    cvb.trace=[](void* p,void* a,void* e,unsigned mask,bool* yes,void** blocker) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&e==f.enemy&&mask==0x4081);*yes=f.combat_trace;*blocker=*yes?nullptr:f.replacement;return 1;};
    cvb.in_view=[](void* p,void*,void*,bool* yes) noexcept -> int{*yes=F::self(p).combat_fov;return 1;};
    cvb.fire_output=[](void* p,void* a,unsigned off,bool value,std::uint32_t handle) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.combat_output_count<8);auto i=f.combat_output_count++;f.combat_output_offsets[i]=off;f.combat_output_value[i]=value;f.combat_output_handles[i]=handle;return 1;};
    f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x988,0x8000u);f.put(f.actor,0x7cc,0xffffffffu);f.actor[0xb6]=0;
    f.rel_special=true;f.combat_fov=false;
    ServerNpcGruntCombatVisibilityResult cvr{};
    assert(cv->gather(f.actor,sizeof(f.actor),f.enemy,&cvb,&cvr)&&cvr.visible&&!cvr.in_view&&!cvr.previous_visible&&cvr.outputs==2);
    assert(f.combat_output_offsets[0]==0xad8&&f.combat_output_offsets[1]==0xa90&&f.combat_output_value[0]&&f.combat_output_handles[1]==0x7001u);
    assert((f.get<unsigned>(0x7cc)&((1u<<14)|(1u<<10)|(1u<<12)|(1u<<27)|(1u<<28)))==(1u<<14)&&f.get<unsigned>(0x988)==0x28000u);
    f.combat_fov=true;assert(cv->gather(f.actor,sizeof(f.actor),f.enemy,&cvb,&cvr)&&cvr.visible&&cvr.in_view&&cvr.previous_visible&&!cvr.outputs);
    f.combat_trace=false;assert(cv->gather(f.actor,sizeof(f.actor),f.enemy,&cvb,&cvr)&&!cvr.visible&&cvr.previous_visible&&cvr.outputs==2&&cvr.blocker==0x8002u);
    assert(f.combat_output_offsets[2]==0xaf0&&f.combat_output_offsets[3]==0xaa8&&!f.combat_output_value[2]&&f.get<unsigned>(0x98c)==0x8002u&&f.get<unsigned>(0x988)==0x8000u);
    f.put(f.enemy,0x8c,1);assert(cv->gather(f.actor,sizeof(f.actor),f.enemy,&cvb,&cvr)&&cvr.dead&&!cvr.outputs);
    assert((f.get<unsigned>(0x7cc)&((1u<<29)|(1u<<12)|(1u<<10)))==(1u<<29));f.put(f.enemy,0x8c,0);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryMarkEluded,0,0,&mb,&ctr)&&ctr.written);
    f.actor[0xb6]=1;f.combat_trace=true;assert(cv->query(f.actor,sizeof(f.actor),f.enemy,0x4081,&cvb,&cvr)&&cvr.suppressed&&!cvr.traced&&!cvr.visible);
    f.actor[0xb6]=0;assert(cv->query(f.actor,sizeof(f.actor),f.enemy,0x4081,&cvb,&cvr)&&cvr.traced&&cvr.visible);
    auto gcrange=reinterpret_cast<GetServerNpcGruntCombatRangeApi>(GetProcAddress(s,server_npc_grunt_combat_range_export));
    assert(gcrange&&!GetProcAddress(c,server_npc_grunt_combat_range_export)&&!gcrange(0,sizeof(ServerNpcGruntCombatRangeApi))&&!gcrange(1,0));
    const auto* crange=gcrange(1,sizeof(ServerNpcGruntCombatRangeApi));assert(crange);
    ServerNpcGruntCombatRangeBindings crb{};crb.context=&f;crb.visibility=cvb;crb.report=report_binding;
    // The legacy BestEnemy extent leaf only exposes 0xc0; combat needs the actual fixture extents.
    crb.visibility.extent=[](void* p,void* e,unsigned* n) noexcept -> int{auto& f=F::self(p);if(e==f.enemy)*n=sizeof(f.enemy);else if(e==f.replacement)*n=sizeof(f.replacement);else return 0;return 1;};
    crb.visibility.memory.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);switch(token){case 100:*out=f.nodes;*n=sizeof(f.nodes);return 1;case 101:*out=f.record;*n=sizeof(f.record);return 1;case 102:*out=f.fallback;*n=sizeof(f.fallback);return 1;case 103:*out=f.memory;*n=sizeof(f.memory);return 1;case 700:*out=f.combat_nav;*n=sizeof(f.combat_nav);return 1;default:return 0;}
    };
    crb.center=[](void* p,void* e,float* v) noexcept -> int{auto& f=F::self(p);if(e==f.actor)v[0]=v[1]=v[2]=0;else {assert(e==f.enemy);std::memcpy(v,f.combat_center,12);}return 1;};
    crb.origin=[](void* p,void*,float* v) noexcept -> int{std::memcpy(v,F::self(p).combat_origin,12);return 1;};
    crb.velocity=[](void* p,void*,float* v) noexcept -> int{std::memcpy(v,F::self(p).combat_velocity,12);return 1;};
    crb.random=[](void*,float lo,float hi,float* v) noexcept -> int{assert(lo==-.05f&&hi==0.f);*v=-.05f;return 1;};
    crb.enemy_facing_actor=[](void* p,void* e,void* a,bool* yes) noexcept -> int{auto& f=F::self(p);assert(e==f.enemy&&a==f.actor);*yes=f.combat_facing;return 1;};
    f.put(f.actor,0x10c,0.f);f.put(f.actor,0x118,60.f);f.put(f.enemy,0x10c,0.f);f.put(f.enemy,0x118,40.f);
    f.combat_center[0]=30;f.combat_center[1]=40;f.combat_center[2]=50;
    float combat_distance_value=0;assert(crange->distance(f.actor,sizeof(f.actor),f.enemy,&crb,&combat_distance_value)&&combat_distance_value==50);
    f.combat_center[2]=170;assert(crange->distance(f.actor,sizeof(f.actor),f.enemy,&crb,&combat_distance_value)&&combat_distance_value==130);
    f.combat_center[2]=-170;assert(crange->distance(f.actor,sizeof(f.actor),f.enemy,&crb,&combat_distance_value)&&combat_distance_value==130);
    f.combat_center[0]=100;f.combat_center[1]=f.combat_center[2]=0;
    f.put(f.actor,0x988,0x28000u);f.put(f.actor,0x8f8,700u);f.put(f.actor,0x78c,0x8002u);
    f.put(f.actor,0xa20,100.f);f.put(f.replacement,0x500,150.f);f.combat_velocity[0]=100;
    f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);f.combat_trace=f.combat_fov=true;
    ServerNpcGruntCombatRangeResult crr{};
    assert(crange->gather(f.actor,sizeof(f.actor),f.enemy,530,&crb,&crr)&&crr.distance==100&&crr.threshold==150&&crr.weapon_range_used&&crr.attack_gate&&crr.firsthand&&crr.velocity_queries==2&&crr.observation[0]==75&&crr.facing_actor);
    assert((f.get<unsigned>(0x7cc)&((1u<<27)|(1u<<28)|(1u<<26)))==(1u<<27));
    f.put(f.replacement,0x500,80.f);f.combat_facing=false;f.combat_velocity[0]=0;
    assert(crange->gather(f.actor,sizeof(f.actor),f.enemy,531,&crb,&crr)&&crr.threshold==100&&crr.too_far&&!crr.attack_gate&&!crr.weapon_range_used&&crr.velocity_queries==1);
    assert((f.get<unsigned>(0x7cc)&((1u<<27)|(1u<<28)|(1u<<26)))==((1u<<28)|(1u<<26)));
    f.combat_center[0]=50;f.put(f.combat_nav,0xc,1);
    assert(crange->gather(f.actor,sizeof(f.actor),f.enemy,532,&crb,&crr)&&!crr.too_far&&!crr.attack_gate);
    f.put(f.combat_nav,0xc,0);f.combat_fov=false;f.combat_center[0]=256;
    assert(crange->gather(f.actor,sizeof(f.actor),f.enemy,533,&crb,&crr)&&crr.reported&&!crr.firsthand&&!crr.velocity_queries&&!crr.weapon_range_used);
    f.combat_center[0]=256.01f;
    assert(crange->gather(f.actor,sizeof(f.actor),f.enemy,534,&crb,&crr)&&!crr.reported);
    f.combat_trace=false;f.combat_center[0]=1;
    assert(crange->gather(f.actor,sizeof(f.actor),f.enemy,535,&crb,&crr)&&!crr.reported);
    f.put(f.enemy,0x8c,1);assert(crange->gather(f.actor,sizeof(f.actor),f.enemy,536,&crb,&crr)&&crr.dead&&!crr.reported&&!crr.attack_gate);
    f.put(f.enemy,0x8c,0);
    auto gattack=reinterpret_cast<GetServerNpcGruntAttackApi>(GetProcAddress(s,server_npc_grunt_attack_conditions_export));
    assert(gattack&&!GetProcAddress(c,server_npc_grunt_attack_conditions_export)&&!gattack(0,sizeof(ServerNpcGruntAttackApi))&&!gattack(1,0));
    const auto* attack=gattack(1,sizeof(ServerNpcGruntAttackApi));assert(attack);
    ServerNpcGruntAttackBindings ab{};ab.context=&f;ab.original_vtable=0x2249aaa4;ab.entries=f.entries;ab.count=3;ab.conditions=eb.conditions;
    ab.origin=crb.center;
    ab.forward=[](void* p,void*,float* v) noexcept -> int{std::memcpy(v,F::self(p).attack_forward,12);return 1;};
    ab.aim_point=[](void*,void*,const float*,float* v) noexcept -> int{v[0]=10;v[1]=v[2]=0;return 1;};
    ab.eye=[](void*,void*,float* v) noexcept -> int{v[0]=20;v[1]=v[2]=0;return 1;};
    ab.line=[](void* p,void*,const float*,const float* target,bool* clear) noexcept -> int{auto& f=F::self(p);++f.attack_line_calls;*clear=f.attack_line_mode==0||(f.attack_line_mode==1&&target[0]==20);return 1;};
    ab.weapon_caps=[](void* p,void* w,unsigned* caps) noexcept -> int{auto& f=F::self(p);assert(w==f.replacement);*caps=f.attack_weapon_caps;return 1;};
    ab.weapon_condition=[](void* p,void* w,unsigned slot,float dot,float dist,int* id) noexcept -> int{
        auto& f=F::self(p);assert(w==f.replacement&&dot==1&&dist==100);switch(slot){case 0x4ac:*id=0x14;break;case 0x4b0:*id=0x15;break;case 0x4b4:*id=0x16;break;case 0x4b8:*id=0x17;break;default:return 0;}return 1;
    };
    ab.melee_trace=[](void* p,void* a,void* e,unsigned mask,void** hit) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&e==f.enemy&&mask==0x46004003u);*hit=f.melee_hits?e:nullptr;return 1;};
    f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x8ec,0x20000u);
    f.combat_center[0]=50;f.combat_center[1]=f.combat_center[2]=0;f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);
    ServerNpcGruntAttackResult ar{};
    assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,63,&ab,&ar)&&ar.conditions[0]==0x24&&ar.dot==1&&ar.native_queries==1);
    assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,64,&ab,&ar)&&ar.conditions[0]==0x14&&ar.line_clear&&ar.range_cleared==1);
    f.attack_line_mode=2;assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,100,&ab,&ar)&&ar.suppressed==1&&ar.line_calls==2&&ar.cleanup_calls==2&&!(f.get<unsigned>(0x7cc)&(1u<<20)));
    f.attack_line_mode=1;assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,784,&ab,&ar)&&ar.conditions[0]==0x14&&ar.line_clear&&ar.line_calls==2);
    f.attack_line_mode=0;assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,785,&ab,&ar)&&ar.conditions[0]==0x25);
    f.put(f.actor,0x8ec,0x180000u);f.combat_center[2]=64;
    assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,64,&ab,&ar)&&ar.conditions[2]==0x16&&ar.conditions[3]==0x17&&ar.melee_traced);
    f.combat_center[2]=65;assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,64,&ab,&ar)&&ar.conditions[2]==0&&!ar.melee_traced);
    f.combat_center[2]=0;f.melee_hits=false;assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,64,&ab,&ar)&&ar.conditions[2]==0&&ar.melee_traced);
    f.put(f.actor,0x78c,0x8002u);f.put(f.actor,0x8ec,0x2000u);f.attack_weapon_caps=0x1c000u;
    assert(attack->gather(f.actor,sizeof(f.actor),f.enemy,100,&ab,&ar)&&ar.capabilities==0x1e000u&&ar.weapon_queries==4&&!ar.native_queries&&ar.conditions[0]==0x14&&ar.conditions[1]==0x15&&ar.conditions[2]==0x16&&ar.conditions[3]==0x17);
    auto gcombat=reinterpret_cast<GetServerNpcGruntCombatConditionsApi>(GetProcAddress(s,server_npc_grunt_combat_conditions_export));
    assert(gcombat&&!GetProcAddress(c,server_npc_grunt_combat_conditions_export)&&!gcombat(0,sizeof(ServerNpcGruntCombatConditionsApi))&&!gcombat(1,0));
    const auto* combat=gcombat(1,sizeof(ServerNpcGruntCombatConditionsApi));assert(combat);
    ServerNpcGruntCombatConditionsBindings ccb;ccb.range=crb;ccb.attack=ab;ccb.cache=ub;ccb.context=&f;
    ccb.range.origin=[](void* p,void* e,float* v) noexcept -> int{auto& f=F::self(p);if(e==f.actor)v[0]=v[1]=v[2]=0;else {assert(e==f.enemy);std::memcpy(v,f.combat_origin,12);}return 1;};
    ccb.navigation_update=[](void* p,void* a,unsigned phase,float) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&phase==f.combat_nav_calls%2);++f.combat_nav_calls;return 1;};
    ccb.point_visible=[](void* p,void* a,const float*,unsigned mask,bool* yes) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&mask==0x4081);*yes=f.remembered_visible;return 1;};
    f.combat_output_count=0;f.combat_trace=f.combat_fov=true;f.combat_center[0]=64;f.combat_center[2]=0;f.combat_origin[0]=47;
    f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x8ec,0x20000u);f.put(f.actor,0x920,0);f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);
    f.put(f.actor,0x988,0x8000u);f.put(f.actor,0x8a0,0x7001u);
    ServerNpcGruntCombatConditionsResult ccr;
    assert(combat->gather(f.actor,sizeof(f.actor),f.enemy,540,&ccb,&ccr)&&ccr.completed&&ccr.attack_called&&ccr.attack.conditions[0]==0x14&&ccr.nav_calls==2&&!ccr.old_sight);
    // Original update thresholds permit remembered location to lag a small report.
    assert(mem->lookup(f.memory,sizeof(f.memory),1000,false,&mb,&ml)&&ml.record);
    void* current_record=nullptr;unsigned current_record_bytes=0;assert(mb.storage(mb.context,ml.record,&current_record,&current_record_bytes)&&current_record_bytes>=0x38);
    f.put(current_record,4,47.f);f.put(current_record,8,0.f);f.put(current_record,12,0.f);
    f.combat_trace=false;f.combat_center[0]=300;
    assert(combat->gather(f.actor,sizeof(f.actor),f.enemy,548,&ccb,&ccr)&&ccr.completed&&!ccr.old_sight&&ccr.cleanup_called&&!ccr.attack_called);
    assert(combat->gather(f.actor,sizeof(f.actor),f.enemy,548.01f,&ccb,&ccr)&&ccr.old_sight&&ccr.close_mark&&ccr.horizontal_distance==47&&!ccr.point_queries);
    assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryEluded,0,0,&mb,&ctr)&&ctr.value==1);
    assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,550,0,true,&mutation,&mut));
    f.put(current_record,4,48.f);f.put(current_record,8,0.f);f.put(current_record,12,0.f);
    assert(combat->gather(f.actor,sizeof(f.actor),f.enemy,559,&ccb,&ccr)&&ccr.old_sight&&ccr.horizontal_distance==48&&!ccr.close_mark&&!ccr.point_queries);
    f.put(f.actor,0x920,1);unreachable_row(0,0x7001u,600,f.distance[0]);f.put(f.actor,0x7cc,0u);
    assert(combat->gather(f.actor,sizeof(f.actor),f.enemy,559,&ccb,&ccr)&&ccr.unreachable_set&&ccr.point_queries==1&&ccr.point_mark);
    f.combat_nav[0x3a]=1;f.put(f.actor,0x7cc,0u);
    assert(combat->gather(f.actor,sizeof(f.actor),f.enemy,559,&ccb,&ccr)&&!ccr.unreachable_set&&!ccr.point_queries);
    f.put(f.enemy,0x8c,1);const unsigned before_nav=f.combat_nav_calls;
    assert(combat->gather(f.actor,sizeof(f.actor),f.enemy,560,&ccb,&ccr)&&ccr.completed&&ccr.range.dead&&!ccr.nav_calls&&f.combat_nav_calls==before_nav);
    auto ggoal=reinterpret_cast<GetServerNpcGruntGoalRefreshApi>(GetProcAddress(s,server_npc_grunt_goal_refresh_export));
    assert(ggoal&&!GetProcAddress(c,server_npc_grunt_goal_refresh_export)&&!ggoal(0,sizeof(ServerNpcGruntGoalRefreshApi))&&!ggoal(1,0));
    const auto* goal=ggoal(1,sizeof(ServerNpcGruntGoalRefreshApi));assert(goal);
    ServerNpcGruntGoalRefreshBindings goal_bindings{};goal_bindings.memory=crb.visibility.memory;goal_bindings.context=&f;goal_bindings.original_vtable=0x2249aaa4;
    float global_deadline=-1;goal_bindings.global_deadline=&global_deadline;goal_bindings.origin=ccb.range.origin;goal_bindings.velocity=crb.velocity;goal_bindings.special=rb.special_target;
    goal_bindings.memory.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);switch(token){case 100:*out=f.nodes;*n=sizeof(f.nodes);return 1;case 101:*out=f.record;*n=sizeof(f.record);return 1;case 102:*out=f.fallback;*n=sizeof(f.fallback);return 1;case 103:*out=f.memory;*n=sizeof(f.memory);return 1;case 700:*out=f.combat_nav;*n=sizeof(f.combat_nav);return 1;case 701:*out=f.goal_path;*n=sizeof(f.goal_path);return 1;default:return 0;}
    };
    goal_bindings.retarget=[](void* p,std::uint32_t token,void* e) noexcept -> int{auto& f=F::self(p);assert(token==700);++f.goal_retargets;f.goal_target=e;return 1;};
    goal_bindings.refresh=[](void* p,std::uint32_t token,bool* yes) noexcept -> int{auto& f=F::self(p);assert(token==700);++f.goal_refreshes;*yes=f.goal_success;return 1;};
    goal_bindings.failure=[](void* p,void* a,int id) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&id==11);++f.goal_failures;return 1;};
    f.put(f.enemy,0x8c,0);f.put(f.actor,0x8c8,0.f);f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x8a8,0x7001u);
    f.put(f.combat_nav,0xc,1);f.put(f.combat_nav,0x24,701u);
    f.put(f.goal_path,0x44,2u);f.put(f.goal_path,0x10,0x8002u);
    ServerNpcGruntGoalRefreshResult gr{};
    assert(goal->update(f.actor,sizeof(f.actor),0,570,&goal_bindings,&gr)&&gr.skipped_mode);
    f.put(f.combat_nav,0xc,0);global_deadline=571;
    assert(goal->update(f.actor,sizeof(f.actor),0,570,&goal_bindings,&gr)&&gr.skipped_timer);
    assert(goal->update(f.actor,sizeof(f.actor),0,571,&goal_bindings,&gr)&&gr.retargeted&&f.goal_target==f.enemy);
    f.put(f.goal_path,0x10,0x7001u);f.put(current_record,4,100.f);f.put(current_record,8,0.f);f.put(current_record,12,0.f);
    f.rel_special=true;f.combat_velocity[0]=100;f.put(f.goal_path,0x68,0.f);f.put(f.goal_path,0x34,220.f);
    assert(goal->update(f.actor,sizeof(f.actor),0,571,&goal_bindings,&gr)&&gr.threshold==120&&gr.deviation==120&&!gr.refreshed);
    f.put(f.goal_path,0x34,221.f);f.goal_success=false;
    assert(goal->update(f.actor,sizeof(f.actor),0,571,&goal_bindings,&gr)&&gr.refreshed&&gr.failed&&gr.deadline_written&&f.goal_failures==1);
    assert(goal->update(f.actor,sizeof(f.actor),0,571,&goal_bindings,&gr)&&gr.skipped_timer);
    f.put(f.goal_path,0x44,1u);f.put(f.goal_path,0x48,0u);
    assert(goal->update(f.actor,sizeof(f.actor),1,571,&goal_bindings,&gr)&&!gr.threshold_called&&!gr.refreshed);
    f.put(f.goal_path,0x48,2u);f.combat_origin[0]=100;f.goal_success=true;
    assert(goal->update(f.actor,sizeof(f.actor),1,571,&goal_bindings,&gr)&&gr.refreshed&&!gr.deadline_written&&f.goal_refreshes==2);
    f.put(f.actor,0x8a8,0xffffffffu);f.put(f.goal_path,0x10,0xffffffffu);
    assert(goal->update(f.actor,sizeof(f.actor),1,571,&goal_bindings,&gr)&&gr.refreshed&&!gr.threshold_called&&f.goal_refreshes==3);
    auto gcgoal=reinterpret_cast<GetServerNpcGruntCombatGoalApi>(GetProcAddress(s,server_npc_grunt_combat_goal_export));
    assert(gcgoal&&!GetProcAddress(c,server_npc_grunt_combat_goal_export)&&!gcgoal(0,sizeof(ServerNpcGruntCombatGoalApi))&&!gcgoal(1,0));
    const auto* cgoal=gcgoal(1,sizeof(ServerNpcGruntCombatGoalApi));assert(cgoal);
    ccb.navigation_update=nullptr;ccb.range.visibility.memory.storage=goal_bindings.memory.storage;
    f.combat_trace=f.combat_fov=true;f.combat_center[0]=64;f.combat_velocity[0]=0;
    f.put(f.actor,0x988,0x28000u);f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x78c,0xffffffffu);
    f.put(f.actor,0x8ec,0x20000u);f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);f.put(f.actor,0x920,0);
    f.put(f.goal_path,0x44,2u);f.put(f.goal_path,0x10,0x8002u);global_deadline=0;
    ServerNpcGruntGoalRefreshResult goals[2]{};
    assert(cgoal->gather(f.actor,sizeof(f.actor),f.enemy,580,&ccb,&goal_bindings,&ccr,goals)&&ccr.completed&&ccr.attack_called&&ccr.nav_calls==2&&goals[0].retargeted&&goals[1].skipped_type&&f.goal_target==f.enemy);
    f.put(f.goal_path,0x10,0x7001u);f.put(f.goal_path,0x34,400.f);
    assert(cgoal->gather(f.actor,sizeof(f.actor),f.enemy,581,&ccb,&goal_bindings,&ccr,goals)&&ccr.completed&&goals[0].refreshed&&goals[0].deadline_written&&goals[1].skipped_type);
    f.put(f.enemy,0x8c,1);
    assert(cgoal->gather(f.actor,sizeof(f.actor),f.enemy,582,&ccb,&goal_bindings,&ccr,goals)&&ccr.range.dead&&!ccr.nav_calls&&!goals[0].refreshed&&!goals[1].skipped_type);
    f.put(f.enemy,0x8c,0);
    {
        auto get_search=reinterpret_cast<GetServerNpcGruntWeaponSearchApi>(GetProcAddress(s,server_npc_grunt_weapon_search_export));
        assert(get_search&&!GetProcAddress(c,server_npc_grunt_weapon_search_export)&&!get_search(0,sizeof(ServerNpcGruntWeaponSearchApi))&&!get_search(1,0));
        const auto* search=get_search(1,sizeof(ServerNpcGruntWeaponSearchApi));assert(search);
        ServerNpcGruntWeaponSearchBindings search_bindings{};search_bindings.relation=rb;search_bindings.schedule=b.schedule;search_bindings.context=&f;search_bindings.storage=goal_bindings.memory.storage;
        search_bindings.local_player=[](void* p,void** out) noexcept -> int{*out=F::self(p).enemy;return 1;};
        search_bindings.weapon_caps=ab.weapon_caps;
        search_bindings.find_weapon=[](void* p,void* a,const float* size,void** out) noexcept -> int{
            auto& f=F::self(p);assert(a==f.actor&&size[0]==540&&size[1]==540&&size[2]==100);*out=f.replacement;return 1;
        };
        f.put(f.actor,0x9a4,600.f);f.put(f.actor,0x8ec,0x200000u);f.put(f.actor,0x78c,0xffffffffu);
        f.put(f.actor,0x798,0u);f.put(f.actor,0x9c4,0u);f.actor[0x968]=0;
        ServerNpcGruntWeaponSearchResult search_result{};
        assert(search->search(f.actor,sizeof(f.actor),599,&search_bindings,&search_result)&&search_result.cooldown_blocked);
        assert(search->search(f.actor,sizeof(f.actor),600,&search_bindings,&search_result)&&search_result.eligible&&search_result.found&&search_result.condition_set&&search_result.deadline==602);
        assert(f.get<unsigned>(0x7d0)&(1u<<12));
        f.put(f.actor,0x78c,0x8002u);f.put(f.actor,0x850,3);
        assert(search->eligible(f.actor,sizeof(f.actor),602,&search_bindings,&search_result)&&search_result.combat_blocked);
        f.put(f.actor,0x850,1);
        assert(search->eligible(f.actor,sizeof(f.actor),602,&search_bindings,&search_result)&&search_result.armed_blocked);
        search_bindings.max_clients=1;f.put(f.actor,0x644,2);relation_row(0,0x7001u,0,3,0);f.actor[0xdb1]=0;
        assert(search->eligible(f.actor,sizeof(f.actor),602,&search_bindings,&search_result)&&search_result.eligible&&search_result.friendly);
        f.actor[0x968]=1;
        assert(search->eligible(f.actor,sizeof(f.actor),602,&search_bindings,&search_result)&&search_result.disabled);
        f.actor[0x968]=0;f.put(f.actor,0x798,1u);f.put(f.desc,4,npc_global_condition_base);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.actor,0x79c,npc_global_condition_base+0x3d);
        assert(search->eligible(f.actor,sizeof(f.actor),602,&search_bindings,&search_result)&&search_result.schedule_blocked);
        f.put(f.actor,0x798,0u);f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x8ec,0u);
        assert(search->eligible(f.actor,sizeof(f.actor),602,&search_bindings,&search_result)&&search_result.capability_blocked);
    }
    {
        auto get_aux=reinterpret_cast<GetServerNpcGruntAuxConditionsApi>(GetProcAddress(s,server_npc_grunt_aux_conditions_export));
        assert(get_aux&&!GetProcAddress(c,server_npc_grunt_aux_conditions_export)&&!get_aux(0,sizeof(ServerNpcGruntAuxConditionsApi))&&!get_aux(1,0));
        const auto* aux=get_aux(1,sizeof(ServerNpcGruntAuxConditionsApi));assert(aux);
        ServerNpcGruntAuxConditionsBindings aux_bindings{};aux_bindings.visibility=crb.visibility;aux_bindings.goals=goal_bindings;aux_bindings.context=&f;
        aux_bindings.weapon_predicate=[](void* p,void* w,unsigned slot,bool* yes) noexcept -> int{
            auto& f=F::self(p);assert(w==f.replacement);*yes=slot==0x43c;return slot==0x330||slot==0x334||slot==0x43c;
        };
        aux_bindings.as_npc=[](void*,void* e,void** npc) noexcept -> int{*npc=e;return 1;};
        f.put(f.actor,0x8a8,0x7001u);f.put(f.goal_path,0x44,2u);f.actor[0xb6]=0;f.combat_trace=true;
        ServerNpcGruntAuxConditionsResult aux_result{};
        f.put(f.actor,0x7cc,0xffffffffu);
        assert(aux->target(f.actor,sizeof(f.actor),610,&aux_bindings,&aux_result)&&aux_result.target_live&&aux_result.target_visible&&aux_result.target_condition==0xf&&aux_result.goal.skipped_type);
        assert((f.get<unsigned>(0x7cc)&((1u<<13)|(1u<<15)))==(1u<<15));
        f.combat_trace=false;assert(aux->target(f.actor,sizeof(f.actor),610,&aux_bindings,&aux_result)&&aux_result.target_condition==0xd);
        assert((f.get<unsigned>(0x7cc)&((1u<<13)|(1u<<15)))==(1u<<13));
        f.put(f.actor,0x8a8,0xffffffffu);assert(aux->target(f.actor,sizeof(f.actor),610,&aux_bindings,&aux_result)&&!aux_result.target_live);
        f.put(f.actor,0x78c,0x8002u);f.put(f.replacement,0x4e4,0);f.put(f.actor,0x7cc,0u);
        assert(aux->ammo(f.actor,sizeof(f.actor),&aux_bindings,&aux_result)&&aux_result.primary_empty&&aux_result.secondary_empty&&aux_result.secondary_uses_ammo);
        assert((f.get<unsigned>(0x7cc)&0x30u)==0x30u);
        f.put(f.replacement,0x4e4,-1);f.put(f.actor,0x7cc,0u);
        assert(aux->ammo(f.actor,sizeof(f.actor),&aux_bindings,&aux_result)&&!aux_result.primary_checked&&aux_result.secondary_empty&&f.get<unsigned>(0x7cc)==0x20u);
        f.put(f.actor,0x78c,0xffffffffu);assert(aux->ammo(f.actor,sizeof(f.actor),&aux_bindings,&aux_result)&&!aux_result.weapon_live&&f.get<unsigned>(0x7cc)==0x20u);
        f.put(f.actor,0x850,4);f.put(f.actor,0x970,0x8002u);f.put(f.replacement,0x3d0,0x7001u);f.replacement[0x3cc]=1;
        f.put(f.enemy,0x7cc,0x30000u);f.put(f.enemy,0x8c,0);f.put(f.actor,0x7cc,0xffu);f.put(f.actor,0x82c,0x0fu);
        assert(aux->snapshot(f.actor,sizeof(f.actor),&aux_bindings,&aux_result)&&aux_result.snapshot&&aux_result.script_live&&!aux_result.script_clear);
        assert(f.get<unsigned>(0x80c)==0xffu&&f.get<unsigned>(0x7cc)==0xfu);
        f.put(f.enemy,0x8c,1);
        assert(aux->snapshot(f.actor,sizeof(f.actor),&aux_bindings,&aux_result)&&aux_result.script_clear==2);
        unsigned script_conditions=0;std::memcpy(&script_conditions,f.enemy+0x7cc,4);assert(script_conditions==0);
    }
    {
        auto get_post=reinterpret_cast<GetServerNpcGruntPostUpdateApi>(GetProcAddress(s,server_npc_grunt_post_update_export));
        assert(get_post&&!GetProcAddress(c,server_npc_grunt_post_update_export)&&!get_post(0,sizeof(ServerNpcGruntPostUpdateApi))&&!get_post(1,0));
        const auto* post=get_post(1,sizeof(ServerNpcGruntPostUpdateApi));assert(post);
        ServerNpcGruntPostUpdateBindings post_bindings{};post_bindings.schedule=b.schedule;post_bindings.combat=ccb;post_bindings.enemy=eb;
        post_bindings.aux.visibility=crb.visibility;post_bindings.aux.goals=goal_bindings;
        post_bindings.search.relation=rb;post_bindings.search.schedule=b.schedule;post_bindings.search.context=&f;
        post_bindings.flinch.schedule=b.schedule;post_bindings.flinch.context=&f;
        post_bindings.flinch.select_activity=[](void*,void*,bool heavy,int one,int* activity) noexcept -> int{assert(one==1);*activity=heavy?43:42;return 1;};
        post_bindings.flinch.can_animate=[](void*,void*,bool* yes) noexcept -> int{*yes=true;return 1;};
        post_bindings.flinch.activity_exists=[](void*,void*,int activity,bool* yes) noexcept -> int{*yes=activity==42||activity==43;return 1;};
        post_bindings.flinch.play_activity=[](void*,void*,int activity,int a,int b) noexcept -> int{return (activity==42||activity==43)&&a==1&&b==1;};
        post_bindings.flinch.random=[](void*,float lo,float hi,float* delay) noexcept -> int{assert(lo==.5f&&hi==1.f);*delay=.75f;return 1;};
        f.put(f.actor,0x798,1u);f.put(f.schedule,0xc,1u<<17);f.put(f.actor,0x7cc,1u<<17);f.put(f.actor,0x988,0u);
        ServerNpcGruntFlinchResult flinch_result{};
        assert(post->flinch(f.actor,sizeof(f.actor),620,&post_bindings.flinch,&flinch_result)&&flinch_result.heavy&&flinch_result.interrupted&&!flinch_result.called);
        f.put(f.schedule,0xc,0u);
        assert(post->flinch(f.actor,sizeof(f.actor),620,&post_bindings.flinch,&flinch_result)&&flinch_result.played&&flinch_result.timer_started&&flinch_result.deadline==620.75f);
        assert(post->flinch(f.actor,sizeof(f.actor),620.75f,&post_bindings.flinch,&flinch_result)&&flinch_result.heavy_cleared&&!flinch_result.timer_cleared);
        assert(post->flinch(f.actor,sizeof(f.actor),620.76f,&post_bindings.flinch,&flinch_result)&&flinch_result.timer_cleared);
        f.put(f.actor,0x850,1);f.put(f.actor,0x970,0xffffffffu);f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x8a8,0x7001u);
        f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x8ec,0u);f.put(f.actor,0xb4,0u);f.put(f.actor,0x9a4,700.f);f.put(f.enemy,0x8c,0);
        f.put(f.combat_nav,0xc,1);f.put(f.actor,0x988,0x28000u);f.combat_trace=f.combat_fov=true;f.combat_output_count=0;
        f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);f.put(f.schedule,0xc,1u<<25);
        for(unsigned i=0;i<8;++i)f.put(f.actor,0x82c+4*i,0xffffffffu);
        ServerNpcGruntPostUpdateResult post_result;
        assert(post->update(f.actor,sizeof(f.actor),621,true,&post_bindings,&post_result)&&post_result.completed&&post_result.search.cooldown_blocked&&post_result.new_enemy_condition&&post_result.combat_called&&post_result.target.target_visible&&post_result.snapshot.snapshot&&f.get<float>(0x9a0)==621);
        f.put(f.actor,0xb4,0x10u);f.condition(7);f.condition(48);
        assert(post->update(f.actor,sizeof(f.actor),622,false,&post_bindings,&post_result)&&post_result.completed&&post_result.skipped_perception&&post_result.cleared_sense_conditions>=2&&post_result.enemy_cleared&&!post_result.combat_called&&f.get<unsigned>(0x8a0)==0xffffffffu);
        f.put(f.actor,0x850,7);f.condition(1);
        assert(post->update(f.actor,sizeof(f.actor),623,false,&post_bindings,&post_result)&&post_result.completed&&post_result.skipped_state&&post_result.snapshot.snapshot&&!(f.get<unsigned>(0x80c)&2u));
    }
    {
        auto get_gate=reinterpret_cast<GetServerNpcGruntUpdateGateApi>(GetProcAddress(s,server_npc_grunt_update_gate_export));
        assert(get_gate&&!GetProcAddress(c,server_npc_grunt_update_gate_export)&&!get_gate(0,sizeof(ServerNpcGruntUpdateGateApi))&&!get_gate(1,0));
        const auto* update_gate=get_gate(1,sizeof(ServerNpcGruntUpdateGateApi));assert(update_gate);
        ServerNpcGruntUpdateGateBindings gate_bindings{};gate_bindings.context=&f;gate_bindings.original_vtable=0x2249aaa4;gate_bindings.conditions=eb.conditions;
        gate_bindings.storage.context=&f;
        gate_bindings.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            auto& f=F::self(p);if(token==710){*out=f.update_motor;*n=sizeof(f.update_motor);return 1;}if(token==711){*out=f.update_component;*n=sizeof(f.update_component);return 1;}return 0;
        };
        gate_bindings.pvs_index=[](void* p,unsigned index,bool* yes) noexcept -> int{assert(index==14);*yes=F::self(p).update_pvs;return 1;};
        gate_bindings.component_gate=[](void* p,std::uint32_t token,bool* yes) noexcept -> int{auto& f=F::self(p);assert(token==711&&f.update_component[8]==1);f.update_component[8]=f.update_component_handled;*yes=f.update_component_value;return 1;};
        gate_bindings.pre_perception=[](void* p,void* a) noexcept -> int{return a==F::self(p).actor;};
        gate_bindings.random_int=[](void* p,int lo,int hi,int* value) noexcept -> int{assert(lo==0&&hi==99);*value=F::self(p).update_roll;return 1;};
        gate_bindings.idle_speech=[](void* p,void* a,unsigned member,const char* cue,int priority,int type,int* result) noexcept -> int{
            assert(a==F::self(p).actor&&member==0xcc0&&!std::strcmp(cue,"GRUNT_IDLE")&&priority==0&&type==2);*result=3;return 1;
        };
        f.put(f.actor,0x850,7);f.actor[0x84c]=1;ServerNpcGruntUpdateGateResult gate_result{};
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&gate_result.inactive&&f.actor[0x84d]&&f.actor[0x84c]);
        f.put(f.actor,0x850,1);f.put(f.actor,0x908,710u);f.put(f.actor,0x370,90.f);f.put(f.update_motor,0x14,90.f);
        f.put(f.actor,0x988,0x2000u);f.put(f.actor,0xb4,0u);f.put(f.actor,8,0u);f.put(f.actor,0x16c,14u);f.put(f.actor,0xc18,0u);
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&gate_result.forced&&gate_result.yaw_flag_cleared&&gate_result.perceive&&gate_result.idle_emitted&&gate_result.speech_result==3&&!f.actor[0x84c]);
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&!gate_result.perceive&&!gate_result.forced);
        auto native_gate=gate_bindings;native_gate.idle_speech=nullptr;
        f.actor[0x84c]=1;f.put(f.actor,0x9c4,0u);
        assert(pspeech->prepare(f.actor,sizeof(f.actor),625,&native_gate,&psb,&gate_result,&psr)&&gate_result.idle_emitted&&gate_result.speech_result==-1&&psr.idle_calls==1&&!psr.idle.mode_allowed&&!psr.idle.queue_reset);
        f.update_pvs=true;f.update_roll=1;f.put(f.update_motor,0x14,91.f);f.put(f.actor,0x988,0x2000u);
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&gate_result.pvs&&gate_result.perceive&&!gate_result.idle_emitted&&!gate_result.yaw_flag_cleared);
        f.update_pvs=false;f.put(f.actor,0xc18,711u);f.update_component[8]=9;f.update_component_handled=true;f.update_component_value=true;
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&gate_result.component_called&&gate_result.component_handled&&gate_result.perceive&&f.update_component[8]==9);
        f.update_component_handled=false;f.put(f.actor,0xb4,0x400u);
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&!gate_result.component_handled&&gate_result.gate_value&&gate_result.perceive&&f.update_component[8]==9);
        f.put(f.actor,0xb4,0x10u);f.actor[0x84c]=1;
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&gate_result.disabled&&!gate_result.perceive&&!f.actor[0x84c]);
        f.put(f.actor,0xb4,0u);f.put(f.actor,0xc18,0u);f.put(f.actor,0x850,3);
        assert(update_gate->prepare(f.actor,sizeof(f.actor),&gate_bindings,&gate_result)&&gate_result.perceive&&!gate_result.idle_roll);
        auto get_full=reinterpret_cast<GetServerNpcGruntConditionUpdateApi>(GetProcAddress(s,server_npc_grunt_condition_update_export));
        assert(get_full&&!GetProcAddress(c,server_npc_grunt_condition_update_export)&&!get_full(0,sizeof(ServerNpcGruntConditionUpdateApi))&&!get_full(1,0));
        const auto* full=get_full(1,sizeof(ServerNpcGruntConditionUpdateApi));assert(full);
        ServerNpcGruntConditionUpdateBindings full_bindings{};
        full_bindings.gate=gate_bindings;full_bindings.perception=ptb;
        full_bindings.post.schedule=b.schedule;full_bindings.post.combat=ccb;full_bindings.post.enemy=eb;
        full_bindings.post.aux.visibility=crb.visibility;full_bindings.post.aux.goals=goal_bindings;
        full_bindings.post.search.relation=rb;
        f.put(f.actor,0x850,7);f.condition(1);
        ServerNpcGruntConditionUpdateResult full_result;
        assert(full->update(f.actor,sizeof(f.actor),630,&full_bindings,&full_result)&&full_result.completed&&full_result.gate.inactive&&!full_result.perception_called&&full_result.post.skipped_state&&full_result.post.snapshot.snapshot);
        f.put(f.actor,0x850,1);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x8a8,0xffffffffu);f.put(f.actor,0x798,0u);
        f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x988,0u);f.put(f.actor,0x9a4,700.f);f.put(f.actor,0xb4,0u);f.actor[0x84c]=0;
        f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);f.condition(7);
        assert(full->update(f.actor,sizeof(f.actor),630,&full_bindings,&full_result)&&full_result.completed&&!full_result.perception_called&&full_result.post.skipped_perception&&full_result.post.cleared_sense_conditions==1);
        // Exercise original end-to-end update with a real sensed and remembered candidate.
        f.actor[0x84c]=1;f.actor[0xb6]=0;f.actor[0xdb1]=0;f.rel_special=false;
        f.put(f.actor,0x8c4,0.f);f.put(f.actor,0x920,0);f.put(f.actor,0x644,2);
        relation_row(0,0x7001u,0,1,5);relation_row(1,0x8002u,0,4,0);
        assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.enemy,new_pos,630,0,true,&mutation,&mut));
        assert(f.mutation_api->upsert(f.memory,sizeof(f.memory),f.replacement,new_pos,630,0,true,&mutation,&mut));
        f.put(f.enemy,0x8c,0);f.put(f.replacement,0x8c,0);f.put(f.enemy,0x868,1);
        f.combat_trace=f.combat_fov=true;f.combat_output_count=0;f.combat_center[0]=64;f.combat_velocity[0]=0;
        f.put(f.combat_nav,0xc,1);f.put(f.actor,0x7cc,0u);f.put(f.actor,0x7d0,0u);
        const unsigned before_rebuild=f.rebuild_calls;
        assert(full->update(f.actor,sizeof(f.actor),630,&full_bindings,&full_result)&&full_result.completed&&full_result.gate.forced&&full_result.perception_called&&full_result.perception.completed&&full_result.post_called&&full_result.post.combat_called&&full_result.post.snapshot.snapshot);
        assert(full_result.perception.visual.report_calls==1&&full_result.perception.enemy_switch.assignment.new_handle==0x7001u&&f.get<unsigned>(0x8a0)==0x7001u&&f.get<float>(0x9a0)==630&&f.rebuild_calls==before_rebuild+3);
        // Reacquisition through visual memory update uses native request route, no hosted cue.
        auto native_full=full_bindings;native_full.gate.idle_speech=nullptr;native_full.perception.enemy_switch.lost_cue=nullptr;native_full.perception.visual.report.cue=nullptr;native_full.post.combat.range.report.cue=nullptr;
        assert(control->control(f.memory,sizeof(f.memory),1000,NpcMemoryMarkEluded,0,0,&mb,&ctr));
        f.actor[0x84c]=1;f.update_roll=0;f.put(f.actor,0x9c4,0u);
        assert(pspeech->update(f.actor,sizeof(f.actor),630,&native_full,&psb,&full_result,&psr)&&full_result.completed&&psr.idle_calls==1&&psr.refind_calls==1&&!psr.refind.mode_allowed&&full_result.perception.visual.report_calls==1);
        auto get_gather=reinterpret_cast<GetServerNpcGruntGatherConditionsApi>(GetProcAddress(s,server_npc_grunt_gather_conditions_export));
        assert(get_gather&&!GetProcAddress(c,server_npc_grunt_gather_conditions_export)&&!get_gather(0,sizeof(ServerNpcGruntGatherConditionsApi))&&!get_gather(1,0));
        const auto* gather=get_gather(1,sizeof(ServerNpcGruntGatherConditionsApi));assert(gather);
        ServerNpcGruntGatherConditionsBindings gather_bindings{};gather_bindings.base=full_bindings;gather_bindings.context=&f;gather_bindings.max_clients=2;
        gather_bindings.component_update=[](void*,std::uint32_t token) noexcept -> int{return token==711;};
        gather_bindings.classname_matches=[](void*,void*,const char* name,bool* yes) noexcept -> int{assert(!std::strcmp(name,"npc_gib_hopper"));*yes=false;return 1;};
        gather_bindings.find_object=[](void* p,void* actor,unsigned radius,float now) noexcept -> int{auto& f=F::self(p);assert(actor==f.actor&&radius==150&&now==631);f.put(actor,0xcb4,0x8002u);return 1;};
        gather_bindings.point_predicate=[](void* p,void* actor,const float* point,bool* yes) noexcept -> int{assert(actor==F::self(p).actor&&point[0]==123);*yes=true;return 1;};
        gather_bindings.local_player=[](void* p,void** player) noexcept -> int{*player=F::self(p).replacement;return 1;};
        gather_bindings.player_interaction=[](void* p,void* player,void* actor,bool* yes) noexcept -> int{auto& f=F::self(p);assert(player==f.replacement&&actor==f.actor);*yes=true;return 1;};
        f.put(f.actor,0x850,3);f.put(f.actor,0xc18,711u);f.actor[0xcb2]=1;f.actor[0xc6c]=1;f.put(f.actor,0xcb4,0xffffffffu);
        f.put(f.actor,0x7cc,1u<<10);f.put(f.actor,0x7d4,0u);
        f.put(f.enemy,0xc,0x22490cfcu);
        ServerNpcGruntGatherConditionsResult gather_result;
        assert(gather->gather(f.actor,sizeof(f.actor),631,&gather_bindings,&gather_result)&&gather_result.completed&&gather_result.component_called&&gather_result.hopper_exit&&!gather_result.search_called);
        f.put(f.enemy,0xc,0u);f.put(current_record,4,123.f);f.put(current_record,8,0.f);f.put(current_record,12,0.f);
        f.put(f.actor,0x798,0u);gather_bindings.max_clients=1;
        assert(gather->gather(f.actor,sizeof(f.actor),631,&gather_bindings,&gather_result)&&gather_result.completed&&gather_result.search_called&&gather_result.object_live&&gather_result.object_condition&&gather_result.point_called&&gather_result.point_condition&&gather_result.player_called&&gather_result.player_condition);
        assert(f.get<unsigned>(0x7cc)&(1u<<22));assert((f.get<unsigned>(0x7d4)&((1u<<5)|(1u<<13)))==((1u<<5)|(1u<<13)));
        f.put(f.actor,0x798,1u);f.put(f.actor,0x79c,npc_global_condition_base+0x4d);f.condition(0x46);
        assert(gather->gather(f.actor,sizeof(f.actor),632,&gather_bindings,&gather_result)&&gather_result.point_schedule_blocked&&!gather_result.point_called&&!gather_result.player_called&&!gather_result.search_called);
        f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0xcb4,0x9002u);
        assert(gather->gather(f.actor,sizeof(f.actor),632,&gather_bindings,&gather_result)&&gather_result.search_called&&gather_result.search_no_enemy&&!gather_result.object_live&&f.get<unsigned>(0xcb4)==0xffffffffu);
        f.put(f.actor,0xc18,0u);f.put(f.actor,0x850,7);f.actor[0xc6c]=0;gather_bindings.max_clients=2;
        assert(gather->gather(f.actor,sizeof(f.actor),632,&gather_bindings,&gather_result)&&gather_result.base_called&&gather_result.base.completed&&gather_result.base.gate.inactive);
    }
    {
        auto get_point=reinterpret_cast<GetServerNpcGruntPointPolicyApi>(GetProcAddress(s,server_npc_grunt_point_policy_export));
        assert(get_point&&!GetProcAddress(c,server_npc_grunt_point_policy_export)&&!get_point(0,sizeof(ServerNpcGruntPointPolicyApi))&&!get_point(1,0));
        const auto* policy=get_point(1,sizeof(ServerNpcGruntPointPolicyApi));assert(policy);
        ServerNpcGruntPointPolicyBindings point_bindings{};point_bindings.context=&f;point_bindings.original_vtable=0x2249aaa4;point_bindings.entries=f.entries;point_bindings.count=3;point_bindings.storage=report_binding.mutation.storage;
        point_bindings.origin=[](void* p,void* e,float* v) noexcept -> int{auto& f=F::self(p);v[0]=e==f.actor?0.f:f.distance[f.idx(e)];v[1]=v[2]=0;return 1;};
        point_bindings.notify_squad=[](void*,const float* p,unsigned type,int radius,float duration) noexcept -> int{return p[0]==500&&type==0x801000u&&radius==250&&duration==.1f;};
        point_bindings.in_view=[](void* p,void*,const float*,bool* yes) noexcept -> int{*yes=F::self(p).combat_fov;return 1;};
        point_bindings.visible=[](void* p,void*,const float*,unsigned mask,bool* yes) noexcept -> int{assert(mask==0x4081);*yes=F::self(p).combat_trace;return 1;};
        point_bindings.headroom=[](void* p,void*,bool* yes) noexcept -> int{*yes=F::self(p).remembered_visible;return 1;};
        point_bindings.solve=[](void* p,void*,const float*,bool direct,float* v) noexcept -> int{auto& f=F::self(p);v[0]=f.melee_hits?(direct?10.f:20.f):0.f;v[1]=v[2]=0;return 1;};
        float point[3]{127,0,0};ServerNpcGruntPointPolicyResult point_result{};
        f.put(f.actor,0xc38,0);f.put(f.actor,0xc80,640.f);f.actor[0xc6c]=1;
        assert(policy->query(f.actor,sizeof(f.actor),point,639,&point_bindings,&point_result)&&point_result.no_inventory);
        f.put(f.actor,0xc38,1);
        assert(policy->query(f.actor,sizeof(f.actor),point,639,&point_bindings,&point_result)&&point_result.cooldown_blocked);
        assert(policy->query(f.actor,sizeof(f.actor),point,640,&point_bindings,&point_result)&&point_result.range_blocked&&point_result.deadline==640.25f);
        point[0]=128;f.put(f.actor,0x394,0.f);
        assert(policy->query(f.actor,sizeof(f.actor),point,641,&point_bindings,&point_result)&&point_result.scalar_blocked&&!point_result.deadline_written);
        f.put(f.actor,0x394,1.f);f.put(f.actor,0x9c4,0u);f.combat_fov=f.combat_trace=true;f.melee_hits=true;
        assert(policy->query(f.actor,sizeof(f.actor),point,641,&point_bindings,&point_result)&&point_result.accepted&&point_result.direct&&point_result.solution[0]==10&&f.get<float>(0xc58)==10);
        point[0]=1024;f.combat_fov=false;f.remembered_visible=false;
        assert(policy->query(f.actor,sizeof(f.actor),point,642,&point_bindings,&point_result)&&point_result.headroom_called&&!point_result.headroom_clear&&!point_result.solve_called&&!point_result.deadline_written);
        f.remembered_visible=true;
        assert(policy->query(f.actor,sizeof(f.actor),point,642,&point_bindings,&point_result)&&point_result.accepted&&!point_result.direct&&point_result.solution[0]==20&&f.get<float>(0xc58)==20);
        f.melee_hits=false;f.actor[0xc6c]=0;
        assert(policy->query(f.actor,sizeof(f.actor),point,643,&point_bindings,&point_result)&&!point_result.accepted&&point_result.solve_called&&point_result.deadline==644&&f.get<float>(0xc58)==20);
        point[0]=500;f.put(f.actor,0x9c4,105u);f.put(f.squad_data,8,0x9002u);f.put(f.squad_data,12,0x8002u);f.put(f.squad_data,0x48,2);f.distance[1]=250;
        assert(policy->query(f.actor,sizeof(f.actor),point,644,&point_bindings,&point_result)&&point_result.squad_blocked&&point_result.squad_stale==1&&point_result.squad_visited==2&&point_result.notified&&!point_result.solve_called&&point_result.deadline==645);
    }
    {
        auto getter=reinterpret_cast<GetServerNpcGruntGatherPointApi>(GetProcAddress(s,server_npc_grunt_gather_point_export));
        assert(getter&&!GetProcAddress(c,server_npc_grunt_gather_point_export)&&!getter(0,sizeof(ServerNpcGruntGatherPointApi))&&!getter(1,0));
        const auto* integrated=getter(1,sizeof(ServerNpcGruntGatherPointApi));assert(integrated);
        ServerNpcGruntGatherConditionsBindings gb2{};gb2.base.gate.original_vtable=0x2249aaa4;gb2.base.perception.enemy_switch.best.memory=bb.memory;gb2.base.post.schedule=b.schedule;
        gb2.base.post.aux.visibility=crb.visibility;gb2.context=&f;gb2.max_clients=2;
        gb2.component_update=[](void*,std::uint32_t token) noexcept -> int{return token==711;};
        gb2.classname_matches=[](void*,void*,const char*,bool* yes) noexcept -> int{*yes=false;return 1;};
        ServerNpcGruntPointPolicyBindings pb{};pb.context=&f;
        pb.origin=[](void* p,void* actor,float* v) noexcept -> int{assert(actor==F::self(p).actor);v[0]=v[1]=v[2]=0;return 1;};
        pb.in_view=[](void*,void*,const float*,bool* yes) noexcept -> int{*yes=true;return 1;};
        pb.visible=[](void*,void*,const float*,unsigned,bool* yes) noexcept -> int{*yes=true;return 1;};
        pb.solve=[](void*,void*,const float* point,bool direct,float* v) noexcept -> int{assert(point[0]==500&&direct);v[0]=30;v[1]=v[2]=0;return 1;};
        f.put(f.actor,0xc18,711u);f.put(f.actor,0x850,3);f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x798,0u);
        f.put(f.actor,0x9c4,0u);f.actor[0xcb2]=0;f.actor[0xc6c]=1;f.put(f.actor,0xcb4,0xffffffffu);f.put(f.actor,0xc80,650.f);
        f.put(f.actor,0xc38,1);f.put(f.actor,0x394,1.f);f.put(f.enemy,0xc,0u);
        f.put(current_record,4,500.f);f.put(current_record,8,0.f);f.put(current_record,12,0.f);
        f.put(f.actor,0x7cc,1u<<10);f.put(f.actor,0x7d4,0u);
        ServerNpcGruntGatherConditionsResult gather_point_result;
        ServerNpcGruntPointPolicyResult point_policy_result{};
        assert(integrated->gather(f.actor,sizeof(f.actor),650,&gb2,&pb,&gather_point_result,&point_policy_result)&&gather_point_result.completed&&gather_point_result.point_called&&gather_point_result.point_condition&&point_policy_result.accepted&&point_policy_result.direct&&f.get<float>(0xc58)==30);
        f.put(f.actor,0x7d4,0u);
        assert(integrated->gather(f.actor,sizeof(f.actor),650.1f,&gb2,&pb,&gather_point_result,&point_policy_result)&&gather_point_result.point_called&&!gather_point_result.point_condition&&point_policy_result.cooldown_blocked&&!(f.get<unsigned>(0x7d4)&(1u<<5)));
        f.put(f.actor,0x798,1u);f.put(f.actor,0x79c,npc_global_condition_base+0x4d);
        assert(integrated->gather(f.actor,sizeof(f.actor),651,&gb2,&pb,&gather_point_result,&point_policy_result)&&gather_point_result.point_schedule_blocked&&!gather_point_result.point_called&&!point_policy_result.solve_called);
    }
    {
        auto getter=reinterpret_cast<GetServerNpcGruntObjectSearchApi>(GetProcAddress(s,server_npc_grunt_object_search_export));
        assert(getter&&!GetProcAddress(c,server_npc_grunt_object_search_export)&&!getter(0,sizeof(ServerNpcGruntObjectSearchApi))&&!getter(1,0));
        const auto* objects=getter(1,sizeof(ServerNpcGruntObjectSearchApi));assert(objects);
        ServerNpcGruntObjectSearchBindings ob{};ob.context=&f;ob.visibility=crb.visibility;
        ob.visibility.trace=[](void*,void*,void*,unsigned mask,bool* yes,void** blocker) noexcept -> int{assert(mask==0x4081);*yes=true;*blocker=nullptr;return 1;};
        ob.enumerate=[](void*,const float* lo,const float* hi,const std::uint32_t** list,unsigned* count) noexcept -> int{
            static const std::uint32_t handles[]={0x9002u,0x7001u,0x8002u};assert(lo[0]==-60&&hi[0]==60&&lo[2]==-60&&hi[2]==60);*list=handles;*count=3;return 1;
        };
        ob.origin=[](void* p,void* e,float* pos) noexcept -> int{auto& f=F::self(p);pos[0]=e==f.actor?0:f.distance[f.idx(e)];pos[1]=pos[2]=0;return 1;};
        ob.center=ob.origin;
        ob.eye=[](void*,void*,float* pos) noexcept -> int{pos[0]=pos[1]=0;pos[2]=64;return 1;};
        ob.actor_reference_height=[](void*,void*,float* height) noexcept -> int{*height=16;return 1;};
        ob.object_properties=[](void* p,void* e,bool* exists,float* mass,bool* movable,bool* enabled) noexcept -> int{
            auto& f=F::self(p);*exists=e==f.replacement;*mass=f.combat_velocity[0];*movable=*enabled=true;return 1;
        };
        ob.world_model_top=[](void*,void*,bool* model,float* top) noexcept -> int{*model=true;*top=20;return 1;};
        ob.attached_object=[](void*,void*,bool* yes) noexcept -> int{*yes=false;return 1;};
        ob.point_test=[](void*,void*,const float*,unsigned mask,bool* yes) noexcept -> int{assert(mask==0x200400b);*yes=true;return 1;};
        ob.classname_matches=[](void* p,void*,const char* name,bool* yes) noexcept -> int{*yes=F::self(p).rel_special&&!std::strcmp(name,"item_health*");return 1;};
        ob.as_physics_prop=[](void*,void* e,void** out) noexcept -> int{*out=e;return 1;};
        f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0xcb4,0x8002u);
        ServerNpcGruntObjectSearchResult object_result{};
        assert(objects->search(f.actor,sizeof(f.actor),150,&ob,&object_result)&&object_result.no_enemy&&f.get<unsigned>(0xcb4)==0xffffffffu);
        f.put(f.actor,0x8a0,0x7001u);f.distance[0]=501;f.put(f.actor,0xcb4,0x8002u);
        assert(objects->search(f.actor,sizeof(f.actor),150,&ob,&object_result)&&object_result.enemy_range_blocked&&f.get<unsigned>(0xcb4)==0x8002u);
        f.distance[0]=100;f.distance[1]=30;f.put(f.replacement,0x128,20.f);f.put(f.replacement,0xb4,0u);f.actor[0xb6]=0;f.rel_special=false;f.combat_velocity[0]=150;
        assert(objects->search(f.actor,sizeof(f.actor),150,&ob,&object_result)&&object_result.completed&&object_result.stale==1&&object_result.property_rejected==1&&object_result.qualified==1&&object_result.found&&object_result.best_distance==30&&f.get<unsigned>(0xcb4)==0x8002u);
        f.combat_velocity[0]=151;
        assert(objects->search(f.actor,sizeof(f.actor),150,&ob,&object_result)&&!object_result.found&&object_result.property_rejected==2);
        f.combat_velocity[0]=150;f.distance[1]=60;
        assert(objects->search(f.actor,sizeof(f.actor),150,&ob,&object_result)&&object_result.distance_rejected==1&&!object_result.found);
        f.distance[1]=30;f.rel_special=true;
        assert(objects->search(f.actor,sizeof(f.actor),150,&ob,&object_result)&&object_result.class_rejected==1&&!object_result.found);
        f.rel_special=false;f.put(f.replacement,0xb4,0x2000u);
        assert(objects->search(f.actor,sizeof(f.actor),150,&ob,&object_result)&&object_result.prop_rejected==1&&!object_result.found);
        auto native_getter=reinterpret_cast<GetServerNpcGruntGatherNativeApi>(GetProcAddress(s,server_npc_grunt_gather_native_export));
        assert(native_getter&&!GetProcAddress(c,server_npc_grunt_gather_native_export)&&!native_getter(0,sizeof(ServerNpcGruntGatherNativeApi))&&!native_getter(1,0));
        const auto* native_gather=native_getter(1,sizeof(ServerNpcGruntGatherNativeApi));assert(native_gather);
        ServerNpcGruntGatherNativeBindings all_bindings{};
        auto& base=all_bindings.gather;
        base.base.gate.original_vtable=0x2249aaa4;base.base.perception.enemy_switch.best.memory=bb.memory;base.base.post.schedule=b.schedule;
        base.base.post.aux.visibility=crb.visibility;base.max_clients=2;
        base.component_update=[](void*,std::uint32_t token) noexcept -> int{return token==711;};
        base.classname_matches=[](void*,void*,const char*,bool* yes) noexcept -> int{*yes=false;return 1;};
        all_bindings.object=ob;all_bindings.point.context=&f;all_bindings.point.origin=ob.origin;
        all_bindings.point.in_view=[](void*,void*,const float*,bool* yes) noexcept -> int{*yes=true;return 1;};
        all_bindings.point.visible=[](void*,void*,const float*,unsigned,bool* yes) noexcept -> int{*yes=true;return 1;};
        all_bindings.point.solve=[](void*,void*,const float*,bool,float* v) noexcept -> int{v[0]=40;v[1]=v[2]=0;return 1;};
        f.put(f.actor,0xc18,711u);f.put(f.actor,0x850,3);f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x798,0u);
        f.put(f.actor,0xcb4,0xffffffffu);f.put(f.actor,0xc80,0.f);f.actor[0xcb2]=f.actor[0xc6c]=1;
        f.put(f.actor,0x7cc,1u<<10);f.put(f.actor,0x7d4,0u);f.put(f.replacement,0xb4,0u);
        ServerNpcGruntGatherNativeResult native_result;
        assert(native_gather->gather(f.actor,sizeof(f.actor),660,&all_bindings,&native_result)&&native_result.gather.completed&&native_result.gather.search_called&&native_result.object.found&&native_result.gather.object_condition&&native_result.gather.point_condition&&native_result.point.accepted);
        assert(f.get<unsigned>(0xcb4)==0x8002u&&f.get<float>(0xc58)==40);
        f.put(f.actor,0x7cc,1u<<10);f.put(f.actor,0x7d4,0u);
        assert(native_gather->gather(f.actor,sizeof(f.actor),660.1f,&all_bindings,&native_result)&&!native_result.gather.search_called&&native_result.gather.object_condition&&!native_result.gather.point_condition&&native_result.point.cooldown_blocked);
        f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0xcb4,0xffffffffu);
        assert(native_gather->gather(f.actor,sizeof(f.actor),661,&all_bindings,&native_result)&&native_result.gather.search_no_enemy&&native_result.object.no_enemy&&!native_result.gather.point_called);
    }
    {
        auto getter=reinterpret_cast<GetServerNpcGruntPrescheduleApi>(GetProcAddress(s,server_npc_grunt_preschedule_export));
        assert(getter&&!GetProcAddress(c,server_npc_grunt_preschedule_export)&&!getter(0,sizeof(ServerNpcGruntPrescheduleApi))&&!getter(1,0));
        const auto* pre=getter(1,sizeof(ServerNpcGruntPrescheduleApi));assert(pre);
        ServerNpcGruntPrescheduleBindings pb{};pb.entries=f.entries;pb.count=3;pb.original_vtable=0x2249aaa4;pb.conditions=b.schedule.conditions;pb.context=&f;pb.pending_range=100;
        pb.origin=[](void*,void* e,float* out) noexcept -> int{std::memcpy(out,static_cast<unsigned char*>(e)+0x2bc,12);return 1;};
        pb.center=pb.origin;
        pb.trace_fraction=[](void* p,void* a,const float*,const float*,unsigned mask,float* out) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&mask==0x202400b);*out=f.combat_trace?1.f:.5f;return 1;};
        pb.as_pending_prop=[](void* p,void* o,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(o==f.replacement);*out=f.rel_special?nullptr:o;*n=sizeof(f.replacement);return 1;};
        pb.entity_handle=eb.entity_handle;
        pb.component_tick=[](void* p,std::uint32_t token) noexcept -> int{assert(token==711);F::self(p).actor[0x101]=1;return 1;};
        pb.speech_queue_tick=[](void* p,void* a,float) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.actor[0x101]);f.actor[0x102]=1;return 1;};
        pb.rule_query=[](void* p,bool* yes) noexcept -> int{*yes=F::self(p).rel_player;return 1;};
        pb.death=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.get<int>(0x9c)==35);f.actor[0x103]=1;return 1;};
        std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0xcb8,0x8002u);
        f.put(f.actor,0xc18,711u);f.put(f.actor,0xc70,5.f);
        f.put(f.enemy,0x2bc,100.f);f.put(f.enemy,0x2c0,0.f);f.put(f.enemy,0x2c4,5.f);
        f.put(f.replacement,0x2bc,30.f);f.put(f.replacement,0x2c0,0.f);f.put(f.replacement,0x2c4,0.f);
        f.put(f.replacement,0x8c,0);f.put(f.replacement,0x79c,701.f);f.combat_trace=true;f.rel_special=false;
        ServerNpcGruntPrescheduleResult result{};
        assert(pre->pending(f.actor,sizeof(f.actor),700,&pb,&result)&&result.completed&&result.accepted&&result.distance==30&&f.get<unsigned>(0xcb8)==0x8002u);
        f.put(f.replacement,0x2c4,13.f);assert(pre->pending(f.actor,sizeof(f.actor),700,&pb,&result)&&result.height_rejected&&!result.accepted);
        f.put(f.replacement,0x2c4,0.f);f.put(f.replacement,0x2bc,50.f);assert(pre->pending(f.actor,sizeof(f.actor),700,&pb,&result)&&result.range_rejected);
        f.put(f.replacement,0x2bc,-30.f);assert(pre->pending(f.actor,sizeof(f.actor),700,&pb,&result)&&result.direction_rejected);
        f.put(f.replacement,0x2bc,30.f);f.combat_trace=false;assert(pre->pending(f.actor,sizeof(f.actor),700,&pb,&result)&&result.trace_rejected);
        f.combat_trace=true;f.rel_special=true;assert(pre->pending(f.actor,sizeof(f.actor),700,&pb,&result)&&result.cast_rejected);
        f.rel_special=false;f.put(f.replacement,0x8c,1);assert(pre->pending(f.actor,sizeof(f.actor),700,&pb,&result)&&result.state_rejected);
        f.put(f.replacement,0x8c,0);assert(pre->pending(f.actor,sizeof(f.actor),700.5f,&pb,&result)&&result.deadline_rejected);
        f.actor[0xbf]=8;f.rel_player=false;
        assert(pre->run(f.actor,sizeof(f.actor),700,&pb,&result)&&result.completed&&result.height_written&&f.actor[0xc6c]&&result.component_called&&result.speech_called&&result.rule_called&&result.accepted&&result.target_handle==0x8002u);
        assert(f.get<unsigned>(0x8a8)==0x8002u&&f.get<unsigned>(0xcb4)==0x8002u&&f.get<unsigned>(0xcb8)==0xffffffffu&&(f.get<unsigned>(0x7d4)&(1u<<6))&&(f.get<unsigned>(0x7d4)&(1u<<12)));
        f.rel_player=true;f.put(f.actor,0x9c,35);f.put(f.enemy,0x2c4,4.f);
        assert(pre->run(f.actor,sizeof(f.actor),701,&pb,&result)&&result.death_called&&f.actor[0x103]&&f.get<int>(0x9c)==0&&!f.actor[0xc6c]&&!result.accepted&&!(f.get<unsigned>(0x7d4)&(1u<<12)));
        f.actor[0xbf]=0;
        assert(pre->run(f.actor,sizeof(f.actor),702,&pb,&result)&&!result.rule_called&&!(f.get<unsigned>(0x7d4)&(1u<<6)));
        assert(!pre->run(f.actor,0xdb1,702,&pb,&result));
    }
    {
        auto getter=reinterpret_cast<GetServerNpcGruntSpeechQueueApi>(GetProcAddress(s,server_npc_grunt_speech_queue_export));
        assert(getter&&!GetProcAddress(c,server_npc_grunt_speech_queue_export)&&!getter(0,sizeof(ServerNpcGruntSpeechQueueApi))&&!getter(1,0));
        const auto* api=getter(1,sizeof(ServerNpcGruntSpeechQueueApi));assert(api);
        ServerNpcGruntSpeechQueueBindings qb{};qb.original_vtable=0x2249aaa4;qb.context=&f;qb.volume=1;qb.level=75;
        qb.squad_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            auto& f=F::self(p);assert(token==91);*out=f.speech_squad;*n=sizeof(f.speech_squad);return 1;
        };
        qb.deliver=[](void* p,std::uint32_t token,int index,float volume,int level,int zero,std::uint32_t extra) noexcept -> int{
            auto& f=F::self(p);assert(token==37&&index==17&&volume==1&&level==75&&zero==0&&extra==9&&f.get<int>(0xccc)==17);
            ++f.queue_deliveries;return 1;
        };
        qb.random_float=[](void* p,float lo,float hi,float* out) noexcept -> int{
            auto& f=F::self(p);assert(lo==1.5f&&hi==2.f&&f.queue_deliveries&&f.get<int>(0xccc)==17);
            *out=(++f.queue_randoms%2)?1.5f:2.f;return 1;
        };
        std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0xccc,-1);
        ServerNpcGruntSpeechQueueResult qr{};
        assert(api->tick(f.actor,sizeof(f.actor),100,&qb,&qr)&&qr.empty&&!qr.cleared&&!f.queue_deliveries);
        auto pending=[&](float expires,int priority){f.put(f.actor,0xccc,17);f.put(f.actor,0xcd0,expires);f.put(f.actor,0xcd4,priority);f.put(f.actor,0x16c,37u);f.put(f.actor,0xcc8,9u);};
        pending(99,2);assert(api->tick(f.actor,sizeof(f.actor),100,&qb,&qr)&&qr.expired&&qr.cleared&&f.get<int>(0xccc)==-1&&!f.queue_deliveries);
        pending(100,2);f.put(f.actor,0xc04,101.f);f.put(f.actor,0xc08,2);
        assert(api->tick(f.actor,sizeof(f.actor),100,&qb,&qr)&&qr.priority_blocked&&!qr.cleared&&f.get<int>(0xccc)==17);
        f.put(f.actor,0xc04,100.f);
        assert(api->tick(f.actor,sizeof(f.actor),100,&qb,&qr)&&qr.priority_blocked&&!qr.cleared);
        f.put(f.actor,0xcd4,3);f.put(f.actor,0xb4,2u);
        assert(api->tick(f.actor,sizeof(f.actor),100,&qb,&qr)&&qr.state_blocked&&!qr.cleared);
        f.put(f.actor,0x850,3);
        assert(api->tick(f.actor,sizeof(f.actor),100,&qb,&qr)&&qr.delivered&&qr.owner_written&&qr.cleared&&f.get<int>(0xccc)==-1&&f.get<int>(0xc08)==3&&f.get<float>(0xc04)==101.5f&&f.queue_deliveries==1&&f.queue_randoms==1);
        // Squad gate overrides local deadline, blocks equal priority at equality, updates independent random deadlines.
        pending(110,1);f.put(f.actor,0x9c4,91u);f.put(f.speech_squad,0x50,105.f);f.put(f.speech_squad,0x54,2);
        assert(api->tick(f.actor,sizeof(f.actor),104,&qb,&qr)&&qr.squad_used&&qr.priority_blocked&&!qr.cleared);
        assert(api->tick(f.actor,sizeof(f.actor),105,&qb,&qr)&&qr.squad_used&&qr.priority_blocked&&!qr.cleared);
        f.queue_randoms=0;f.put(f.actor,0xc04,999.f);f.put(f.actor,0xc08,99);
        assert(api->tick(f.actor,sizeof(f.actor),105.25f,&qb,&qr)&&qr.delivered&&qr.squad_written&&qr.owner_written&&qr.cleared&&qr.owner_deadline==106.75f&&qr.squad_deadline==107.25f&&f.queue_randoms==2);
        float sd=0;int sp=0;std::memcpy(&sd,f.speech_squad+0x50,4);std::memcpy(&sp,f.speech_squad+0x54,4);assert(sd==107.25f&&sp==1);
        // Native preschedule replaces the legacy callback, preserving order.
        std::memset(f.actor,0,sizeof(f.actor));pending(120,4);f.put(f.actor,0xc18,711u);
        ServerNpcGruntPrescheduleBindings pb{};pb.original_vtable=0x2249aaa4;pb.conditions=b.schedule.conditions;pb.context=&f;
        pb.component_tick=[](void* p,std::uint32_t token) noexcept -> int{assert(token==711);F::self(p).actor[0x101]=1;return 1;};
        qb.deliver=[](void* p,std::uint32_t token,int index,float volume,int level,int zero,std::uint32_t extra) noexcept -> int{
            auto& f=F::self(p);assert(token==37&&index==17&&volume==1&&level==75&&!zero&&extra==9&&f.actor[0x101]);++f.queue_deliveries;return 1;
        };
        ServerNpcGruntPrescheduleResult pr{};
        assert(api->preschedule(f.actor,sizeof(f.actor),120,&pb,&qb,&pr,&qr)&&pr.completed&&pr.component_called&&pr.speech_called&&qr.delivered&&qr.cleared&&f.get<int>(0xccc)==-1);
        assert(!api->tick(f.actor,0xcd7,120,&qb,&qr));
    }
    {
        auto getter=reinterpret_cast<GetServerNpcGruntSentenceApi>(GetProcAddress(s,server_npc_grunt_sentence_export));
        assert(getter&&!GetProcAddress(c,server_npc_grunt_sentence_export)&&!getter(0,sizeof(ServerNpcGruntSentenceApi))&&!getter(1,0));
        const auto* sentence=getter(1,sizeof(ServerNpcGruntSentenceApi));assert(sentence);
        ServerNpcGruntSentenceBindings sb{};sb.speech.original_vtable=0x2249aaa4;sb.speech.context=&f;sb.entries=f.entries;sb.count=4;
        sb.speech.squad_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==91);*out=f.speech_squad;*n=sizeof(f.speech_squad);return 1;};
        sb.extent=[](void* p,void* e,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(e==f.actor||e==f.enemy||e==f.replacement);*n=e==f.actor?sizeof(f.actor):sizeof(f.enemy);return 1;};
        sb.component_eligible=[](void* p,void* e,bool* yes) noexcept -> int{auto& f=F::self(p);*yes=e==f.enemy;return 1;};
        sb.entity_rejected=[](void* p,void* e,bool* yes) noexcept -> int{*yes=e==F::self(p).replacement;return 1;};
        sb.enqueue=[](void* p,void* a,const char* cue,int* code) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.get<int>(0xccc)==-1&&!std::strcmp(cue,"GRUNT_IDLE"));++f.speech_calls;*code=f.speech_code;return 1;};
        sb.speech.random_float=[](void* p,float lo,float hi,float* out) noexcept -> int{auto& f=F::self(p);assert(lo==1.5f&&hi==2.f&&f.get<int>(0xccc)==-1);*out=(++f.queue_randoms%2)?1.5f:2.f;return 1;};
        std::memset(f.actor,0,sizeof(f.actor));std::memset(f.speech_squad,0,sizeof(f.speech_squad));
        f.entries[1].serial=7;f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
        f.entries[2].serial=8;f.entries[2].entity=reinterpret_cast<CBaseEntity*>(f.replacement);
        f.entries[3].serial=9;f.entries[3].entity=reinterpret_cast<CBaseEntity*>(f.actor);
        f.put(f.actor,0x204,1);f.put(f.enemy,0x204,0);f.put(f.replacement,0x204,1);
        f.put(f.actor,0xb4,2u);f.put(f.actor,0xccc,17);f.speech_calls=f.queue_randoms=0;
        ServerNpcGruntSentenceResult sr{};
        assert(sentence->gate(f.actor,sizeof(f.actor),0,&sb,&sr)&&sr.mode_allowed&&!sr.group_scans);
        assert(sentence->gate(f.actor,sizeof(f.actor),0xffffffffu,&sb,&sr)&&sr.mode_allowed);
        assert(sentence->dispatch(f.actor,sizeof(f.actor),100,"GRUNT_IDLE",0,1,&sb,&sr)&&!sr.mode_allowed&&!sr.queue_reset&&f.get<int>(0xccc)==17);
        f.put(f.actor,0x850,3);
        assert(sentence->gate(f.actor,sizeof(f.actor),1,&sb,&sr)&&sr.mode_allowed);
        assert(sentence->gate(f.actor,sizeof(f.actor),2,&sb,&sr)&&!sr.mode_allowed&&!sr.group_scans);
        f.put(f.actor,0x9c4,91u);
        f.put(f.speech_squad,8,0x9003u);f.put(f.speech_squad,12,0x7001u);f.put(f.speech_squad,16,0x8002u);f.put(f.speech_squad,20,0x6001u);f.put(f.speech_squad,0x48,4);
        assert(sentence->gate(f.actor,sizeof(f.actor),2,&sb,&sr)&&sr.mode_allowed&&sr.eligible_count==2&&sr.group_scans==1);
        assert(sentence->gate(f.actor,sizeof(f.actor),3,&sb,&sr)&&sr.mode_allowed&&sr.leader_match&&sr.group_scans==2&&sr.first_eligible==f.actor);
        f.put(f.speech_squad,8,0x7001u);f.put(f.speech_squad,12,0x9003u);
        assert(sentence->gate(f.actor,sizeof(f.actor),3,&sb,&sr)&&!sr.mode_allowed&&sr.candidate_eligible&&sr.first_eligible==f.enemy);
        f.put(f.speech_squad,0x48,1);
        assert(sentence->gate(f.actor,sizeof(f.actor),2,&sb,&sr)&&!sr.mode_allowed&&sr.eligible_count==1);
        f.put(f.speech_squad,12,0x7001u);f.put(f.speech_squad,0x48,2);
        assert(sentence->gate(f.actor,sizeof(f.actor),2,&sb,&sr)&&sr.mode_allowed&&sr.eligible_count==2); // duplicate native entries count
        f.put(f.speech_squad,0x50,100.f);f.put(f.speech_squad,0x54,2);
        assert(sentence->dispatch(f.actor,sizeof(f.actor),100,"GRUNT_IDLE",2,2,&sb,&sr)&&sr.queue_reset&&sr.cooldown.priority_blocked&&!sr.queued&&f.get<int>(0xccc)==-1);
        f.speech_code=-1;
        assert(sentence->dispatch(f.actor,sizeof(f.actor),100,"GRUNT_IDLE",3,2,&sb,&sr)&&sr.queued&&sr.request_result==-1&&sr.cooldown.owner_written&&sr.cooldown.squad_written&&sr.cooldown.owner_deadline==103.5f&&sr.cooldown.squad_deadline==104.f&&f.speech_calls==1);
        f.speech_code=42;
        assert(sentence->dispatch(f.actor,sizeof(f.actor),100,"GRUNT_IDLE",-1,1,&sb,&sr)&&sr.queued&&sr.request_result==42&&!sr.cooldown.owner_written&&f.queue_randoms==2);
        char oversized[129];std::memset(oversized,'x',128);oversized[128]=0;
        assert(!sentence->dispatch(f.actor,sizeof(f.actor),100,oversized,-1,0,&sb,&sr)&&!sr.queued);
        auto gn=reinterpret_cast<GetServerNpcGruntNativeSpeechApi>(GetProcAddress(s,server_npc_grunt_native_speech_export));
        assert(gn&&!GetProcAddress(c,server_npc_grunt_native_speech_export)&&!gn(0,sizeof(ServerNpcGruntNativeSpeechApi))&&!gn(1,0));
        const auto* native=gn(1,sizeof(ServerNpcGruntNativeSpeechApi));assert(native);
        sb.enqueue=[](void* p,void* a,const char* cue,int* code) noexcept -> int{
            auto& f=F::self(p);assert(a==f.actor&&f.get<int>(0xccc)==-1);f.speech_name=cue;++f.speech_calls;*code=f.speech_code;return 1;
        };
        sb.speech.random_float=[](void* p,float lo,float hi,float* out) noexcept -> int{auto& f=F::self(p);assert(lo==1.5f&&hi==2.f);*out=(++f.queue_randoms%2)?1.5f:2.f;return 1;};
        ServerNpcGruntSpeechBindings cues{};cues.original_vtable=sb.speech.original_vtable;cues.context=&f;cues.memory.entries=f.entries;cues.memory.count=4;cues.random=spb.random;
        ServerNpcGruntNativeSpeechResult nr{};
        f.speech_code=42;f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0xc88,0.f);f.put(f.actor,0xc7c,0.f);
        f.queue_randoms=f.random_calls=0;
        assert(native->speak(f.actor,sizeof(f.actor),NpcGruntLost,200,&cues,&sb,&nr)&&nr.cue.emitted&&nr.sentence.queued&&nr.cue.speech_result==42&&nr.cue.cooldown_written&&nr.cue.deadline==207&&nr.sentence.cooldown.squad_written&&f.random_calls==1);
        assert(!std::strcmp(f.speech_name,"GRUNT_LOST_LONG"));
        assert(native->speak(f.actor,sizeof(f.actor),NpcGruntRefind,200,&cues,&sb,&nr)&&nr.sentence.queued&&!std::strcmp(f.speech_name,"GRUNT_REFIND_ENEMY")&&!nr.cue.cooldown_written);
        assert(native->speak(f.actor,sizeof(f.actor),NpcGruntAlert,201,&cues,&sb,&nr)&&nr.sentence.cooldown.priority_blocked&&!nr.sentence.queued&&nr.cue.speech_result==-1&&nr.cue.deadline==211);
        f.put(f.speech_squad,0x48,1);f.put(f.replacement,0x9c,1);
        ServerNpcGruntSurvivorResult loss{};
        f.actor[0xbf]=8;
        assert(native->survivor(f.actor,sizeof(f.actor),nullptr,0,300,&sb,&loss)&&loss.flag_blocked);
        f.actor[0xbf]=0;
        assert(native->survivor(f.actor,sizeof(f.actor),f.replacement,sizeof(f.replacement),300,&sb,&loss)&&loss.removed_positive);
        f.put(f.replacement,0x9c,0);f.speech_code=-1;f.queue_randoms=0;
        assert(native->survivor(f.actor,sizeof(f.actor),f.replacement,sizeof(f.replacement),300,&sb,&loss)&&loss.completed&&loss.last_member&&loss.sentence.queued&&loss.sentence.request_result==-1&&loss.final_mark.owner_written&&loss.final_mark.squad_written&&loss.final_mark.owner_deadline==301.5f&&loss.final_mark.squad_deadline==302.f&&!std::strcmp(loss.cue,"GRUNT_LAST_OF_SQUAD"));
        f.put(f.actor,0x850,1);f.put(f.actor,0xb4,2u);f.queue_randoms=0;
        assert(native->survivor(f.actor,sizeof(f.actor),f.replacement,sizeof(f.replacement),301,&sb,&loss)&&!loss.sentence.mode_allowed&&!loss.sentence.queue_reset&&loss.final_mark.owner_written&&loss.final_mark.squad_written);
        f.put(f.actor,0x850,3);f.put(f.speech_squad,0x48,2);f.speech_code=12;f.queue_randoms=0;
        assert(native->survivor(f.actor,sizeof(f.actor),f.replacement,sizeof(f.replacement),310,&sb,&loss)&&loss.eligible_count==2&&!loss.last_member&&loss.sentence.queued&&loss.sentence.request_result==12&&loss.sentence.cooldown.owner_written&&!loss.final_mark.owner_written&&!std::strcmp(loss.cue,"GRUNT_MAN_DOWN"));
    }
    {
        auto g=reinterpret_cast<GetServerNpcGruntSentenceRequestApi>(GetProcAddress(s,server_npc_grunt_sentence_request_export));
        assert(g&&!GetProcAddress(c,server_npc_grunt_sentence_request_export)&&!g(0,sizeof(ServerNpcGruntSentenceRequestApi))&&!g(1,0));
        const auto* api=g(1,sizeof(ServerNpcGruntSentenceRequestApi));assert(api);
        ServerNpcGruntSentenceBindings sb{};sb.speech.original_vtable=0x2249aaa4;sb.speech.context=&f;sb.speech.volume=1;sb.speech.level=75;
        ServerNpcGruntSentenceRequestBindings rb{};rb.context=&f;
        rb.suffix=[](void*,std::uint32_t token,const char** out,unsigned* n) noexcept -> int{assert(token==91);*out="_ELITE";*n=7;return 1;};
        rb.group_index=[](void* p,const char* name,int* out) noexcept -> int{auto& f=F::self(p);assert(!std::strcmp(name,f.get<unsigned>(0x8e8)?"GRUNT_IDLE_ELITE":"GRUNT_IDLE"));*out=f.query;return 1;};
        rb.pick_name=[](void* p,int group,char* name,unsigned n,int* code) noexcept -> int{auto& f=F::self(p);assert(group==4&&n==64);if(f.rel_special){*code=-1;return 1;}*code=0;if(f.rel_player)return 1;std::memcpy(name,"!GRUNT_IDLE_01",15);return 1;};
        rb.name_index=[](void* p,const char* name,int* out) noexcept -> int{assert(!std::strcmp(name,"GRUNT_IDLE_01"));*out=F::self(p).speech_code;return 1;};
        rb.submit=[](void* p,std::uint32_t token,int index,float volume,int level,int zero,std::uint32_t extra) noexcept -> int{auto& f=F::self(p);assert(token==37&&index==f.speech_code&&volume==1&&level==75&&!zero&&extra==9);++f.queue_deliveries;return 1;};
        sb.speech.random_float=[](void*,float lo,float hi,float* out) noexcept -> int{assert(lo==1.5f&&hi==2);*out=1.5f;return 1;};
        std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x16c,37u);f.put(f.actor,0xcc8,9u);f.put(f.actor,0x8e8,91u);
        ServerNpcGruntSentenceRequestResult qr{};
        assert(api->request(f.actor,sizeof(f.actor),"GRUNT_IDLE",&sb.speech,&rb,&qr)&&qr.manager_missing&&qr.suffix_used&&!qr.group_called&&qr.request_result==-1);
        rb.manager_ready=true;f.query=-1;
        assert(api->request(f.actor,sizeof(f.actor),"GRUNT_IDLE",&sb.speech,&rb,&qr)&&qr.group_called&&!qr.pick_called&&qr.request_result==-1);
        f.query=4;f.rel_special=true;f.rel_player=false;
        assert(api->request(f.actor,sizeof(f.actor),"GRUNT_IDLE",&sb.speech,&rb,&qr)&&qr.pick_called&&!qr.lookup_called&&qr.request_result==-1);
        f.rel_special=false;f.rel_player=true;
        assert(api->request(f.actor,sizeof(f.actor),"GRUNT_IDLE",&sb.speech,&rb,&qr)&&qr.pick_called&&!qr.lookup_called);
        f.rel_player=false;f.speech_code=-4;f.queue_deliveries=0;
        assert(api->request(f.actor,sizeof(f.actor),"GRUNT_IDLE",&sb.speech,&rb,&qr)&&qr.submitted&&qr.sentence_id==-4&&qr.request_result==-1&&f.queue_deliveries==1);
        f.speech_code=17;f.put(f.actor,0x8e8,0u);f.put(f.actor,0xccc,9);
        ServerNpcGruntSentenceResult dr{};
        assert(api->dispatch(f.actor,sizeof(f.actor),100,"GRUNT_IDLE",2,0,&sb,&rb,&dr,&qr)&&dr.queue_reset&&dr.queued&&dr.request_result==17&&qr.submitted&&!qr.suffix_used&&dr.cooldown.owner_deadline==103.5f&&f.queue_deliveries==2);
        f.put(f.actor,0x8e8,91u);
        char large[124];std::memset(large,'a',123);large[123]=0;
        assert(!api->request(f.actor,sizeof(f.actor),large,&sb.speech,&rb,&qr)&&!qr.group_called);
        auto gfull=reinterpret_cast<GetServerNpcGruntRequestSpeechApi>(GetProcAddress(s,server_npc_grunt_request_speech_export));
        assert(gfull&&!GetProcAddress(c,server_npc_grunt_request_speech_export)&&!gfull(0,sizeof(ServerNpcGruntRequestSpeechApi))&&!gfull(1,0));
        const auto* full=gfull(1,sizeof(ServerNpcGruntRequestSpeechApi));assert(full);
        sb.entries=f.entries;sb.count=4;
        sb.speech.squad_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==91);*out=f.speech_squad;*n=sizeof(f.speech_squad);return 1;};
        sb.extent=[](void* p,void* e,unsigned* n) noexcept -> int{assert(e==F::self(p).enemy);*n=sizeof(F::self(p).enemy);return 1;};
        sb.entity_rejected=[](void*,void*,bool* out) noexcept -> int{*out=false;return 1;};
        std::memset(f.speech_squad,0,sizeof(f.speech_squad));f.put(f.speech_squad,8,0x7001u);f.put(f.speech_squad,12,0x7001u);f.put(f.speech_squad,0x48,2);f.put(f.enemy,0x204,1);
        f.put(f.actor,0x9c4,91u);f.put(f.actor,0x850,3);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0xc88,0.f);
        ServerNpcGruntSpeechBindings cb{};cb.original_vtable=0x2248df04;cb.memory.entries=f.entries;cb.memory.count=4;cb.context=&f;cb.random=spb.random;
        rb.group_index=[](void* p,const char* name,int* out) noexcept -> int{auto& f=F::self(p);assert(!std::strcmp(name,f.speech_name));*out=f.query;return 1;};
        f.speech_name="GRUNT_LOST_LONG_ELITE";f.query=4;f.speech_code=17;
        ServerNpcGruntRequestSpeechResult result{};
        assert(full->speak(f.actor,sizeof(f.actor),NpcGruntLost,200,&cb,&sb,&rb,&result)&&result.speech.cue.cooldown_written&&result.speech.cue.deadline==207&&result.speech.sentence.cooldown.squad_written&&result.request.suffix_used&&result.request.submitted&&result.speech.cue.speech_result==17);
        assert(full->speak(f.actor,sizeof(f.actor),NpcGruntLost,201,&cb,&sb,&rb,&result)&&result.speech.cue.suppressed&&!result.request.group_called);
        f.speech_name="GRUNT_GO_ALERT_ELITE";rb.manager_ready=false;
        assert(full->speak(f.actor,sizeof(f.actor),NpcGruntAlert,202,&cb,&sb,&rb,&result)&&result.request.manager_missing&&result.speech.cue.speech_result==-1&&result.speech.cue.deadline==212);
        f.put(f.speech_squad,0x48,1);f.put(f.replacement,0x9c,0);
        ServerNpcGruntRequestSurvivorResult sr{};rb.manager_ready=true;f.speech_name="GRUNT_LAST_OF_SQUAD_ELITE";f.query=-1;
        assert(full->survivor(f.actor,sizeof(f.actor),f.replacement,sizeof(f.replacement),220,&sb,&rb,&sr)&&sr.survivor.completed&&sr.survivor.last_member&&sr.request.group_called&&sr.request.request_result==-1&&sr.survivor.final_mark.owner_written&&sr.survivor.final_mark.squad_written);
        f.put(f.speech_squad,0x48,2);f.query=4;f.speech_name="GRUNT_MAN_DOWN_ELITE";
        assert(full->survivor(f.actor,sizeof(f.actor),f.replacement,sizeof(f.replacement),230,&sb,&rb,&sr)&&sr.survivor.completed&&!sr.survivor.last_member&&sr.request.submitted&&sr.survivor.sentence.cooldown.owner_written&&!sr.survivor.final_mark.owner_written);
    }
    {
        auto g=reinterpret_cast<GetServerNpcGruntSquadLeaveApi>(GetProcAddress(s,server_npc_grunt_squad_leave_export));
        assert(g&&!GetProcAddress(c,server_npc_grunt_squad_leave_export)&&!g(0,sizeof(ServerNpcGruntSquadLeaveApi))&&!g(1,0));
        const auto* api=g(1,sizeof(ServerNpcGruntSquadLeaveApi));assert(api);
        struct SquadFixture {
            alignas(4) unsigned char actor[3][0xdb2]{},group[0x58]{};
            sin_independent::EntityHandleEntry slots[4]{};
            unsigned requests=0,submissions=0,randoms=0;const char* expected=nullptr;
            static SquadFixture& self(void* p){return *static_cast<SquadFixture*>(p);}
            static unsigned rd(const void* p,unsigned offset){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+offset,4);return v;}
        } q;
        for(unsigned i=0;i<3;++i){
            q.slots[i+1].serial=i+1;q.slots[i+1].entity=reinterpret_cast<CBaseEntity*>(q.actor[i]);
            f.put(q.actor[i],0x204,1);f.put(q.actor[i],0x850,3);f.put(q.actor[i],0x9c4,91u);f.put(q.actor[i],0x9c8,123u);f.put(q.actor[i],0x16c,i+1);
            f.put(q.actor[i],0xccc,-1);
        }
        f.put(q.group,8,0x2002u);f.put(q.group,12,0x1001u);f.put(q.group,16,0x9003u);f.put(q.group,20,0x3003u);f.put(q.group,0x48,4);
        ServerNpcGruntSquadLeaveBindings lb{};auto& sb=lb.sentence;auto& rb=lb.request;
        lb.context=&q;sb.speech.context=&q;rb.context=&q;sb.speech.original_vtable=0x2249aaa4;sb.entries=q.slots;sb.count=4;sb.speech.volume=1;sb.speech.level=75;
        sb.speech.squad_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& q=SquadFixture::self(p);assert(token==91);*out=q.group;*n=sizeof(q.group);return 1;};
        lb.member_class=[](void* p,void* a,std::uint32_t* vt,unsigned* n) noexcept -> int{auto& q=SquadFixture::self(p);assert(a==q.actor[1]||a==q.actor[2]);*vt=a==q.actor[1]?0x2248df04:0x2248c514;*n=sizeof(q.actor[0]);return 1;};
        sb.extent=[](void*,void*,unsigned* n) noexcept -> int{*n=0xdb2;return 1;};
        sb.entity_rejected=[](void*,void*,bool* out) noexcept -> int{*out=false;return 1;};
        sb.speech.random_float=[](void* p,float lo,float hi,float* out) noexcept -> int{auto& q=SquadFixture::self(p);assert(lo==1.5f&&hi==2);++q.randoms;*out=1.5f;return 1;};
        rb.manager_ready=true;
        rb.group_index=[](void* p,const char* cue,int* id) noexcept -> int{
            auto& q=SquadFixture::self(p);assert(!std::strcmp(cue,q.expected));
            // During notifications the removed member still owns its group, but is no longer in the list.
            if(q.requests==0){assert(SquadFixture::rd(q.actor[0],0x9c4)==91&&SquadFixture::rd(q.group,0x48)==3);}
            else{assert(SquadFixture::rd(q.actor[1],0x9c4)==91&&SquadFixture::rd(q.group,0x48)==2);}
            ++q.requests;*id=4;return 1;
        };
        rb.pick_name=[](void*,int id,char* s,unsigned n,int* out) noexcept -> int{assert(id==4&&n==64);std::memcpy(s,"!TEST",6);*out=0;return 1;};
        rb.name_index=[](void*,const char* name,int* id) noexcept -> int{assert(!std::strcmp(name,"TEST"));*id=19;return 1;};
        rb.submit=[](void* p,std::uint32_t token,int index,float volume,int level,int zero,std::uint32_t extra) noexcept -> int{auto& q=SquadFixture::self(p);assert(token==q.submissions+2&&index==19&&volume==1&&level==75&&!zero&&!extra);++q.submissions;return 1;};
        ServerNpcGruntSquadLeaveResult lr{};
        q.expected="GRUNT_MAN_DOWN";
        assert(api->leave(q.actor[0],sizeof(q.actor[0]),91,0x1001u,true,100,&lb,&lr)&&lr.completed&&lr.removed&&lr.cleared&&lr.index==1&&lr.remaining==3&&lr.native_notifications==2&&lr.stale==1&&q.requests==1&&q.submissions==1&&q.randoms==2);
        assert(lr.last.survivor.sentence.cooldown.priority_blocked&&SquadFixture::rd(q.actor[0],0x9c4)==0&&SquadFixture::rd(q.actor[0],0x9c8)==0&&SquadFixture::rd(q.group,8)==0x2002u&&SquadFixture::rd(q.group,12)==0x9003u&&SquadFixture::rd(q.group,16)==0x3003u&&SquadFixture::rd(q.group,20)==0x3003u);
        q.expected="GRUNT_LAST_OF_SQUAD";
        assert(api->leave(q.actor[1],sizeof(q.actor[1]),91,0x2002u,true,101,&lb,&lr)&&lr.completed&&lr.remaining==2&&lr.native_notifications==1&&lr.stale==1&&lr.last.survivor.last_member&&lr.last.survivor.final_mark.squad_written&&q.requests==2&&q.submissions==2&&q.randoms==4);
        f.put(q.actor[1],0x9c4,91u);f.put(q.actor[1],0x9c8,123u);
        assert(api->leave(q.actor[1],sizeof(q.actor[1]),91,0x2002u,true,102,&lb,&lr)&&lr.missing&&!lr.cleared&&SquadFixture::rd(q.actor[1],0x9c4)==91);
        assert(api->leave(q.actor[2],sizeof(q.actor[2]),91,0x3003u,false,102,&lb,&lr)&&lr.completed&&lr.remaining==1&&!lr.notifications&&lr.cleared&&q.requests==2);
    }
    {
        auto g=reinterpret_cast<GetServerNpcGruntSquadJoinApi>(GetProcAddress(s,server_npc_grunt_squad_join_export));
        assert(g&&!GetProcAddress(c,server_npc_grunt_squad_join_export)&&!g(0,sizeof(ServerNpcGruntSquadJoinApi))&&!g(1,0));
        const auto* api=g(1,sizeof(ServerNpcGruntSquadJoinApi));assert(api);
        struct JoinFixture {
            alignas(4) unsigned char actors[3][0xdb2]{},groups[2][0x58]{},memory[2][0x40]{},nodes[2][12]{},records[2][0x38]{};
            sin_independent::EntityHandleEntry slots[4]{};
            unsigned requests=0,submitted=0;
            static JoinFixture& self(void* p){return *static_cast<JoinFixture*>(p);}
            static unsigned rd(const void* p,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;}
        } q;
        for(unsigned i=0;i<3;++i){
            q.slots[i+1].serial=i+1;q.slots[i+1].entity=reinterpret_cast<CBaseEntity*>(q.actors[i]);
            f.put(q.actors[i],0x204,1);f.put(q.actors[i],0x850,1);f.put(q.actors[i],0x16c,i+1);f.put(q.actors[i],0xccc,-1);
        }
        for(unsigned i=0;i<2;++i){
            f.put(q.actors[i],0x984,201u+i);f.put(q.actors[i],0x9c4,91u+i);f.put(q.actors[i],0x9c8,123u+i);
            f.put(q.groups[i],4,123u+i);f.put(q.groups[i],8,(i+1)*0x1001u);f.put(q.groups[i],0x48,1);
            f.put(q.memory[i],0,0x22068d70u);f.put(q.memory[i],4,211u+i);f.put(q.memory[i],8,1u);f.put(q.memory[i],0x18,0x22068b40u);
            q.memory[i][0x10]=0;q.memory[i][0x11]=1;q.memory[i][0x12]=255;q.memory[i][0x13]=1;
            f.put(q.memory[i],0x1c,1.75f);f.put(q.memory[i],0x20,60.f);
            q.nodes[i][0]=q.nodes[i][1]=q.nodes[i][2]=255;q.nodes[i][3]=1;f.put(q.nodes[i],4,1000u);f.put(q.nodes[i],8,221u+i);
            f.put(q.records[i],0,0x3003u);f.put(q.records[i],0x1c,40.f);f.put(q.records[i],0x30,40.f);
        }
        f.put(q.records[0],4,30.f);q.records[1][0x35]=1;
        ServerNpcGruntSquadJoinBindings jb{};auto& sb=jb.group.sentence;auto& rb=jb.group.request;
        jb.group.context=&q;sb.speech.context=&q;rb.context=&q;sb.speech.original_vtable=0x2249aaa4;sb.entries=q.slots;sb.count=4;sb.speech.volume=1;sb.speech.level=75;
        sb.speech.squad_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& q=JoinFixture::self(p);if(token<91||token>92)return 0;*out=q.groups[token-91];*n=sizeof(q.groups[0]);return 1;};
        jb.group.member_class=[](void* p,void* a,std::uint32_t* vt,unsigned* n) noexcept -> int{assert(a==JoinFixture::self(p).actors[0]);*vt=0x2248df04;*n=0xdb2;return 1;};
        sb.extent=[](void*,void*,unsigned* n) noexcept -> int{*n=0xdb2;return 1;};
        sb.entity_rejected=[](void*,void*,bool* yes) noexcept -> int{*yes=false;return 1;};
        sb.speech.random_float=[](void*,float lo,float hi,float* out) noexcept -> int{assert(lo==1.5f&&hi==2);*out=1.5f;return 1;};
        rb.manager_ready=true;
        rb.group_index=[](void* p,const char* name,int* id) noexcept -> int{auto& q=JoinFixture::self(p);assert(!std::strcmp(name,"GRUNT_REFIND_ENEMY")&&JoinFixture::rd(q.actors[1],0x9c4)==91&&JoinFixture::rd(q.groups[0],0x48)==2&&JoinFixture::rd(q.groups[1],0x48)==0&&q.records[1][0x35]);++q.requests;*id=4;return 1;};
        rb.pick_name=[](void*,int id,char* name,unsigned n,int* out) noexcept -> int{assert(id==4&&n==64);std::memcpy(name,"!TEST",6);*out=0;return 1;};
        rb.name_index=[](void*,const char* name,int* id) noexcept -> int{assert(!std::strcmp(name,"TEST"));*id=19;return 1;};
        rb.submit=[](void* p,std::uint32_t token,int index,float volume,int level,int zero,std::uint32_t extra) noexcept -> int{assert(token==2&&index==19&&volume==1&&level==75&&!zero&&!extra);++JoinFixture::self(p).submitted;return 1;};
        jb.source_memory.context=&q;
        jb.source_memory.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            auto& q=JoinFixture::self(p);
            if(token>=201&&token<=202){*out=q.memory[token-201];*n=0x40;return 1;}
            if(token>=211&&token<=212){*out=q.nodes[token-211];*n=12;return 1;}
            if(token>=221&&token<=222){*out=q.records[token-221];*n=0x38;return 1;}
            return 0;
        };
        jb.report.mutation.storage=jb.source_memory;jb.report.mutation.context=&q;
        jb.report.mutation.entity_key=[](void* p,void* e,std::uint32_t* key) noexcept -> int{assert(e==JoinFixture::self(p).actors[2]);*key=1000;return 1;};
        ServerNpcGruntSquadJoinResult jr{};
        f.put(q.actors[1],0x8c,1);
        assert(api->join(q.actors[1],sizeof(q.actors[1]),91,77,0x2002u,100,&jb,&jr)&&jr.blocked&&!jr.detached);
        f.put(q.actors[1],0x8c,0);
        assert(api->join(q.actors[1],sizeof(q.actors[1]),91,77,0x2002u,100,&jb,&jr)&&jr.completed&&jr.detached&&jr.previous.cleared&&jr.appended&&jr.owner_written&&jr.first_resolved&&jr.index==1&&jr.count==2&&jr.reports==1&&jr.updated==1&&!jr.last_report.firsthand&&jr.dialogue.refind_calls==1&&jr.dialogue.refind_request.submitted&&q.requests==1&&q.submitted==1);
        assert(!q.records[1][0x35]&&JoinFixture::rd(q.records[1],4)==JoinFixture::rd(q.records[0],4)&&JoinFixture::rd(q.groups[0],0x4c)==77&&JoinFixture::rd(q.actors[1],0x9c8)==123);
        assert(api->join(q.actors[1],sizeof(q.actors[1]),91,77,0x2002u,100,&jb,&jr)&&jr.already_member&&!jr.appended);
        auto gr=reinterpret_cast<GetServerNpcGruntSquadReportApi>(GetProcAddress(s,server_npc_grunt_squad_report_export));
        assert(gr&&!GetProcAddress(c,server_npc_grunt_squad_report_export)&&!gr(0,sizeof(ServerNpcGruntSquadReportApi))&&!gr(1,0));
        const auto* reports=gr(1,sizeof(ServerNpcGruntSquadReportApi));assert(reports);
        struct ReportContext {
            JoinFixture* q;
            ServerNpcGruntMemoryReportBindings bound;
        } ctx{&q,jb.report};
        ctx.bound.original_vtable=0x2249aaa4;ctx.bound.mutation.entries=q.slots;ctx.bound.mutation.count=4;
        ctx.bound.mutation.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            auto& q=JoinFixture::self(p);
            if(token>=91&&token<=92){*out=q.groups[token-91];*n=0x58;return 1;}
            if(token>=201&&token<=202){*out=q.memory[token-201];*n=0x40;return 1;}
            if(token>=211&&token<=212){*out=q.nodes[token-211];*n=12;return 1;}
            if(token>=221&&token<=222){*out=q.records[token-221];*n=0x38;return 1;}
            return 0;
        };
        ServerNpcGruntSquadReportBindings srb{};srb.context=&ctx;srb.perception.dialogue.sentence=sb;srb.perception.dialogue.request=rb;
        srb.member_report=[](void* p,void* a,std::uint32_t* vt,unsigned* n,ServerNpcGruntMemoryReportBindings* out) noexcept -> int{
            auto& ctx=*static_cast<ReportContext*>(p);assert(a==ctx.q->actors[1]);*vt=0x2248df04;*n=0xdb2;*out=ctx.bound;return 1;
        };
        float pos[]{60,0,0};q.records[1][0x35]=1;
        ServerNpcGruntMemoryReportResult rr{};ServerNpcGruntSquadReportResult stats{};
        assert(reports->report(q.actors[0],sizeof(q.actors[0]),q.actors[2],pos,nullptr,110,&ctx.bound,&srb,&rr,&stats)&&rr.firsthand&&rr.mutation.updated&&rr.forwarded==1&&stats.native_reports==1&&stats.forwarded==1&&stats.updated==1&&!stats.last_member.firsthand&&stats.last_member.refind_cue&&stats.last_dialogue.refind_request.submitted&&q.requests==2&&q.submitted==2);
        assert(JoinFixture::rd(q.records[0],4)==JoinFixture::rd(q.records[1],4)&&!q.records[1][0x35]);
        q.actors[1][0xb6]=1;
        assert(reports->report(q.actors[0],sizeof(q.actors[0]),q.actors[2],pos,nullptr,111,&ctx.bound,&srb,&rr,&stats)&&stats.forwarded==1&&stats.blocked==1&&!stats.updated&&q.requests==2);
        q.actors[1][0xb6]=0;
        // Full-group eviction only removes the last array slot, NOT its owner linkage.
        for(unsigned i=0;i<15;++i)f.put(q.groups[0],8+4*i,0x1001u);
        f.put(q.groups[0],8+15*4,0x2002u);f.put(q.groups[0],0x48,16);
        q.actors[2][0xb6]=1;
        assert(api->join(q.actors[2],sizeof(q.actors[2]),91,77,0x3003u,101,&jb,&jr)&&jr.completed&&jr.evicted&&jr.evicted_handle==0x2002u&&jr.count==16&&jr.index==15&&jr.last_report.blocked&&q.requests==2&&JoinFixture::rd(q.actors[1],0x9c4)==91&&JoinFixture::rd(q.actors[1],0x9c8)==123);
        ServerNpcGruntSquadLeaveResult lr{};
        assert(api->detach(q.actors[1],sizeof(q.actors[1]),0x2002u,102,&jb.group,&lr)&&lr.completed&&lr.missing&&!lr.cleared&&JoinFixture::rd(q.actors[1],0x9c4)==0&&JoinFixture::rd(q.actors[1],0x9c8)==123);
    }
    {
        auto g=reinterpret_cast<GetServerNpcGruntSquadRegistryApi>(GetProcAddress(s,server_npc_grunt_squad_registry_export));
        assert(g&&!GetProcAddress(c,server_npc_grunt_squad_registry_export)&&!g(0,sizeof(ServerNpcGruntSquadRegistryApi))&&!g(1,0));
        const auto* api=g(1,sizeof(ServerNpcGruntSquadRegistryApi));assert(api);
        struct RegistryFixture{
            alignas(4) unsigned char groups[4][0x78]{},actor[0xdb2]{};
            sin_independent::EntityHandleEntry slots[2]{};
            unsigned allocated=0,interned=0,released=0;
            static RegistryFixture& self(void* p){return *static_cast<RegistryFixture*>(p);}
            static unsigned rd(const void* p,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;}
        } q;
        ServerNpcGruntSquadRegistryBindings rb{};rb.context=rb.storage.context=&q;
        rb.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& q=RegistryFixture::self(p);if(token<10||token>=10+q.allocated)return 0;*out=q.groups[token-10];*n=0x78;return 1;};
        rb.allocate=[](void* p,unsigned size,std::uint32_t* token,std::uint32_t* array,void** out,unsigned* n) noexcept -> int{auto& q=RegistryFixture::self(p);assert(size==0x78&&q.allocated<4);unsigned i=q.allocated++;*token=10+i;*array=20+i;*out=q.groups[i];*n=0x78;std::memset(*out,0xa5,*n);return 1;};
        rb.release=[](void* p,std::uint32_t) noexcept -> int{++RegistryFixture::self(p).released;return 1;};
        rb.intern=[](void* p,const char* name,std::uint32_t* key) noexcept -> int{auto& q=RegistryFixture::self(p);assert(!std::strcmp(name,"Alpha")||!std::strcmp(name,"Bravo"));++q.interned;*key=!std::strcmp(name,"Alpha")?31u:32u;return 1;};
        rb.name=[](void*,std::uint32_t token,const char** name,unsigned* n) noexcept -> int{assert(token==31||token==32);*name=token==31?"Alpha":"Bravo";*n=6;return 1;};
        ServerNpcGruntSquadRegistryResult rr{};std::uint32_t head=0;
        assert(api->lookup(head,"Alpha",&rb,&rr)&&rr.completed&&!rr.found&&!rr.visited);
        assert(api->create(&head,"Alpha",&rb,&rr)&&rr.constructed&&rr.linked&&rr.token==10&&head==10&&rr.name_token==31);
        assert(RegistryFixture::rd(q.groups[0],0)==0&&RegistryFixture::rd(q.groups[0],4)==31&&RegistryFixture::rd(q.groups[0],8)==0xa5a5a5a5u&&RegistryFixture::rd(q.groups[0],0x54)==0xa5a5a5a5u&&RegistryFixture::rd(q.groups[0],0x58)==0xffffffffu&&RegistryFixture::rd(q.groups[0],0x4c)==20&&RegistryFixture::rd(q.groups[0],0x70)==0);
        assert(api->create(&head,nullptr,&rb,&rr)&&rr.constructed&&!rr.linked&&rr.token==11&&!rr.name_token&&head==10&&q.interned==1);
        assert(api->lookup(head,nullptr,&rb,&rr)&&!rr.found&&rr.visited==1);
        assert(api->create(&head,"",&rb,&rr)&&rr.linked&&head==12&&RegistryFixture::rd(q.groups[2],0)==10&&!rr.name_token&&q.interned==1);
        assert(api->lookup(head,nullptr,&rb,&rr)&&rr.found&&rr.token==12&&rr.visited==1);
        assert(api->lookup(head,"aLPHa",&rb,&rr)&&rr.found&&rr.token==10&&rr.visited==2);
        ServerNpcGruntSquadJoinBindings jb{};jb.group.sentence.speech.original_vtable=0x2249aaa4;
        jb.group.sentence.speech.context=&q;jb.group.sentence.speech.squad_storage=rb.storage.storage;
        q.slots[1].serial=1;q.slots[1].entity=reinterpret_cast<CBaseEntity*>(q.actor);jb.group.sentence.entries=q.slots;jb.group.sentence.count=2;
        assert(api->attach(q.actor,sizeof(q.actor),0x1001u,50,&head,"ALPHA",&rb,&jb,&rr)&&rr.found&&!rr.allocated&&rr.join.appended&&rr.join.index==0&&RegistryFixture::rd(q.actor,0x9c4)==10&&RegistryFixture::rd(q.actor,0x9c8)==31);
        assert(api->attach(q.actor,sizeof(q.actor),0x1001u,51,&head,"Bravo",&rb,&jb,&rr)&&rr.linked&&rr.join.detached&&rr.join.appended&&head==13&&RegistryFixture::rd(q.actor,0x9c4)==13&&RegistryFixture::rd(q.groups[0],0x48)==0);
        auto gi=reinterpret_cast<GetServerNpcGruntSquadInputApi>(GetProcAddress(s,server_npc_grunt_squad_input_export));
        assert(gi&&!GetProcAddress(c,server_npc_grunt_squad_input_export)&&!gi(0,sizeof(ServerNpcGruntSquadInputApi))&&!gi(1,0));
        const auto* input=gi(1,sizeof(ServerNpcGruntSquadInputApi));assert(input);
        ServerNpcGruntSquadInputResult ir{};
        ServerNpcGruntSquadInputBindings ib{};ib.registry=rb;ib.join=jb;ib.context=&q;
        ib.weapon_capabilities=[](void* p,void* w,std::uint32_t* out) noexcept -> int{assert(w==RegistryFixture::self(p).actor);*out=0x04000000u;return 1;};
        assert(input->change(q.actor,sizeof(q.actor),0x1001u,51,&head,2,31,&ib,&ir)&&ir.capability_blocked&&RegistryFixture::rd(q.actor,0x9c4)==13&&RegistryFixture::rd(q.actor,0x9c8)==32);
        f.put(q.actor,0x78c,0x1001u); // synthetic live capability provider
        assert(input->change(q.actor,sizeof(q.actor),0x1001u,51,&head,2,31,&ib,&ir)&&ir.weapon_queried&&ir.attachment.join.detached&&ir.attachment.join.appended&&ir.owner_written&&RegistryFixture::rd(q.actor,0x9c4)==10&&RegistryFixture::rd(q.groups[3],0x48)==0);
        f.put(q.actor,0x78c,0xffffffffu);f.put(q.actor,0x8ec,0x04000000u);
        assert(input->change(q.actor,sizeof(q.actor),0x1001u,51,&head,1,32,&ib,&ir)&&ir.nonstring&&ir.clearing&&ir.departure.removed&&ir.owner_written&&RegistryFixture::rd(q.actor,0x9c4)==0&&RegistryFixture::rd(q.actor,0x9c8)==0);
        f.put(q.actor,0x8c,1);
        assert(input->change(q.actor,sizeof(q.actor),0x1001u,51,&head,2,32,&ib,&ir)&&ir.attachment.join.blocked&&ir.owner_written&&RegistryFixture::rd(q.actor,0x9c4)==13&&RegistryFixture::rd(q.groups[3],0x48)==0);
        assert(input->change(q.actor,sizeof(q.actor),0x1001u,51,&head,2,0,&ib,&ir)&&ir.departure.missing&&ir.owner_written&&RegistryFixture::rd(q.actor,0x9c4)==0);
        f.put(q.actor,0x8c,0);
        assert(input->change(q.actor,sizeof(q.actor),0x1001u,51,&head,2,32,&ib,&ir)&&ir.attachment.join.appended&&RegistryFixture::rd(q.actor,0x9c4)==13);
        f.put(q.groups[0],0,10u);assert(!api->lookup(head,"absent",&rb,&rr)&&!rr.completed);f.put(q.groups[0],0,0u);
        assert(q.allocated==4&&q.interned==2&&!q.released);
        auto gd=reinterpret_cast<GetServerNpcGruntSquadDestroyApi>(GetProcAddress(s,server_npc_grunt_squad_destroy_export));
        assert(gd&&!GetProcAddress(c,server_npc_grunt_squad_destroy_export)&&!gd(0,sizeof(ServerNpcGruntSquadDestroyApi))&&!gd(1,0));
        const auto* destroy=gd(1,sizeof(ServerNpcGruntSquadDestroyApi));assert(destroy);
        ServerNpcGruntSquadDestroyBindings db{};db.registry=rb;db.context=&q;
        db.release_records=[](void* p,std::uint32_t token) noexcept -> int{
            auto& q=RegistryFixture::self(p);assert(token==90&&RegistryFixture::rd(q.groups[0],0x6c)==0&&RegistryFixture::rd(q.groups[2],0)==0);++q.interned;return 1;
        };
        db.registry.release=[](void* p,std::uint32_t token) noexcept -> int{
            auto& q=RegistryFixture::self(p);assert(token>=10&&token<=13&&RegistryFixture::rd(q.groups[token-10],0x48)==0);++q.released;return 1;
        };
        ServerNpcGruntSquadDestroyResult dr{};
        assert(destroy->destroy(&head,13,&db,&dr)&&dr.empty_required&&!dr.group_released&&head==13);
        f.put(q.groups[0],0x60,90u);f.put(q.groups[0],0x64,16u);f.put(q.groups[0],0x6c,3u);f.put(q.groups[0],0x70,90u);
        assert(destroy->destroy(&head,10,&db,&dr)&&dr.completed&&dr.unlinked&&dr.predecessor==12&&!dr.successor&&dr.record_release_calls==1&&dr.inline_reset&&dr.group_released&&head==13&&q.released==1&&q.interned==3);
        assert(RegistryFixture::rd(q.groups[0],0x60)==0&&RegistryFixture::rd(q.groups[0],0x64)==0&&RegistryFixture::rd(q.groups[0],0x70)==0);
        f.put(q.groups[1],0x60,91u);f.put(q.groups[1],0x64,12u);f.put(q.groups[1],0x68,-1);f.put(q.groups[1],0x6c,2u);
        assert(destroy->destroy(&head,11,&db,&dr)&&dr.completed&&!dr.unlinked&&dr.external_records&&!dr.record_release_calls&&dr.group_released&&q.released==2);
        assert(RegistryFixture::rd(q.groups[1],0x60)==91&&RegistryFixture::rd(q.groups[1],0x64)==12&&RegistryFixture::rd(q.groups[1],0x70)==91&&RegistryFixture::rd(q.groups[1],0x6c)==0);
        auto gj=reinterpret_cast<GetServerNpcGruntSquadJoinApi>(GetProcAddress(s,server_npc_grunt_squad_join_export));assert(gj);
        ServerNpcGruntSquadLeaveResult lr{};assert(gj(1,sizeof(ServerNpcGruntSquadJoinApi))->detach(q.actor,sizeof(q.actor),0x1001u,52,&jb.group,&lr)&&lr.removed);
        assert(destroy->destroy(&head,13,&db,&dr)&&dr.completed&&dr.unlinked&&dr.successor==12&&!dr.predecessor&&head==12&&q.released==3);
        assert(destroy->destroy(&head,12,&db,&dr)&&dr.completed&&dr.unlinked&&!head&&q.released==4);
        assert(destroy->destroy(&head,0,&db,&dr)&&dr.completed&&!dr.group_released);
    }
    {
        auto g=reinterpret_cast<GetServerNpcGruntSquadSlotsApi>(GetProcAddress(s,server_npc_grunt_squad_slots_export));
        assert(g&&!GetProcAddress(c,server_npc_grunt_squad_slots_export)&&!g(0,sizeof(ServerNpcGruntSquadSlotsApi))&&!g(1,0));
        const auto* api=g(1,sizeof(ServerNpcGruntSquadSlotsApi));assert(api);
        struct SlotFixture {
            alignas(4) unsigned char actors[2][0xdb2]{},enemies[2][0x20]{},group[0x78]{},rows[64]{};
            sin_independent::EntityHandleEntry slots[5]{};
            unsigned capacity=0;
            static SlotFixture& self(void* p){return *static_cast<SlotFixture*>(p);}
            static unsigned rd(const void* p,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;}
        } q;
        for(unsigned i=0;i<2;++i){
            q.slots[i+1].serial=i+1;q.slots[i+1].entity=reinterpret_cast<CBaseEntity*>(q.actors[i]);
            q.slots[i+3].serial=i+3;q.slots[i+3].entity=reinterpret_cast<CBaseEntity*>(q.enemies[i]);
            f.put(q.actors[i],0x9c4,91u);f.put(q.actors[i],0x9cc,-1);f.put(q.actors[i],0x8a0,0x3003u);
            f.put(q.group,8+4*i,(i+1)*0x1001u);
        }
        f.put(q.group,0x48,2);
        ServerNpcGruntSquadSlotsBindings sb{};sb.context=sb.storage.context=&q;sb.entries=q.slots;sb.count=5;
        sb.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            auto& q=SlotFixture::self(p);if(token==91){*out=q.group;*n=sizeof(q.group);return 1;}if(token==200&&q.capacity){*out=q.rows;*n=q.capacity*8;return 1;}return 0;
        };
        sb.resize=[](void* p,std::uint32_t old,unsigned cap,std::uint32_t* token,void** out,unsigned* n) noexcept -> int{
            auto& q=SlotFixture::self(p);assert((!old&&cap==4)||(old==200&&cap==8));q.capacity=cap;*token=200;*out=q.rows;*n=cap*8;return 1;
        };
        sb.member_class=[](void* p,void* a,std::uint32_t* vt,unsigned* n) noexcept -> int{auto& q=SlotFixture::self(p);assert(a==q.actors[0]||a==q.actors[1]);*vt=0x2249aaa4;*n=0xdb2;return 1;};
        sb.entity_handle=[](void* p,void* e,std::uint32_t* out) noexcept -> int{auto& q=SlotFixture::self(p);if(e==q.enemies[0]){*out=0x3003u;return 1;}if(e==q.enemies[1]){*out=(q.slots[4].serial<<12)|4;return 1;}return 0;};
        ServerNpcGruntSquadSlotsResult sr{};
        assert(api->reserve(q.actors[0],sizeof(q.actors[0]),12,12,10,&sb,&sr)&&sr.allowed&&sr.new_set&&sr.appended==1&&sr.grown==1&&sr.refreshed==1&&sr.deadline==40&&sr.accesses==2&&sr.bitmap==(1u<<12));
        assert(api->reserve(q.actors[0],sizeof(q.actors[0]),12,12,10,&sb,&sr)&&sr.allowed&&sr.retained&&!sr.accesses);
        assert(api->reserve(q.actors[1],sizeof(q.actors[1]),12,12,10,&sb,&sr)&&!sr.allowed&&sr.accesses==1);
        assert(api->reserve(q.actors[1],sizeof(q.actors[1]),12,14,10,&sb,&sr)&&sr.allowed&&sr.selected==13&&sr.bitmap==((1u<<12)|(1u<<13)));
        assert(api->occupied(91,q.enemies[0],12,13,10,&sb,&sr)&&sr.allowed&&sr.accesses==2);
        assert(api->reserve(q.actors[0],sizeof(q.actors[0]),14,14,10,&sb,&sr)&&sr.allowed&&sr.old_cleared&&!sr.old_unmarked&&sr.accesses==4&&sr.bitmap==((1u<<13)|(1u<<14)));
        assert(api->release(91,q.enemies[0],13,10,&sb,&sr)&&sr.old_cleared&&!sr.old_unmarked&&sr.bitmap==(1u<<14)&&SlotFixture::rd(q.actors[1],0x9cc)==13);
        assert(api->release(91,q.enemies[0],13,10,&sb,&sr)&&sr.old_unmarked);
        assert(api->release(0,nullptr,-1,10,&sb,&sr)&&!sr.accesses);
        assert(api->bit(91,q.enemies[1],31,NpcSquadBitSet,11,&sb,&sr)&&sr.appended&&sr.bitmap==0x80000000u);
        assert(api->get(91,nullptr,11,&sb,&sr)&&sr.appended&&sr.index==2);
        q.slots[4].serial=5;
        assert(api->get(91,nullptr,40,&sb,&sr)&&sr.cached&&!sr.refreshed&&sr.index==2);
        assert(api->get(91,q.enemies[0],40.25f,&sb,&sr)&&sr.refreshed==1&&sr.refresh_members==2&&sr.pruned==2&&sr.index==0&&sr.bitmap==(1u<<14)&&sr.deadline==70.25f&&SlotFixture::rd(q.group,0x6c)==1);
        assert(api->get(91,q.enemies[1],40.25f,&sb,&sr)&&sr.appended&&sr.index==1&&!sr.bitmap&&SlotFixture::rd(q.rows,8)==0x5004u);
        assert(!api->bit(91,q.enemies[0],32,NpcSquadBitSet,40.25f,&sb,&sr));
        auto gn=reinterpret_cast<GetServerNpcGruntPerceptionSlotsApi>(GetProcAddress(s,server_npc_grunt_perception_slots_export));
        assert(gn&&!GetProcAddress(c,server_npc_grunt_perception_slots_export)&&!gn(0,sizeof(ServerNpcGruntPerceptionSlotsApi))&&!gn(1,0));
        const auto* native=gn(1,sizeof(ServerNpcGruntPerceptionSlotsApi));assert(native);
        ServerNpcEnemyStateBindings esb{};esb.context=&q;esb.entries=q.slots;esb.count=5;esb.conditions=eb.conditions;esb.entity_handle=sb.entity_handle;
        ServerNpcGruntPerceptionSlotsResult nsr{};ServerNpcEnemyStateResult er{};
        assert(native->set_enemy(q.actors[0],sizeof(q.actors[0]),q.enemies[1],true,40.5f,&esb,&sb,&er,&nsr)&&er.changed&&er.notified&&er.squad_released&&nsr.release_calls==1&&nsr.last_release.old_cleared&&!nsr.last_release.old_unmarked&&!nsr.last_release.bitmap&&er.new_handle==0x5004u&&SlotFixture::rd(q.actors[0],0x9cc)==0xffffffffu);
        assert(native->set_enemy(q.actors[0],sizeof(q.actors[0]),q.enemies[1],true,40.6f,&esb,&sb,&er,&nsr)&&!er.changed&&!nsr.release_calls);
        assert(native->set_enemy(q.actors[0],sizeof(q.actors[0]),nullptr,true,40.7f,&esb,&sb,&er,&nsr)&&er.changed&&nsr.release_calls==1&&!nsr.last_release.accesses);
        f.put(q.actors[0],0x9c4,0u);
        assert(api->reserve(q.actors[0],sizeof(q.actors[0]),12,12,41,&sb,&sr)&&sr.allowed&&sr.ungrouped&&!sr.accesses);
    }
    {
        auto g=reinterpret_cast<GetServerNpcGruntSquadEnemyApi>(GetProcAddress(s,server_npc_grunt_squad_enemy_export));
        assert(g&&!GetProcAddress(c,server_npc_grunt_squad_enemy_export)&&!g(0,sizeof(ServerNpcGruntSquadEnemyApi))&&!g(1,0));
        const auto* api=g(1,sizeof(ServerNpcGruntSquadEnemyApi));assert(api);
        struct BroadcastFixture {
            alignas(4) unsigned char actor[3][0xdb2]{},targets[2][0x20]{},group[0x78]{},bitmap[8]{},memory[0x40]{},node[12]{},record[0x38]{};
            sin_independent::EntityHandleEntry entries[6]{};
            ServerNpcConditionMapBindings conditions;
            ServerNpcEnemyMemoryBindings storage;
            static BroadcastFixture& self(void* p){return *static_cast<BroadcastFixture*>(p);}
            static unsigned rd(const void* p,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;}
        } q;
        q.conditions=eb.conditions;
        for(unsigned i=0;i<3;++i){
            q.entries[i+1].serial=i+1;q.entries[i+1].entity=reinterpret_cast<CBaseEntity*>(q.actor[i]);
            f.put(q.actor[i],0x9c4,91u);f.put(q.actor[i],0x9cc,-1);f.put(q.actor[i],0x8a0,0xffffffffu);f.put(q.group,8+4*i,(i+1)*0x1001u);
        }
        for(unsigned i=0;i<2;++i){q.entries[i+4].serial=i+4;q.entries[i+4].entity=reinterpret_cast<CBaseEntity*>(q.targets[i]);}
        f.put(q.actor[1],0x8a0,0x4004u);f.put(q.actor[1],0x984,103u);f.put(q.actor[1],0x9cc,7);
        f.put(q.actor[2],0x8a0,0x5005u);
        f.put(q.group,20,0x9002u);f.put(q.group,0x48,4);f.put(q.group,0x5c,100u);f.put(q.group,0x60,100u);f.put(q.group,0x64,1);f.put(q.group,0x6c,1);f.put(q.group,0x70,100u);f.put(q.group,0x74,130.f);
        f.put(q.bitmap,0,0x4004u);f.put(q.bitmap,4,1u<<7);
        f.put(q.memory,0,0x22068d70u);f.put(q.memory,4,104u);f.put(q.memory,8,1u);f.put(q.memory,0x18,0x22068b40u);
        q.memory[0x10]=0;q.memory[0x11]=1;q.memory[0x12]=255;q.memory[0x13]=1;
        q.node[0]=q.node[1]=q.node[2]=255;q.node[3]=1;f.put(q.node,4,1000u);f.put(q.node,8,105u);
        f.put(q.record,0,0x4004u);f.put(q.record,0x1c,97.f);
        q.storage.context=&q;q.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            auto& q=BroadcastFixture::self(p);switch(token){case 91:*out=q.group;*n=0x78;return 1;case 100:*out=q.bitmap;*n=8;return 1;case 103:*out=q.memory;*n=0x40;return 1;case 104:*out=q.node;*n=12;return 1;case 105:*out=q.record;*n=0x38;return 1;default:return 0;}
        };
        ServerNpcGruntSquadEnemyBindings bb{};bb.context=&q;bb.slots.storage=q.storage;bb.slots.entries=q.entries;bb.slots.count=6;
        bb.member=[](void* p,void* a,std::uint32_t* vt,unsigned* n,ServerNpcEnemyStateBindings* eb,ServerNpcEnemyMemoryQueryBindings* mb) noexcept -> int{
            auto& q=BroadcastFixture::self(p);assert(a==q.actor[0]||a==q.actor[1]||a==q.actor[2]);*vt=a==q.actor[1]?0x2248df04:0x2249aaa4;*n=0xdb2;
            eb->context=p;eb->conditions=q.conditions;
            eb->entity_handle=[](void* p,void* e,std::uint32_t* out) noexcept -> int{assert(e==BroadcastFixture::self(p).targets[1]);*out=0x5005u;return 1;};
            mb->storage=q.storage;mb->context=p;
            mb->entity_key=[](void* p,void* e,std::uint32_t* out) noexcept -> int{assert(e==BroadcastFixture::self(p).targets[0]);*out=1000;return 1;};
            return 1;
        };
        ServerNpcGruntSquadEnemyResult br{};
        assert(api->broadcast(91,nullptr,100,&bb,&br)&&br.null_target&&!br.visited);
        assert(api->broadcast(91,q.targets[1],100,&bb,&br)&&br.completed&&br.visited==4&&br.stale==1&&br.unassigned==1&&br.assigned==1&&br.recent==1&&br.same==1&&br.last_seen==97&&BroadcastFixture::rd(q.actor[0],0x8a0)==0x5005u&&BroadcastFixture::rd(q.actor[1],0x8a0)==0x4004u);
        f.put(q.actor[1],0x7cc,1u<<10);
        assert(api->broadcast(91,q.targets[1],101,&bb,&br)&&br.condition_blocked==1&&!br.assigned&&br.same==2);
        f.put(q.actor[1],0x7cc,0u);
        assert(api->broadcast(91,q.targets[1],101,&bb,&br)&&br.assigned==1&&br.same==2&&br.last_slots.release_calls==1&&br.last_slots.last_release.old_cleared&&BroadcastFixture::rd(q.bitmap,4)==0&&BroadcastFixture::rd(q.actor[1],0x8a0)==0x5005u&&BroadcastFixture::rd(q.actor[1],0x9cc)==0xffffffffu);
        auto gs=reinterpret_cast<GetServerNpcGruntNativeStateApi>(GetProcAddress(s,server_npc_grunt_native_state_export));
        assert(gs&&!GetProcAddress(c,server_npc_grunt_native_state_export)&&!gs(0,sizeof(ServerNpcGruntNativeStateApi))&&!gs(1,0));
        const auto* native=gs(1,sizeof(ServerNpcGruntNativeStateApi));assert(native);
        ServerNpcStateBindings state{};state.schedule.conditions=q.conditions;state.entities.entries=q.entries;state.entities.count=6;
        state.state_query=[](void*,void*,int current,int* out) noexcept -> int{assert(current==1);*out=3;return 1;};
        state.state_changed=[](void*,void*,int old,int next) noexcept -> int{assert(old==3&&next==1);return 1;};
        ServerNpcGruntNativeStateBindings nb{};nb.squad=bb;nb.original_vtable=0x2249aaa4;nb.context=&q;
        nb.target_classification=[](void* p,void* e,int* out) noexcept -> int{auto& q=BroadcastFixture::self(p);assert(e==q.targets[1]);*out=static_cast<int>(BroadcastFixture::rd(q.targets[1],0));return 1;};
        ServerNpcGruntNativeStateResult nr{};
        f.put(q.actor[0],0x850,1);f.put(q.actor[0],0x7cc,1u<<25);f.put(q.actor[1],0x8a0,0xffffffffu);
        assert(native->choose(q.actor[0],sizeof(q.actor[0]),102,&state,&nb,&nr)&&nr.state.squad_called&&nr.state.ideal==3&&nr.broadcast_calls==1&&nr.broadcast.assigned==1&&BroadcastFixture::rd(q.actor[1],0x8a0)==0x5005u);
        // Known original kill cue selection; no squad means mode2 rejects before engine request.
        f.put(q.actor[0],0x850,3);f.put(q.actor[0],0x9c4,0u);f.put(q.actor[0],0x7cc,1u<<29);
        for(int category:{1,10,7}){
            f.put(q.targets[1],0,category);
            assert(native->choose(q.actor[0],sizeof(q.actor[0]),103,&state,&nb,&nr)&&nr.state.lost_enemy_called&&nr.cue_calls==1&&nr.classification==category&&!nr.sentence.mode_allowed&&!std::strcmp(nr.cue,category==1?"GRUNT_PLAYER_DEAD":category==10?"GRUNT_KILLMUTANT":"GRUNT_KILL_MONST"));
        }
        f.put(q.actor[0],0x9c4,91u);
        assert(native->apply(q.actor[0],sizeof(q.actor[0]),1,104,&state,&nb,&nr)&&nr.state.changed&&nr.state.enemy_cleared&&nr.clear_calls==1&&nr.clear.squad_released&&nr.clear_slots.release_calls==1&&!nr.clear_slots.last_release.accesses&&BroadcastFixture::rd(q.actor[0],0x8a0)==0xffffffffu);
        auto gremove=reinterpret_cast<GetServerNpcGruntRemovalApi>(GetProcAddress(s,server_npc_grunt_removal_export));
        assert(gremove&&!GetProcAddress(c,server_npc_grunt_removal_export)&&!gremove(0,sizeof(ServerNpcGruntRemovalApi))&&!gremove(1,0));
        const auto* removal=gremove(1,sizeof(ServerNpcGruntRemovalApi));assert(removal);
        ServerNpcGruntRemovalBindings rb{};rb.context=&q;rb.leave.sentence.speech.original_vtable=0x2249aaa4;rb.leave.sentence.entries=q.entries;rb.leave.sentence.count=6;
        rb.leave.sentence.speech.context=&q;rb.leave.sentence.speech.squad_storage=q.storage.storage;
        rb.reports.perception.slots=bb.slots;rb.memory.mutation.storage=q.storage;
        rb.state4_link=[](void* p,void* a,void* link) noexcept -> int{auto& q=BroadcastFixture::self(p);assert(a==q.actor[0]&&link==q.targets[0]&&q.actor[0][0xa44]);q.actor[0][0xa45]=1;return 1;};
        rb.clear_link=[](void* p,void* a,void* link) noexcept -> int{auto& q=BroadcastFixture::self(p);assert(a==q.actor[0]&&link==q.targets[1]&&q.actor[0][0xa45]==1);q.actor[0][0xa45]=2;return 1;};
        rb.clear_owner=[](void* p,void* a) noexcept -> int{auto& q=BroadcastFixture::self(p);assert(a==q.actor[0]&&q.actor[0][0xa45]==2);q.actor[0][0xa45]=3;return 1;};
        rb.event=[](void* p,void* a,void* attacker) noexcept -> int{auto& q=BroadcastFixture::self(p);assert(a==q.actor[0]&&attacker==q.targets[1]&&q.actor[0][0xa45]==3);q.actor[0][0xa45]=4;return 1;};
        rb.finalize=[](void* p,void* a) noexcept -> int{auto& q=BroadcastFixture::self(p);assert(!BroadcastFixture::rd(a,0x9c4)&&!BroadcastFixture::rd(q.group,0x48));return 1;};
        f.put(q.group,8,0x1001u);f.put(q.group,0x48,1);f.put(q.actor[0],0x850,4);f.put(q.actor[0],0x970,0x4004u);f.put(q.actor[0],0xa18,0x5005u);
        f.put(q.actor[0],0x8a0,0x4004u);f.put(q.actor[0],0x9cc,7);f.put(q.bitmap,4,1u<<7);
        ServerNpcGruntRemovalResult rr{};
        assert(removal->run(q.actor[0],sizeof(q.actor[0]),0x1001u,q.targets[1],true,105,&rb,&rr)&&rr.completed&&rr.latched&&rr.state4_link&&rr.cleared_link&&rr.event&&rr.slot_released&&rr.release.old_cleared&&rr.departure.removed&&rr.owner_cleared&&rr.finalized&&!rr.report_checked&&q.actor[0][0xa45]==4&&!BroadcastFixture::rd(q.bitmap,4));
        assert(removal->run(q.actor[0],sizeof(q.actor[0]),0x1001u,q.targets[1],true,105,&rb,&rr)&&rr.repeated&&!rr.latched&&!rr.finalized);
        // A present zero-key record suppresses idle attacker report.
        q.actor[0][0xa44]=0;f.put(q.actor[0],0x850,1);f.put(q.actor[0],0xa18,0xffffffffu);f.put(q.actor[0],0x984,103u);f.put(q.node,4,0u);
        f.put(q.actor[0],0x9c4,91u);f.put(q.group,0x48,1);
        assert(removal->run(q.actor[0],sizeof(q.actor[0]),0x1001u,q.targets[1],false,106,&rb,&rr)&&rr.completed&&rr.report_checked&&rr.zero_key.record==105&&!rr.report_called&&rr.finalized);
        // Missing zero key calls report before removal; b6 suppression is native.
        q.actor[0][0xa44]=0;q.actor[0][0xb6]=1;f.put(q.node,4,1000u);f.put(q.actor[0],0x9c4,91u);f.put(q.group,0x48,1);
        rb.origin=[](void* p,void* a,float* pos) noexcept -> int{auto& q=BroadcastFixture::self(p);assert(a==q.actor[0]&&BroadcastFixture::rd(a,0x9c4)==91&&BroadcastFixture::rd(q.group,0x48)==1);pos[0]=1;pos[1]=2;pos[2]=3;return 1;};
        assert(removal->run(q.actor[0],sizeof(q.actor[0]),0x1001u,q.targets[1],false,107,&rb,&rr)&&rr.completed&&rr.report_checked&&!rr.zero_key.record&&rr.report_called&&rr.report.blocked&&rr.departure.removed&&rr.finalized);
    }
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native NPC state reconsideration and Grunt ideal-state transitions");
}
