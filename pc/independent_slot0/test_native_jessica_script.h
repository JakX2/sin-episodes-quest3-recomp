#pragma once
#include "native_jessica_script.h"
#include "native_npc_dialogue.h"
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

struct JessicaCombatFixture {
    JessicaFixture j;
    int scene=4;
    unsigned predicate=0,query=0,resolutions=0,vanish_at=0;
    bool fail_speak=false,mutate_invulnerable=false;
    float observed_damage=0;
};
inline void check_jessica_combat(const JessicaCombatApi& api){
    JessicaCombatFixture f;auto& x=f.j;JessicaCombatBindings b{};b.context=&f;
    b.speak=[](void* p,void* npc,const char* name,int a,int c) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);auto& x=f.j;
        x.record(1);assert(npc==x.npc&&!std::strcmp(name,"TLK_INPOISONCLOUD")&&!a&&!c);
        if(f.mutate_invulnerable)x.put(0xb4,0x10000);
        return !f.fail_speak;
    };
    b.base_damage=[](void* p,void* npc,const PlayerDamageInfo* info,int* result) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);f.j.record(2);assert(npc==f.j.npc);
        std::memcpy(&f.observed_damage,info->bytes+0x2c,4);*result=19;return 1;
    };
    b.scene_predicate=[](void* p,void* scene,std::uint32_t arg,unsigned* out) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);f.j.record(3);assert(scene==&f.scene&&arg==99);*out=f.predicate;return 1;
    };
    b.base_scene=[](void* p,void* npc,void* scene,std::uint32_t arg) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);f.j.record(4);assert(npc==f.j.npc&&scene==&f.scene&&arg==99);return 1;
    };
    b.resolve_weapon=[](void* p,std::uint32_t handle,void** out) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);f.j.record(5);assert(handle==0x123004);
        ++f.resolutions;*out=f.resolutions==f.vanish_at?nullptr:f.j.weapon;return 1;
    };
    b.holster=[](void* p,void* weapon,int arg) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);f.j.record(6);assert(weapon==f.j.weapon&&arg==0);return 1;
    };
    b.weapon_query=[](void* p,void* weapon,unsigned* out) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);f.j.record(7);assert(weapon==f.j.weapon);*out=f.query;return 1;
    };
    b.weapon_restore=[](void* p,void* weapon) noexcept -> int {
        auto& f=*static_cast<JessicaCombatFixture*>(p);f.j.record(8);assert(weapon==f.j.weapon);return 1;
    };
    PlayerDamageInfo info{};const float damage=12;std::memcpy(info.bytes+0x2c,&damage,4);
    auto flags=[&](unsigned value){std::memcpy(info.bytes+0x38,&value,4);};
    int result=0;auto take=[&](){return api.take_damage(&x.ctrl,&b,&info,&result);};
    assert(take()==1&&result==19&&f.observed_damage==12);x.expect({2});
    flags(0x20000);assert(take()==1&&f.observed_damage==12);x.expect({2});
    flags(0x10000);assert(take()==1&&f.observed_damage==12);x.expect({1,2});
    x.put(0xb4,0x10000);assert(take()==1&&f.observed_damage==0);x.expect({1,2});
    flags(0);assert(take()==1&&f.observed_damage==0);x.expect({2});
    float source=0;std::memcpy(&source,info.bytes+0x2c,4);assert(source==12);
    x.put(0xb4,0);flags(0x10000);f.mutate_invulnerable=true;
    assert(take()==1&&f.observed_damage==0);x.expect({1,2});f.mutate_invulnerable=false;
    f.fail_speak=true;assert(take()==-1&&x.ctrl.blocked);x.expect({1});assert(api.acknowledge(&x.ctrl));f.fail_speak=false;
    auto enter=[&](){return api.scene_enter(&x.ctrl,&b,&f.scene,99);};
    x.put(0x78c,0xffffffffu);assert(enter()==1);x.expect({3,4});
    x.put(0x78c,0x123004);assert(enter()==1);x.expect({3,5,6,4});
    f.predicate=1;assert(enter()==1);x.expect({3,4});
    auto exit=[&](){return api.scene_exit(&x.ctrl,&b);};
    x.put(0x8c,1);assert(exit()==1);x.expect({});x.put(0x8c,0);
    x.put(0x78c,0xffffffffu);assert(exit()==1);x.expect({});x.put(0x78c,0x123004);
    f.query=1;assert(exit()==1);x.expect({5,5,7});
    f.query=0;assert(exit()==1);x.expect({5,5,7,5,8});
    f.vanish_at=f.resolutions+2;assert(exit()==-1&&x.ctrl.blocked);x.expect({5,5});
    assert(api.acknowledge(&x.ctrl));
}


struct JessicaAttentionFixture {
    JessicaFixture j;
    alignas(4) unsigned char obs[0x5ec]{};
    JessicaAttentionState state{};
    float now=0;
    unsigned clocks=0,look=0,use=0;
    bool fail=false;
    void stamp(unsigned offset,float value){std::memcpy(obs+offset,&value,4);}
};
inline void check_jessica_attention(const JessicaAttentionApi& api){
    JessicaAttentionFixture f;auto& x=f.j;
    JessicaAttentionBindings b{};
    b.context=&f;
    b.clock=[](void* p,float* out) noexcept -> int {
        auto& f=*static_cast<JessicaAttentionFixture*>(p);assert(f.j.ctrl.busy);++f.clocks;*out=f.now;return 1;
    };
    b.speak_criteria=[](void* p,void* npc,const void* record,float now,const char* concept_name) noexcept -> int {
        auto& f=*static_cast<JessicaAttentionFixture*>(p);assert(f.j.ctrl.busy&&npc==f.j.npc&&now==f.now);
        if(!std::strcmp(concept_name,"TLK_PLLOOKING")){assert(record==f.obs+0x18);++f.look;}
        else {assert(!std::strcmp(concept_name,"TLK_PLUSING")&&record==f.obs+0x308);++f.use;}
        return !f.fail;
    };
    auto tick=[&](){return api.observe(&x.ctrl,&f.state,&b,f.obs,sizeof(f.obs));};
    assert(api.observe(&x.ctrl,&f.state,&b,f.obs,0x5e8)==0);
    assert(tick()==1&&!f.clocks&&!f.look&&!f.use);
    f.obs[0x300]=1;f.obs[0x18+0x108]=1;f.obs[0x5e8]=1;f.obs[0x308+0x260]=1;
    f.now=1;assert(tick()==1&&f.look==1&&f.use==1&&f.clocks==4&&f.state.looking==1&&f.state.using_part==1);
    f.now=4;assert(tick()==1&&f.look==1&&f.use==1&&f.clocks==6); // strict >3
    f.now=4.25f;assert(tick()==1&&f.look==2&&f.use==2&&f.state.looking==4.25f&&f.state.using_part==4.25f);
    f.stamp(0x18+0x2bc,4);f.stamp(0x308+0x2bc,4);f.now=5;
    assert(tick()==1&&f.state.looking==1&&f.state.using_part==4.25f&&f.look==2&&f.use==2);
    f.obs[0x18+0x2c0]=1;f.stamp(0x18+0x2e4,0);f.now=5.5f;
    assert(tick()==1&&f.state.looking==5.5f&&f.look==3&&f.use==2);
    f.obs[0x300]=0;f.obs[0x5e8]=0;assert(tick()==1&&f.state.looking==0&&f.state.using_part==0);
    f.obs[0x300]=1;f.obs[0x18+0x108]=0;f.obs[0x18+0x260]=0;
    assert(tick()==1&&f.look==3);f.obs[0x18+0x260]=1;
    f.now=6;f.fail=true;assert(tick()==-1&&x.ctrl.blocked&&f.state.looking==6&&f.look==4);
    assert(tick()==0&&api.acknowledge(&x.ctrl));f.fail=false;
    assert(tick()==1&&f.look==4); // committed timer, no automatic retry
}


struct JessicaInteractionFixture {
    JessicaFixture j;
    const JessicaInteractionApi& api;
    JessicaInteractionBindings b{};
    unsigned result=99, present=1, eligible=1, timing=1, in_range=1, suppressed=0, used=0, hello=0, fallback=1;
    unsigned fail_at=0;
    bool player=true, missing_exp=false;
    char exp=0;
    explicit JessicaInteractionFixture(const JessicaInteractionApi& a):api(a){
        b.context=this;
        b.speech_manager=[](void* p) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(1);assert(f.j.ctrl.busy);return f.fail_at!=1;};
        b.resolve=[](void* p,std::uint32_t h,unsigned* v) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(2);assert(h==0x123004);*v=f.present;return f.fail_at!=2;};
        b.expeditor=[](void* p,void* n,void** out) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(3);assert(n==f.j.npc);*out=f.missing_exp?nullptr:&f.exp;return f.fail_at!=3;};
        b.eligible=[](void* p,void* e,void* n,unsigned* v) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(4);assert(e==&f.exp&&n==f.j.npc);*v=f.eligible;return f.fail_at!=4;};
        b.timing=[](void* p,void* e,unsigned* v) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(5);assert(e==&f.exp);*v=f.timing;return f.fail_at!=5;};
        b.player=[](void* p,void** out) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(6);*out=f.player?f.j.weapon:nullptr;return f.fail_at!=6;};
        b.within_range=[](void* p,void* n,void* pl,unsigned* v) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(7);assert(n==f.j.npc&&pl==f.j.weapon);*v=f.in_range;return f.fail_at!=7;};
        b.suppressed=[](void* p,void* n,int arg,unsigned* v) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(8);assert(n==f.j.npc&&!arg);*v=f.suppressed;return f.fail_at!=8;};
        b.speak=[](void* p,void* n,const char* name,int arg,unsigned* v) noexcept -> int {
            auto& f=*static_cast<JessicaInteractionFixture*>(p);assert(n==f.j.npc&&!arg);
            const unsigned event=!std::strcmp(name,"TLK_USE")?9:!std::strcmp(name,"TLK_HELLO")?12:13;
            assert(event!=13||!std::strcmp(name,"TLK_IDLE"));f.j.record(event);
            unsigned nested=99;assert(f.api.invoke(&f.j.ctrl,&f.b,&nested)==0&&nested==99);
            *v=event==9?f.used:f.fallback;return f.fail_at!=event;
        };
        b.has_concept=[](void* p,void* e,const char* name,unsigned* v) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(10);assert(e==&f.exp&&!std::strcmp(name,"TLK_HELLO"));*v=f.hello;return f.fail_at!=10;};
        b.reset_manager=[](void* p,float v,int arg) noexcept -> int {auto& f=*static_cast<JessicaInteractionFixture*>(p);f.j.record(11);assert(v==-1.f&&!arg);return f.fail_at!=11;};
        j.put(0x16c,0x123004);
    }
    int run(){result=99;return api.invoke(&j.ctrl,&b,&result);}
};
inline void check_jessica_interaction(const JessicaInteractionApi& api){
    JessicaInteractionFixture f(api);auto& x=f.j;
    for(const auto off:{0x8cu,0xb4u,0x850u,0xb8u,0x968u}){
        x.put(off,off==0xb4?2:off==0x850?4:off==0xb8?0x100000:1);
        assert(f.run()==1&&!f.result);x.expect({1});x.put(off,0);
    }
    f.present=0;assert(f.run()==1&&!f.result);x.expect({1,2});f.present=1;
    f.eligible=0;assert(f.run()==1&&!f.result);x.expect({1,2,3,4});f.eligible=1;
    f.timing=0;assert(f.run()==1&&!f.result);x.expect({1,2,3,4,3,5});f.timing=1;
    f.in_range=0;assert(f.run()==1&&!f.result);x.expect({1,2,3,4,3,5,6,7});f.in_range=1;
    f.suppressed=1;assert(f.run()==1&&!f.result);x.expect({1,2,3,4,3,5,6,7,8});f.suppressed=0;
    f.used=1;assert(f.run()==1&&!f.result);x.expect({1,2,3,4,3,5,6,7,8,9,11});f.used=0;
    assert(f.run()==1&&f.result==1);x.expect({1,2,3,4,3,5,6,7,8,9,3,10,12});
    f.hello=1;f.fallback=0;assert(f.run()==1&&!f.result);x.expect({1,2,3,4,3,5,6,7,8,9,3,10,13});
    f.player=false;assert(f.run()==1&&!f.result);x.expect({1,2,3,4,3,5,6,8,9,3,10,13});
    f.fail_at=13;assert(f.run()==-1&&f.result==99&&x.ctrl.blocked);x.expect({1,2,3,4,3,5,6,8,9,3,10,13});
    assert(f.run()==0&&f.result==99&&api.acknowledge(&x.ctrl));f.fail_at=0;
    f.missing_exp=true;assert(f.run()==-1&&f.result==99);x.expect({1,2,3});assert(api.acknowledge(&x.ctrl));
    f.missing_exp=false;f.used=1;f.fail_at=11;
    assert(f.run()==-1&&f.result==99);x.expect({1,2,3,4,3,5,6,8,9,11});assert(api.acknowledge(&x.ctrl));
}


