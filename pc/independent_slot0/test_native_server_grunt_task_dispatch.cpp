#include "native_server_grunt_task_dispatch.h"
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
struct F{
    alignas(4) unsigned char actor[0xfcc]{},peer[0xfcc]{},helper[0xfcc]{},desc[0x48]{};
    ServerGruntTaskPeer peers[2]{{peer,sizeof(peer)},{actor,sizeof(actor)}};
    sin_independent::EntityHandleEntry slots[2]{};
    std::vector<int> trace;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerGruntTaskDispatchApi>(GetProcAddress(s,server_grunt_task_dispatch_export));
    assert(g&&!GetProcAddress(c,server_grunt_task_dispatch_export)&&!g(0,sizeof(ServerGruntTaskDispatchApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerGruntTaskDispatchApi));assert(api);
    unsigned start=0,run=0,route=0;
    for(int id=-1;id<200;++id){assert(api->route(id,false,&route));start+=route!=GruntTaskParent;assert(api->route(id,true,&route));run+=route!=GruntTaskParent;}
    assert(start==26&&run==11);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.peer);
    ServerGruntTaskBindings b{};b.context=&f;b.entities={f.slots,2,nullptr,nullptr,nullptr};
    b.lifecycle.conditions.context=&f;b.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.parent=[](void* p,void* a,const ServerNpcScheduleTask* t,bool run) noexcept -> int{auto& f=F::self(p);if(!run)assert(f.get<float>(a,0x9f0)==.75f);f.trace.push_back(t->id);return 1;};
    b.special=[](void* p,void*,const ServerNpcScheduleTask* t,bool) noexcept -> int{F::self(p).trace.push_back(1000+t->id);return 1;};
    b.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(token==1)*out=f.peer;else if(token==2)*out=f.helper;else return 0;*n=sizeof(f.peer);return 1;};
    b.peers=[](void* p,std::uint32_t token,const ServerGruntTaskPeer** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==3);*out=f.peers;*n=2;return 1;};
    b.cue=[](void* p,void*,unsigned slot,int x,int y) noexcept -> int{F::self(p).trace.push_back(slot);assert((slot==0x7e0&&x==2&&y==0)||(slot==0x7dc&&x==0&&y==2));return 1;};
    b.turn_update=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(77);return 1;};
    b.set_enemy=[](void* p,void*,void* e,bool flag) noexcept -> int{auto& f=F::self(p);assert(e==f.peer&&flag);f.trace.push_back(78);return 1;};
    b.finish_hook=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(79);return 1;};
    b.path_refresh=[](void* p,void*,void* target) noexcept -> int{auto& f=F::self(p);assert(target==f.peer);f.trace.push_back(80);return 1;};
    ServerGruntTaskResult out{};
    auto execute=[&](int id,bool run=false){f.put(f.actor,0x7a4,1);f.trace.clear();ServerNpcScheduleTask task{id,0};assert(api->execute(f.actor,sizeof(f.actor),&task,run,10,&b,&out));};
    execute(10);assert(out.parent_called&&f.trace==std::vector<int>{10});
    execute(143);assert(out.special_called&&!out.parent_called&&f.trace==std::vector<int>{1143});
    f.put(f.actor,0x988,0xffffffffu);execute(43);assert(out.parent_called&&f.get<unsigned>(f.actor,0x988)==0xfffffffdu);
    execute(139);assert(f.trace.empty()&&f.get<int>(f.actor,0x7a4)==1);
    f.put(f.actor,0x7cc,1u<<20);execute(60);assert(out.lifecycle.completed&&!out.parent_called);f.put(f.actor,0x7cc,0u);
    f.put(f.actor,0x9c4,3u);execute(142);assert(out.peer_writes==2&&out.lifecycle.completed&&f.get<float>(f.peer,0xc80)==16.f);
    f.actor[0xc6c]=1;execute(142);assert(f.get<float>(f.peer,0xc80)==12.5f);
    f.put(f.peer,0xcb8,123u);execute(155);assert(f.get<unsigned>(f.peer,0xcb8)==0xffffffffu&&out.peer_writes==2);
    f.actor[0xca0]=1;f.put(f.actor,0x7d4,(1u<<9)|(1u<<7));f.put(f.actor,0xca8,1u);f.put(f.peer,0xca4,8u);
    execute(147);assert(!f.actor[0xca0]&&!f.get<unsigned>(f.actor,0x7d4)&&!f.get<unsigned>(f.actor,0xca8)&&!f.get<unsigned>(f.peer,0xca4)&&f.get<unsigned>(f.peer,0x8a8)==0xffffffffu&&!out.lifecycle.completed);
    execute(147,true);assert(out.lifecycle.completed);
    f.put(f.actor,0xca4,1u);f.put(f.peer,0xca8,2u);f.actor[0xca0]=1;f.peer[0xca0]=1;
    execute(148);assert(out.lifecycle.completed&&out.helper_writes==2&&!f.peer[0xca0]&&(f.get<unsigned>(f.peer,0x7d4)&(1u<<9))&&!f.get<unsigned>(f.peer,0xca8));
    execute(148,true);assert(out.lifecycle.completed&&!out.helper_writes);
    execute(153);assert(out.lifecycle.completed&&f.trace==std::vector<int>{0x7e0});execute(157);assert(out.lifecycle.completed&&f.trace==std::vector<int>{0x7dc});
    f.put(f.actor,0xcbc,0x7001u);execute(156);assert(out.lifecycle.completed&&f.trace==std::vector<int>{78});execute(158);assert(out.lifecycle.completed&&f.trace==std::vector<int>{79});
    f.actor[0x3e0]=1;f.put(f.actor,0x3ec,1);f.put(f.actor,0x884,2);execute(141,true);assert(!out.lifecycle.completed);
    f.put(f.actor,0x884,1);execute(154,true);assert(out.lifecycle.completed&&f.trace==std::vector<int>{77});
    f.put(f.actor,0x8a8,0x7001u);f.put(f.actor,0x7b4,0u);execute(149,true);assert(f.get<int>(f.actor,0x7b4)==1&&f.trace.empty());
    execute(149,true);assert(!f.get<int>(f.actor,0x7b4)&&f.trace==std::vector<int>{80});
    f.put(f.actor,0x8a8,0x8001u);execute(149,true);assert(out.lifecycle.failed&&f.get<int>(f.actor,0x7b0)==6);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original base Grunt task routes and native task lifecycle");
}
