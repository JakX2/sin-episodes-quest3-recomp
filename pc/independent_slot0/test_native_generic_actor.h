#pragma once
#include "native_generic_actor.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct GenericActorFixture {
    alignas(4) unsigned char npc[generic_actor_bytes]{};
    HologramControl control{npc,sizeof(npc)};
    PlayerDamageInfo info{};
    int observation=7;
    unsigned dirty=0,base=0,behavior=0,observed=0;
    bool fail=false;
    void put(unsigned off,unsigned w){std::memcpy(npc+off,&w,4);}
    unsigned word(unsigned off){unsigned w=0;std::memcpy(&w,npc+off,4);return w;}
    float damage(){float f=0;std::memcpy(&f,info.bytes+0x2c,4);return f;}
};
inline void check_generic_actor(const GenericActorApi& api){
    GenericActorFixture f;ActorBindings b{};b.context=&f;
    const auto* schema=api.schema();
    assert(schema&&schema->bytes==0x1014&&schema->count==6&&!std::strcmp(schema->classname,"npc_generic_actor"));
    assert(schema->fields[0].offset==0x1000&&schema->fields[5].type==6&&schema->fields[5].offset==0x100c);
    b.health_dirty=[](void* p,void* npc) noexcept -> int{
        auto& f=*static_cast<GenericActorFixture*>(p);assert(f.control.busy&&npc==f.npc&&f.word(0x614)==247&&f.word(0x9c)==0);
        ++f.dirty;f.put(0x1004,9);return !f.fail;
    };
    b.behavior_damage=[](void* p,std::uint32_t behavior,PlayerDamageInfo* info,int* result) noexcept -> int{
        auto& f=*static_cast<GenericActorFixture*>(p);assert(f.control.busy&&behavior==0x12345678&&info==&f.info&&f.damage()==5);
        ++f.behavior;*result=12;return !f.fail;
    };
    b.base_damage=[](void* p,void* npc,PlayerDamageInfo* info,int* result) noexcept -> int{
        auto& f=*static_cast<GenericActorFixture*>(p);assert(f.control.busy&&npc==f.npc&&info==&f.info&&f.damage()==12);
        ++f.base;const float damage=5;std::memcpy(info->bytes+0x2c,&damage,4);*result=8;return 1;
    };
    b.behavior_observe=[](void* p,std::uint32_t behavior,const void* obs) noexcept -> int{
        auto& f=*static_cast<GenericActorFixture*>(p);assert(f.control.busy&&behavior==0x12345678&&obs==&f.observation);
        ++f.observed;return 1;
    };
    auto key=[&](const char* name,unsigned type,unsigned word){ActorValue v{type,word};return api.key(&f.control,name,&v);};
    assert(key("bloodColor",5,247)&&key("initialHealth",5,8)&&key("heads",5,0xffffffffu)&&key("dontHammerBBox",5,1));
    assert(f.word(0x1000)==247&&f.word(0x1004)==8&&f.word(0x1008)==0xffffffffu&&f.word(0x1010)==1);
    assert(key("initialHealth",2,99)==0&&key("PlayerCollide",6,1)==0);
    f.put(0x8ec,0x10);f.npc[0xbb]=0x80;
    assert(api.spawn_profile(&f.control,&b)==1&&f.dirty==1&&f.word(0x9c)==9&&f.word(0x618)==0x3f000000&&f.word(0x850)==0&&f.npc[0x100c]==1&&f.word(0x8ec)==0x02800811&&f.npc[0xbb]==0xd8);
    assert(api.spawn_profile(&f.control,&b)==1&&f.dirty==1);
    unsigned uses=0;assert(api.uses_speech_semaphore(&f.control,&uses)==1&&uses==1);
    f.npc[0xb6]=3;assert(api.uses_speech_semaphore(&f.control,&uses)==1&&!uses);
    const float d=12;std::memcpy(f.info.bytes+0x2c,&d,4);int result=0;
    assert(api.take_damage(&f.control,&b,&f.info,&result)==1&&result==8&&f.base==1&&f.damage()==5);
    assert(api.observe(&f.control,&b,&f.observation)==1&&!f.observed);
    f.put(0xc18,0x12345678);
    assert(api.take_damage(&f.control,&b,&f.info,&result)==1&&result==12&&f.behavior==1&&f.base==1);
    assert(api.observe(&f.control,&b,&f.observation)==1&&f.observed==1);
    f.fail=true;assert(api.take_damage(&f.control,&b,&f.info,&result)==-1&&f.control.blocked&&f.behavior==2);
    assert(api.observe(&f.control,&b,&f.observation)==0&&api.acknowledge(&f.control));f.fail=false;
    assert(api.observe(&f.control,&b,&f.observation)==1&&f.observed==2);
}
}