struct ResponseSelectionFixture {
    alignas(4) unsigned char exp[0x7c]{},response[0x124]{};
    int system=0,criteria=0,draw=50,chance=50;
    unsigned events[32]{},count=0,fail=0,destroys=0,ends=0;
    bool no_system=false,no_match=false,retained_miss=true,empty=false,flag=true,mutate=false;
    ResponseSelectionBindings b{};
    void record(unsigned n){assert(count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> list){assert(count==list.size()&&!std::memcmp(events,list.begin(),count*4));count=0;}
    void put(unsigned off,unsigned w){std::memcpy(response+off,&w,4);}
    unsigned word(unsigned off){unsigned w=0;std::memcpy(&w,response+off,4);return w;}
    ResponseSelectionFixture(){
        b.context=this;
        b.system=[](void* p,void* e,void** out) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(1);assert(e==f.exp);*out=f.no_system?nullptr:&f.system;return f.fail!=1;};
        b.begin_criteria=[](void* p,void** out) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(2);*out=&f.criteria;return f.fail!=2;};
        b.add=[](void* p,void* c,const char* k,const char* v,float w) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(3);assert(c==&f.criteria&&!std::strcmp(k,"concept")&&!std::strcmp(v,"TLK_IDLE")&&w==5.f);return f.fail!=3;};
        b.modifiers=[](void* p,void* c,const char* m,float w) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(4);assert(c==&f.criteria&&!std::strcmp(m,"fixture:yes")&&w==5.f);return f.fail!=4;};
        b.npc_criteria=[](void* p,void* e,void* c) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(5);assert(e==f.exp&&c==&f.criteria);return f.fail!=5;};
        b.player_criteria=[](void* p,void* c,int i) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(6);assert(c==&f.criteria&&i==1);return f.fail!=6;};
        b.allocate=[](void* p,unsigned n,void** out,unsigned* bytes) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(7);assert(n==0x124);std::memset(f.response,0xa5,sizeof(f.response));*out=f.response;*bytes=sizeof(f.response);return f.fail!=7;};
        b.find=[](void* p,void* s,void* c,void* r,void* e,unsigned* out) noexcept -> int{
            auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(8);assert(s==&f.system&&c==&f.criteria&&r==f.response&&e==f.exp);
            assert(!f.word(0)&&!f.response[4]&&!f.word(0x88)&&f.word(0x8c)==100&&!f.word(0x98)&&!f.response[0xa4]&&f.response[0xa5]==0xa5);
            f.response[4]=f.empty?0:'x';f.response[5]=0;f.put(0x88,f.flag?4:0);f.put(0x8c,f.chance);*out=!f.no_match;if(f.fail==8)return 0;
            return f.no_match&&f.retained_miss?response_find_retained_miss:1;
        };
        b.draw=[](void* p,int l,int h,int* out) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(9);assert(l==1&&h==100);*out=f.draw;if(f.mutate)f.put(0x8c,f.draw);return f.fail!=9;};
        b.destroy=[](void* p,void* r) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(10);assert(r==f.response);++f.destroys;return f.fail!=10;};
        b.end_criteria=[](void* p,void* c) noexcept -> int{auto& f=*static_cast<ResponseSelectionFixture*>(p);f.record(11);assert(c==&f.criteria);++f.ends;return f.fail!=11;};
    }
};
inline void check_response_select(const ResponseSelectApi& api){
    ResponseSelectionFixture f;void* out=&f;unsigned n=99;
    auto run=[&](const char* mods=nullptr){out=&f;n=99;return api.select(f.exp,sizeof(f.exp),&f.b,"TLK_IDLE",mods,&out,&n);};
    f.no_system=true;assert(run()==1&&!out&&!n);f.expect({1});f.no_system=false;
    f.no_match=true;f.retained_miss=false;
    assert(run()==1&&!out&&!n&&!f.destroys);f.expect({1,2,3,5,6,7,8,11});
    f.retained_miss=true;
    for(unsigned i=0;i<8;++i){assert(run()==1&&!out&&!n&&f.destroys==i+1);f.expect({1,2,3,5,6,7,8,10,11});}
    f.fail=10;assert(run()==-1&&out==&f&&n==99&&f.destroys==9);f.expect({1,2,3,5,6,7,8,10,11});
    f.fail=0;f.no_match=false;
    f.empty=true;assert(run()==1&&!out&&!n&&f.destroys==10);f.expect({1,2,3,5,6,7,8,10,11});f.empty=false;
    assert(run("fixture:yes")==1&&!out&&!n);f.expect({1,2,3,4,5,6,7,8,9,10,11});
    f.draw=51;assert(run()==1&&out==f.response&&n==0x124);f.expect({1,2,3,5,6,7,8,9,11});
    f.mutate=true;assert(run()==1&&!out);f.expect({1,2,3,5,6,7,8,9,10,11});f.mutate=false;
    f.chance=100;assert(run()==1&&out==f.response);f.expect({1,2,3,5,6,7,8,11});
    f.flag=false;f.chance=-1;assert(run()==1&&out==f.response);f.expect({1,2,3,5,6,7,8,11});
    f.fail=3;assert(run()==-1&&out==&f&&n==99);f.expect({1,2,3,11});
    f.fail=8;assert(run()==-1&&out==&f&&n==99);f.expect({1,2,3,5,6,7,8,10,11});
    f.fail=11;assert(run()==-1&&out==&f&&n==99);f.expect({1,2,3,5,6,7,8,11,10});
}



inline void check_response_native_modifiers(const ResponseNativeModifierApi& api,const ResponseSelectApi& select){
    struct Fixture : ResponseSelectionFixture {
        char keys[8][128]{},values[8][128]{};
        unsigned pairs=0,warnings=0;
        bool reject=false;
    } f;
    ResponseNativeModifierBindings cfg{};cfg.host=f.b;cfg.host.context=&f;cfg.host.modifiers=nullptr;
    cfg.host.add=[](void* p,void* c,const char* key,const char* value,float weight) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(c==&f.criteria&&weight==5.f&&f.pairs<8);
        f.record(3);std::strcpy(f.keys[f.pairs],key);std::strcpy(f.values[f.pairs++],value);return !f.reject;};
    cfg.warning=[](void* p,const char*) noexcept -> int{++static_cast<Fixture*>(p)->warnings;return 1;};
    auto parse=[&](const char* input){f.pairs=f.count=0;return api.parse(&cfg,&f.criteria,input,5);};
    assert(parse("part:head, state :a:b,empty:,:value")==1&&f.pairs==4);
    assert(!std::strcmp(f.keys[0],"part")&&!std::strcmp(f.values[0],"head"));
    assert(!std::strcmp(f.keys[1]," state ")&&!std::strcmp(f.values[1],"a:b"));
    assert(!std::strcmp(f.keys[2],"empty")&&!*f.values[2]&&!*f.keys[3]&&!std::strcmp(f.values[3],"value"));
    assert(parse("x:y,broken")==1&&f.pairs==2&&f.warnings==1&&!std::strcmp(f.keys[1],"x")&&!std::strcmp(f.values[1],"y"));
    assert(parse("no colon")==0&&!f.pairs&&f.warnings==2);
    assert(parse("a,b:c")==1&&f.pairs==1&&!std::strcmp(f.keys[0],"a,b"));
    char long_key[131];std::memset(long_key,'k',128);std::strcpy(long_key+128,":v");
    assert(!parse(long_key)&&!f.pairs);
    char bounded[300];std::memset(bounded,'z',sizeof(bounded));bounded[0]='a';bounded[1]=':';bounded[2]='b';bounded[3]=',';
    std::memset(bounded+4,'k',123);bounded[127]=':';bounded[299]=0;
    assert(parse(bounded)==1&&f.pairs==2&&std::strlen(f.keys[1])==123&&std::strlen(f.values[1])==126);
    f.reject=true;assert(!parse("a:b,c:d")&&f.pairs==1);f.reject=false;
    ResponseSelectionBindings adapted{};assert(api.adapt(&cfg,&adapted));f.chance=100;
    void* out=nullptr;unsigned n=0;f.pairs=f.count=0;
    assert(select.select(f.exp,sizeof(f.exp),&adapted,"TLK_IDLE","part:head,state:alert",&out,&n)==1&&out==f.response&&n==0x124);
    assert(f.pairs==3&&!std::strcmp(f.keys[0],"concept")&&!std::strcmp(f.values[0],"TLK_IDLE")&&!std::strcmp(f.keys[2],"state"));
    f.expect({1,2,3,3,3,5,6,7,8,11});
    f.pairs=0;out=&f;n=99;
    assert(select.select(f.exp,sizeof(f.exp),&adapted,"TLK_IDLE","broken",&out,&n)==-1&&out==&f&&n==99);
    f.expect({1,2,3,11});
}




inline void check_scene_metadata_index(const SceneMetadataIndexApi& api){
    struct Fixture {
        alignas(4) unsigned char row[16]{},meta[0x1c]{};
        unsigned allocations=0,releases=0,appends=0,undos=0,inserts=0,fail=0;
        bool allocated=false,appended=false,published=false;
        static Fixture& get(void* p){return *static_cast<Fixture*>(p);}
        static unsigned word(const void* p,unsigned offset){unsigned w;std::memcpy(&w,static_cast<const unsigned char*>(p)+offset,4);return w;}
    } f;
    SceneMetadataIndexBindings b{};b.context=&f;b.metadata_vtable=0x12345678;
    b.filename=[](void*,const char* name,std::uint32_t* out) noexcept -> int{
        assert(!std::strcmp(name,"scenes/test.vcd"));*out=0x321;return 1;};
    b.find=[](void* p,std::uint32_t handle,std::uint16_t* out) noexcept -> int{
        auto& f=Fixture::get(p);assert(handle==0x321);*out=f.published?7:0xffff;return f.fail!=1;};
    b.row=[](void* p,std::uint16_t index,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::get(p);assert(index==7&&f.published&&f.appended);*out=f.row;*n=sizeof(f.row);return 1;};
    b.allocate=[](void* p,unsigned n,void** out,unsigned* bytes) noexcept -> int{
        auto& f=Fixture::get(p);assert(n==sizeof(f.meta)&&!f.allocated&&!f.appended);
        std::memset(f.meta,0xa5,sizeof(f.meta));f.allocated=true;++f.allocations;*out=f.meta;*bytes=f.fail==2?n-1:n;return 1;};
    b.release=[](void* p,void* meta) noexcept -> int{
        auto& f=Fixture::get(p);assert(meta==f.meta&&f.allocated&&!f.appended&&!f.published);
        f.allocated=false;++f.releases;return 1;};
    b.append=[](void* p,void* meta,std::uint32_t* out) noexcept -> int{
        auto& f=Fixture::get(p);assert(meta==f.meta&&f.allocated&&!f.appended);
        assert(Fixture::word(meta,0)==0x12345678);
        for(unsigned i=4;i<0x1c;i+=4)assert(!Fixture::word(meta,i));
        ++f.appends;if(f.fail==3)return 0;f.appended=true;*out=4;return 1;};
    b.undo_append=[](void* p,std::uint32_t slot,void* meta) noexcept -> int{
        auto& f=Fixture::get(p);assert(slot==4&&meta==f.meta&&f.appended&&!f.published);
        ++f.undos;f.appended=false;return 1;};
    b.insert=[](void* p,const void* row,unsigned n,std::uint16_t* index) noexcept -> int{
        auto& f=Fixture::get(p);assert(n==16&&f.appended&&!f.published);
        assert(Fixture::word(row,0)==0x321&&!Fixture::word(row,4)&&Fixture::word(row,8)==0xfffffffeu&&Fixture::word(row,12)==4);
        ++f.inserts;if(f.fail==4)return 0;std::memcpy(f.row,row,n);f.published=true;*index=7;return 1;};
    void* row=&f;unsigned bytes=99;
    auto run=[&](){return api.ensure(&b,"scenes/test.vcd",&row,&bytes);};
    for(unsigned fail=1;fail<=4;++fail){
        f.fail=fail;assert(!run()&&row==&f&&bytes==99&&!f.allocated&&!f.appended&&!f.published);
    }
    assert(f.allocations==3&&f.releases==3&&f.appends==2&&f.undos==1&&f.inserts==1);
    f.fail=0;assert(run()==1&&row==f.row&&bytes==16&&f.allocated&&f.appended&&f.published);
    assert(run()==1&&row==f.row&&f.allocations==4&&f.appends==3&&f.inserts==2);
    // Teardown of cache-owned metadata is host responsibility, not ensure.
    assert(f.releases==3&&f.undos==1);
}

