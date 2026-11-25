#include "native_server_npc_task_lifecycle.h"
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
    alignas(4) unsigned char actor[0xbf4]{},desc[0x48]{};
    std::vector<int> trace;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcTaskLifecycleApi>(GetProcAddress(s,server_npc_task_lifecycle_export));
    assert(g&&!GetProcAddress(c,server_npc_task_lifecycle_export)&&!g(0,sizeof(ServerNpcTaskLifecycleApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcTaskLifecycleApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base+10);f.put(f.desc,0x38,1);f.put(f.desc,0x3c,127);
    ServerNpcTaskLifecycleBindings b{};b.context=&f;b.conditions.context=&f;
    b.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);f.trace.push_back(4);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.random_int=[](void* p,unsigned lo,unsigned hi,unsigned* out) noexcept -> int{assert(lo==2&&hi==5);F::self(p).trace.push_back(1);*out=3;return 1;};
    b.random_float=[](void* p,float lo,float hi,double* out) noexcept -> int{auto& f=F::self(p);assert(lo==.5f&&hi==1.f&&f.get<std::uint16_t>(0x9ae)==3);f.trace.push_back(2);*out=.75;return 1;};
    b.cleanup=[](void* p,void* actor) noexcept -> int{auto& f=F::self(p);assert(actor==f.actor&&!f.actor[0x9ec]);f.trace.push_back(3);return 1;};
    b.failure_name=[](void* p,int code,std::uint32_t* out) noexcept -> int{assert(code==7);F::self(p).trace.push_back(5);*out=99;return 1;};
    f.put(f.actor,0x9b0,std::uint16_t{2});f.put(f.actor,0x9b2,std::uint16_t{5});f.put(f.actor,0x9b4,.5f);f.put(f.actor,0x9b8,1.f);
    f.actor[0x9ec]=1;f.put(f.actor,0x7a4,1);
    ServerNpcTaskLifecycleResult out{};
    assert(api->complete(f.actor,sizeof(f.actor),10,false,&b,&out)&&out.completed&&out.cleanup&&out.randomized&&out.status==4);
    assert(f.get<float>(0x9a8)==10.75f&&(f.trace==std::vector<int>{1,2,3,4}));
    f.trace.clear();f.actor[0x9ec]=1;f.put(f.actor,0x7a4,1);f.put(f.actor,0x798,42u);f.actor[0x989]=0xff;b.record_failure=true;
    assert(api->fail(f.actor,sizeof(f.actor),10.5f,7,&b,&out)&&out.failed&&out.cleanup&&!out.randomized&&out.recorded);
    assert(f.get<int>(0x7b0)==7&&f.get<unsigned>(0x7d0)==(1u<<10)&&f.get<unsigned>(0xbec)==42&&f.get<unsigned>(0xbe4)==99&&f.actor[0x989]==0xdf);
    assert((f.trace==std::vector<int>{3,5,4}));
    f.trace.clear();assert(api->complete(f.actor,sizeof(f.actor),11,false,&b,&out)&&!out.completed&&out.status==1&&(f.trace==std::vector<int>{4}));
    f.trace.clear();assert(api->complete(f.actor,sizeof(f.actor),11,true,&b,&out)&&out.completed&&f.trace.empty());
    unsigned yes=9;f.actor[0x3e0]=1;f.put(f.actor,0x3ec,3);f.put(f.actor,0x884,4);
    assert(api->animation_finished(f.actor,sizeof(f.actor),&yes)&&!yes);f.put(f.actor,0x884,3);
    assert(api->animation_finished(f.actor,sizeof(f.actor),&yes)&&yes);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native task lifecycle and finished animation predicate");
}
