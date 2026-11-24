#pragma once
#include "native_action_monitor.h"
#include "native_jessica_script.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct ActionFixture {
    alignas(4) unsigned char monitor[0x5ec]{},npc[0x1458]{};
    HologramControl ctrl{monitor,sizeof(monitor)},jessica{npc,sizeof(npc)};
    int owner=1,other=2;
    float now=10;
    unsigned clocks=0,resolves=0,samples=0,spoken=0;
    const ActionMonitorApi* api=nullptr;
    ActionCriteria last{};
    void put(unsigned off,unsigned v){std::memcpy(monitor+off,&v,4);}
    unsigned word(unsigned off)const{unsigned v=0;std::memcpy(&v,monitor+off,4);return v;}
    float stamp(unsigned off)const{float v=0;std::memcpy(&v,monitor+off,4);return v;}
};
inline void check_action_monitor(const ActionMonitorApi& api,const JessicaAttentionApi& attention){
    ActionFixture f;f.api=&api;f.put(4,0x1001);f.put(0x14,0xffffffffu);
    ActionMonitorBindings b{};b.context=&f;
    b.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{
        auto& f=*static_cast<ActionFixture*>(p);assert(f.ctrl.busy);++f.resolves;
        *out=h==0x1001?static_cast<void*>(&f.owner):h==0x1002?static_cast<void*>(f.npc):h==0x1003?static_cast<void*>(&f.other):nullptr;return 1;
    };
    b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<ActionFixture*>(p);assert(f.ctrl.busy);++f.clocks;*out=f.now;return 1;};
    b.packed_handle=[](void* p,void* e,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<ActionFixture*>(p);assert(f.ctrl.busy);*out=e==f.npc?0x1002:0x1003;return 1;};
    b.sample=[](void* p,void* e,ActionEntitySample* out) noexcept -> int{
        auto& f=*static_cast<ActionFixture*>(p);assert(f.ctrl.busy);++f.samples;
        if(e==&f.owner){std::strcpy(out->classname,"player");}
        else if(e==f.npc){std::strcpy(out->classname,"npc_jessica");std::strcpy(out->name,"jessica");}
        else std::strcpy(out->classname,"prop_dynamic");
        return 1;
    };
    ActionLookSample q{f.npc,7,"chest"};
    auto update=[&](){return api.update_look(&f.ctrl,&b,&q);};
    assert(update()==1&&f.word(0x14)==0x1002&&f.word(0x2d0)==1&&f.word(0x2f8)==7&&f.stamp(0x2d4)==10&&f.stamp(0x2fc)==10&&f.clocks==2);
    f.now=14.5f;assert(update()==1&&f.clocks==2);
    assert(api.look_criteria(f.monitor+0x18,0x2e8,f.now,&f.last)==1);
    assert(!std::strcmp(f.last.text,"PlayerLookingAt:jessica,PlayerLookingAtEnt:npc_jessica,PlayerLookingAtForSeconds:4.50,PlayerLookingAtPart:chest,PlayerLookingAtPartForSeconds:4.50"));
    // Producer -> recovered Jessica consumer -> original criteria text.
    JessicaAttentionState state{};JessicaAttentionBindings ab{};ab.context=&f;
    ab.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<ActionFixture*>(p);assert(f.jessica.busy);*out=f.now;return 1;};
    ab.speak_criteria=[](void* p,void* npc,const void* rec,float now,const char* concept_name) noexcept -> int{
        auto& f=*static_cast<ActionFixture*>(p);assert(npc==f.npc&&rec==f.monitor+0x18&&!std::strcmp(concept_name,"TLK_PLLOOKING"));
        ++f.spoken;return f.api->look_criteria(rec,0x2e8,now,&f.last);
    };
    f.monitor[0x300]=1;assert(attention.observe(&f.jessica,&state,&ab,f.monitor,sizeof(f.monitor))==1&&f.spoken==1);
    assert(std::strstr(f.last.text,"PlayerLookingAtPartForSeconds:4.50"));
    q.part="head";f.now=15;assert(update()==1&&f.clocks==3&&f.stamp(0x2d4)==10&&f.stamp(0x2fc)==15);
    assert(api.look_criteria(f.monitor+0x18,0x2e8,16,&f.last)==1&&std::strstr(f.last.text,"PlayerLookingAtForSeconds:6.00")&&std::strstr(f.last.text,"PlayerLookingAtPartForSeconds:1.00"));
    q.part=nullptr;f.now=17;assert(update()==1&&f.clocks==4&&!f.monitor[0x2d8]);
    assert(api.look_criteria(f.monitor+0x18,0x2e8,17,&f.last)==1&&!std::strstr(f.last.text,"PlayerLookingAtPart"));
    q.target=&f.other;f.now=18;assert(update()==1&&f.stamp(0x2d4)==18&&f.stamp(0x2fc)==18&&f.word(0x14)==0x1003);
    assert(api.look_criteria(f.monitor+0x18,0x2e8,19,&f.last)==1&&!std::strcmp(f.last.text,"PlayerLookingAtEnt:prop_dynamic,PlayerLookingAtForSeconds:1.00"));
    q.target=nullptr;assert(update()==1&&f.word(0x14)==0xffffffffu);
    alignas(4) unsigned char use[0x2e0]{};
    std::strcpy(reinterpret_cast<char*>(use+0x108),"player");std::strcpy(reinterpret_cast<char*>(use+0x160),"car");
    std::strcpy(reinterpret_cast<char*>(use+0x260),"prop_vehicle");std::strcpy(reinterpret_cast<char*>(use+0x2c0),"radio");
    unsigned action=4;std::memcpy(use+0x2b8,&action,4);
    assert(api.use_criteria(use,sizeof(use),&f.last)==1&&!std::strcmp(f.last.text,"PlayerUsing:car,PlayerUsingEnt:prop_vehicle,PlayerUsingPart:radio,PlayerUsingAction:USE_ACTION_LOCKED"));
    assert(api.use_criteria(use,sizeof(use)-1,&f.last)==0);
    action=13;std::memcpy(use+0x2b8,&action,4);assert(api.use_criteria(use,sizeof(use),&f.last)==0);
    use[0x108]=0;use[0x260]=0;assert(api.use_criteria(use,sizeof(use),&f.last)==1&&!f.last.text[0]);
}
}