inline void check_scene_response_services(const SceneResponseServicesApi& api,const ResponseSceneFilterApi& filter){
    struct Fixture {
        alignas(4) unsigned char exp[0x7c]{},cache[0x50]{},row[16]{},data[8]{};
        int npc=0;unsigned queries=0,refreshes=0,models=0,expands=0,mode=99,fail=0;
        std::uint32_t disk=20;
        bool no_model=false,missing=false,change_during_refresh=false;
        static Fixture& self(void* p){return *static_cast<Fixture*>(p);}
        static void put(void* p,unsigned offset,std::uint32_t value){std::memcpy(static_cast<unsigned char*>(p)+offset,&value,4);}
        static std::uint32_t word(const void* p,unsigned offset){std::uint32_t v;std::memcpy(&v,static_cast<const unsigned char*>(p)+offset,4);return v;}
        void duration(float v){std::memcpy(data+4,&v,4);}
    } f;
    Fixture::put(f.exp,4,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&f.npc)));
    Fixture::put(f.row,4,10);Fixture::put(f.row,8,0xfffffffeu);Fixture::put(f.row,0xc,3);
    Fixture::put(f.cache,0x48,1);
    SceneResponseServicesBindings b{};b.context=&f;b.cache=f.cache;b.cache_bytes=sizeof(f.cache);
    b.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(w==Fixture::word(f.exp,4));*out=&f.npc;*n=sizeof(f.npc);return 1;};
    b.model_name=[](void* p,void* npc,const char** out) noexcept -> int{
        auto& f=Fixture::self(p);assert(npc==&f.npc);++f.models;*out=f.no_model?nullptr:"models/fixture.mdl";return f.fail!=1;};
    b.expand=[](void* p,const char* model,const char* name,char* out,unsigned n) noexcept -> int{
        auto& f=Fixture::self(p);assert(!std::strcmp(model,f.no_model?"":"models/fixture.mdl"));
        assert(!std::strcmp(name,"scenes/npc/$gender01/hi01.vcd")&&(n==128||n==256));++f.expands;
        std::strcpy(out,"scenes/npc/female01/hi01.vcd");return f.fail!=2;};
    b.row=[](void* p,const char* path,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(!std::strcmp(path,"scenes/npc/female01/hi01.vcd"));*out=f.row;*n=sizeof(f.row);return 1;};
    b.stamp=[](void* p,const char*,unsigned mode,std::uint32_t* stamp) noexcept -> int{
        auto& f=Fixture::self(p);++f.queries;f.mode=mode;*stamp=f.disk;return f.fail!=3;};
    b.metadata=[](void* p,std::uint32_t i,void** out,unsigned* n) noexcept -> int{
        auto& f=Fixture::self(p);assert(i==3);*out=f.missing?nullptr:f.data;*n=sizeof(f.data);return 1;};
    b.refresh=[](void* p,void* data,const char*) noexcept -> int{
        auto& f=Fixture::self(p);assert(data==f.data&&f.cache[0x3c]);++f.refreshes;
        if(f.fail==4)return 0;
        f.duration(2.5f);if(f.change_during_refresh)Fixture::put(f.row,8,77);return 1;};
    ResponseSceneFilterBindings adapted{};assert(api.adapt(&b,&adapted)==1);
    unsigned allowed=99;
    auto run=[&](){allowed=99;return filter.allow(f.exp,sizeof(f.exp),&adapted,3,"scenes/npc/$gender01/hi01.vcd",&allowed);};
    assert(run()==1&&allowed==1&&f.queries==1&&f.mode==1&&f.refreshes==1&&f.cache[0x3c]);
    assert(Fixture::word(f.row,4)==20&&Fixture::word(f.row,8)==20);
    assert(run()==1&&allowed==1&&f.queries==1&&f.refreshes==1);
    // Freeze: sentinel is replaced with saved stamp without a filesystem call.
    f.cache[0x4c]=1;Fixture::put(f.row,8,0xfffffffeu);f.disk=30;
    assert(run()==1&&allowed==1&&f.queries==1&&Fixture::word(f.row,8)==20);
    // Suppress refresh: stamp still commits, dirty byte remains unchanged.
    f.cache[0x4c]=0;f.cache[0x4d]=1;f.cache[0x3c]=0;Fixture::put(f.cache,0x48,2);
    Fixture::put(f.row,8,0xfffffffeu);f.duration(0);
    assert(run()==1&&!allowed&&f.queries==2&&f.mode==0&&f.refreshes==1&&!f.cache[0x3c]&&Fixture::word(f.row,4)==30);
    f.cache[0x4d]=0;f.change_during_refresh=true;f.disk=40;Fixture::put(f.row,8,0xfffffffeu);
    assert(run()==1&&allowed&&f.refreshes==2&&Fixture::word(f.row,4)==77);
    f.no_model=true;char path[128]{};
    assert(api.resolve_name(&b,&f.npc,"scenes/npc/$gender01/hi01.vcd",path,sizeof(path))==1&&!std::strcmp(path,"scenes/npc/female01/hi01.vcd"));
    f.missing=true;assert(run()==1&&!allowed);f.missing=false;
    f.fail=3;Fixture::put(f.row,8,0xfffffffeu);
    assert(run()==-1&&allowed==99&&Fixture::word(f.row,8)==0xfffffffeu);
    f.fail=4;f.disk=90;f.cache[0x3c]=0;
    assert(run()==-1&&allowed==99&&f.cache[0x3c]&&Fixture::word(f.row,4)==77&&Fixture::word(f.row,8)==90);
    f.fail=0;f.change_during_refresh=false;assert(run()==1&&allowed&&Fixture::word(f.row,4)==90);
    auto short_b=b;short_b.cache_bytes=0x4d;assert(!api.adapt(&short_b,&adapted));
}

inline void check_response_scene_filter(const ResponseSceneFilterApi& api){
    struct State {
        alignas(4) unsigned char exp[0x7c]{},row[8]{};
        int npc=0;unsigned storage=0,resolved=0,lookup=0,fail=0;
        bool missing=false,unterminated=false;
        static State& self(void* p){return *static_cast<State*>(p);}
        void duration(float n){std::memcpy(row+4,&n,4);}
    } f;
    const std::uint32_t owner=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&f.npc));
    std::memcpy(f.exp+4,&owner,4);
    ResponseSceneFilterBindings b{};b.context=&f;
    b.storage=[](void* p,std::uint32_t w,void** out,unsigned* bytes) noexcept -> int{
        auto& f=State::self(p);assert(w==static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&f.npc)));
        ++f.storage;*out=&f.npc;*bytes=sizeof(f.npc);return f.fail!=1;};
    b.resolve_name=[](void* p,void* npc,const char* name,char* dest,unsigned n) noexcept -> int{
        auto& f=State::self(p);assert(npc==&f.npc&&n==256&&!std::strcmp(name,"scenes/$gender/greeting.vcd"));
        ++f.resolved;if(f.unterminated)std::memset(dest,'x',n);else std::strcpy(dest,"scenes/female/greeting.vcd");return f.fail!=2;};
    b.metadata=[](void* p,const char* path,void** out,unsigned* bytes) noexcept -> int{
        auto& f=State::self(p);assert(!std::strcmp(path,"scenes/female/greeting.vcd"));++f.lookup;
        *out=f.missing?nullptr:f.row;*bytes=f.fail==4?4:sizeof(f.row);return f.fail!=3;};
    unsigned allowed=99;
    for(unsigned kind:{0u,1u,2u,4u,5u,255u}){
        assert(api.allow(f.exp,sizeof(f.exp),nullptr,kind,nullptr,&allowed)==1&&allowed==1);
    }
    assert(!f.storage&&!f.resolved&&!f.lookup);
    auto run=[&](){allowed=99;return api.allow(f.exp,sizeof(f.exp),&b,3,"scenes/$gender/greeting.vcd",&allowed);};
    f.duration(2.5f);assert(run()==1&&allowed==1&&f.storage==1&&f.resolved==1&&f.lookup==1);
    f.duration(0);assert(run()==1&&!allowed);f.duration(-1);assert(run()==1&&!allowed);
    const std::uint32_t nan=0x7fc00000u;std::memcpy(f.row+4,&nan,4);
    assert(run()==1&&!allowed);
    f.duration(2);f.missing=true;assert(run()==1&&!allowed);f.missing=false;
    for(unsigned fail=1;fail<=4;++fail){f.fail=fail;assert(run()==-1&&allowed==99);}
    f.fail=0;f.unterminated=true;const auto before=f.lookup;assert(run()==-1&&allowed==99&&f.lookup==before);
}

struct ResponseDispatchFixture {
    alignas(4) unsigned char exp[0x7c]{},response[0x124]{};
    ResponseDispatchBindings b{};
    unsigned events[32]{},count=0,fail=0,played=99,busy=0,blocked=0,enabled=1;
    int sentence_index=0,level=80;
    float duration=2,delay=0,now=10;
    void record(unsigned v){assert(count<32);events[count++]=v;}
    void expect(std::initializer_list<unsigned> l){assert(count==l.size()&&!std::memcmp(events,l.begin(),count*4));count=0;}
    void put(unsigned off,unsigned v){std::memcpy(response+off,&v,4);}
    float last(){float v=0;std::memcpy(&v,exp+0x78,4);return v;}
    static ResponseDispatchFixture& get(void* p,void* e){auto& f=*static_cast<ResponseDispatchFixture*>(p);assert(e==f.exp);return f;}
    static void text_ok(const char* t){assert(!std::strcmp(t,"test_response"));}
    ResponseDispatchFixture(){
        std::strcpy(reinterpret_cast<char*>(response+4),"test_response");b.context=this;
        b.sample=[](void* p,const void* interval,float* out) noexcept -> int{auto& f=*static_cast<ResponseDispatchFixture*>(p);f.record(1);assert(interval==f.response+0x90);*out=f.delay;return f.fail!=1;};
        b.speaking=[](void* p,void* e,unsigned* out) noexcept -> int{auto& f=get(p,e);f.record(2);*out=f.busy;return f.fail!=2;};
        b.force_log=[](void* p,void* e,const char* n) noexcept -> int{auto& f=get(p,e);f.record(3);assert(!std::strcmp(n,"TLK_IDLE"));return f.fail!=3;};
        b.stop_scene=[](void* p,void* e,int n) noexcept -> int{auto& f=get(p,e);f.record(4);assert(n==1);return f.fail!=4;};
        b.interrupt=[](void* p,void* e) noexcept -> int{auto& f=get(p,e);f.record(5);return f.fail!=5;};
        b.scene_blocked=[](void* p,void* e,int n,unsigned* out) noexcept -> int{auto& f=get(p,e);f.record(6);assert(n==1);*out=f.blocked;return f.fail!=6;};
        b.refused_log=[](void* p,void* e,const char* n) noexcept -> int{auto& f=get(p,e);f.record(7);assert(!std::strcmp(n,"TLK_IDLE"));return f.fail!=7;};
        b.sound=[](void* p,void* e,const char* t,float* out) noexcept -> int{auto& f=get(p,e);f.record(8);text_ok(t);*out=f.duration;return f.fail!=8;};
        b.scene=[](void* p,void* e,const char* t,float d,float* out) noexcept -> int{auto& f=get(p,e);f.record(9);text_ok(t);assert(d==f.delay);*out=f.duration;return f.fail!=9;};
        b.sound_duration=[](void* p,void* e,const char* t,float* out) noexcept -> int{auto& f=get(p,e);f.record(10);text_ok(t);*out=f.duration;return f.fail!=10;};
        b.emit_raw=[](void* p,void* e,const char* t,int a,int z) noexcept -> int{auto& f=get(p,e);f.record(11);text_ok(t);assert(!a&&!z);return f.fail!=11;};
        b.note_duration=[](void* p,void* e,float d,float delay) noexcept -> int{auto& f=get(p,e);f.record(12);assert(d==f.duration&&delay==f.delay);return f.fail!=12;};
        b.sentence=[](void* p,void* e,const char* t,float d,float v,int level,int a,int* out) noexcept -> int{auto& f=get(p,e);f.record(13);text_ok(t);assert(d==f.delay&&v==1.f&&level==f.level&&!a);*out=f.sentence_index;return f.fail!=13;};
        b.print_enabled=[](void* p,unsigned* out) noexcept -> int{auto& f=*static_cast<ResponseDispatchFixture*>(p);f.record(14);*out=f.enabled;return f.fail!=14;};
        b.print=[](void* p,void* e,const char* t) noexcept -> int{auto& f=get(p,e);f.record(15);text_ok(t);return f.fail!=15;};
        b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<ResponseDispatchFixture*>(p);f.record(16);*out=f.now;return f.fail!=16;};
        b.debug_success=[](void* p,void* e,const char* n,const char* t) noexcept -> int{auto& f=get(p,e);f.record(17);assert(n&&f.last()==f.now);text_ok(t);return f.fail!=17;};
        b.destroy=[](void* p,void* r) noexcept -> int{auto& f=*static_cast<ResponseDispatchFixture*>(p);f.record(18);assert(r==f.response);return f.fail!=18;};
        b.record=[](void* p,void* e,const char* n,void* r) noexcept -> int{auto& f=get(p,e);f.record(19);assert(n&&r==f.response&&f.last()==f.now);return f.fail!=19;};
    }
    int run(const ResponseDispatchApi& api,ResponseKind k,const char* name="TLK_IDLE"){put(0,static_cast<unsigned>(k));played=99;return api.dispatch(exp,sizeof(exp),&b,name,response,sizeof(response),&played);}
};
inline void check_response_dispatch(const ResponseDispatchApi& api){
    ResponseDispatchFixture f;
    auto run=[&](ResponseKind k){return f.run(api,k);};
    assert(run(ResponseKind::Speak)==1&&f.played==1&&f.last()==10);f.expect({2,8,12,16,17,19});
    f.duration=0;assert(run(ResponseKind::Speak)==1&&!f.played);f.expect({2,8,18});
    f.put(0x88,0x20);assert(run(ResponseKind::Speak)==1&&f.played);f.expect({2,10,11,12,16,17,19});
    f.put(0x88,0x11);f.put(0xa0,91);f.level=91;f.delay=.5f;
    f.sentence_index=-1;assert(run(ResponseKind::Sentence)==1&&!f.played);f.expect({1,2,13,18});
    f.sentence_index=0;assert(run(ResponseKind::Sentence)==1&&f.played);f.expect({1,2,13,16,17,19});
    f.duration=2;assert(run(ResponseKind::Scene)==1&&f.played);f.expect({1,2,9,12,16,17,19});
    f.duration=-1;assert(run(ResponseKind::Scene)==1&&!f.played);f.expect({1,2,9,18});
    f.enabled=0;assert(run(ResponseKind::Print)==1&&!f.played);f.expect({1,2,14,18});
    f.enabled=1;assert(run(ResponseKind::Print)==1&&f.played);f.expect({1,2,14,15,16,19});
    assert(run(ResponseKind::Response)==1&&!f.played);f.expect({1,2,18});
    f.busy=1;f.blocked=1;assert(run(ResponseKind::Speak)==1&&!f.played);f.expect({1,2,3,4,5,6,7,18});
    f.blocked=0;f.duration=2;assert(run(ResponseKind::Speak)==1&&f.played);f.expect({1,2,3,4,5,6,8,12,16,17,19});
    assert(f.run(api,ResponseKind::Speak,"")==1&&f.played);f.expect({1,2,8,12,16,17,19});
    f.busy=0;f.fail=19;f.now=20;assert(run(ResponseKind::Speak)==-1&&f.played==99&&f.last()==20);f.expect({1,2,8,12,16,17,19});
    f.fail=12;assert(run(ResponseKind::Speak)==-1&&f.played==99);f.expect({1,2,8,12,18});
}


