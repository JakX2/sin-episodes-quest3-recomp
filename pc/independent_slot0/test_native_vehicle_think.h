#pragma once
#include "native_vehicle_think.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct VehicleThinkFixture {
    alignas(4) unsigned char vehicle[vehicle_object_bytes]{},link[camera_link_bytes]{};
    HologramControl v{vehicle,sizeof(vehicle)},l{link,sizeof(link)};
    const VehicleThinkApi* api=nullptr;
    int camera=0;
    bool named=true,cast=true,create=true,live=false,active=true;
    unsigned log[64]{},n=0,diagnostics=0,created=0,removed=0;
    float now=2,frame=0.25f;
    void push(unsigned e){assert(v.busy&&n<64);log[n++]=e;}
    void expect(std::initializer_list<unsigned> x){assert(n==x.size()&&!std::memcmp(log,x.begin(),n*4));n=0;}
    void put(void* p,unsigned off,unsigned word){std::memcpy(static_cast<unsigned char*>(p)+off,&word,4);}
    unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
    float number(unsigned off){float x=0;std::memcpy(&x,vehicle+off,4);return x;}
};
inline void check_vehicle_think(const VehicleThinkApi& api){
    VehicleThinkFixture f;f.api=&api;
    VehicleThinkBindings b{};b.context=&f;b.camera.context=&f;b.passengers.context=&f;
    auto& c=b.camera;
    c.resolve=[](void* p,std::uint32_t word,void** out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(word==0x3001);f.push(1);*out=f.live?f.link:nullptr;return 1;};
    c.find_named=[](void* p,std::uint32_t name,void** out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(name==7);f.push(2);*out=f.named?&f.camera:nullptr;return 1;};
    c.is_point_camera=[](void* p,void* camera,unsigned* out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(camera==&f.camera);f.push(3);*out=f.cast;return 1;};
    c.create=[](void* p,const char* name,HologramControl** out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(!std::strcmp(name,"info_camera_link"));f.push(4);*out=nullptr;if(f.create){assert(f.api->link_defaults(&f.l)==1);++f.created;*out=&f.l;}return 1;};
    c.handle=[](void* p,void* object,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(f.l.busy);f.push(5);*out=object==&f.camera?0x1001:object==f.vehicle?0x2001:0x3001;return 1;};
    c.spawn=[](void* p,void* link) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(f.l.busy&&link==f.link&&f.word(link,0x394)==0x1001&&f.word(link,0x398)==0x2001);f.push(6);f.live=true;return 1;};
    c.remove_link=[](void* p,void* link) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(link==f.link&&f.live);f.push(7);++f.removed;f.live=false;return 1;};
    c.diagnostic=[](void* p,std::uint32_t name) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(name==7);f.push(8);++f.diagnostics;return 1;};
    auto refresh=[&](){return api.refresh_camera(&f.v,&c);};
    f.put(f.vehicle,0x760,0xffffffffu);f.vehicle[0x758]=1;
    assert(refresh()==1);f.expect({});
    f.put(f.vehicle,0x75c,7);assert(refresh()==1&&f.created==1&&f.word(f.vehicle,0x760)==0x3001);f.expect({2,3,4,5,5,6,5,1});
    assert(refresh()==1&&f.created==1);f.expect({1});
    f.put(f.vehicle,0x75c,0);f.vehicle[0x758]=0;assert(refresh()==1&&f.live);f.expect({});
    f.put(f.vehicle,0x75c,7);assert(refresh()==1&&!f.live&&f.removed==1&&f.word(f.vehicle,0x760)==0xffffffffu);f.expect({1,1,1,7});
    f.vehicle[0x758]=1;f.named=false;assert(refresh()==1&&!f.word(f.vehicle,0x75c)&&f.diagnostics==1);f.expect({2,8});
    f.put(f.vehicle,0x75c,7);f.named=true;f.cast=false;assert(refresh()==1&&!f.word(f.vehicle,0x75c));f.expect({2,3,8});
    f.put(f.vehicle,0x75c,7);f.cast=true;f.create=false;assert(refresh()==1&&!f.word(f.vehicle,0x75c));f.expect({2,3,4,8});
    f.create=true;f.put(f.vehicle,0x75c,7);f.put(f.vehicle,0x760,0x3001);
    assert(refresh()==1&&f.created==2&&f.live);f.expect({1,2,3,4,5,5,6,5,1});
    b.base_think=[](void* p,void*) noexcept -> int{static_cast<VehicleThinkFixture*>(p)->push(10);return 1;};
    b.wiper_update=[](void* p,void*) noexcept -> int{static_cast<VehicleThinkFixture*>(p)->push(11);return 1;};
    b.driver_update=[](void* p,void*) noexcept -> int{static_cast<VehicleThinkFixture*>(p)->push(12);return 1;};
    b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);f.push(13);*out=f.now;return 1;};
    b.dirty_edict=[](void* p,void*,std::uint32_t word) noexcept -> int{assert(word==0x4001);static_cast<VehicleThinkFixture*>(p)->push(14);return 1;};
    b.schedule=[](void* p,void*,float now,unsigned arg) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(now==f.now&&!arg);f.push(15);return 1;};
    b.startup=[](void* p,void*,unsigned stage) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(stage<2&&f.vehicle[0x740]);f.push(16+stage);return 1;};
    b.active_predicate=[](void* p,void*,unsigned* out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);f.push(18);*out=f.active;return 1;};
    b.frame_time=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);f.push(19);*out=f.frame;return 1;};
    b.tail=[](void* p,void*) noexcept -> int{static_cast<VehicleThinkFixture*>(p)->push(20);return 1;};
    auto& a=b.passengers;a.clock=b.clock;
    a.occupant=[](void* p,void*,int seat,void** out) noexcept -> int{auto& f=*static_cast<VehicleThinkFixture*>(p);assert(seat>=0&&seat<3);f.push(21+seat);*out=nullptr;return 1;};
    a.combat_character=[](void*,void*,void**) noexcept -> int{return 0;};
    a.base_player_cast=[](void*,void*,unsigned*) noexcept -> int{return 0;};
    a.view_index=[](void*,void*,int*) noexcept -> int{return 0;};
    a.ammo_index=[](void*,const char*,int*) noexcept -> int{return 0;};
    a.ammo_allowed=[](void*,void*,int,unsigned*) noexcept -> int{return 0;};
    a.ammo_count=[](void*,void*,int,int*) noexcept -> int{return 0;};
    a.ammo_max=[](void*,int,int*) noexcept -> int{return 0;};
    a.give_ammo=[](void*,void*,int,int,unsigned) noexcept -> int{return 0;};
    f.put(f.vehicle,0x16c,0x4001);f.vehicle[0x74c]=1;
    auto think=[&](){return api.think(&f.v,&b);};
    assert(think()==1&&f.number(0x30)==2&&f.vehicle[0x2d9]==1&&f.number(0x744)==0.25f&&f.number(0x88c)==2);
    f.expect({10,11,12,13,14,13,15,14,16,17,13,21,22,23,1,18,19,20});
    assert(think()==1&&f.number(0x744)==0.5f);f.expect({10,11,12,13,13,15,13,1,18,19,20});
    f.now=3.01f;f.active=false;f.vehicle[0x1f0]=1;f.vehicle[0x758]=0;
    assert(think()==1&&f.vehicle[0x200]&&f.number(0x744)==0&&f.number(0x88c)==3.01f&&!f.live);
    f.expect({10,11,12,13,13,15,13,21,22,23,1,1,1,7,18,20});
}
}
