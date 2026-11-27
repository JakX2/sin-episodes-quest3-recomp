#include "native_server_activity_transition.h"
#include "native_server_recombinant_activity_transition.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xfcc]{},desc[3][0x40]{};
    ServerActivityDescriptor descriptors[3]{};
    std::uint32_t gen=3;ServerActivityGenerationNode root{&gen,-1};
    ServerActivityCompositeView activities{descriptors,3,&root,1};
    ServerTransitionEntry entries[3]{{1,1,false},{1,2,false},{2,2,false}};
    std::uint8_t matrix[4]{1,2,1,2};
    ServerTransitionView nodes{entries,3,2,matrix,4,false};
    unsigned notifications=0;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerNpcTransitionApi>(GetProcAddress(server,server_npc_transition_export));
    assert(get&&!GetProcAddress(client,server_npc_transition_export)&&!get(0,sizeof(ServerNpcTransitionApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerNpcTransitionApi));assert(api);
    F f;ServerTransitionChoice choice{};
    assert(api->choose(&f.nodes,0,2,1,true,&choice)&&choice.sequence==1&&choice.direction==1&&choice.scanned==2);
    f.entries[1]={2,1,true};
    assert(api->choose(&f.nodes,0,2,1,false,&choice)&&choice.sequence==1&&choice.direction==-1);
    assert(api->choose(&f.nodes,0,2,1,true,&choice)&&choice.sequence==-1&&choice.direction==-1);
    f.entries[1].reversible=false;
    assert(api->choose(&f.nodes,0,2,1,true,&choice)&&choice.sequence==2&&choice.missing_edge);
    f.matrix[1]=0;assert(api->choose(&f.nodes,0,2,1,true,&choice)&&choice.sequence==2&&!choice.missing_edge&&!choice.scanned);
    f.nodes.composite=true;assert(api->choose(&f.nodes,0,2,1,true,&choice)&&choice.missing_edge);
    f.nodes.composite=false;f.matrix[1]=2;f.entries[1]={1,2,false};
    assert(api->choose(&f.nodes,-1,2,-1,false,&choice)&&choice.sequence==2&&choice.direction==-1);
    assert(api->choose(nullptr,0,2,1,true,&choice)&&choice.sequence==2);
    assert(api->choose(&f.nodes,1,2,1,true,&choice)&&choice.sequence==2&&!choice.scanned);
    int activities[]{6,40,10};
    for(int i=0;i<3;++i){f.descriptors[i]={f.desc[i],0x40,0};f.desc[i][0xd]=0x10;f.put(f.desc[i],0x10,activities[i]);f.put(f.desc[i],0x14,1);}
    ServerEventDictionaryEntry ar[4]{},er[4]{};
    ServerActivityDictionary a{{ar,4,0,3,0},-1};ServerEventDictionary e{er,4,0,0,0};
    ServerNpcTransitionBindings b{};b.context=&f;auto& as=b.assignment;as.context=&f;
    auto& t=as.translation;t.context=&f;t.actor_class=NpcActivityGrunt;t.activities=&a;t.events=&e;
    t.metadata=[](void* p,void*,const ServerActivityCompositeView** out) noexcept -> int {*out=&F::self(p).activities;return 1;};
    t.weapon=[](void*,void*,int in,bool* required,int* out) noexcept -> int {*required=false;*out=in;return 1;};
    t.grunt.component_identity=[](void*,void*,std::uint32_t,bool* out) noexcept -> int {*out=false;return 1;};
    t.grunt.parent_translate=[](void*,void*,int in,int* out) noexcept -> int {*out=in;return 1;};
    t.random.random=[](void*,int lo,int hi,int,int* out) noexcept -> int {assert(lo==0&&hi==0);*out=0;return 1;};
    as.reset.duration=[](void*,void*,int,double* out) noexcept -> int {*out=1.;return 1;};
    as.reset.distance=[](void*,void*,int,double* out) noexcept -> int {*out=10.;return 1;};
    as.reset.flags=[](void*,void*,int seq,unsigned* out) noexcept -> int {*out=seq==1?0u:1u;return 1;};
    as.dictionary.dictionary=&e;
    as.dictionary.descriptor=[](void*,void*,int,void**,unsigned*,unsigned*) noexcept -> int {return 0;};
    as.capabilities=[](void*,void*,unsigned* out) noexcept -> int {*out=0;return 1;};
    as.component_notify=[](void* p,void*,std::uint32_t c) noexcept -> int {assert(c==0x1000);++F::self(p).notifications;return 1;};
    b.nodes=[](void* p,void*,const ServerTransitionView** out) noexcept -> int {*out=&F::self(p).nodes;return 1;};
    f.put(f.actor,0x908,0x1000u);f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x878,6);f.put(f.actor,0x880,6);
    f.actor[0x3e1]=1;f.put(f.actor,0x3e8,.5f);
    ServerNpcTransitionResult out{};
    assert(api->set_pending(f.actor,sizeof(f.actor),2,&b,&out)&&out.skipped&&f.get<int>(0x880)==6);
    assert(api->set_pending(f.actor,sizeof(f.actor),154,&b,&out)&&out.skipped&&f.get<int>(0x880)==154);
    assert(api->update(f.actor,sizeof(f.actor),&b,&out)&&out.skipped);
    assert(api->set_pending(f.actor,sizeof(f.actor),10,&b,&out)&&out.reselected&&f.get<int>(0x884)==2&&f.get<int>(0x878)==6);
    assert(api->update(f.actor,sizeof(f.actor),&b,&out)&&out.reselected&&out.intermediate&&out.assignment.assigned&&out.transition.sequence==1);
    assert(f.get<int>(0x878)==2&&f.get<int>(0x3ec)==1&&f.get<int>(0x880)==10&&f.get<int>(0x884)==2&&f.get<int>(0x87c)==40&&f.notifications==1);
    assert(api->update(f.actor,sizeof(f.actor),&b,&out)&&out.skipped&&f.notifications==1);
    f.actor[0x3e0]=1;f.put(f.actor,0x3e8,.99f);
    assert(api->update(f.actor,sizeof(f.actor),&b,&out)&&!out.reselected&&!out.intermediate&&out.transition.sequence==2&&out.assignment.assigned);
    assert(f.get<int>(0x878)==10&&f.get<int>(0x3ec)==2&&f.get<int>(0x87c)==10&&f.notifications==2);
    assert(api->update(f.actor,sizeof(f.actor),&b,&out)&&out.skipped);
    f.put(f.actor,0x8c,2);assert(api->set_pending(f.actor,sizeof(f.actor),6,&b,&out)&&out.reselected);
    assert(api->update(f.actor,sizeof(f.actor),&b,&out)&&out.skipped&&f.get<int>(0x878)==10);

    // Exercise deferred activity changes through the native mutant class
    // callback, not the original hosted activity_changed service.
    auto get_mutant=reinterpret_cast<GetServerRecombinantPendingActivityApi>(
        GetProcAddress(server,server_recombinant_pending_activity_export));
    assert(get_mutant&&!GetProcAddress(client,server_recombinant_pending_activity_export)&&
        !get_mutant(0,sizeof(ServerRecombinantPendingActivityApi))&&!get_mutant(1,0));
    const auto* mutant=get_mutant(1,sizeof(ServerRecombinantPendingActivityApi));assert(mutant);
    struct ScalarHost {F* fixture;unsigned draws,calls;float value;int previous;};
    ScalarHost scalar{&f,0,0,.125f,-1};
    ServerRecombinantActivityTransitionBindings activity{};
    activity.context=&scalar;
    activity.random_float=[](void* p,float lo,float hi,float* out) noexcept ->int{
        auto& s=*static_cast<ScalarHost*>(p);assert(lo==0.f&&hi==.25f);
        ++s.draws;*out=s.value;return 1;
    };
    activity.set_minor_scalar=[](void* p,void* a,float v) noexcept ->int{
        auto& s=*static_cast<ScalarHost*>(p);assert(a==s.fixture->actor&&v==s.value);
        ++s.calls;s.previous=s.fixture->get<int>(0x878);return 1;
    };
    as.translation.actor_class=NpcActivityHosted;
    as.translation.other=[](void*,void*,int in,int* out) noexcept ->int{*out=in;return 1;};
    as.locomotion=[](void*,void*,int act,bool* out) noexcept ->int{*out=act==6||act==10;return 1;};
    as.view_vector=[](void*,void*,int,float* out) noexcept ->int{out[0]=out[1]=out[2]=0.f;return 1;};
    as.view_dirty=[](void*,void*) noexcept ->int{return 1;};
    as.activity_changed=[](void*,void*,int) noexcept ->int{assert(false);return 0;};
    auto seed_actor=[&](unsigned vt,int old){
        std::memset(f.actor,0,sizeof(f.actor));f.notifications=0;
        f.put(f.actor,0,vt);f.put(f.actor,0x908,0x1000u);
        f.put(f.actor,0x78c,0xffffffffu);f.put(f.actor,0x878,old);f.put(f.actor,0x880,old);
        f.actor[0x3e1]=1;f.put(f.actor,0x3e8,.5f);
    };
    seed_actor(recombinant_minor_vtable,6);
    ServerRecombinantPendingActivityBindings mb{&b,&activity};
    ServerRecombinantPendingActivityResult mo{};
    assert(mutant->set_pending(f.actor,sizeof(f.actor),10,&mb,&mo)&&mo.pending.reselected&&
        !mo.transition_calls&&f.get<int>(0x878)==6&&f.get<int>(0x884)==2);
    assert(mutant->update(f.actor,sizeof(f.actor),&mb,&mo)&&mo.pending.intermediate&&
        mo.pending.assignment.assigned&&mo.transition_calls==1&&mo.activity.completed&&mo.activity.activity==2);
    assert(f.get<int>(0x878)==2&&f.get<int>(0x3ec)==1&&f.notifications==1&&!scalar.calls&&!scalar.draws);
    assert(mutant->update(f.actor,sizeof(f.actor),&mb,&mo)&&mo.pending.skipped&&!mo.transition_calls);
    f.actor[0x3e0]=1;
    assert(mutant->update(f.actor,sizeof(f.actor),&mb,&mo)&&mo.pending.assignment.assigned&&
        mo.transition_calls==1&&mo.activity.activity==10&&mo.activity.scalar_calls==1);
    assert(f.get<int>(0x878)==10&&f.get<int>(0x3ec)==2&&f.notifications==2&&
        scalar.draws==1&&scalar.calls==1&&scalar.previous==2);
    assert(mutant->update(f.actor,sizeof(f.actor),&mb,&mo)&&mo.pending.skipped&&!mo.transition_calls&&scalar.calls==1);

    // Rejected scalar draw leaves the requested activity unpublished;
    // earlier sequence changes are not rolled back by the wrapper.
    seed_actor(recombinant_minor_vtable,6);scalar.value=.5f;
    assert(mutant->set_pending(f.actor,sizeof(f.actor),10,&mb,&mo));
    assert(mutant->update(f.actor,sizeof(f.actor),&mb,&mo)&&mo.pending.intermediate);
    f.actor[0x3e0]=1;
    assert(!mutant->update(f.actor,sizeof(f.actor),&mb,&mo)&&mo.transition_calls==1&&
        !mo.activity.completed&&!mo.pending.assignment.assigned&&f.get<int>(0x878)==2&&
        f.get<int>(0x3ec)==2&&f.notifications==1&&scalar.calls==1&&scalar.draws==2);

    // Major uses a real generation-bound registration and the original
    // previous activity when the final pending animation is published.
    const auto get_reg=reinterpret_cast<GetServerRecombinantRegistrationApi>(
        GetProcAddress(server,server_recombinant_registration_export));
    assert(get_reg);
    const auto* reg=get_reg(1,sizeof(ServerRecombinantRegistrationApi));assert(reg);
    ServerEventDictionaryEntry mr[16]{};
    ServerActivityDictionary md{{mr,16,0,1,920},0};
    ServerRecombinantActivityRegistration ids{};
    assert(reg->register_activities(RecombinantMajor,&md,nullptr,&ids)&&reg->valid(&ids));
    ServerRecombinantPolicyBindings policy{};policy.activities=&md;
    activity.policy=&policy;activity.major_activities=&ids;
    as.translation.activities=&md;f.gen=md.names.generation;
    f.put(f.desc[2],0x10,29);
    b.nodes=[](void*,void*,const ServerTransitionView** out) noexcept ->int{*out=nullptr;return 0;};
    seed_actor(recombinant_major_vtable,27);f.actor[0xc14]=f.actor[0xc15]=1;
    assert(mutant->set_pending(f.actor,sizeof(f.actor),29,&mb,&mo)&&!mo.transition_calls);
    assert(mutant->update(f.actor,sizeof(f.actor),&mb,&mo)&&mo.transition_calls==1&&
        mo.activity.kind==RecombinantMajor&&mo.activity.previous==27&&mo.activity.latch_cleared&&
        f.get<int>(0x878)==29&&!f.actor[0xc14]&&!f.actor[0xc15]&&f.notifications==1);
    ++md.names.generation;
    assert(!mutant->set_pending(f.actor,sizeof(f.actor),27,&mb,&mo)&&!mo.transition_calls&&
        f.get<int>(0x880)==29&&f.notifications==1);
    assert(!mutant->update(f.actor,0xc15,&mb,&mo)&&!mo.transition_calls);
    seed_actor(0u,6);
    assert(!mutant->set_pending(f.actor,sizeof(f.actor),10,&mb,&mo)&&!mo.transition_calls&&
        f.get<int>(0x880)==6);
    assert(!mutant->update(f.actor,sizeof(f.actor),nullptr,&mo)&&!mo.transition_calls);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC pending activity transition and forward-only studio node selection");
}
