#pragma once
#include "native_npc_spawn.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct NpcSpawnFixture {
    alignas(4) unsigned char jessica[0x1458]{},count[0x103c]{},expeditor[0x7c]{};
    HologramControl j{jessica,sizeof(jessica)},c{count,sizeof(count)};
    std::uint32_t entries[3]{};
    unsigned log[32]{},n=0;
    bool parent_live=true,base_ok=true,expeditor_live=true,fail_seed=false;
    void push(unsigned e){assert((j.busy||c.busy)&&n<32);log[n++]=e;}
    void expect(std::initializer_list<unsigned> e){assert(n==e.size()&&!std::memcmp(log,e.begin(),4*n));n=0;}
    static unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    static void put(void* p,unsigned off,unsigned w){std::memcpy(static_cast<unsigned char*>(p)+off,&w,4);}
    static unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
};
inline void check_npc_spawn(const NpcSpawnApi& api){
    NpcSpawnFixture f;JessicaSpawnBindings j{};j.context=&f;
    j.base_spawn=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.jessica);f.push(1);return 1;};
    j.resolve_parent=[](void* p,std::uint32_t handle,void** out) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(handle==0x1001);f.push(2);*out=f.parent_live?f.count:nullptr;return 1;};
    j.parented_setup=[](void* p,void*,unsigned a,unsigned b) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(!a&&!b);f.push(3);return 1;};
    j.restore_unparented=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);f.push(4);f.put(npc,0x8ec,0x1357);return 1;};
    j.health_dirty=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.jessica&&f.word(npc,0x9c)!=80&&(f.jessica[0xbb]&0x58)==0x58);f.push(5);return 1;};
    j.npc_initialize=[](void* p,void* npc) noexcept -> int{
        auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.jessica&&f.word(npc,0x9c)==80&&f.word(npc,0x1450)==0xffffffffu&&f.word(npc,0x1454)==0xffffffffu);f.push(6);return 1;
    };
    j.install_interaction=[](void* p,void* npc,unsigned off,unsigned thunk) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.jessica&&off==0x80&&thunk==0x22361e40);f.push(7);return 1;};
    j.expeditor=[](void* p,void*,void** out) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);f.push(8);*out=f.expeditor;return 1;};
    j.seed_concept=[](void* p,void* exp,const char* name,unsigned a,unsigned b) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(exp==f.expeditor&&!std::strcmp(name,"TLK_HELLO")&&!a&&b==1);f.push(9);return !f.fail_seed;};
    f.put(f.jessica,0xf0,0x1001);f.put(f.jessica,0x8ec,0xffffffffu);
    assert(api.jessica(&f.j,&j)==1&&f.word(f.jessica,0x8ec)==0x02801000);f.expect({1,2,3,5,6,7,8,9});
    f.parent_live=false;f.put(f.jessica,0x1450,8);
    assert(api.jessica(&f.j,&j)==1&&f.word(f.jessica,0x8ec)==0x1357);f.expect({1,2,4,6,7,8,9});
    f.put(f.jessica,0xf0,0xffffffffu);f.fail_seed=true;
    assert(api.jessica(&f.j,&j)==-1&&f.j.blocked&&!f.j.busy);f.expect({1,4,6,7,8,9});assert(api.acknowledge(&f.j)==1);
    CountClassInitBindings c{};c.context=&f;c.behaviors.context=&f;
    c.base_postconstruct=[](void* p,void* npc,std::uint32_t name) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.count&&name==7);f.push(10);return 1;};
    c.base_create=[](void* p,void*,unsigned* accepted) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);f.push(11);*accepted=f.base_ok;return 1;};
    c.behaviors.append=[](void* p,void* npc,std::uint32_t behavior) noexcept -> int{
        auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.count&&behavior==f.ptr(f.count+0x1000)&&!f.word(npc,0xc28));
        f.push(12);f.entries[0]=behavior;f.put(npc,0xc1c,f.ptr(f.entries));f.put(npc,0xc28,1);return 1;
    };
    c.behaviors.register_base=[](void* p,void* npc) noexcept -> int{
        auto& f=*static_cast<NpcSpawnFixture*>(p);assert(f.word(npc,0x1004)==f.ptr(npc)&&f.word(npc,0x100c)==f.ptr(f.count+0xc14));f.push(13);
        f.entries[1]=f.ptr(f.count+0xc34);f.entries[2]=f.ptr(f.count+0xd34);f.put(npc,0xc28,3);return 1;
    };
    c.create_expeditor=[](void* p,void* npc,void** exp,unsigned* bytes) noexcept -> int{
        auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.count&&f.word(npc,0xc28)==3);f.push(14);*exp=f.expeditor_live?f.expeditor:nullptr;*bytes=f.expeditor_live?sizeof(f.expeditor):0;return 1;
    };
    unsigned accepted=99;f.base_ok=false;
    assert(api.count_class_init(&f.c,&c,7,&accepted)==1&&!accepted&&!f.word(f.count,0xc28));f.expect({10,11});
    f.base_ok=true;f.expeditor_live=false;
    assert(api.count_class_init(&f.c,&c,7,&accepted)==1&&!accepted&&!f.word(f.count,0xffc));f.expect({10,11,12,13,14});
    // Fresh fixture lifetime for the successful path, no second append on
    // the same constructed object. Test storage is deliberately reused.
    std::memset(f.count,0,sizeof(f.count));f.expeditor_live=true;
    assert(api.count_class_init(&f.c,&c,7,&accepted)==1&&accepted&&f.word(f.count,0xffc)==f.ptr(f.expeditor)&&f.word(f.expeditor,0xc)==f.ptr(f.count+0xdfc));
    f.expect({10,11,12,13,14});
    JcProfileBindings p{};p.context=&f;
    p.health_dirty=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.count&&f.word(npc,0x614)==247&&f.word(npc,0x9c)!=8);f.push(15);return 1;};
    p.npc_initialize=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<NpcSpawnFixture*>(p);assert(npc==f.count&&f.word(npc,0x9c)==8&&f.word(npc,0xc28)==3&&f.word(npc,0x8ec)==0x02801801);f.push(16);return 1;};
    assert(api.count_profile(&f.c,&p)==1);f.expect({15,16});
    assert(api.count_profile(&f.c,&p)==1);f.expect({16});
}
}
