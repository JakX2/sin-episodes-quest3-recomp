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
    bool no_system=false,no_match=false,empty=false,flag=true,mutate=false;
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
            f.response[4]=f.empty?0:'x';f.response[5]=0;f.put(0x88,f.flag?4:0);f.put(0x8c,f.chance);*out=!f.no_match;return f.fail!=8;
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
    f.no_match=true;assert(run()==1&&!out&&!n&&!f.destroys);f.expect({1,2,3,5,6,7,8,11});f.no_match=false;
    f.empty=true;assert(run()==1&&!out&&!n&&f.destroys==1);f.expect({1,2,3,5,6,7,8,10,11});f.empty=false;
    assert(run("fixture:yes")==1&&!out&&!n);f.expect({1,2,3,4,5,6,7,8,9,10,11});
    f.draw=51;assert(run()==1&&out==f.response&&n==0x124);f.expect({1,2,3,5,6,7,8,9,11});
    f.mutate=true;assert(run()==1&&!out);f.expect({1,2,3,5,6,7,8,9,10,11});f.mutate=false;
    f.chance=100;assert(run()==1&&out==f.response);f.expect({1,2,3,5,6,7,8,11});
    f.flag=false;f.chance=-1;assert(run()==1&&out==f.response);f.expect({1,2,3,5,6,7,8,11});
    f.fail=3;assert(run()==-1&&out==&f&&n==99);f.expect({1,2,3,11});
    f.fail=8;assert(run()==-1&&out==&f&&n==99);f.expect({1,2,3,5,6,7,8,10,11});
    f.fail=11;assert(run()==-1&&out==&f&&n==99);f.expect({1,2,3,5,6,7,8,11,10});
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

}
