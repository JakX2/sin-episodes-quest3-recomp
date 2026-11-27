#include "native_server_recombinant_task.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc60]{},desc[0x48]{},enemy[4]{};
    std::string calls;bool match=true,accepted=true;
    float draw=.2f;
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    bool has(int id){ServerNpcConditionResult r{};assert(ca->test(actor,sizeof(actor),id,NpcConditionCurrent,&conditions,&r)&&r.mapped);return r.value!=0;}
    void cond(int id,bool value){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,NpcConditionCurrent,value,&conditions,&r)&&r.mapped);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantTaskApi>(GetProcAddress(server,server_recombinant_task_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_task_export)&&!get(2,sizeof(ServerRecombinantTaskApi)));
    const auto* api=get(1,sizeof(ServerRecombinantTaskApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_major_vtable);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.conditions=f.conditions;
    policy.schedule_matches=[](void* p,void*,int id,bool ideal,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==39&&ideal);f.calls+='S';*out=f.match;return 1;};
    ServerRecombinantDecisionBindings decision{};decision.policy=&policy;decision.context=&f;
    decision.flinch_activity=[](void* p,void* a,int big,int gesture,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&!gesture&&(f.get<unsigned>(a,0x988)&0x40));f.calls+='F';*out=big?101:102;return 1;};
    decision.random_float=[](void* p,float lo,float hi,float* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert((lo==1&&hi==1.5f)||(lo==0&&hi==1));f.calls+='R';*out=f.draw;return 1;};
    ServerRecombinantTaskBindings b{};b.decision=&decision;b.context=&f;b.goal_owner=916;
    b.parent=[](void* p,void* a,const ServerNpcScheduleTask* t,bool running) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&t);f.calls+=running?'U':'P';return 1;};
    b.activity=[](void* p,void*,int v) noexcept ->int{auto& f=*static_cast<F*>(p);assert(v==101||v==102);f.calls+='A';return 1;};
    b.submit_goal=[](void* p,unsigned nav,const ServerRecombinantTaskGoal* g,int mode,bool* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(nav==44&&!mode&&g->kind==4&&g->position[0]==1&&g->position[1]==2&&g->position[2]==3&&g->field10==-1&&g->activity==10&&g->arrival_activity==-1&&g->arrival_sequence==-1&&g->distance==-1&&g->tolerance==-1&&!g->flags&&g->owner==916);f.calls+='G';*out=f.accepted;f.put(f.actor,0x8f8,45u);return 1;
    };
    b.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.enemy;f.put(f.actor,0x8f8,46u);return 1;};
    b.goal_enemy=[](void* p,unsigned nav,void* enemy) noexcept ->int{auto& f=*static_cast<F*>(p);assert(nav==45&&enemy==f.enemy);f.calls+='N';return 1;};
    b.failure_token=[](void* p,const char* s,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!std::strcmp(s,"Unable to navigate to physics attack target!\n")&&f.get<unsigned>(f.actor,0xc44)==0xffffffffu);f.calls+='X';*out=1234;return 1;};
    ServerRecombinantTaskResult o{};ServerNpcScheduleTask task{};
    auto call=[&](int id,bool running=false){task.id=id;return api->major(f.actor,sizeof(f.actor),&task,running,10.f,&b,&o);};
    for(int id=0;id<160;++id){assert(api->supports_major(id,false)==(id==54||id==55||id==69||id==121||id==139));assert(!api->supports_major(id,true));}
    f.draw=1.25f;f.put(f.actor,0x988,0x100u);
    assert(call(54)&&o.custom&&!o.delegated&&o.activity==102&&o.deadline==11.25f&&f.get<unsigned>(f.actor,0x988)==0x140u&&f.calls=="FAR");
    f.calls.clear();f.draw=1.5f;assert(call(55)&&o.activity==101&&o.deadline==11.5f&&f.calls=="FAR");
    f.calls.clear();f.match=false;f.put(f.actor,0xc30,.3f);assert(call(69)&&o.delegated&&!o.random_calls&&f.get<float>(f.actor,0xc30)==.3f&&f.calls=="SP");
    f.calls.clear();f.match=true;f.draw=.3f;assert(call(69)&&o.probability==.4f&&!f.has(69)&&f.calls=="SRP");
    f.calls.clear();f.draw=.39f;assert(call(69)&&o.probability==.2f&&f.has(69)&&o.condition_writes==1&&f.calls=="SRP");
    f.calls.clear();f.actor[0xc28]=1;assert(call(121)&&o.latch_cleared&&!f.actor[0xc28]&&f.calls=="P");
    f.calls.clear();f.put(f.actor,0x8f8,44u);f.put(f.actor,0xc48,1.f);f.put(f.actor,0xc4c,2.f);f.put(f.actor,0xc50,3.f);f.put(f.actor,0xc44,0x7001u);
    assert(call(139)&&o.goal_accepted&&o.lifecycle.completed&&f.get<int>(f.actor,0x7a4)==4&&f.get<unsigned>(f.actor,0xc44)==0x7001u&&f.calls=="GEN");
    f.calls.clear();f.put(f.actor,0x8f8,44u);f.put(f.actor,0x7a4,1);f.accepted=false;
    assert(call(139)&&o.handle_cleared&&o.failure==1234&&o.lifecycle.failed&&f.has(33)&&f.get<int>(f.actor,0x7b0)==1234&&f.calls=="GX");
    f.calls.clear();assert(call(139,true)&&!o.custom&&o.delegated&&f.calls=="U");
    f.calls.clear();assert(call(12)&&!o.custom&&o.delegated&&f.calls=="P");
    f.put(f.actor,0,recombinant_minor_vtable);assert(!call(54)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
