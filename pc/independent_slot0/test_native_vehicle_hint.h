#pragma once
#include "native_vehicle_hint.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct VehicleHintFixture {
    alignas(4) unsigned char vehicle[0x900]{},hud[0x178]{};
    HologramControl vehicle_control{vehicle,sizeof(vehicle)},hud_control{hud,sizeof(hud)};
    const HintHudApi* api=nullptr;
    HintHudBindings h{};
    int player[2]{};
    float now=10;
    unsigned sends=0,rebuilt=0,shown=0,hidden=0,visible=0,registrations=0,finds=0;
    bool hud_present=true,participant=true,fail=false;
    char token[128]{};
    float deadline(){float f=0;std::memcpy(&f,hud+0x154,4);return f;}
    void seat(int seat,unsigned h){std::memcpy(vehicle+0x6a4+seat*4,&h,4);}
};
inline void check_vehicle_hint(const VehicleHintApi& v,const HintHudApi& h){
    VehicleHintFixture f;f.api=&h;auto& b=f.h;b.context=&f;
    b.rebuild=[](void* p,void* hud,const char* token,unsigned* built) noexcept -> int{
        auto& f=*static_cast<VehicleHintFixture*>(p);assert(f.hud_control.busy&&hud==f.hud);++f.rebuilt;
        std::strcpy(f.token,token);std::strcpy(reinterpret_cast<char*>(f.hud+0xd4),token);
        *built=*token!=0&&std::strcmp(token,"missing")!=0;return 1;
    };
    b.visible=[](void* p,void* hud,unsigned visible) noexcept -> int{
        auto& f=*static_cast<VehicleHintFixture*>(p);assert(f.hud_control.busy&&hud==f.hud&&visible==1);++f.visible;return 1;
    };
    b.animation=[](void* p,const char* name) noexcept -> int{
        auto& f=*static_cast<VehicleHintFixture*>(p);assert(f.hud_control.busy);
        if(!std::strcmp(name,"HintMessageShow"))++f.shown;else{assert(!std::strcmp(name,"HintMessageHide"));++f.hidden;}return !f.fail;
    };
    b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<VehicleHintFixture*>(p);assert(f.hud_control.busy);*out=f.now;return 1;};
    b.register_message=[](void* p,const char* name,int bytes) noexcept -> int{
        auto& f=*static_cast<VehicleHintFixture*>(p);assert(!std::strcmp(name,"HintText")&&bytes==-1);++f.registrations;return 1;
    };
    b.find_hud=[](void* p,const char* name,HologramControl** out) noexcept -> int{
        auto& f=*static_cast<VehicleHintFixture*>(p);assert(!std::strcmp(name,"CHudHintDisplay"));++f.finds;*out=f.hud_present?&f.hud_control:nullptr;return 1;
    };
    VehicleHintBindings s{};s.context=&f;s.register_message=b.register_message;
    s.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{
        auto& f=*static_cast<VehicleHintFixture*>(p);*out=h==0x1001?&f.player[0]:h==0x1002?&f.player[1]:nullptr;return 1;
    };
    s.participant=[](void* p,void*,unsigned* out) noexcept -> int{*out=static_cast<VehicleHintFixture*>(p)->participant;return 1;};
    s.send=[](void* p,void* player,unsigned reliable,const char* name,const unsigned char* data,unsigned bytes) noexcept -> int{
        auto& f=*static_cast<VehicleHintFixture*>(p);assert((player==&f.player[0]||player==&f.player[1])&&reliable==1&&!std::strcmp(name,"HintText")&&data[0]==1);
        assert(bytes==std::strlen(reinterpret_cast<const char*>(data+1))+2);++f.sends;return f.api->route(&f.h,data,bytes)==1;
    };
    assert(v.register_message(&s)==1&&h.register_message(&b)==1&&f.registrations==2);
    f.seat(0,0x1001);f.seat(1,0x1002);f.seat(2,0xffffffffu);
    auto notify=[&](int seat,unsigned entering){return v.notify(f.vehicle,sizeof(f.vehicle),&s,seat,entering);};
    assert(notify(0,1)==1&&!f.sends);
    assert(v.enable(&f.vehicle_control,2,1)==1&&!f.vehicle[0x764]);
    assert(v.enable(&f.vehicle_control,5,1)==1&&f.vehicle[0x764]);
    assert(notify(0,1)==1&&f.sends==1&&f.shown==1&&!std::strcmp(f.token,"#SE1_Hint_Driver")&&f.deadline()==10);
    assert(h.tick(&f.hud_control,&b)==1&&!f.hidden);
    f.now=10.01f;assert(h.tick(&f.hud_control,&b)==1&&f.hidden==1&&f.deadline()==0);
    assert(notify(1,1)==1&&f.shown==2&&!std::strcmp(f.token,"#SE1_Hint_Vehicle"));
    assert(notify(1,0)==1&&f.hidden==2&&!*f.token&&f.deadline()==10.01f);
    assert(notify(2,1)==1&&f.sends==3);
    f.participant=false;assert(notify(0,1)==1&&f.sends==3);f.participant=true;
    f.hud_present=false;assert(notify(0,1)==1&&f.sends==4&&f.rebuilt==3);f.hud_present=true;
    unsigned char data[256]{};unsigned bytes=0;float duration=3.5f;
    assert(v.encode("fixture",&duration,data,sizeof(data),&bytes)==1&&bytes==13&&data[0]==1);
    assert(h.route(&b,data,bytes)==1&&f.shown==3&&f.deadline()==13.51f);
    data[0]=2;assert(h.route(&b,data,bytes)==1&&f.rebuilt==4);data[0]=1;
    assert(h.route(&b,data,7)==0&&f.rebuilt==4);
    f.now=f.deadline();assert(h.tick(&f.hud_control,&b)==1&&f.hidden==2);
    f.now+=0.01f;assert(h.tick(&f.hud_control,&b)==1&&f.hidden==3&&f.deadline()==0);
    assert(v.encode("missing",nullptr,data,sizeof(data),&bytes)==1&&h.route(&b,data,bytes)==1&&f.hidden==4);
    f.fail=true;assert(notify(0,1)==-1&&f.hud_control.blocked);assert(h.acknowledge(&f.hud_control)==1);
}
}
