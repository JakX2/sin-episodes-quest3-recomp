#include "native_server_activity_transition.h"
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
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC pending activity transition and forward-only studio node selection");
}
