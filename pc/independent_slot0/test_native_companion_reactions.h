#pragma once
#include "native_companion_reactions.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <limits>
namespace sin_port::native_weapon_tables::test {
struct CompanionFixture {
    alignas(4) unsigned char jc[jc_bytes]{},jessica[0x1458]{};
    HologramControl jc_control{jc,sizeof(jc)},control{jessica,sizeof(jessica)};
    unsigned events[16]{},count=0;
    int parent=1,base_schedule=1;
    bool parent_live=false,dead=false,ready=false,fail=false;
    float now=10,stamp=0;
    const char* speech=nullptr;
    void record(unsigned e){assert(count<16);events[count++]=e;}
    void expect(std::initializer_list<unsigned> l){assert(l.size()==count&&!std::memcmp(events,l.begin(),count*4));count=0;}
    void put(unsigned off,unsigned v){std::memcpy(jessica+off,&v,4);}
    void time(unsigned off,float v){std::memcpy(jessica+off,&v,4);}
    unsigned jc_word(unsigned off){unsigned v=0;std::memcpy(&v,jc+off,4);return v;}
};
inline void check_companion_reactions(const CompanionReactionApi& api){
    CompanionFixture f;JcProfileBindings jc{};jc.context=&f;
    jc.health_dirty=[](void* p,void* npc) noexcept -> int{
        auto& f=*static_cast<CompanionFixture*>(p);assert(f.jc_control.busy&&npc==f.jc&&f.jc_word(0x614)==247&&f.jc_word(0x9c)==0);f.record(1);return 1;
    };
    jc.npc_initialize=[](void* p,void* npc) noexcept -> int{
        auto& f=*static_cast<CompanionFixture*>(p);assert(f.jc_control.busy&&npc==f.jc&&f.jc_word(0x9c)==8&&f.jc_word(0x618)==0x3f000000&&f.jc_word(0x8ec)==0x02801801&&f.jc[0xbb]==0x58);f.record(2);return 1;
    };
    assert(api.jc_profile(&f.jc_control,&jc)==1);f.expect({1,2});
    assert(api.jc_profile(&f.jc_control,&jc)==1);f.expect({2});
    JessicaReactionBindings b{};b.context=&f;
    b.base_update=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy&&npc==f.jessica);f.record(3);return 1;};
    b.resolve=[](void* p,std::uint32_t parent,void** out) noexcept -> int{
        auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy&&parent==0x1001);f.record(4);*out=f.parent_live?&f.parent:nullptr;return 1;
    };
    b.restore_unparented=[](void* p,void*) noexcept -> int{auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy);f.record(5);return 1;};
    b.post_restore=[](void* p,void*) noexcept -> int{auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy);f.record(6);return 1;};
    b.condition=[](void* p,void*,unsigned cond,unsigned* out) noexcept -> int{auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy&&cond==0x43);f.record(7);*out=f.dead;return 1;};
    b.speak=[](void* p,void* npc,const char* name,unsigned a,unsigned b) noexcept -> int{
        auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy&&npc==f.jessica&&!a&&!b);f.record(8);f.speech=name;return !f.fail;
    };
    b.speech_ready=[](void* p,void*,unsigned* out) noexcept -> int{auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy);f.record(9);*out=f.ready;return 1;};
    b.base_event=[](void* p,void*,int id,std::uint32_t a,std::uint32_t b,unsigned* out) noexcept -> int{
        auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy&&id==99&&a==12&&b==34);f.record(10);*out=0;return 1;
    };
    b.base_schedule=[](void* p,void*,int requested,int* out) noexcept -> int{
        auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy&&requested==7);f.record(11);*out=f.base_schedule;return 1;
    };
    b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy);f.record(12);*out=f.now;return 1;};
    b.secondary_timestamp=[](void* p,void*,float* out) noexcept -> int{auto& f=*static_cast<CompanionFixture*>(p);assert(f.control.busy);f.record(13);*out=f.stamp;return 1;};
    f.put(0xf0,0xffffffffu);
    auto update=[&](){return api.jessica_update(&f.control,&b);};
    assert(update()==1);f.expect({3,5,6,7});
    f.put(0xf0,0x1001);f.parent_live=true;assert(update()==1);f.expect({3,4,7});
    f.parent_live=false;f.dead=true;assert(update()==1&&!std::strcmp(f.speech,"TLK_PLDEAD"));f.expect({3,4,5,6,7,8});
    f.put(0x204,1);assert(update()==1);f.expect({3,7,8});
    unsigned handled=9;
    auto event=[&](int id){return api.jessica_event(&f.control,&b,100,id,12,34,&handled);};
    assert(event(99)==1&&!handled);f.expect({10});
    assert(event(100)==1&&handled);f.expect({9});
    f.ready=true;assert(event(100)==1&&handled&&!std::strcmp(f.speech,"TLK_PLYR_PHYSATK"));f.expect({9,8});
    int result=0;auto schedule=[&](){return api.jessica_schedule(&f.control,&b,7,&result);};
    assert(schedule()==1&&result==1);f.expect({11});
    f.put(0x850,2);f.base_schedule=5;assert(schedule()==1&&result==5);f.expect({11});
    f.base_schedule=1;f.time(0x99c,7.01f);assert(schedule()==1&&result==0x48);f.expect({11,12});
    f.time(0x99c,7);f.stamp=2;assert(schedule()==1&&result==1);f.expect({11,12,13,12});
    f.stamp=2.01f;assert(schedule()==1&&result==0x48);f.expect({11,12,13,12});
    f.put(0x850,3);f.time(0x99c,std::numeric_limits<float>::quiet_NaN());f.stamp=0;
    assert(schedule()==1&&result==1);f.expect({11,12,13,12});
    f.fail=true;assert(event(100)==-1&&f.control.blocked);f.expect({9,8});
    assert(update()==0&&api.acknowledge(&f.control));f.fail=false;
    assert(update()==1);f.expect({3,7,8});
}
}
