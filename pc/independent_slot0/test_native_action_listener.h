#pragma once
#include "native_action_listener.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct ActionListenerFixture {
    const char* name="look";int descriptor=1,owner=11,subject=12,token=1,a=2,s=3;
    bool a_ok=true,s_ok=false;unsigned subscriptions=0,prepares=0,unsubscribes=0,checks=0,records=0,routes=0;
    ActionRoute last=ActionRoute::Activate;
    unsigned char data[0x2e8]{};unsigned bytes=0;
    float now=9.5f;
};
inline void check_action_listener(const ActionListenerApi& api){
    ActionListenerFixture f;ActionListenerBindings b{};b.context=&f;
    b.prepare=[](void* p) noexcept -> int{++static_cast<ActionListenerFixture*>(p)->prepares;return 1;};
    b.subscribe=[](void* p,void* listener,const char* name,unsigned server) noexcept -> int{
        auto& f=*static_cast<ActionListenerFixture*>(p);
        const char* expected[]={"look","use","weapon","move","health","map","game","dynamic_difficulty","player_activate"};
        assert(listener==&f.token&&server==1&&f.prepares==1&&f.subscriptions<9&&!std::strcmp(name,expected[f.subscriptions++]));return 1;
    };
    b.unsubscribe=[](void* p,void* listener) noexcept -> int{auto& f=*static_cast<ActionListenerFixture*>(p);assert(listener==&f.token);++f.unsubscribes;return 1;};
    b.get_int=[](void* p,const void* e,const char* key,int fallback,int* out) noexcept -> int {
        auto& f=*static_cast<ActionListenerFixture*>(p);assert(e==&f.token&&!fallback);
        *out=!std::strcmp(key,"descriptor")?f.descriptor:!std::strcmp(key,"owner")?f.owner:f.subject;return 1;
    };
    b.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=*static_cast<ActionListenerFixture*>(p);*out=h==11?&f.a:h==12?&f.s:nullptr;return 1;};
    b.participant=[](void* p,void* e,unsigned* out) noexcept -> int{auto& f=*static_cast<ActionListenerFixture*>(p);++f.checks;*out=e==&f.a?f.a_ok:f.s_ok;return 1;};
    b.name=[](void* p,const void*,const char** out) noexcept -> int{*out=static_cast<ActionListenerFixture*>(p)->name;return 1;};
    b.get_string=[](void*,const void*,const char* key,const char* fallback,const char** out) noexcept -> int{assert(!std::strcmp(key,"subobject")&&!*fallback);*out="head";return 1;};
    b.get_float=[](void*,const void*,const char* key,float fallback,float* out) noexcept -> int{assert(!std::strcmp(key,"distance")&&fallback==0);*out=55;return 1;};
    b.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<ActionListenerFixture*>(p)->now;return 1;};
    b.sample=[](void* p,void* e,ActionEntitySample* out) noexcept -> int{auto& f=*static_cast<ActionListenerFixture*>(p);std::strcpy(out->classname,e==&f.a?"player":"npc_jessica");return 1;};
    b.record=[](void* p,ActionRoute route,const void* record,unsigned bytes) noexcept -> int{auto& f=*static_cast<ActionListenerFixture*>(p);++f.records;f.last=route;f.bytes=bytes;std::memcpy(f.data,record,bytes);return 1;};
    b.route=[](void* p,ActionRoute route,const void* event,int descriptor,void* a,void* s) noexcept -> int{auto& f=*static_cast<ActionListenerFixture*>(p);assert(event==&f.token&&descriptor==f.descriptor);++f.routes;f.last=route;if(route==ActionRoute::Weapon)assert(a==&f.a&&s==&f.s);return 1;};
    assert(api.subscribe(&f.token,&b)==1&&f.subscriptions==9);
    auto dispatch=[&](){return api.dispatch(&b,&f.token);};
    assert(dispatch()==1&&f.last==ActionRoute::Look&&f.records==1&&f.checks==1&&f.bytes==0x2e8);
    auto word=[&](unsigned off){unsigned w=0;std::memcpy(&w,f.data+off,4);return w;};
    auto number=[&](unsigned off){float w=0;std::memcpy(&w,f.data+off,4);return w;};
    assert(word(4)==0&&word(0x2b8)==1&&number(0x2bc)==9.5f&&number(0x2e0)==55);
    assert(!std::strcmp(reinterpret_cast<char*>(f.data+0x108),"player")&&!std::strcmp(reinterpret_cast<char*>(f.data+0x260),"npc_jessica")&&!std::strcmp(reinterpret_cast<char*>(f.data+0x2c0),"head"));
    f.a_ok=false;assert(dispatch()==1&&f.records==1&&f.checks==3);
    f.s_ok=true;f.name="use_extra";f.descriptor=4;assert(dispatch()==1&&f.last==ActionRoute::Use&&f.records==2&&f.bytes==0x2e0&&word(4)==1&&word(0x2b8)==4&&!f.data[0x2c0]);
    f.name="weapon";assert(dispatch()==1&&f.last==ActionRoute::Weapon&&f.routes==1);
    f.a_ok=f.s_ok=false;unsigned prior=f.checks;
    for(const char* n:{"map_start","game","dynamic_difficulty_changed","player_activate"}){f.name=n;assert(dispatch()==1);}
    assert(f.routes==5&&f.checks==prior&&f.last==ActionRoute::Activate);
    f.name="Look";assert(dispatch()==1&&f.records==2); // case sensitive
    assert(api.unsubscribe(&f.token,&b)==1&&f.unsubscribes==1);
}
}