struct ResponseTimingFixture {
    alignas(4) unsigned char exp[0x7c]{};
    SpeechSemaphorePair state{};
    ResponseNoteBindings b{};
    ResponseIntervalBindings interval{};
    unsigned events[32]{},count=0,fail=0,uses=1,classified=1,clock_calls=0;
    std::uint32_t owner=0x123004;
    float now=10,expected_total=0,expected_delay=0,expected_until=0,offset=.5f,width=2;
    void record(unsigned n){assert(count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> l){assert(count==l.size()&&!std::memcmp(events,l.begin(),count*4));count=0;}
    float read(unsigned off){float v=0;std::memcpy(&v,exp+off,4);return v;}
    void put(unsigned off,float v){std::memcpy(exp+off,&v,4);}
    static ResponseTimingFixture& get(void* p,void* e){auto& f=*static_cast<ResponseTimingFixture*>(p);assert(e==f.exp);return f;}
    ResponseTimingFixture(){
        b.context=this;
        b.owner_start=[](void* p,void* e) noexcept -> int{auto& f=get(p,e);f.record(1);return f.fail!=1;};
        b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<ResponseTimingFixture*>(p);f.record(2);*out=f.now+(f.clock_calls++?1:0);return f.fail!=2;};
        b.diagnostic=[](void* p,void* e,float total,float delay,float until) noexcept -> int{
            auto& f=get(p,e);f.record(3);assert(total==f.expected_total&&delay==f.expected_delay&&until==f.expected_until&&f.read(0x68)==until&&f.read(0x6c)==until-delay);return f.fail!=3;};
        b.uses_semaphore=[](void* p,void* e,unsigned* out) noexcept -> int{auto& f=get(p,e);f.record(4);*out=f.uses;return f.fail!=4;};
        b.classify=[](void* p,void* e,int a,unsigned* out) noexcept -> int{auto& f=get(p,e);f.record(5);assert(!a);*out=f.classified;return f.fail!=5;};
        b.owner_handle=[](void* p,void* e,std::uint32_t* out) noexcept -> int{
            auto& f=get(p,e);f.record(6);const auto& sem=f.classified?f.state.classified:f.state.other;
            assert(sem.release==f.now+1+f.expected_delay);*out=f.owner;return f.fail!=6;};
        interval.context=this;
        interval.random=[](void* p,float low,float high,float* out) noexcept -> int{auto& f=*static_cast<ResponseTimingFixture*>(p);f.record(7);assert(low==0&&high==f.width);*out=f.offset;return f.fail!=7;};
    }
};
inline void check_response_timing_update(const ResponseTimingUpdateApi& api){
    ResponseTimingFixture f;unsigned result=99;
    auto note=[&](float duration,float delay,float total,float until){f.clock_calls=0;f.expected_delay=delay;f.expected_total=total;f.expected_until=until;return api.note(f.exp,sizeof(f.exp),&f.state,&f.b,duration,delay);};
    assert(note(2,.5f,2.5f,12.5f)==1&&f.state.classified.release==11.5f&&f.state.classified.owner==f.owner&&f.state.other.owner==0xffffffffu);f.expect({1,2,3,4,5,2,6});
    assert(api.speaking(f.exp,sizeof(f.exp),12,&result)==1&&result);
    assert(api.speaking(f.exp,sizeof(f.exp),12.5f,&result)==1&&!result);
    f.put(0x78,12.5f);assert(api.speaking(f.exp,sizeof(f.exp),12.5f,&result)==1&&result);
    f.classified=0;f.owner=0xffffffffu;
    assert(note(-2,1,0,13)==1&&f.state.other.release==12&&f.state.other.owner==0xffffffffu&&f.state.classified.release==11.5f);f.expect({1,2,3,4,5,2,6});
    f.uses=0;assert(note(1,0,1,11)==1&&f.state.other.release==12);f.expect({1,2,3,4});
    f.uses=1;f.fail=6;f.now=20;assert(note(1,0,1,21)==-1&&f.state.other.release==21&&f.state.other.owner==0xffffffffu);f.expect({1,2,3,4,5,2,6});
    float interval[]={3,0},sample=99;
    assert(api.sample(interval,sizeof(interval),&f.interval,&sample)==1&&sample==3);f.expect({});
    interval[1]=2;f.fail=0;assert(api.sample(interval,sizeof(interval),&f.interval,&sample)==1&&sample==3.5f);f.expect({7});
    f.fail=7;sample=99;assert(api.sample(interval,sizeof(interval),&f.interval,&sample)==-1&&sample==99);f.expect({7});
    assert(api.sample(interval,7,&f.interval,&sample)==0);f.expect({});
}


struct ResponseRecordFixture {
    alignas(4) unsigned char exp[0x7c]{},row[0x1c]{},first[0x124]{},second[0x124]{};
    ResponseRecordBindings b{};
    int index=-1;
    unsigned events[32]{},count=0,clocks=0,fail=0;
    float now=10;
    void* next=first;
    void record(unsigned n){assert(count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> l){assert(count==l.size()&&!std::memcmp(events,l.begin(),count*4));count=0;}
    static std::uint32_t ptr(void* v){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(v));}
    unsigned word(unsigned off){unsigned v=0;std::memcpy(&v,row+off,4);return v;}
    float timestamp(){float v=0;std::memcpy(&v,row+0x14,4);return v;}
    static ResponseRecordFixture& get(void* p,void* e,const char* n){auto& f=*static_cast<ResponseRecordFixture*>(p);assert(e==f.exp&&!std::strcmp(n,"TLK_IDLE"));return f;}
    ResponseRecordFixture(){
        b.context=this;b.history.context=this;
        b.history.lookup=[](void* p,void* e,const char* n,int* out) noexcept -> int{auto& f=get(p,e,n);f.record(1);*out=f.index;return f.fail!=1;};
        b.history.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<ResponseRecordFixture*>(p);f.record(2);*out=f.now+f.clocks++;return f.fail!=2;};
        b.history.insert=[](void* p,void* e,const char* n,float t,int* out) noexcept -> int{
            auto& f=get(p,e,n);f.record(3);assert(f.index==-1&&t==f.now);f.index=*out=7;std::memcpy(f.row+0x14,&t,4);return f.fail!=3;};
        b.history.entry=[](void* p,void* e,int i,void** out,unsigned* n) noexcept -> int{
            auto& f=*static_cast<ResponseRecordFixture*>(p);f.record(4);assert(e==f.exp&&i==7);*out=f.row;*n=sizeof(f.row);return f.fail!=4;};
        b.destroy=[](void* p,void* response) noexcept -> int{
            auto& f=*static_cast<ResponseRecordFixture*>(p);f.record(response==f.next?6:5);assert(response==f.first||response==f.second);
            if(response!=f.next)assert(f.timestamp()==f.now&&f.word(0x18)==ptr(response));
            return f.fail!=5;
        };
        b.history.notify=[](void* p,void* e,const char* n) noexcept -> int{
            auto& f=get(p,e,n);f.record(7);assert(f.word(0x18)==ptr(f.next)&&f.timestamp()==f.now+f.clocks-1);return f.fail!=7;};
    }
};
inline void check_response_record(const ResponseRecordApi& api){
    ResponseRecordFixture f;
    auto run=[&](void* response,unsigned notify=1){f.next=response;f.clocks=0;return api.record(f.exp,sizeof(f.exp),&f.b,"TLK_IDLE",response,0x124,notify);};
    assert(run(f.first)==1&&f.timestamp()==11&&f.word(0x18)==f.ptr(f.first));f.expect({1,2,3,4,2,7});
    f.now=20;assert(run(f.second)==1&&f.timestamp()==20&&f.word(0x18)==f.ptr(f.second));f.expect({1,4,2,5,7});
    f.now=21;assert(run(f.first,0x100)==1&&f.word(0x18)==f.ptr(f.first));f.expect({1,4,2,5});
    f.now=22;assert(run(f.first)==-1&&f.timestamp()==21&&f.word(0x18)==f.ptr(f.first));f.expect({1,4,2});
    f.fail=7;assert(run(f.second)==-1&&f.timestamp()==22&&f.word(0x18)==f.ptr(f.second));f.expect({1,4,2,5,7});
    f.fail=5;f.now=23;assert(run(f.first)==-1&&f.timestamp()==23&&!f.word(0x18));f.expect({1,4,2,5,6});
    f.fail=1;assert(run(f.first)==-1&&!f.word(0x18));f.expect({1,6});
    assert(api.record(f.exp,sizeof(f.exp),&f.b,"TLK_IDLE",f.first,0x123,1)==0);f.expect({});
}


