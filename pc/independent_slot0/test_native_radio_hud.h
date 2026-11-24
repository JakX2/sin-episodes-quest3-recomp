#pragma once
#include "native_radio_hud.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct RadioHudFixture {
    alignas(4) unsigned char object[0xc0]{};
    HologramControl ctrl{object,sizeof(object)};
    char stored[2048]{};
    const char* sequence=nullptr;
    unsigned stages=0,active=0;
    bool fail=false;
};
inline void check_radio_wire_hud(const RadioWireApi& wire,const RadioHudApi& hud){
    RadioHudFixture f;RadioHudBindings b{};b.context=&f;
    b.store_speaker=[](void* p,const char* s,const char** out) noexcept -> int{
        auto& f=*static_cast<RadioHudFixture*>(p);assert(f.ctrl.busy&&f.stages==0);
        std::strcpy(f.stored,s);*out=f.stored;++f.stages;return 1;
    };
    b.panel_flag=[](void* p,void* panel,unsigned off,unsigned value) noexcept -> int{
        auto& f=*static_cast<RadioHudFixture*>(p);assert(f.ctrl.busy&&panel==f.object+0x14);
        assert(off==(f.stages==1?0x78u:0xfcu)&&f.stages>=1&&f.stages<=2);
        assert(value==f.object[0xb6]);f.active=value;++f.stages;return 1;
    };
    b.animation=[](void* p,const char* sequence) noexcept -> int{
        auto& f=*static_cast<RadioHudFixture*>(p);assert(f.ctrl.busy&&f.stages==3);
        f.sequence=sequence;f.stages=0;return !f.fail;
    };
    assert(hud.initialize(&f.ctrl)==1);
    unsigned char payload[2052]{};unsigned n=0;
    RadioIconRequest icon{};
    auto receive=[&](unsigned active,int priority,const char* s){
        assert(wire.encode(active,priority,s,payload,sizeof(payload),&n)==1);
        return hud.receive(&f.ctrl,&b,payload,n);
    };
    auto expect=[&](const char* sequence,const char* name){
        assert(!std::strcmp(f.sequence,sequence)&&hud.icon(&f.ctrl,&icon)==1);
        assert(icon.present&&icon.width==1600&&icon.height==1200&&!std::strcmp(icon.name,name));
    };
    assert(receive(1,0,"npc_jc")==1&&n==11&&payload[0]==1&&payload[1]==0&&payload[2]==0&&payload[3]==0);
    expect("PoliceRadioShowObjective","police_radio_obj");
    assert(receive(1,1,"npc_jessica")==1&&n==16&&payload[2]==1&&payload[3]==0);
    expect("PoliceRadioShowExtraneous","police_radio_npc_jessica");
    assert(receive(257,-1,"npc_jc")==1&&payload[0]==1&&payload[2]==255&&payload[3]==255);
    expect("PoliceRadioShowExtraneous","police_radio_npc_jc");
    assert(receive(0,0,nullptr)==1&&n==5&&payload[4]==0&&!f.active);
    assert(!std::strcmp(f.sequence,"PoliceRadioHide")&&hud.icon(&f.ctrl,&icon)==1&&!icon.present);
    assert(receive(1,1,"npc_jc")==1);
    assert(hud.receive(&f.ctrl,&b,payload,n-1)==0&&f.object[0xb6]==1&&!f.stages);
    assert(hud.reset(&f.ctrl)==1&&!f.object[0xb6]&&!f.object[0xb8]);
    const char* stored=nullptr;std::memcpy(&stored,f.object+0xbc,4);assert(stored==f.stored);
    f.fail=true;assert(receive(1,0,"npc_jc")==-1&&f.ctrl.blocked);
    assert(hud.acknowledge(&f.ctrl)==1&&!f.ctrl.blocked);f.fail=false;
    assert(receive(1,1,"npc_jc")==1);
    char long_name[128];std::memset(long_name,'x',127);long_name[127]=0;
    assert(receive(1,1,long_name)==1&&hud.icon(&f.ctrl,&icon)==1&&std::strlen(icon.name)==63);
    assert(wire.encode(1,0,"npc_jc",payload,10,&n)==0&&n==0);
}
}
