#pragma once
#include "native_jessica_script.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct JessicaFixture {
    alignas(4) unsigned char npc[0x1458]{},weapon[0x4dc]{};
    HologramControl ctrl{npc,sizeof(npc)};
    unsigned events[32]{},count=0,condition=0;
    int player=1,item=2,maxclients=1;
    bool has_player=true,has_item=true,is_weapon=true,fail_toss=false;
    void record(unsigned n){assert(ctrl.busy&&count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> list){assert(count==list.size()&&!std::memcmp(events,list.begin(),count*4));count=0;}
    void put(unsigned off,unsigned word){std::memcpy(npc+off,&word,4);}
    int word(unsigned off)const{int w=0;std::memcpy(&w,npc+off,4);return w;}
    int ammo(unsigned off)const{int w=0;std::memcpy(&w,weapon+off,4);return w;}
};
inline void check_jessica(const JessicaApi& api){
    JessicaFixture f;
    JessicaBindings b{};
    b.context=&f;
    b.condition=[](void* p,void* npc,JessicaCondition op,unsigned id,unsigned* out) noexcept -> int {
        auto& x=*static_cast<JessicaFixture*>(p);assert(npc==x.npc&&id==0x47);
        x.record(1+static_cast<unsigned>(op));if(op==JessicaCondition::Set)x.condition=1;
        if(op==JessicaCondition::Clear)x.condition=0;*out=x.condition;return 1;
    };
    b.base_schedule=[](void* p,void*,int* result) noexcept -> int {static_cast<JessicaFixture*>(p)->record(5);*result=9;return 1;};
    b.base_interrupts=[](void* p,void*) noexcept -> int {static_cast<JessicaFixture*>(p)->record(6);return 1;};
    b.resolve_target=[](void* p,std::uint32_t handle,void** out) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(7);assert(handle==0x123004);*out=x.has_player?&x.player:nullptr;return 1;};
    b.holster=[](void* p,void* target,int arg) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(8);assert(target==&x.player&&arg==0);return 1;};
    b.base_event=[](void* p,void*,const void*) noexcept -> int {static_cast<JessicaFixture*>(p)->record(9);return 1;};
    b.max_clients=[](void* p,int* out) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(10);*out=x.maxclients;return 1;};
    b.single_player=[](void* p,void** out) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(11);*out=x.has_player?&x.player:nullptr;return 1;};
    b.attachment=[](void* p,void*,const char* name,JessicaAttachment* a) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(12);assert(!std::strcmp(name,"anim_attachment_RH"));a->origin[0]=12;return 1;};
    b.spawn=[](void* p,void* npc,std::uint32_t name,const JessicaAttachment* a,void** out) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(13);assert(npc==x.npc&&name==static_cast<unsigned>(x.word(0x144c))&&a->origin[0]==12);*out=x.has_item?&x.item:nullptr;return 1;};
    b.weapon_cast=[](void* p,void* item,void** out,unsigned* size) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(14);assert(item==&x.item);*out=x.is_weapon?x.weapon:nullptr;*size=sizeof(x.weapon);return 1;};
    b.post_spawn=[](void* p,void* item) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(15);assert(item==&x.item);return 1;};
    b.toss=[](void* p,void* item,void* player,const float* origin,float extra) noexcept -> int {auto& x=*static_cast<JessicaFixture*>(p);x.record(16);assert(item==&x.item&&player==&x.player&&origin[0]==12&&extra==15);return !x.fail_toss;};
    assert(api.spawn_state(&f.ctrl)==1&&f.word(0x1450)==-1&&f.word(0x1454)==-1);
    auto input=[&](JessicaInput op,unsigned tag,unsigned value){JessicaValue v{tag,value};return api.input(&f.ctrl,&b,op,&v);};
    assert(input(JessicaInput::ThrowItem,5,99)==1&&f.word(0x144c)==0);f.expect({1});
    assert(input(JessicaInput::ThrowItem,2,0x1234)==1&&f.word(0x144c)==0x1234);f.expect({1});
    assert(input(JessicaInput::PrimaryAmmo,5,30)==1&&input(JessicaInput::SecondaryAmmo,2,99)==1&&f.word(0x1454)==0);
    int schedule=0;assert(api.select_schedule(&f.ctrl,&b,&schedule)==1&&schedule==0x59);f.expect({2});
    assert(api.build_interrupts(&f.ctrl,&b)==1);f.expect({4,6});
    f.put(0x78c,0xffffffffu);assert(api.input(&f.ctrl,&b,JessicaInput::Holster,nullptr)==1);f.expect({});
    f.put(0x78c,0x123004);assert(api.input(&f.ctrl,&b,JessicaInput::Holster,nullptr)==1);f.expect({7,8});
    f.has_player=false;assert(api.input(&f.ctrl,&b,JessicaInput::Holster,nullptr)==1);f.expect({7});f.has_player=true;
    JessicaEventResult result{};int event=100;
    auto animate=[&](int id){return api.animation(&f.ctrl,&b,&event,id,123,&result);};
    assert(animate(0x59)==1&&result==JessicaEventResult::Forwarded&&f.condition);f.expect({9});
    f.has_item=false;assert(animate(123)==1&&result==JessicaEventResult::NoSpawn&&!f.condition&&f.word(0x1450)==30);f.expect({3,10,11,12,13});
    f.has_item=true;f.is_weapon=false;assert(animate(123)==1&&result==JessicaEventResult::Thrown&&f.word(0x1450)==30);f.expect({3,10,11,12,13,14,15,16});
    f.is_weapon=true;assert(animate(123)==1&&f.ammo(0x4d4)==30&&f.ammo(0x4d8)==0&&f.word(0x1450)==-1&&f.word(0x1454)==-1);f.expect({3,10,11,12,13,14,15,16});
    assert(api.select_schedule(&f.ctrl,&b,&schedule)==1&&schedule==9);f.expect({2,5});
    assert(input(JessicaInput::PrimaryAmmo,5,0xfffffffe)==1&&input(JessicaInput::SecondaryAmmo,5,4)==1);
    f.fail_toss=true;assert(animate(123)==-1&&f.ctrl.blocked&&f.ammo(0x4d4)==30&&f.ammo(0x4d8)==4&&f.word(0x1450)==-2&&f.word(0x1454)==-1);f.expect({3,10,11,12,13,14,15,16});
    assert(animate(123)==0&&api.acknowledge(&f.ctrl));f.expect({});f.fail_toss=false;
    f.maxclients=2;assert(animate(123)==-1&&f.ctrl.blocked);f.expect({3,10,12,13,14,15});
    assert(api.acknowledge(&f.ctrl));
}
}