struct ResponseSpeakFixture {
    const ResponseTimingUpdateApi& timing;
    const ResponseRecordApi& record_api;
    alignas(4) unsigned char exp[0x7c]{},pool[2][0x124]{},row[0x1c]{};
    SpeechSemaphorePair sem{};
    ResponseSpeakBindings b{};
    ResponseNoteBindings note{};
    ResponseRecordBindings history{};
    bool live[2]{},has_system=true,has_match=true,fail_trace=false,fail_sound=false,fail_notify=false;
    int system=0,criteria=0,index=-1,allocated=-1;
    unsigned allocations=0,destroys=0,traces=0,sounds=0,notifications=0,criteria_ends=0;
    float now=10;
    const char* expected_concept="TLK_IDLE";
    const char* expected_modifiers=nullptr;
    unsigned modifier_calls=0;
    void* notify_context=nullptr;
    int (__cdecl *after_notify)(void*,void*,const char*) noexcept=nullptr;
    static std::uint32_t ptr(void* p){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));}
    float read(const void* p,unsigned off){float v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;}
    unsigned word(unsigned off){unsigned v=0;std::memcpy(&v,row+off,4);return v;}
    static ResponseSpeakFixture& get(void* p,void* e){auto& f=*static_cast<ResponseSpeakFixture*>(p);assert(e==f.exp);return f;}
    static int __cdecl destroy(void* p,void* response) noexcept {
        auto& f=*static_cast<ResponseSpeakFixture*>(p);const auto i=response==f.pool[0]?0:1;
        assert(response==f.pool[i]&&f.live[i]);f.live[i]=false;++f.destroys;return 1;
    }
    explicit ResponseSpeakFixture(const ResponseTimingUpdateApi& t,const ResponseRecordApi& r):timing(t),record_api(r){
        b.context=this;b.selection.context=this;b.dispatch.context=this;
        b.selection.system=[](void* p,void* e,void** out) noexcept -> int{auto& f=get(p,e);*out=f.has_system?&f.system:nullptr;return 1;};
        b.selection.begin_criteria=[](void* p,void** out) noexcept -> int{*out=&static_cast<ResponseSpeakFixture*>(p)->criteria;return 1;};
        b.selection.add=[](void* p,void* c,const char* k,const char* n,float w) noexcept -> int{auto& f=*static_cast<ResponseSpeakFixture*>(p);assert(c==&f.criteria&&!std::strcmp(k,"concept")&&!std::strcmp(n,f.expected_concept)&&w==5.f);return 1;};
        b.selection.modifiers=[](void* p,void* c,const char* mods,float weight) noexcept -> int{
            auto& f=*static_cast<ResponseSpeakFixture*>(p);assert(c==&f.criteria&&weight==5.f&&f.expected_modifiers&&!std::strcmp(mods,f.expected_modifiers));++f.modifier_calls;return 1;
        };
        b.selection.npc_criteria=[](void*,void*,void*) noexcept -> int{return 1;};
        b.selection.player_criteria=[](void*,void*,int i) noexcept -> int{assert(i==1);return 1;};
        b.selection.allocate=[](void* p,unsigned n,void** out,unsigned* extent) noexcept -> int{
            auto& f=*static_cast<ResponseSpeakFixture*>(p);assert(n==0x124);
            const auto i=!f.live[0]?0:!f.live[1]?1:-1;assert(i>=0);f.allocated=i;f.live[i]=true;++f.allocations;*out=f.pool[i];*extent=n;return 1;};
        b.selection.find=[](void* p,void* s,void* c,void* response,void* e,unsigned* out) noexcept -> int{
            auto& f=get(p,e);assert(s==&f.system&&c==&f.criteria&&response==f.pool[f.allocated]&&f.live[f.allocated]);
            *out=f.has_match;if(!*out){f.live[f.allocated]=false;return 1;} // finder-false host ownership
            const unsigned type=1;std::memcpy(response,&type,4);std::strcpy(static_cast<char*>(response)+4,"native_line");return 1;};
        b.selection.draw=[](void*,int,int,int*) noexcept -> int{assert(false);return 0;};
        b.selection.destroy=destroy;
        b.selection.end_criteria=[](void* p,void* c) noexcept -> int{auto& f=*static_cast<ResponseSpeakFixture*>(p);assert(c==&f.criteria);++f.criteria_ends;return 1;};
        b.trace_selected=[](void* p,void* e,const char* n) noexcept -> int{auto& f=get(p,e);assert(!std::strcmp(n,f.expected_concept)&&f.live[f.allocated]&&f.allocations==f.criteria_ends);++f.traces;return !f.fail_trace;};
        b.dispatch.sample=[](void*,const void*,float*) noexcept -> int{assert(false);return 0;};
        b.dispatch.speaking=[](void* p,void* e,unsigned* out) noexcept -> int{auto& f=get(p,e);return f.timing.speaking(e,sizeof(f.exp),f.now,out)==1;};
        b.dispatch.force_log=[](void*,void*,const char*) noexcept -> int{assert(false);return 0;};
        b.dispatch.stop_scene=[](void*,void*,int) noexcept -> int{assert(false);return 0;};
        b.dispatch.interrupt=[](void*,void*) noexcept -> int{assert(false);return 0;};
        b.dispatch.scene_blocked=[](void*,void*,int,unsigned*) noexcept -> int{assert(false);return 0;};
        b.dispatch.refused_log=[](void*,void*,const char*) noexcept -> int{assert(false);return 0;};
        b.dispatch.sound=[](void* p,void* e,const char* text,float* out) noexcept -> int{auto& f=get(p,e);assert(!std::strcmp(text,"native_line")&&f.traces>f.sounds);++f.sounds;*out=2;return !f.fail_sound;};
        b.dispatch.scene=[](void*,void*,const char*,float,float*) noexcept -> int{assert(false);return 0;};
        b.dispatch.sound_duration=[](void*,void*,const char*,float*) noexcept -> int{assert(false);return 0;};
        b.dispatch.emit_raw=[](void*,void*,const char*,int,int) noexcept -> int{assert(false);return 0;};
        b.dispatch.note_duration=[](void* p,void* e,float d,float delay) noexcept -> int{auto& f=get(p,e);return f.timing.note(e,sizeof(f.exp),&f.sem,&f.note,d,delay)==1;};
        b.dispatch.sentence=[](void*,void*,const char*,float,float,int,int,int*) noexcept -> int{assert(false);return 0;};
        b.dispatch.print_enabled=[](void*,unsigned*) noexcept -> int{assert(false);return 0;};
        b.dispatch.print=[](void*,void*,const char*) noexcept -> int{assert(false);return 0;};
        b.dispatch.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<ResponseSpeakFixture*>(p)->now;return 1;};
        b.dispatch.debug_success=[](void* p,void* e,const char*,const char*) noexcept -> int{auto& f=get(p,e);assert(f.read(e,0x78)==f.now&&f.read(e,0x68)==f.now+2);return 1;};
        b.dispatch.destroy=destroy;
        b.dispatch.record=[](void* p,void* e,const char* n,void* response) noexcept -> int{auto& f=get(p,e);return f.record_api.record(e,sizeof(f.exp),&f.history,n,response,0x124,1)==1;};
        note.context=this;note.owner_start=[](void*,void*) noexcept -> int{return 1;};
        note.clock=b.dispatch.clock;
        note.diagnostic=[](void* p,void* e,float total,float delay,float until) noexcept -> int{auto& f=get(p,e);assert(total==2&&!delay&&until==f.now+2);return 1;};
        note.uses_semaphore=[](void*,void*,unsigned* out) noexcept -> int{*out=1;return 1;};
        note.classify=[](void*,void*,int a,unsigned* out) noexcept -> int{assert(!a);*out=1;return 1;};
        note.owner_handle=[](void*,void*,std::uint32_t* out) noexcept -> int{*out=0x123004;return 1;};
        history.context=this;history.destroy=destroy;history.history.context=this;
        history.history.lookup=[](void* p,void* e,const char* n,int* out) noexcept -> int{auto& f=get(p,e);assert(!std::strcmp(n,f.expected_concept));*out=f.index;return 1;};
        history.history.insert=[](void* p,void* e,const char*,float t,int* out) noexcept -> int{auto& f=get(p,e);assert(f.index==-1&&t==f.now);f.index=*out=7;return 1;};
        history.history.entry=[](void* p,void* e,int i,void** out,unsigned* extent) noexcept -> int{auto& f=get(p,e);assert(i==7);*out=f.row;*extent=sizeof(f.row);return 1;};
        history.history.clock=b.dispatch.clock;
        history.history.notify=[](void* p,void* e,const char* name) noexcept -> int{
            auto& f=get(p,e);assert(f.read(f.row,0x14)==f.now&&f.word(0x18)==ptr(f.pool[f.allocated])&&f.live[f.allocated]);++f.notifications;
            return !f.fail_notify&&(!f.after_notify||f.after_notify(f.notify_context,e,name));};
    }
};
inline void check_response_speak(const ResponseSpeakApi& api,const ResponseTimingUpdateApi& timing,const ResponseRecordApi& record){
    ResponseSpeakFixture f(timing,record);unsigned played=99;
    auto run=[&](){played=99;return api.speak(f.exp,sizeof(f.exp),&f.b,"TLK_IDLE",nullptr,&played);};
    f.has_system=false;assert(run()==1&&!played&&!f.allocations);f.has_system=true;
    f.has_match=false;assert(run()==1&&!played&&f.allocations==1&&!f.live[0]&&!f.traces);f.has_match=true;
    assert(run()==1&&played&&f.live[0]&&!f.live[1]&&f.word(0x18)==f.ptr(f.pool[0])&&f.notifications==1&&f.sem.classified.release==10&&f.sem.classified.owner==0x123004);
    f.now=13;assert(run()==1&&played&&!f.live[0]&&f.live[1]&&f.word(0x18)==f.ptr(f.pool[1])&&f.notifications==2&&f.destroys==1);
    f.now=16;f.fail_trace=true;assert(run()==-1&&played==99&&!f.live[0]&&f.live[1]&&f.destroys==2);f.fail_trace=false;
    f.fail_sound=true;assert(run()==-1&&played==99&&!f.live[0]&&f.live[1]&&f.destroys==3&&f.read(f.exp,0x78)==13);f.fail_sound=false;
    const auto saved=f.b.dispatch.sound;f.b.dispatch.sound=nullptr;
    assert(run()==-1&&played==99&&!f.live[0]&&f.live[1]&&f.destroys==4);f.b.dispatch.sound=saved;
    f.fail_notify=true;assert(run()==-1&&played==99&&f.live[0]&&!f.live[1]&&f.destroys==5&&f.notifications==3&&f.word(0x18)==f.ptr(f.pool[0])&&f.read(f.exp,0x78)==16);
    // Explicit fixture teardown of the remaining owned response.
    assert(ResponseSpeakFixture::destroy(&f,f.pool[0])&&!f.live[0]&&!f.live[1]);
}


