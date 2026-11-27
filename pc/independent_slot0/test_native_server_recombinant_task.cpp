#include "native_server_recombinant_task.h"
#include "native_server_recombinant_activity_transition.h"
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

    const auto get_composed=reinterpret_cast<GetServerRecombinantTaskActivityApi>(
        GetProcAddress(server,server_recombinant_task_activity_export));
    const auto get_pending=reinterpret_cast<GetServerRecombinantPendingActivityApi>(
        GetProcAddress(server,server_recombinant_pending_activity_export));
    const auto get_reg=reinterpret_cast<GetServerRecombinantRegistrationApi>(
        GetProcAddress(server,server_recombinant_registration_export));
    assert(get_composed&&get_pending&&get_reg&&!GetProcAddress(client,server_recombinant_task_activity_export)&&
        !get_composed(0,sizeof(ServerRecombinantTaskActivityApi))&&!get_composed(1,0));
    const auto* composed=get_composed(1,sizeof(ServerRecombinantTaskActivityApi));
    const auto* pending=get_pending(1,sizeof(ServerRecombinantPendingActivityApi));
    const auto* reg=get_reg(1,sizeof(ServerRecombinantRegistrationApi));assert(composed&&pending&&reg);
    struct AnimationHost {
        F* f;
        unsigned char rows[2][0x40]{};
        ServerActivityDescriptor descriptors[2]{};
        std::uint32_t gen=1;
        ServerActivityGenerationNode root{&gen,-1};
        ServerActivityCompositeView view{descriptors,2,&root,1};
        bool fail=false;
    } h{&f};
    for(int i=0;i<2;++i){
        h.descriptors[i]={h.rows[i],sizeof(h.rows[i]),0};
        h.rows[i][0xd]=0x10;f.put(h.rows[i],0x10,101+i);f.put(h.rows[i],0x14,1);
    }
    ServerEventDictionaryEntry ar[16]{},er[4]{};
    ServerActivityDictionary ad{{ar,16,0,1,920},0};ServerEventDictionary ed{er,4,0,1,0};
    ServerRecombinantActivityRegistration ids{};
    assert(reg->register_activities(RecombinantMajor,&ad,nullptr,&ids)&&reg->valid(&ids));
    h.gen=ad.names.generation;policy.activities=&ad;
    ServerRecombinantActivityTransitionBindings activity{};activity.policy=&policy;activity.major_activities=&ids;
    ServerNpcTransitionBindings nb{};auto& ab=nb.assignment;auto& tr=ab.translation;
    nb.context=&h;ab.context=&h;tr.context=&h;
    tr.actor_class=NpcActivityHosted;tr.activities=&ad;tr.events=&ed;
    tr.metadata=[](void* p,void* a,const ServerActivityCompositeView** out) noexcept ->int{
        auto& h=*static_cast<AnimationHost*>(p);assert(a==h.f->actor);h.f->calls+='Q';
        *out=h.fail?nullptr:&h.view;return h.fail?-1:1;
    };
    tr.weapon=[](void*,void*,int act,bool* required,int* out) noexcept ->int{*required=false;*out=act;return 1;};
    tr.other=[](void*,void*,int act,int* out) noexcept ->int{*out=act;return 1;};
    tr.random.random=[](void*,int lo,int hi,int,int* out) noexcept ->int{assert(lo==0&&hi==0);*out=0;return 1;};
    ab.reset.duration=[](void*,void*,int,double* out) noexcept ->int{*out=1.;return 1;};
    ab.reset.distance=[](void*,void*,int,double* out) noexcept ->int{*out=0.;return 1;};
    ab.reset.flags=[](void*,void*,int,unsigned* out) noexcept ->int{*out=1;return 1;};
    ab.dictionary.dictionary=&ed;
    ab.dictionary.descriptor=[](void*,void*,int,void**,unsigned*,unsigned*) noexcept ->int{return 0;};
    ab.locomotion=[](void*,void*,int act,bool* out) noexcept ->int{*out=act==6;return 1;};
    ab.view_vector=[](void*,void*,int,float* out) noexcept ->int{out[0]=out[1]=out[2]=0.f;return 1;};
    ab.component_notify=[](void* p,void* a,std::uint32_t c) noexcept ->int{
        auto& h=*static_cast<AnimationHost*>(p);assert(a==h.f->actor&&c==0x1000u);
        h.f->calls+='C';return 1;
    };
    ab.activity_changed=[](void*,void*,int) noexcept ->int{assert(false);return 0;};
    nb.nodes=[](void* p,void* a,const ServerTransitionView** out) noexcept ->int{
        auto& h=*static_cast<AnimationHost*>(p);assert(a==h.f->actor);
        *out=nullptr;return 0;
    };
    ServerRecombinantPendingActivityBindings pb{&nb,&activity};
    ServerRecombinantTaskActivityBindings cb{&b,&pb};
    ServerRecombinantTaskActivityResult cr{};
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0,recombinant_major_vtable);
    f.put(f.actor,0x908,0x1000u);f.put(f.actor,0x78c,0xffffffffu);
    f.put(f.actor,0x878,6);f.put(f.actor,0x880,6);f.actor[0x3e1]=1;
    auto composed_call=[&](int id,bool run=false){
        task.id=id;return composed->major(f.actor,sizeof(f.actor),&task,run,10.f,&cb,&cr);
    };
    f.calls.clear();f.draw=1.25f;
    assert(composed_call(54)&&cr.task.completed&&cr.task.activity==102&&cr.pending_calls==1&&
        cr.pending.pending.reselected&&!cr.pending.transition_calls&&
        f.get<int>(f.actor,0x880)==102&&f.get<int>(f.actor,0x884)==1&&f.get<int>(f.actor,0x878)==6&&
        f.get<float>(f.actor,0x9e0)==11.25f);
    assert(f.calls.front()=='F'&&f.calls.back()=='R'&&f.calls.find('A')==std::string::npos&&f.calls.find('Q')!=std::string::npos);
    ServerRecombinantPendingActivityResult pr{};
    f.calls.clear();
    assert(pending->update(f.actor,sizeof(f.actor),&pb,&pr)&&pr.pending.assignment.assigned&&
        pr.transition_calls==1&&pr.activity.previous==6&&pr.activity.activity==102&&
        f.get<int>(f.actor,0x878)==102&&f.calls.back()=='C');
    f.calls.clear();f.draw=1.5f;
    assert(composed_call(55)&&cr.task.activity==101&&cr.pending_calls==1&&
        f.get<int>(f.actor,0x884)==0&&f.get<int>(f.actor,0x878)==102&&f.get<float>(f.actor,0x9e0)==11.5f);
    f.calls.clear();h.fail=true;f.put(f.actor,0x9e0,99.f);
    assert(!composed_call(54)&&cr.pending_calls==1&&!cr.task.completed&&!cr.task.random_calls&&
        f.get<float>(f.actor,0x9e0)==99.f&&f.get<int>(f.actor,0x880)==102&&f.calls=="FQ");
    h.fail=false;f.calls.clear();
    assert(composed_call(12)&&cr.task.delegated==1&&!cr.pending_calls&&f.calls=="P");
    f.calls.clear();assert(composed_call(12,true)&&cr.task.delegated==1&&!cr.pending_calls&&f.calls=="U");
    f.calls.clear();ServerRecombinantPolicyBindings foreign=policy;activity.policy=&foreign;
    assert(!composed_call(54)&&!cr.pending_calls&&f.calls.empty());
    FreeLibrary(server);FreeLibrary(client);return 0;
}
