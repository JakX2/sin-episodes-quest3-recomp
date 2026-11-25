#include "native_server_activity_assignment.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x1028]{},descriptors[3][0x40]{};
    ServerActivityDescriptor rows[3]{};
    std::uint32_t gen=4;ServerActivityGenerationNode root{&gen,-1};
    ServerActivityCompositeView view{rows,3,&root,1};
    std::vector<int> trace;bool available=true;unsigned caps=0;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
    void seed(int i,int activity){
        rows[i]={descriptors[i],sizeof(descriptors[i]),0};descriptors[i][0xd]=0x10;put(descriptors[i],0x10,activity);put(descriptors[i],0x14,1);
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerNpcActivityAssignmentApi>(GetProcAddress(server,server_npc_activity_assignment_export));
    assert(get&&!GetProcAddress(client,server_npc_activity_assignment_export)&&!get(0,sizeof(ServerNpcActivityAssignmentApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerNpcActivityAssignmentApi));assert(api);
    F f;f.seed(0,199);f.seed(1,6);f.seed(2,10);
    ServerEventDictionaryEntry ar[4]{},er[4]{};ServerActivityDictionary a{{ar,4,0,4,0},-1};ServerEventDictionary e{er,4,0,1,0};
    ServerNpcActivityAssignmentBindings b{};b.context=&f;b.translation.context=&f;b.translation.actor_class=NpcActivityGrunt;
    b.translation.activities=&a;b.translation.events=&e;b.translation.grunt.context=&f;b.translation.random.context=&f;
    b.translation.metadata=[](void* p,void* a,const ServerActivityCompositeView** out) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor);*out=f.available?&f.view:nullptr;return f.available;};
    b.translation.weapon=[](void*,void*,int i,bool* required,int* out) noexcept -> int {*required=false;*out=i;return 1;};
    b.translation.grunt.component_identity=[](void*,void*,std::uint32_t,bool* out) noexcept -> int {*out=false;return 1;};
    b.translation.grunt.parent_translate=[](void*,void*,int i,int* out) noexcept -> int {*out=i;return 1;};
    b.translation.random.random=[](void*,int lo,int hi,int,int* out) noexcept -> int {assert(lo==0&&hi==0);*out=0;return 1;};
    b.reset.context=&f;
    b.reset.duration=[](void* p,void*,int seq,double* out) noexcept -> int {F::self(p).trace.push_back(10+seq);*out=2.;return 1;};
    b.reset.distance=[](void* p,void*,int,double* out) noexcept -> int {F::self(p).trace.push_back(20);*out=100.;return 1;};
    b.reset.flags=[](void* p,void*,int,unsigned* out) noexcept -> int {F::self(p).trace.push_back(30);*out=1;return 1;};
    b.dictionary.context=&f;b.dictionary.dictionary=&e;
    b.dictionary.descriptor=[](void* p,void*,int,void**,unsigned*,unsigned*) noexcept -> int {F::self(p).trace.push_back(40);return 0;};
    b.weapon_notify=[](void* p,void*,std::uint32_t h,int activity,float duration) noexcept -> int {
        auto& f=F::self(p);assert(h==0x1234&&duration==2.f);f.trace.push_back(100+activity);return 1;};
    b.capabilities=[](void* p,void*,unsigned* out) noexcept -> int {auto& f=F::self(p);f.trace.push_back(50);*out=f.caps;return 1;};
    b.component_notify=[](void* p,void*,std::uint32_t c) noexcept -> int {
        auto& f=F::self(p);assert(c==0x5678);f.trace.push_back(60);return 1;};
    f.put(f.actor,0x1f0,std::uint8_t{1});f.put(f.actor,0x908,0x5678u);f.put(f.actor,0x78c,0x1234u);
    f.put(f.actor,0x878,1);f.put(f.actor,0x880,1);f.put(f.actor,0x3ec,2);f.put(f.actor,0x3e8,.75f);
    f.actor[0x3e1]=1;f.actor[0xca0]=1;f.actor[0xc35]=1;f.put(f.actor,0xfc8,8);
    ServerNpcActivityAssignmentResult out{};
    assert(api->request(f.actor,sizeof(f.actor),16,&b,&out)&&out.assigned&&out.precleared_cycle&&out.activity_changed&&out.view_changed&&out.component_notified);
    assert(f.get<int>(0x878)==16&&f.get<int>(0x880)==16&&f.get<int>(0x884)==0&&f.get<int>(0x888)==199&&f.get<int>(0x88c)==16&&f.get<int>(0x87c)==199);
    assert(f.get<int>(0x3ec)==0&&f.get<float>(0x3e8)==0.f&&f.get<float>(0x384)==40.f&&!f.actor[0xc35]&&f.get<int>(0xfc8)==-1);
    assert((f.trace==std::vector<int>{10,20,30,40,10,116,50,60}));
    f.trace.clear();f.actor[0xc35]=1;f.put(f.actor,0xfc8,9);
    assert(api->request(f.actor,sizeof(f.actor),16,&b,&out)&&out.skipped&&!out.assigned&&f.trace.empty()&&f.actor[0xc35]&&f.get<int>(0xfc8)==-1);
    f.put(f.actor,0x878,2);f.put(f.actor,0x880,6);
    assert(api->request(f.actor,sizeof(f.actor),10,&b,&out)&&out.skipped&&f.get<int>(0x880)==6);
    f.put(f.actor,0x878,6);f.put(f.actor,0x3ec,1);f.put(f.actor,0x3e8,.5f);f.actor[0xca0]=0;
    assert(api->request(f.actor,sizeof(f.actor),10,&b,&out)&&out.assigned&&!out.precleared_cycle&&out.sequence.reset.reset);
    assert(f.get<int>(0x3ec)==2&&f.get<float>(0x3e8)==.5f&&f.get<float>(0x384)==66.f&&f.get<int>(0x878)==10);
    f.trace.clear();f.available=false;assert(api->request(f.actor,sizeof(f.actor),6,&b,&out)&&out.skipped&&f.trace.empty());
    f.available=true;f.trace.clear();f.put(f.actor,0x78c,0xffffffffu);
    ServerNpcActivityAssignmentInput direct{6,-1,6,6};
    assert(api->assign(f.actor,sizeof(f.actor),&direct,&b,&out)&&out.assigned&&!out.weapon_notified&&f.get<int>(0x3ec)==0);
    assert((f.trace==std::vector<int>{10,20,30,40,50,60}));
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC activity assignment and Grunt request gates");
}