struct ResponseRepeatFixture {
    alignas(4) unsigned char exp[0x7c]{},row[0x1c]{},response[0x124]{};
    ResponseRepeatBindings b{};SpeechGateBindings gate{};SpeechSemaphorePair sem{};
    unsigned events[32]{},count=0,fail=0,uses=1,classified=1;
    int index=7,npc=0,other=0;
    float now=10,offset=1;
    void rec(unsigned n){assert(count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> l){assert(count==l.size()&&!std::memcmp(events,l.begin(),count*4));count=0;}
    void put(void* p,unsigned off,unsigned v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    void fl(void* p,unsigned off,float v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    static ResponseRepeatFixture& get(void* p){return *static_cast<ResponseRepeatFixture*>(p);}
    ResponseRepeatFixture(){
        b.context=this;b.interval.context=this;gate.context=this;
        put(row,0x18,0x1234);fl(row,0x14,8);put(response,0x88,8);fl(response,0x98,1);fl(response,0x9c,2);
        b.lookup=[](void* p,void* e,const char* n,int* out) noexcept -> int{auto& f=get(p);assert(e==f.exp&&!std::strcmp(n,"TLK_IDLE"));f.rec(1);*out=f.index;return f.fail!=1;};
        b.entry=[](void* p,void* e,int i,void** row,unsigned* extent) noexcept -> int{auto& f=get(p);assert(e==f.exp&&i==7);f.rec(2);*row=f.row;*extent=sizeof(f.row);return f.fail!=2;};
        b.resolve=[](void* p,std::uint32_t address,void** response,unsigned* extent) noexcept -> int{auto& f=get(p);assert(address==0x1234);f.rec(3);*response=f.response;*extent=sizeof(f.response);return f.fail!=3;};
        b.interval.random=[](void* p,float lo,float hi,float* out) noexcept -> int{auto& f=get(p);assert(lo==0&&hi==2);f.rec(4);*out=f.offset;return f.fail!=4;};
        b.clock=[](void* p,float* out) noexcept -> int{auto& f=get(p);f.rec(5);*out=f.now;return f.fail!=5;};
        gate.uses=[](void* p,void* e,unsigned* out) noexcept -> int{auto& f=get(p);assert(e==f.exp);f.rec(6);*out=f.uses;return f.fail!=6;};
        gate.classify=[](void* p,void* n,int a,unsigned* out) noexcept -> int{auto& f=get(p);assert(n==&f.npc&&!a);f.rec(7);*out=f.classified;return f.fail!=7;};
        gate.clock=b.clock;
        gate.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=get(p);f.rec(8);*out=h==1?&f.npc:h==2?&f.other:nullptr;return f.fail!=8;};
    }
};
inline void check_response_repeat(const ResponseRepeatApi& api){
    ResponseRepeatFixture f;unsigned out=99;float last=99;
    auto repeat=[&](){out=99;return api.can_repeat(f.exp,sizeof(f.exp),&f.b,"TLK_IDLE",&out);};
    auto gate=[&](){out=99;return api.semaphore_available(f.exp,sizeof(f.exp),&f.npc,&f.sem,&f.gate,&out);};
    f.index=-1;assert(repeat()==1&&out);f.expect({1});
    assert(api.last_time(f.exp,sizeof(f.exp),&f.b,"TLK_IDLE",&last)==1&&last==-1);f.expect({1});
    f.index=7;assert(api.last_time(f.exp,sizeof(f.exp),&f.b,"TLK_IDLE",&last)==1&&last==8);f.expect({1,2});
    f.put(f.row,0x18,0);assert(repeat()==1&&out);f.expect({1,2});f.put(f.row,0x18,0x1234);
    f.put(f.response,0x88,10);assert(repeat()==1&&!out);f.expect({1,2,3});
    f.put(f.response,0x88,0);assert(repeat()==1&&out);f.expect({1,2,3});
    f.put(f.response,0x88,8);assert(repeat()==1&&out);f.expect({1,2,3,4,5});
    f.now=9.99f;assert(repeat()==1&&!out);f.expect({1,2,3,4,5});
    f.fl(f.row,0x14,-1);assert(repeat()==1&&out);f.expect({1,2,3,4});
    f.fail=4;assert(repeat()==-1&&out==99);f.expect({1,2,3,4});f.fail=0;
    f.fl(f.row,0x14,8);f.fl(f.response,0x98,0);f.offset=0;assert(repeat()==1&&out);f.expect({1,2,3,4});
    f.now=10;f.sem.classified.release=10;f.sem.classified.owner=2;f.sem.other.release=20;f.sem.other.owner=1;
    f.uses=0;assert(gate()==1&&out);f.expect({6});f.uses=1;
    assert(gate()==1&&!out);f.expect({6,7,5,8});
    f.now=10.01f;assert(gate()==1&&out);f.expect({6,7,5});
    f.classified=0;assert(gate()==1&&out);f.expect({6,7,5,8});
    f.sem.other.owner=3;assert(gate()==1&&!out);f.expect({6,7,5,8});
    f.sem.other.owner=0xffffffffu;assert(gate()==1&&!out);f.expect({6,7,5});
    f.sem.other.owner=2;f.fail=8;assert(gate()==-1&&out==99);f.expect({6,7,5,8});
}


struct ExpeditorLifecycleFixture {
    alignas(4) unsigned char exp[0x7c]{},rows[3][0x1c]{};
    ExpeditorControl ctrl{exp,sizeof(exp)};
    ExpeditorTables tables{0x11110000,0x22220000,0x33330000,0x44440000};
    ExpeditorLifecycleBindings b{};SpeechSemaphorePair sem{};
    int npc=0,other=0;
    unsigned events[32]{},count=0,fail=0,class_calls=0,classification=1,second_class=1;
    const ExpeditorLifecycleApi& api;
    void rec(unsigned n){assert(count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> l){assert(count==l.size()&&!std::memcmp(events,l.begin(),count*4));count=0;}
    static unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    static void put(void* p,unsigned off,unsigned v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    static unsigned word(const void* p,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;}
    static ExpeditorLifecycleFixture& get(void* p){return *static_cast<ExpeditorLifecycleFixture*>(p);}
    explicit ExpeditorLifecycleFixture(const ExpeditorLifecycleApi& a):api(a){
        b.context=this;
        b.symbol_construct=[](void* p,void* s,int x,int y,unsigned z) noexcept -> int{
            auto& f=get(p);f.rec(1);assert(s==f.exp+0x34&&!x&&!y&&z==1&&f.ctrl.stage==ExpeditorStage::Constructing);
            assert(word(f.exp,0)==f.tables.primary&&word(f.exp,8)==f.tables.filter&&word(f.exp,0x10)==f.tables.compare&&word(f.exp,0x20)==0xffffffffu&&word(f.exp,0x28)==0xffffffffu&&!word(f.exp,0x74));
            assert(f.api.clear(&f.ctrl,&f.b)==0);put(s,0,0xabcdef);return f.fail!=1;};
        b.row=[](void* p,void* e,unsigned i,void** out,unsigned* n) noexcept -> int{auto& f=get(p);assert(e==f.exp&&i<3&&f.ctrl.stage==ExpeditorStage::Destroying);f.rec(10+i);*out=f.rows[i];*n=sizeof(f.rows[i]);return f.fail!=10+i;};
        b.destroy_response=[](void* p,std::uint32_t v) noexcept -> int{auto& f=get(p);assert(v==0x100||v==0x300);const unsigned event=v==0x100?20:22;f.rec(event);return f.fail!=event;};
        b.classify=[](void* p,void* n,int a,unsigned* out) noexcept -> int{auto& f=get(p);assert(n==&f.npc&&!a);f.rec(2);*out=f.class_calls++?f.second_class:f.classification;return f.fail!=2;};
        b.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=get(p);f.rec(3);*out=h==1?&f.npc:h==2?&f.other:nullptr;return f.fail!=3;};
        b.symbol_destroy=[](void* p,void* s) noexcept -> int{auto& f=get(p);f.rec(4);assert(s==f.exp+0x34);return f.fail!=4;};
        b.free_rows=[](void* p,std::uint32_t a) noexcept -> int{auto& f=get(p);f.rec(5);assert(a==0x7777);return f.fail!=5;};
    }
    int construct(){std::memset(exp,0xa5,sizeof(exp));return api.construct(&ctrl,&npc,&tables,&b);}
    void populate(){
        put(exp,0x14,0x7777);put(exp,0x18,3);put(exp,0x2c,3);put(exp,0x20,0);put(exp,0x24,2);put(exp,0x28,1);
        put(rows[0],0,0xffffffffu);put(rows[0],0x18,0x100);
        put(rows[1],0,1);put(rows[1],0x18,0xdead); // free node: never destroy its stale word
        put(rows[2],0,0xffffffffu);put(rows[2],0x18,0x300);
    }
};
inline void check_expeditor_lifecycle(const ExpeditorLifecycleApi& api){
    using F=ExpeditorLifecycleFixture;F f(api);
    assert(f.construct()==1&&f.ctrl.stage==ExpeditorStage::Live&&F::word(f.exp,0x74)==100&&!F::word(f.exp,0x78)&&F::word(f.exp,4)==F::ptr(&f.npc));f.expect({1});
    f.populate();assert(api.clear(&f.ctrl,&f.b)==1&&f.ctrl.stage==ExpeditorStage::Live);f.expect({12,22,11,10,20});
    assert(F::word(f.exp,0x20)==0xffffffffu&&!F::word(f.exp,0x24)&&!F::word(f.exp,0x28)&&F::word(f.exp,0x2c)==3);
    for(unsigned i=0;i<3;++i){assert(F::word(f.rows[i],0)==i&&F::word(f.rows[i],4)==(i==2?0xffffffffu:i+1));}
    assert(F::word(f.rows[1],0x18)==0xdead&&!F::word(f.rows[0],0x18)&&!F::word(f.rows[2],0x18));
    f.sem.classified={10,1};f.sem.other={20,2};
    assert(api.destroy(&f.ctrl,&f.npc,&f.tables,&f.sem,&f.b)==1&&f.ctrl.stage==ExpeditorStage::Retired);f.expect({12,11,10,2,3,2,4,5});
    assert(!f.sem.classified.release&&f.sem.classified.owner==0xffffffffu&&f.sem.other.owner==2&&F::word(f.exp,0)==f.tables.base&&!F::word(f.exp,0x14)&&!F::word(f.exp,0x18));
    assert(api.destroy(&f.ctrl,&f.npc,&f.tables,&f.sem,&f.b)==0);
    F stale(api);assert(stale.construct()==1);stale.expect({1});stale.sem.classified={10,3};
    assert(api.destroy(&stale.ctrl,&stale.npc,&stale.tables,&stale.sem,&stale.b)==1&&stale.sem.classified.owner==3);stale.expect({2,3,4});
    F external(api);assert(external.construct()==1);external.expect({1});external.populate();F::put(external.exp,0x1c,0xffffffffu);
    external.sem.classified={10,1};external.sem.other={20,2};external.second_class=0;
    assert(api.destroy(&external.ctrl,&external.npc,&external.tables,&external.sem,&external.b)==1);external.expect({12,22,11,10,20,2,3,2,4});
    assert(external.sem.classified.owner==1&&external.sem.other.owner==0xffffffffu&&F::word(external.exp,0x14)==0x7777);
    F failed(api);assert(failed.construct()==1);failed.expect({1});failed.populate();failed.fail=20;
    assert(api.clear(&failed.ctrl,&failed.b)==-1&&failed.ctrl.stage==ExpeditorStage::Failed&&!F::word(failed.rows[0],0x18));failed.expect({12,22,11,10,20});
    assert(api.clear(&failed.ctrl,&failed.b)==0);
}


struct TalkCatalogFixture {
    alignas(4) unsigned char npc[0x1458]{};
    TalkCooldownState state{};TalkCooldownBindings b{};
    unsigned events[32]{},count=0,fail=0,disabled=0;
    bool has_deadline=true;
    float now=10,deadline=0,lo=10,hi=20,draw=15;
    const char* concept_name="TLK_HELLO";
    void rec(unsigned n){assert(count<32);events[count++]=n;}
    void expect(std::initializer_list<unsigned> l){assert(count==l.size()&&!std::memcmp(events,l.begin(),count*4));count=0;}
    static TalkCatalogFixture& get(void* p){return *static_cast<TalkCatalogFixture*>(p);}
    TalkCatalogFixture(){
        b.context=this;
        b.clock=[](void* p,float* out) noexcept -> int{auto& f=get(p);f.rec(1);*out=f.now;return f.fail!=1;};
        b.random=[](void* p,float low,float high,float* out) noexcept -> int{auto& f=get(p);f.rec(2);assert(low==f.lo&&high==f.hi);*out=f.draw;return f.fail!=2;};
        b.concept_deadline=[](void* p,const char* n,float** out) noexcept -> int{auto& f=get(p);f.rec(3);assert(n&&!std::strcmp(n,f.concept_name));*out=f.has_deadline?&f.deadline:nullptr;return f.fail!=3;};
        b.propagate=[](void* p,void* npc,const char* n) noexcept -> int{auto& f=get(p);f.rec(4);assert(npc==f.npc&&!std::strcmp(n,f.concept_name));return f.fail!=4;};
        b.cooldown_disabled=[](void* p,unsigned* out) noexcept -> int{auto& f=get(p);f.rec(5);*out=f.disabled;return f.fail!=5;};
    }
};
inline void check_talk_catalog(const TalkCatalogApi& api){
    TalkCatalogFixture f;unsigned out=99;
    assert(api.count==48&&!api.at(48)&&!api.lookup("unknown"));
    for(unsigned i=0;i<48;++i){const auto* d=api.at(i);assert(d&&d->name&&api.lookup(d->name)&&d->priority==(i<14?0u:i<45?1u:2u));}
    const auto* hello=api.lookup("tlk_hello");assert(hello==api.at(4)&&hello->flags==7&&hello->global_min==5&&hello->global_max==10);
    assert(api.lookup("TLK_INPOISONCLOUD")==api.at(29)&&api.at(32)->priority==api.at(14)->priority);
    assert(api.at(25)->concept_min==15&&api.at(25)->concept_max==30&&api.at(45)->concept_min==5&&api.at(45)->concept_max==7);
    auto note=[&](){return api.note(f.npc,sizeof(f.npc),&f.state,&f.b,f.concept_name);};
    f.lo=5;f.hi=10;f.draw=7;assert(note()==1&&f.state.global[0]==17);f.expect({4,5,2,1});
    f.disabled=1;assert(note()==1&&f.state.global[0]==17);f.expect({4,5});f.disabled=0;
    f.concept_name="TLK_IDLE";f.lo=10;f.hi=20;f.draw=15;assert(note()==1&&f.state.global[0]==25);f.expect({5,2,1});
    f.concept_name="TLK_PLPUSH";f.lo=f.hi=f.draw=15;assert(note()==1&&f.deadline==25&&!f.state.global[1]);f.expect({5,3,2,1});
    f.has_deadline=false;assert(note()==1);f.expect({5,3});f.has_deadline=true;
    assert(api.concept_ready(&f.b,f.concept_name,&out)==1&&!out);f.expect({3,1});
    f.now=24.9995f;assert(api.concept_ready(&f.b,f.concept_name,&out)==1&&out);f.expect({3,1});
    f.now=24.998f;assert(api.concept_ready(&f.b,f.concept_name,&out)==1&&!out);f.expect({3,1});
    auto ready=[&](unsigned p){return api.group_ready(f.npc,sizeof(f.npc),&f.state,&f.b,p,&out);};
    assert(ready(0)==1&&!out);f.expect({1});
    f.now=25;assert(ready(0)==1&&out);f.expect({1,1});
    float future=30;std::memcpy(f.npc+0x1158,&future,4);
    assert(ready(2)==1&&!out);f.expect({1});
    assert(ready(3)==0);f.expect({});
    f.lo=10;f.hi=20;f.draw=15;assert(api.reset_interaction(&f.state,&f.b)==1&&f.state.global[0]==40);f.expect({2,1});
    f.fail=1;assert(api.reset_interaction(&f.state,&f.b)==-1&&f.state.global[0]==40);f.expect({2,1});
}


struct CompanionTalkFixture {
    alignas(4) unsigned char npc[0x1458]{},associated[0x3a4]{},row[0x1c]{};
    ResponseSpeakFixture pipeline;
    CompanionTalkBindings b{};
    TalkCooldownState state{};
    int player=0,entity=0;
    unsigned managers=0,resolves=0,accessors=0,ranges=0,suppressions=0;
    bool live_associated=false,has_player=true,in_range=true,suppressed=false,history=false,fail=false;
    int suppress_argument=-1;
    float deadline=0;
    static CompanionTalkFixture& get(void* p){return *static_cast<CompanionTalkFixture*>(p);}
    void put(unsigned off,unsigned word){std::memcpy(npc+off,&word,4);}
    explicit CompanionTalkFixture(const ResponseTimingUpdateApi& timing,const ResponseRecordApi& record):pipeline(timing,record){
        put(0x850,1);put(0x16c,0x123004);put(0x115c,0x1001);
        b.context=this;b.cooldown.context=this;b.semaphore.context=this;b.repeat.context=this;
        b.manager=[](void* p) noexcept -> int{auto& f=get(p);++f.managers;return !f.fail;};
        b.resolve=[](void* p,std::uint32_t h,void** out,unsigned* n) noexcept -> int{
            auto& f=get(p);++f.resolves;assert(h==0x123004||h==0x1001);
            if(h==0x1001){*out=f.live_associated?f.associated:nullptr;*n=sizeof(f.associated);}
            else {*out=&f.entity;*n=sizeof(f.entity);}return 1;};
        b.expeditor=[](void* p,void* npc,void** out,unsigned* extent) noexcept -> int{
            auto& f=get(p);assert(npc==f.npc);++f.accessors;*out=f.pipeline.exp;*extent=sizeof(f.pipeline.exp);return 1;};
        b.player=[](void* p,void** out) noexcept -> int{auto& f=get(p);*out=f.has_player?&f.player:nullptr;return 1;};
        b.within_range=[](void* p,void* n,void* pl,unsigned* out) noexcept -> int{
            auto& f=get(p);assert(n==f.npc&&pl==&f.player);++f.ranges;*out=f.in_range;return 1;};
        b.suppressed=[](void* p,void* n,int a,unsigned* out) noexcept -> int{
            auto& f=get(p);assert(n==f.npc);++f.suppressions;f.suppress_argument=a;*out=f.suppressed;return 1;};
        b.cooldown.clock=[](void* p,float* out) noexcept -> int{*out=get(p).pipeline.now;return 1;};
        b.cooldown.concept_deadline=[](void* p,const char*,float** out) noexcept -> int{*out=&get(p).deadline;return 1;};
        b.semaphore.uses=[](void*,void*,unsigned* out) noexcept -> int{*out=1;return 1;};
        b.semaphore.classify=[](void* p,void* n,int a,unsigned* out) noexcept -> int{auto& f=get(p);assert(n==f.npc&&!a);*out=1;return 1;};
        b.semaphore.clock=b.cooldown.clock;
        b.semaphore.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=get(p);*out=h==0x123004?f.npc:nullptr;return 1;};
        b.repeat.lookup=[](void* p,void* e,const char*,int* out) noexcept -> int{auto& f=get(p);assert(e==f.pipeline.exp);*out=f.history?0:-1;return 1;};
        b.repeat.entry=[](void* p,void*,int i,void** out,unsigned* n) noexcept -> int{auto& f=get(p);assert(i==0);*out=f.row;*n=sizeof(f.row);return 1;};
        b.repeat.resolve=[](void*,std::uint32_t,void**,unsigned*) noexcept -> int{assert(false);return 0;};
        b.repeat.clock=b.cooldown.clock;b.repeat.interval.random=[](void*,float,float,float*) noexcept -> int{assert(false);return 0;};
        b.speak=pipeline.b;
    }
};
inline void check_companion_talk(const CompanionTalkApi& api,const ResponseTimingUpdateApi& timing,const ResponseRecordApi& record){
    CompanionTalkFixture f(timing,record);unsigned out=99;
    auto group=[&](unsigned priority,unsigned urgent){return api.group_gate(f.npc,sizeof(f.npc),&f.state,&f.pipeline.sem,&f.b,priority,urgent,&out);};
    auto concept=[&](const char* name,unsigned urgent=0){return api.concept_gate(f.npc,sizeof(f.npc),&f.state,&f.pipeline.sem,&f.b,name,urgent,&out);};
    f.state.global[0]=20;assert(group(0,0)==1&&!out&&!f.accessors);
    assert(group(0,1)==1&&out&&f.accessors==2&&f.suppress_argument==0);
    f.state.global[0]=0;f.put(0x850,3);assert(group(0,1)==1&&!out);
    f.put(0x850,1);f.live_associated=true;float neg=-1;std::memcpy(f.associated+0x39c,&neg,4);
    assert(group(0,1)==1&&!out);float zero=0;std::memcpy(f.associated+0x39c,&zero,4);
    assert(group(0,0)==1&&out&&f.suppress_argument==1);
    f.in_range=false;assert(group(1,0)==1&&!out);
    const unsigned accesses=f.accessors,ranges=f.ranges;
    assert(group(2,0)==1&&out&&f.accessors==accesses&&f.ranges==ranges);
    f.in_range=true;f.associated[0x3a0]=1;assert(concept("tlk_hello",1)==1&&!out);
    f.associated[0x3a0]=0;f.history=true;assert(concept("TLK_HELLO",1)==1&&!out);
    float sentinel=-1;std::memcpy(f.row+0x14,&sentinel,4);assert(concept("TLK_HELLO",1)==1&&out);
    f.deadline=20;assert(concept("TLK_IDLE",1)==1&&!out);f.deadline=0;f.history=false;
    f.pipeline.sem.classified={30,0x123004};assert(concept("TLK_IDLE",1)==1&&out);
    f.pipeline.sem.classified={30,0x1004};assert(concept("TLK_IDLE",1)==1&&!out);
    f.pipeline.sem.classified={0,0xffffffffu};
    f.fail=true;out=99;assert(concept("TLK_IDLE")==-1&&out==99);f.fail=false;
    const auto request=[&](){return api.request(f.npc,sizeof(f.npc),&f.state,&f.pipeline.sem,&f.b,"TLK_IDLE",nullptr,0,&out);};
    assert(request()==1&&out&&f.pipeline.sounds==1&&f.pipeline.notifications==1&&f.pipeline.live[0]);
    assert(request()==1&&!out&&f.pipeline.sounds==1); // same-frame timing gate
    f.pipeline.now=13;assert(request()==1&&out&&f.pipeline.sounds==2&&f.pipeline.destroys==1&&f.pipeline.live[1]);
    assert(ResponseSpeakFixture::destroy(&f.pipeline,f.pipeline.pool[1]));
}


struct TalkPropagationFixture {
    alignas(4) unsigned char npc[0x2c8]{},neighbors[4][0x2c8]{},exp[0x7c]{},row[0x1c]{};
    TalkPropagationBindings b{};ResponseHistoryBindings history{};
    unsigned count_calls=0,candidates=0,refreshes=0,visibility=0,accesses=0,clocks=0,inserts=0;
    int limit=6,index=-1;
    bool fail_visibility=false;
    static TalkPropagationFixture& get(void* p){return *static_cast<TalkPropagationFixture*>(p);}
    static void put(void* p,unsigned off,unsigned v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    static void fl(void* p,unsigned off,float v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    static unsigned word(void* p,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;}
    float timestamp(){float f=0;std::memcpy(&f,row+0x14,4);return f;}
    TalkPropagationFixture(){
        b.context=this;history.context=this;
        put(npc,0xb8,0x800);put(neighbors[0],0xb8,0x800);
        fl(neighbors[0],0x2bc,999);fl(neighbors[1],0x2bc,1000);
        fl(neighbors[2],0x2bc,100);put(neighbors[3],0x2bc,0x7fc00000);
        b.list=[](void* p,void** out) noexcept -> int{*out=p;return 1;};
        b.count=[](void* p,int* out) noexcept -> int{auto& f=get(p);++f.count_calls;*out=f.limit;return 1;};
        b.candidate=[](void* p,void* token,unsigned i,void** n,unsigned* extent) noexcept -> int{
            auto& f=get(p);assert(token==p&&i<6);++f.candidates;
            *n=i==0?nullptr:i==1?f.npc:f.neighbors[i-2];*extent=sizeof(f.npc);return 1;};
        b.refresh=[](void* p,void* n) noexcept -> int{auto& f=get(p);assert(n==f.npc||n==f.neighbors[0]);++f.refreshes;put(n,0xb8,0);return 1;};
        b.visible=[](void* p,void* src,void* dst,unsigned mask,void* blocker,unsigned* out) noexcept -> int{
            auto& f=get(p);assert(src==f.npc&&(dst==f.neighbors[0]||dst==f.neighbors[2])&&mask==0x4081&&!blocker);
            ++f.visibility;*out=dst==f.neighbors[0];return !f.fail_visibility;};
        b.expeditor=[](void* p,void* n,void** out,unsigned* bytes,const ResponseHistoryBindings** h) noexcept -> int{
            auto& f=get(p);assert(n==f.neighbors[0]);++f.accesses;*out=f.exp;*bytes=sizeof(f.exp);*h=&f.history;return 1;};
        history.lookup=[](void* p,void* e,const char* n,int* out) noexcept -> int{auto& f=get(p);assert(e==f.exp&&!std::strcmp(n,"TLK_HELLO"));*out=f.index;return 1;};
        history.insert=[](void* p,void* e,const char* n,float initial,int* out) noexcept -> int{
            auto& f=get(p);assert(e==f.exp&&!std::strcmp(n,"TLK_HELLO")&&initial==10);++f.inserts;*out=f.index=0;return 1;};
        history.entry=[](void* p,void* e,int i,void** out,unsigned* bytes) noexcept -> int{
            auto& f=get(p);assert(e==f.exp&&i==0);*out=f.row;*bytes=sizeof(f.row);return 1;};
        history.clock=[](void* p,float* out) noexcept -> int{auto& f=get(p);*out=10.f+f.clocks++;return 1;};
        // Deliberately absent: silent seed must not require or invoke notify.
    }
};
inline void check_talk_propagation(const TalkPropagationApi& api){
    TalkPropagationFixture f;
    auto run=[&](){return api.propagate(f.npc,sizeof(f.npc),&f.b,"TLK_HELLO");};
    assert(run()==1&&f.count_calls==7&&f.candidates==6&&f.refreshes==2&&f.visibility==2&&f.accesses==1);
    assert(f.inserts==1&&f.clocks==2&&f.timestamp()==11);
    f.put(f.row,0x18,0x12345678);
    assert(run()==1&&f.timestamp()==12&&f.inserts==1&&f.word(f.row,0x18)==0x12345678&&f.refreshes==2);
    assert(api.seed_silent(f.exp,sizeof(f.exp),&f.history,"TLK_HELLO")==1&&f.timestamp()==13&&f.word(f.row,0x18)==0x12345678);
    f.limit=2;assert(run()==1&&f.accesses==2);
    f.limit=6;f.fail_visibility=true;assert(run()==-1&&f.timestamp()==13&&f.accesses==2);
    assert(api.propagate(f.npc,0x2c7,&f.b,"TLK_HELLO")==0);
}


struct CountDialogueFixture {
    alignas(4) unsigned char npc[0x103c]{},obs[0x5ec]{};
    HologramControl ctrl{npc,sizeof(npc)};
    ResponseSpeakFixture pipeline;
    CountDialogueBindings b{};
    unsigned accessors=0,criteria_calls=0;
    bool malformed=false,absent=false;
    static CountDialogueFixture& get(void* p){return *static_cast<CountDialogueFixture*>(p);}
    unsigned phase(){unsigned n=0;std::memcpy(&n,npc+0x1014,4);return n;}
    void part(const char* p){std::strcpy(reinterpret_cast<char*>(obs+0x2d8),p);}
    CountDialogueFixture(const ResponseTimingUpdateApi& timing,const ResponseRecordApi& record):pipeline(timing,record){
        const std::uint32_t handle=0x1004;std::memcpy(obs+0x14,&handle,4);
        part("head");pipeline.expected_modifiers="part:fixture";
        b.context=this;b.speak=pipeline.b;
        b.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=get(p);assert(h==0x1004&&f.ctrl.busy);*out=f.npc;return 1;};
        b.clock=[](void* p,float* out) noexcept -> int{*out=get(p).pipeline.now;return 1;};
        b.expeditor=[](void* p,void* n,void** out,unsigned* extent) noexcept -> int{
            auto& f=get(p);assert(n==f.npc&&f.ctrl.busy);++f.accessors;*out=f.absent?nullptr:f.pipeline.exp;*extent=sizeof(f.pipeline.exp);return 1;};
        b.criteria=[](void* p,const void* rec,float now,CountCriteria* out) noexcept -> int{
            auto& f=get(p);assert(rec==f.obs+0x18&&now==f.pipeline.now&&f.ctrl.busy);++f.criteria_calls;
            if(f.malformed)std::memset(out->bytes,'x',sizeof(out->bytes));
            else std::strcpy(reinterpret_cast<char*>(out->bytes),"part:fixture");return 1;};
    }
};
inline void check_npc_dialogue(const NpcDialogueApi& api,const ResponseTimingUpdateApi& timing,const ResponseRecordApi& record){
    // Count's original behavior uses direct response selection, not the
    // companion group/once/cooldown gate, and advances on normal no-match.
    CountDialogueFixture c(timing,record);
    auto count=[&](){return api.count(&c.ctrl,&c.b,c.obs,sizeof(c.obs));};
    c.pipeline.expected_concept="TLK_INTERROGATE_COCKY";
    c.absent=true;assert(count()==1&&c.accessors==1&&!c.phase());c.absent=false;
    assert(count()==1&&c.phase()==1&&c.pipeline.sounds==1&&c.pipeline.modifier_calls==1&&c.criteria_calls==1&&c.accessors==4);
    c.part("torso");assert(count()==1&&c.phase()==1&&c.criteria_calls==1);
    c.pipeline.now=13;c.pipeline.expected_concept="TLK_INTERROGATE_NERVOUS";c.pipeline.has_match=false;
    assert(count()==1&&c.phase()==2&&c.pipeline.sounds==1&&c.pipeline.modifier_calls==2);
    c.part("hand");c.pipeline.expected_concept="TLK_INTERROGATE_TALKING";
    assert(count()==1&&!c.phase()&&!c.npc[0x1018]&&c.pipeline.modifier_calls==3&&c.pipeline.sounds==1);
    c.part("arm");c.malformed=true;
    assert(count()==-1&&c.ctrl.blocked&&!c.phase()&&!std::strcmp(reinterpret_cast<char*>(c.npc+0x1018),"arm"));
    assert(count()==0&&c.pipeline.modifier_calls==3);
    assert(ResponseSpeakFixture::destroy(&c.pipeline,c.pipeline.pool[0]));
    // Jessica uses the same native response leaves, but retains her full
    // companion gate and original normal-urgency request.
    CompanionTalkFixture j(timing,record);HologramControl ctrl{j.npc,sizeof(j.npc)};
    alignas(4) unsigned char obs[0x5ec]{};
    JessicaAttentionState state{};JessicaDialogueBindings jb{};
    struct Context {CompanionTalkFixture* f;HologramControl* ctrl;unsigned char* obs;unsigned calls=0;bool malformed=false;};
    Context context{&j,&ctrl,obs};
    jb.context=&context;jb.talk=j.b;jb.cooldown=&j.state;jb.semaphore=&j.pipeline.sem;
    jb.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<Context*>(p)->f->pipeline.now;return 1;};
    jb.criteria=[](void* p,const void* rec,float now,CountCriteria* out) noexcept -> int{
        auto& x=*static_cast<Context*>(p);assert(x.ctrl->busy&&(rec==x.obs+0x18||rec==x.obs+0x308)&&now==x.f->pipeline.now);++x.calls;
        if(x.malformed)std::memset(out->bytes,'x',sizeof(out->bytes));
        else std::strcpy(reinterpret_cast<char*>(out->bytes),"part:fixture");return 1;};
    j.pipeline.expected_concept="TLK_PLLOOKING";j.pipeline.expected_modifiers="part:fixture";
    obs[0x300]=1;obs[0x18+0x108]=1;
    auto look=[&](){return api.jessica(&ctrl,&state,&jb,obs,sizeof(obs));};
    assert(look()==1&&state.looking==10&&j.pipeline.sounds==1&&j.pipeline.modifier_calls==1&&context.calls==1&&j.suppress_argument==1);
    j.pipeline.now=14;j.state.global[0]=20;
    assert(look()==1&&state.looking==14&&j.pipeline.sounds==1&&context.calls==2); // gated, timer still committed
    j.pipeline.now=18;j.state.global[0]=0;obs[0x300]=0;obs[0x5e8]=1;obs[0x308+0x260]=1;
    j.pipeline.expected_concept="TLK_PLUSING";j.pipeline.has_match=false;
    assert(look()==1&&state.looking==0&&state.using_part==18&&j.pipeline.modifier_calls==2&&j.pipeline.sounds==1&&context.calls==3);
    j.pipeline.now=22;context.malformed=true;
    assert(look()==-1&&ctrl.blocked&&state.using_part==22&&context.calls==4);
    assert(look()==0&&j.pipeline.modifier_calls==2);
    assert(ResponseSpeakFixture::destroy(&j.pipeline,j.pipeline.pool[0]));
}


struct TalkManagerFixture {
    const TalkManagerApi& api;const TalkCatalogApi& catalog;
    alignas(4) unsigned char object[talk_manager_bytes]{};
    TalkManagerControl control{object,sizeof(object)};
    TalkManagerSession session{};TalkManagerBindings b{};
    TalkManagerTables tables{0x11110000,0x22220000,0x33330000};
    unsigned creations=0,bases=0,registrations=0,inserts=0,frees=0,destroys=0;
    bool absent=false,fail_register=false,fail_free=false;
    static TalkManagerFixture& get(void* p){return *static_cast<TalkManagerFixture*>(p);}
    unsigned word(unsigned off){unsigned x=0;std::memcpy(&x,object+off,4);return x;}
    void put(unsigned off,unsigned x){std::memcpy(object+off,&x,4);}
    TalkManagerFixture(const TalkManagerApi& a,const TalkCatalogApi& c):api(a),catalog(c){
        std::memset(object,0xa5,sizeof(object));b.context=this;
        b.base_construct=[](void* p,void* e,int arg) noexcept -> int{
            auto& f=get(p);assert(e==f.object&&arg==1&&!f.session.instance&&f.control.stage==TalkManagerStage::Constructing);
            ++f.bases;f.put(0x100,0x87654321);return 1;
        };
        b.create=[](void* p,TalkManagerSession* s,const char* name,int edict) noexcept -> int{
            auto& f=get(p);assert(s==&f.session&&s->creating&&!s->instance&&!std::strcmp(name,"ai_ally_speech_manager")&&edict==-1);
            ++f.creations;void* nested=&f;assert(f.api.ensure(s,&f.b,&nested)==0&&nested==&f);
            return f.absent||f.api.construct(s,&f.control,&f.tables,&f.b)==1;
        };
        b.register_created=[](void* p,void* e) noexcept -> int{
            auto& f=get(p);assert(e==f.object&&f.session.instance==&f.control&&f.control.stage==TalkManagerStage::Live);
            ++f.registrations;if(f.fail_register)return 0;
            return f.api.populate(&f.session,&f.control,&f.b)==1;
        };
        b.insert_deadline=[](void* p,void* tree,const char* n,float value) noexcept -> int{
            auto& f=get(p);assert(tree==f.object+0x3a0&&value==-1.f&&f.control.stage==TalkManagerStage::Populating);
            assert(f.inserts<48&&!std::strcmp(n,f.catalog.at(f.inserts)->name));++f.inserts;
            // Actual tree storage and symbol intern are host-owned.
            return 1;
        };
        b.free_tree=[](void* p,std::uint32_t v) noexcept -> int{
            auto& f=get(p);assert(v==0x12340000&&!f.session.instance&&f.control.stage==TalkManagerStage::Destroying);
            ++f.frees;return !f.fail_free;
        };
        b.base_destroy=[](void* p,void* e) noexcept -> int{
            auto& f=get(p);assert(e==f.object&&!f.session.instance&&f.control.stage==TalkManagerStage::Destroying);++f.destroys;return 1;
        };
    }
};
inline void check_talk_manager(const TalkManagerApi& api,const TalkCatalogApi& catalog){
    TalkManagerFixture f(api,catalog);void* out=&f;
    f.absent=true;assert(api.ensure(&f.session,&f.b,&out)==1&&!out&&!f.bases&&f.creations==1);
    f.absent=false;assert(api.ensure(&f.session,&f.b,&out)==1&&out==f.object&&f.creations==2&&f.bases==1&&f.registrations==1&&f.inserts==48);
    assert(f.word(0)==f.tables.entity&&f.word(0x100)==0x87654321&&f.word(0x390)==0xa5a5a5a5);
    assert(f.word(0x394)==0xbf800000&&f.word(0x398)==0xbf800000&&f.word(0x39c)==0xbf800000);
    assert(f.word(0x3a0)==f.tables.tree_thunk&&!f.word(0x3a4)&&!f.word(0x3a8)&&!f.word(0x3ac)&&f.word(0x3b0)==0x00ff00ff&&!f.word(0x3b4)&&f.word(0x3b8)==f.tables.compare);
    TalkCooldownState deadlines{};assert(api.read_cooldowns(&f.session,&deadlines)==1&&deadlines.global[0]==-1&&deadlines.global[2]==-1);
    deadlines.global[0]=12;deadlines.global[1]=34;assert(api.write_cooldowns(&f.session,&deadlines)==1);
    deadlines={};assert(api.read_cooldowns(&f.session,&deadlines)==1&&deadlines.global[0]==12&&deadlines.global[1]==34&&deadlines.global[2]==-1);
    assert(api.ensure(&f.session,&f.b,&out)==1&&out==f.object&&f.creations==2&&f.registrations==1);
    f.put(0x3a4,0x12340000);f.put(0x3a8,48);
    assert(api.destroy(&f.session,&f.control,&f.tables,&f.b)==1&&!f.session.instance&&f.control.stage==TalkManagerStage::Retired&&f.frees==1&&f.destroys==1&&!f.word(0x3a4)&&!f.word(0x3a8));
    assert(api.read_cooldowns(&f.session,&deadlines)==0&&api.destroy(&f.session,&f.control,&f.tables,&f.b)==0);
    TalkManagerFixture external(api,catalog);assert(api.ensure(&external.session,&external.b,&out)==1);
    external.put(0x3ac,0xffffffffu);external.put(0x3a4,0x12340000);external.put(0x3a8,48);
    assert(api.destroy(&external.session,&external.control,&external.tables,&external.b)==1&&!external.frees&&external.destroys==1&&external.word(0x3a4)==0x12340000);
    TalkManagerFixture failed(api,catalog);failed.fail_register=true;out=&failed;
    assert(api.ensure(&failed.session,&failed.b,&out)==-1&&out==&failed&&failed.session.blocked&&failed.session.instance==&failed.control&&!failed.session.creating);
    assert(api.ensure(&failed.session,&failed.b,&out)==0);
    TalkManagerFixture broken(api,catalog);assert(api.ensure(&broken.session,&broken.b,&out)==1);
    broken.put(0x3a4,0x12340000);broken.fail_free=true;
    assert(api.destroy(&broken.session,&broken.control,&broken.tables,&broken.b)==-1&&!broken.session.instance&&broken.session.blocked&&broken.control.stage==TalkManagerStage::Failed&&!broken.word(0x3a4)&&!broken.destroys);
}


struct ManagedTalkFixture {
    TalkManagerFixture manager;
    TalkCatalogFixture policy;
    alignas(4) unsigned char exp[0x7c]{};
    TalkManagerRuntimeBindings b{};
    explicit ManagedTalkFixture(const TalkManagerApi& m,const TalkCatalogApi& c):manager(m,c){
        const auto owner=ResponseSpeakFixture::ptr(policy.npc);
        const auto ptr=ResponseSpeakFixture::ptr(exp);
        std::memcpy(exp+4,&owner,4);
        const auto adjusted=owner+0xdfc;
        std::memcpy(exp+0xc,&adjusted,4);std::memcpy(policy.npc+0xffc,&ptr,4);
        b.session=&manager.session;b.manager=manager.b;b.cooldown=policy.b;
    }
};
inline void check_managed_talk(const TalkManagerRuntimeApi& api,const TalkManagerApi& manager,const TalkCatalogApi& catalog){
    ManagedTalkFixture f(manager,catalog);auto& p=f.policy;auto& m=f.manager;
    auto notify=[&](){return api.notify(&f.b,p.npc,sizeof(p.npc),f.exp,sizeof(f.exp),p.concept_name);};
    auto deadline=[&](){float d=0;std::memcpy(&d,m.object+0x394,4);return d;};
    unsigned out=99;f.exp[0xc]^=1;
    assert(notify()==0&&!m.creations);f.exp[0xc]^=1;
    p.lo=5;p.hi=10;p.draw=7;
    assert(notify()==1&&m.creations==1&&m.inserts==48&&deadline()==17);p.expect({4,5,2,1});
    assert(api.group_ready(&f.b,p.npc,sizeof(p.npc),0,&out)==1&&!out);p.expect({1});
    p.now=17;assert(api.group_ready(&f.b,p.npc,sizeof(p.npc),0,&out)==1&&out);p.expect({1,1});
    p.concept_name="TLK_IDLE";p.lo=10;p.hi=20;p.draw=15;
    assert(notify()==1&&deadline()==32&&m.creations==1);p.expect({5,2,1});
    p.fail=1;assert(api.reset_interaction(&f.b)==-1&&deadline()==32);p.expect({2,1});p.fail=0;
    p.now=20;assert(api.reset_interaction(&f.b)==1&&deadline()==35);p.expect({2,1});
    p.concept_name="TLK_PLPUSH";p.lo=p.hi=p.draw=15;
    assert(notify()==1&&p.deadline==35&&deadline()==35);p.expect({5,3,2,1});
    // A detached legacy state has no effect on the managed live deadline.
    p.state.global[0]=0;assert(api.group_ready(&f.b,p.npc,sizeof(p.npc),0,&out)==1&&!out);p.expect({1});
    assert(manager.destroy(&m.session,&m.control,&m.tables,&m.b)==1);
    ManagedTalkFixture missing(manager,catalog);missing.manager.absent=true;
    assert(api.reset_interaction(&missing.b)==-1&&!missing.manager.session.instance);
}


struct ManagedCompanionFixture {
    TalkManagerFixture manager;
    CompanionTalkFixture companion;
    const TalkManagerRuntimeApi& runtime;
    TalkManagerRuntimeBindings b{};
    unsigned updates=0,randoms=0;float draw=15;
    ManagedCompanionFixture(const TalkManagerApi& m,const TalkCatalogApi& c,const TalkManagerRuntimeApi& rt,
                            const ResponseTimingUpdateApi& timing,const ResponseRecordApi& record)
        :manager(m,c),companion(timing,record),runtime(rt){
        auto& f=companion;auto& p=f.pipeline;
        auto owner=ResponseSpeakFixture::ptr(f.npc),adjusted=owner+0xdfc,exp=ResponseSpeakFixture::ptr(p.exp);
        std::memcpy(p.exp+4,&owner,4);std::memcpy(p.exp+0xc,&adjusted,4);f.put(0xffc,exp);
        b.session=&manager.session;b.manager=manager.b;b.cooldown.context=this;
        b.cooldown.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<ManagedCompanionFixture*>(p)->companion.pipeline.now;return 1;};
        b.cooldown.random=[](void* p,float lo,float hi,float* out) noexcept -> int{
            auto& f=*static_cast<ManagedCompanionFixture*>(p);assert(lo==10&&hi==20);++f.randoms;*out=f.draw;return 1;};
        b.cooldown.concept_deadline=[](void*,const char*,float** out) noexcept -> int{*out=nullptr;return 1;};
        b.cooldown.propagate=[](void*,void*,const char*) noexcept -> int{assert(false);return 0;};
        b.cooldown.cooldown_disabled=[](void*,unsigned* out) noexcept -> int{*out=0;return 1;};
        // The managed API must not consult detached manager or cooldown fields.
        f.b.manager=nullptr;f.b.cooldown={};f.state.global[0]=1000;
        p.notify_context=this;p.after_notify=[](void* ctx,void* e,const char* n) noexcept -> int{
            auto& f=*static_cast<ManagedCompanionFixture*>(ctx);++f.updates;
            return f.runtime.notify(&f.b,f.companion.npc,sizeof(f.companion.npc),e,sizeof(f.companion.pipeline.exp),n)==1;
        };
    }
};
inline void check_managed_companion(const ManagedCompanionApi& api,const TalkManagerApi& manager,const TalkCatalogApi& catalog,
                                   const TalkManagerRuntimeApi& runtime,const ResponseTimingUpdateApi& timing,const ResponseRecordApi& record){
    ManagedCompanionFixture f(manager,catalog,runtime,timing,record);auto& c=f.companion;auto& p=c.pipeline;unsigned out=99;
    auto request=[&](unsigned urgent=0){return api.request(c.npc,sizeof(c.npc),&f.b,&p.sem,&c.b,"TLK_IDLE",nullptr,urgent,&out);};
    auto deadline=[&](){float v=0;std::memcpy(&v,f.manager.object+0x394,4);return v;};
    assert(request()==1&&out&&p.sounds==1&&f.updates==1&&deadline()==25&&f.manager.creations==1);
    p.now=13;assert(request()==1&&!out&&p.sounds==1&&deadline()==25);
    assert(api.group_gate(c.npc,sizeof(c.npc),&f.b,&p.sem,&c.b,0,1,&out)==1&&out);
    p.now=25;assert(request()==1&&out&&p.sounds==2&&p.destroys==1&&f.updates==2&&deadline()==40);
    p.now=28;p.has_match=false;
    assert(request(1)==1&&!out&&p.sounds==2&&f.updates==2&&deadline()==40);
    p.has_match=true;assert(request(1)==1&&out&&p.sounds==3&&f.updates==3&&deadline()==43);
    assert(f.randoms==3&&f.manager.creations==1&&c.state.global[0]==1000);
    assert(ResponseSpeakFixture::destroy(&p,p.pool[0]));
    assert(manager.destroy(&f.manager.session,&f.manager.control,&f.manager.tables,&f.manager.b)==1);
}

}
