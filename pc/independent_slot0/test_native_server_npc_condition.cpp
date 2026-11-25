#include "native_server_npc_condition.h"
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
    alignas(4) unsigned char actor[0x850]{},desc[0x48]{},rows[2][0x14]{};
    unsigned descriptors=0,lookups=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    unsigned get(unsigned off){unsigned v=0;std::memcpy(&v,actor+off,4);return v;}
    void row(void* p,int global,int first,int last,unsigned next){
        put(p,0,global);put(p,4,first);put(p,8,last);put(p,0x10,next);
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(s,server_npc_condition_export));
    assert(g&&!GetProcAddress(c,server_npc_condition_export)&&!g(0,sizeof(ServerNpcConditionApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcConditionApi));assert(api);
    F f;const int base=npc_global_condition_base;
    f.row(f.desc+0x34,base+50,3,5,1);
    f.row(f.rows[0],base,9999,10000,2);
    f.row(f.rows[1],base+90,3,12,0);
    ServerNpcConditionMapBindings b{&f,
        [](void* p,void* actor,const void** out,unsigned* n) noexcept -> int{
            auto& f=*static_cast<F*>(p);assert(actor==f.actor);++f.descriptors;*out=f.desc;*n=sizeof(f.desc);return 1;},
        [](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{
            auto& f=*static_cast<F*>(p);++f.lookups;if(token<1||token>2)return 0;*out=f.rows[token-1];*n=sizeof(f.rows[0]);return 1;}};
    ServerNpcConditionResult out{};
    assert(api->map(f.actor,4,&b,&out)&&out.bit==51&&out.visited==1&&out.mapped);
    assert(api->map(f.actor,7,&b,&out)&&out.bit==94&&out.visited==3);
    assert(api->map(f.actor,9999,&b,&out)&&!out.mapped&&out.global==-1);
    assert(api->map(f.actor,-1,&b,&out)&&!out.mapped&&!out.visited);
    const auto prior=f.descriptors;
    assert(api->map(f.actor,base+255,nullptr,&out)&&out.bit==255&&f.descriptors==prior);
    assert(!api->map(f.actor,base+256,nullptr,&out));
    f.put(f.rows[1],0x10,1u);assert(!api->map(f.actor,13,&b,&out));f.put(f.rows[1],0x10,0u);
    assert(api->store(f.actor,sizeof(f.actor),4,NpcConditionCurrent,true,&b,&out)&&out.changed&&f.get(0x7d0)==(1u<<19));
    assert(api->store(f.actor,sizeof(f.actor),4,NpcConditionCurrent,true,&b,&out)&&!out.changed);
    assert(api->store(f.actor,sizeof(f.actor),4,NpcConditionInterrupt,true,&b,&out)&&f.get(0x7f0)==(1u<<19));
    assert(api->store(f.actor,sizeof(f.actor),4,NpcConditionCurrent,false,&b,&out)&&out.changed&&!f.get(0x7d0));
    assert(api->test(f.actor,sizeof(f.actor),4,NpcConditionInterrupt,&b,&out)&&out.value);
    assert(!api->store(f.actor,sizeof(f.actor),4,NpcConditionSnapshot,true,&b,&out));
    assert(api->store(f.actor,sizeof(f.actor),9999,NpcConditionCurrent,true,&b,&out)&&!out.changed);
    for(unsigned i=0;i<8;++i){f.put(f.actor,0x7cc+4*i,0xffffffffu-i);f.put(f.actor,0x82c+4*i,0xf0f0f0f0u+i);}
    assert(api->snapshot_mask(f.actor,sizeof(f.actor)));
    for(unsigned i=0;i<8;++i){
        assert(f.get(0x80c+4*i)==0xffffffffu-i);
        assert(f.get(0x7cc+4*i)==((0xffffffffu-i)&(0xf0f0f0f0u+i)));
    }
    assert(!api->snapshot_mask(f.actor,0x84b));
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native NPC condition namespace and isolated bit banks");
}
