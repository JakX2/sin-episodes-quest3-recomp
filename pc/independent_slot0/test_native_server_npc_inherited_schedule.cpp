#include "native_server_npc_inherited_schedule.h"
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
struct F{
    alignas(4) unsigned char actor[0xc1c]{},script[0x3b0]{},desc[0x18]{};
    sin_independent::EntityHandleEntry slots[2]{};
    unsigned resets=0,components=0,kind=0,draws=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcInheritedScheduleApi>(GetProcAddress(s,server_npc_inherited_schedule_export));
    assert(g&&!GetProcAddress(c,server_npc_inherited_schedule_export)&&!g(0,sizeof(ServerNpcInheritedScheduleApi)));
    const auto* api=g(1,sizeof(ServerNpcInheritedScheduleApi));assert(api);
    F f;f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.script);
    f.put(f.actor,0x970,0x7001u);
    f.put(f.desc,4,npc_global_condition_base+100);f.put(f.desc,8,10);f.put(f.desc,12,19);f.put(f.desc,16,npc_global_condition_base+109);
    ServerNpcInheritedScheduleBindings b{};b.context=&f;b.entities={f.slots,2,&f,[](void* p,void*,unsigned* out) noexcept -> int{*out=F::self(p).kind;return 1;},nullptr};
    b.extent=[](void* p,void* e,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(e==f.script);*n=sizeof(f.script);return 1;};
    b.reset_script=[](void* p,void*) noexcept -> int{++F::self(p).resets;return 1;};
    b.component_descriptor=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==77);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.component_translate=[](void* p,std::uint32_t token,int id,int* out) noexcept -> int{assert(token==77);++F::self(p).components;*out=id+1;return 1;};
    b.random=[](void* p,int lo,int hi,int* out) noexcept -> int{assert(lo<=hi);++F::self(p).draws;*out=hi;return 1;};
    ServerNpcInheritedScheduleResult out{};auto translate=[&](int input,int expected){assert(api->translate(f.actor,sizeof(f.actor),input,&b,&out)&&out.output==expected);};
    f.put(f.actor,0x850,2);translate(2,9);f.put(f.actor,0x850,3);translate(2,15);f.put(f.actor,0x850,1);translate(2,2);
    const int expected[]{0x3a,0x37,0x38,0x39,0x3a,0x3b};
    for(int i=0;i<6;++i){f.put(f.script,0x3ac,i);translate(0x36,expected[i]);assert(out.controller_live&&out.phase==i);}
    f.put(f.script,0x3ac,6);translate(0x36,0x36);
    f.put(f.actor,0x970,0x8001u);translate(0x36,1);assert(out.reset&&f.resets==1);
    f.put(f.actor,0xc18,77u);translate(npc_global_condition_base+105,16);assert(out.component_local==15&&out.component);
    translate(2,3);translate(-1,0);translate(npc_global_condition_base+110,0);assert(f.components==4);
    const int low[]{1,2,1,1,1,1,1,1,4},high[]{1,5,3,1,1,1,1,3,7};
    for(unsigned i=0;i<9;++i){int lo=0,hi=0,v=0;f.kind=i;assert(api->burst_range(i,&lo,&hi)&&lo==low[i]&&hi==high[i]);assert(api->burst(f.script,&b,&v)&&v==hi);}
    assert(f.draws==9);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS inherited schedule phase mapping and original randomized weapon burst ranges");
}
