#pragma once
#include "native_vehicle_lifecycle.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct VehicleLifeFixture {
    alignas(4) unsigned char vehicle[vehicle_object_bytes]{},radio[music_radio_bytes]{},views[600]{},sounds[256]{};
    HologramControl v{vehicle,sizeof(vehicle)},r{radio,sizeof(radio)};
    unsigned events[64]{},n=0;bool missing_radio=false,fail_sound=false,missing_config=false;
    void push(unsigned e){assert(v.busy&&n<64);events[n++]=e;}
    void expect(std::initializer_list<unsigned> e){assert(n==e.size()&&!std::memcmp(events,e.begin(),n*4));n=0;}
    unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
    void put(void* p,unsigned off,unsigned w){std::memcpy(static_cast<unsigned char*>(p)+off,&w,4);}
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    float number(unsigned off){float x=0;std::memcpy(&x,vehicle+off,4);return x;}
};
inline void check_vehicle_lifecycle(const VehicleLifecycleApi& api,const MusicRadioApi& music){
    VehicleLifeFixture f;VehicleLifecycleBindings b{};b.context=&f;b.config.context=&f;b.music.playback.context=&f;
    b.base_spawn=[](void* p,void* v) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(v==f.vehicle&&f.word(v,0x5dc)==1&&f.vehicle[0x39c]==1);f.push(1);return 1;};
    b.base_activate=[](void* p,void*) noexcept -> int{static_cast<VehicleLifeFixture*>(p)->push(2);return 1;};
    b.initialize_subsystem=[](void* p,void*,unsigned off) noexcept -> int{assert(off==0x84c);static_cast<VehicleLifeFixture*>(p)->push(3);return 1;};
    b.open_config=[](void* p,void*,VehicleConfigKind kind,unsigned* present) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);f.push(10+static_cast<unsigned>(kind));*present=!f.missing_config;return 1;};
    b.close_config=[](void* p,void*,VehicleConfigKind kind) noexcept -> int{static_cast<VehicleLifeFixture*>(p)->push(20+static_cast<unsigned>(kind));return 1;};
    b.config.entry=[](void*,const char* block,unsigned i,unsigned* present,VehicleConfigEntry* e) noexcept -> int{
        *present=i==0&&(!std::strcmp(block,"damage_mod")||!std::strcmp(block,"vehicle_sounds"));
        if(*present)*e=!std::strcmp(block,"damage_mod")?VehicleConfigEntry{"blast",nullptr,0,0.5f}:VehicleConfigEntry{"gears:grind:0","fixture.gear",0,0};
        return 1;
    };
    b.config.append_view=[](void* p,void* v) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);auto n=f.word(v,0x808);assert(n<6);f.put(v,0x7fc,f.ptr(f.views));f.put(v,0x808,n+1);return 1;};
    b.config.attachment_id=[](void*,void*,const char*,int* id) noexcept -> int{*id=1;return 1;};
    b.append_sound=[](void* p,void* v) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);auto n=f.word(v,0x844);assert(n<2);f.put(v,0x838,f.ptr(f.sounds));f.put(v,0x844,n+1);return 1;};
    b.precache=[](void* p,const char* name) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(!std::strcmp(name,"fixture.gear"));f.push(4);return !f.fail_sound;};
    b.find_radio=[](void* p,const char* name,void** out) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(!std::strcmp(name,"env_radio"));f.push(5);*out=f.missing_radio?nullptr:f.radio;return 1;};
    b.handle=[](void* p,void* object,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(object==f.vehicle||object==f.radio);f.push(6);*out=object==f.vehicle?0x1001:0x1002;return 1;};
    b.resolve_radio=[](void* p,std::uint32_t word,HologramControl** out) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(word==0x1002);f.push(7);*out=&f.r;return 1;};
    b.parent_radio=[](void* p,void* radio,void* vehicle,int attachment) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(radio==f.radio&&vehicle==f.vehicle&&attachment==-1);f.push(8);return 1;};
    b.initialize_components=[](void* p,void*) noexcept -> int{static_cast<VehicleLifeFixture*>(p)->push(9);return 1;};
    b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);f.push(30);*out=10;return 1;};
    b.health_dirty=[](void* p,void*) noexcept -> int{static_cast<VehicleLifeFixture*>(p)->push(31);return 1;};
    b.spawn_tail=[](void* p,void*,unsigned flags) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(flags==0x211&&f.word(f.vehicle,0x9c)==100&&f.vehicle[0x74d]);f.push(32);return 1;};
    b.activate_tail=[](void* p,void*,unsigned off) noexcept -> int{assert(off==0x870);static_cast<VehicleLifeFixture*>(p)->push(33);return 1;};
    auto& m=b.music;
    m.open_script=[](void* p,std::uint32_t,void** out) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(f.r.busy);f.push(41);*out=nullptr;return 1;};
    m.entry=[](void*,void*,unsigned,unsigned*,const char**,const char**) noexcept -> int{return 0;};
    m.close_script=[](void*,void*) noexcept -> int{return 0;};
    m.append_station=[](void*,void*) noexcept -> int{return 0;};
    m.append_track=[](void*,void*,int,const MusicRadioTrack*) noexcept -> int{return 0;};
    m.precache=[](void*,const char*) noexcept -> int{return 0;};
    m.base_spawn=[](void* p,void* radio) noexcept -> int{auto& f=*static_cast<VehicleLifeFixture*>(p);assert(f.r.busy&&radio==f.radio&&f.word(radio,0x394)==0x1001);f.push(40);return 1;};
    m.base_activate=[](void*,void*) noexcept -> int{return 0;};
    m.install_think=[](void* p,void*,const char* name) noexcept -> int{assert(!std::strcmp(name,"RadioThink"));static_cast<VehicleLifeFixture*>(p)->push(42);return 1;};
    m.playback.clock=[](void*,float*) noexcept -> int{return 0;};
    m.playback.schedule=[](void*,void*,float) noexcept -> int{return 0;};
    assert(api.defaults(&f.v)==1&&music.initialize(&f.r,0)==1);
    assert(f.vehicle[0x74a]&&f.vehicle[0x74c]&&f.vehicle[0x74d]&&f.vehicle[0x764]&&f.number(0x754)==20&&f.word(f.vehicle,0x768)==0xffffffffu);
    f.put(f.vehicle,0x120,0x200);f.vehicle[0x74d]=0;
    assert(api.spawn(&f.v,&b)==1&&f.number(0x73c)==10.1f&&f.number(0x864)==0.5f&&f.word(f.vehicle,0x808)==3&&f.word(f.vehicle,0x844)==1);
    f.expect({1,3,10,20,11,4,21,5,6,7,6,7,40,41,42,7,8,12,22,9,30,31,32});
    assert(!std::strcmp(reinterpret_cast<char*>(f.sounds),"gears:grind:0")&&!std::strcmp(reinterpret_cast<char*>(f.sounds+64),"fixture.gear"));
    f.put(f.vehicle,0x218,1);
    assert(api.activate(&f.v,&b)==1&&f.word(f.vehicle,0x808)==6&&f.word(f.vehicle,0x844)==2);
    f.expect({2,3,12,22,10,20,9,11,4,21,33});
    f.missing_config=true;f.put(f.vehicle,0x218,0);
    assert(api.activate(&f.v,&b)==1);f.expect({2,3,12,10,9,11});
    f.missing_radio=true;assert(api.spawn(&f.v,&b)==-1&&f.v.blocked&&f.word(f.vehicle,0x768)==0xffffffffu);
    f.expect({1,3,10,11,5});assert(api.acknowledge(&f.v)==1);
}
}
