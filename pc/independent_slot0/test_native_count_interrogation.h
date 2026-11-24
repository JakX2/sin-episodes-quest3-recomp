#pragma once
#include "native_count_interrogation.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct CountFixture {
    alignas(4) unsigned char npc[0x103c]{},obs[0x2f8]{};
    HologramControl ctrl{npc,sizeof(npc)};
    unsigned events[32]{},count=0;
    int exp=1;
    bool matching=true,absent=false,busy=false,fail_speak=false;
    float now=2;
    const char* last=nullptr;
    void record(unsigned n){assert(ctrl.busy&&count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> list){assert(count==list.size()&&!std::memcmp(events,list.begin(),count*4));count=0;}
    void put(unsigned off,unsigned word){std::memcpy(npc+off,&word,4);}
    int word(unsigned off)const{int w=0;std::memcpy(&w,npc+off,4);return w;}
    void part(const char* name){std::memset(obs+0x2d8,0,32);std::strcpy(reinterpret_cast<char*>(obs+0x2d8),name);}
    const char* prior()const{return reinterpret_cast<const char*>(npc+0x1018);}
};
inline void check_count(const CountInterrogationApi& api){
    CountFixture f;CountBindings b{};b.context=&f;
    b.resolve=[](void* p,std::uint32_t handle,void** out) noexcept -> int {
        auto& f=*static_cast<CountFixture*>(p);f.record(1);assert(handle==0x123004);*out=f.matching?f.npc:nullptr;return 1;
    };
    b.clock=[](void* p,float* out) noexcept -> int {auto& f=*static_cast<CountFixture*>(p);f.record(2);*out=f.now;return 1;};
    b.expeditor=[](void* p,void* npc,void** out) noexcept -> int {auto& f=*static_cast<CountFixture*>(p);f.record(3);assert(npc==f.npc);*out=f.absent?nullptr:&f.exp;return 1;};
    b.speaking=[](void* p,void* exp,unsigned* out) noexcept -> int {auto& f=*static_cast<CountFixture*>(p);f.record(4);assert(exp==&f.exp);*out=f.busy;return 1;};
    b.criteria=[](void* p,const void* rec,float now,CountCriteria* c) noexcept -> int {
        auto& f=*static_cast<CountFixture*>(p);f.record(5);assert(rec==f.obs+0x18&&now==f.now);c->bytes[0]=7;return 1;
    };
    b.speak=[](void* p,void* exp,const char* name,const CountCriteria* c,unsigned* played) noexcept -> int {
        auto& f=*static_cast<CountFixture*>(p);f.record(6);assert(exp==&f.exp&&c->bytes[0]==7);
        f.last=name;*played=0;return !f.fail_speak;
    };
    b.behavior_damage=[](void* p,std::uint32_t behavior,const PlayerDamageInfo* info,int* out) noexcept -> int {
        auto& f=*static_cast<CountFixture*>(p);f.record(7);assert(behavior==0x12345678);
        float v=9;std::memcpy(&v,info->bytes+0x2c,4);assert(v==0);*out=5;return 1;
    };
    b.base_damage=[](void* p,void* npc,const PlayerDamageInfo* info,int* out) noexcept -> int {
        auto& f=*static_cast<CountFixture*>(p);f.record(8);assert(npc==f.npc);
        float v=9;std::memcpy(&v,info->bytes+0x2c,4);assert(v==0);*out=6;return 1;
    };
    std::memset(f.npc+0x1010,0xaa,0x28);assert(api.initialize(&f.ctrl)==1&&f.npc[0x1010]==0&&f.word(0x1014)==0&&!f.prior()[0]&&f.npc[0x1011]==0xaa&&f.npc[0x1019]==0xaa);
    unsigned enabled=9;assert(api.enabled(&f.ctrl,&enabled)==1&&!enabled);
    assert(api.input(&f.ctrl,1)==1&&api.enabled(&f.ctrl,&enabled)==1&&enabled==1);
    const unsigned handle=0x123004;std::memcpy(f.obs+0x14,&handle,4);f.part("head");
    auto tick=[&](){return api.observe(&f.ctrl,&b,f.obs,sizeof(f.obs));};
    assert(api.observe(&f.ctrl,&b,f.obs,0x2f7)==0);
    f.matching=false;assert(tick()==1&&f.word(0x1014)==0);f.expect({1});f.matching=true;
    f.now=1;assert(tick()==1&&f.word(0x1014)==0);f.expect({1,2});f.now=1.01f;
    f.absent=true;assert(tick()==1);f.expect({1,2,3});f.absent=false;
    f.busy=true;assert(tick()==1);f.expect({1,2,3,3,4});f.busy=false;
    assert(tick()==1&&f.word(0x1014)==1&&!std::strcmp(f.last,"TLK_INTERROGATE_COCKY")&&!std::strcmp(f.prior(),"head"));f.expect({1,2,3,3,4,2,5,3,6});
    assert(tick()==1&&f.word(0x1014)==1);f.expect({1,2,3,3,4});
    f.part("chest");assert(tick()==1&&f.word(0x1014)==2&&!std::strcmp(f.last,"TLK_INTERROGATE_NERVOUS"));f.expect({1,2,3,3,4,2,5,3,6});
    assert(api.input(&f.ctrl,0)==1&&api.enabled(&f.ctrl,&enabled)==1&&!enabled&&f.word(0x1014)==2);
    // Direct receiver does not retest enable; scheduler owns eligibility.
    f.part("hand");assert(tick()==1&&f.word(0x1014)==0&&!f.prior()[0]&&!std::strcmp(f.last,"TLK_INTERROGATE_TALKING"));f.expect({1,2,3,3,4,2,5,3,6});
    f.fail_speak=true;assert(tick()==-1&&f.ctrl.blocked&&f.word(0x1014)==0&&!std::strcmp(f.prior(),"hand"));f.expect({1,2,3,3,4,2,5,3,6});
    assert(tick()==0&&api.acknowledge(&f.ctrl));f.fail_speak=false;
    assert(tick()==1&&f.word(0x1014)==0);f.expect({1,2,3,3,4});
    PlayerDamageInfo info{};const float damage=12;std::memcpy(info.bytes+0x2c,&damage,4);int result=0;
    assert(api.take_damage(&f.ctrl,&b,&info,&result)==1&&result==6);f.expect({8});
    f.put(0xc18,0x12345678);assert(api.take_damage(&f.ctrl,&b,&info,&result)==1&&result==5);f.expect({7});
    float original=0;std::memcpy(&original,info.bytes+0x2c,4);assert(original==12);
}
}
