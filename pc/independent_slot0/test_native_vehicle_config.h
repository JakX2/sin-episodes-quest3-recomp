#pragma once
#include "native_vehicle_config.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct VehicleConfigFixture {
    alignas(4) unsigned char vehicle[0x900]{},views[1200]{};
    HologramControl ctrl{vehicle,sizeof(vehicle)};
    unsigned appended=0,attachments=0,calls=0;
    bool fail=false;
    unsigned word(void* p,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;}
    float real(void* p,unsigned off){float v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;}
    void put(void* p,unsigned off,unsigned v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    unsigned char* record(unsigned i){return views+i*100;}
};
inline void check_vehicle_config(const VehicleConfigApi& api){
    VehicleConfigFixture f;
    VehicleConfigBindings b{};b.context=&f;
    b.entry=[](void* p,const char* block,unsigned i,unsigned* present,VehicleConfigEntry* out) noexcept -> int{
        auto& f=*static_cast<VehicleConfigFixture*>(p);assert(f.ctrl.busy);++f.calls;
        static const VehicleConfigEntry links[]={{"PASSENGER1_VIEW","window",0,0},{"missing","ignored",0,0},{"window","passenger2_view",0,0},{"window","window",0,0}};
        static const VehicleConfigEntry temps[]={{"window","lean",0,0},{"lean","lean_more",0,0}};
        static const VehicleConfigEntry damage[]={{"Window",nullptr,0,0.25f},{"missing",nullptr,0,7.f},{"window",nullptr,0,-2.f}};
        static const VehicleConfigEntry weapons[]={{"lean",nullptr,-9,0},{"window",nullptr,1,0},{"window",nullptr,0,0}};
        static const VehicleConfigEntry keys[]={{"lean_more",nullptr,17,0},{"passenger2_view",nullptr,-4,0}};
        static const VehicleConfigEntry mods[]={{"BLAST",nullptr,0,0.75f},{"crush",nullptr,0,0.25f},{"blast",nullptr,0,9.f}};
        const VehicleConfigEntry* e=nullptr;unsigned n=0;
        if(!std::strcmp(block,"view_links")){e=links;n=4;}
        else if(!std::strcmp(block,"view_temp")){e=temps;n=2;}
        else if(!std::strcmp(block,"view_damage_mod")){e=damage;n=3;}
        else if(!std::strcmp(block,"view_allow_weapons")){e=weapons;n=3;}
        else if(!std::strcmp(block,"view_key")){e=keys;n=2;}
        else if(!std::strcmp(block,"damage_mod")){e=mods;n=3;}
        else assert(false);
        *present=i<n;if(*present)*out=e[i];return 1;
    };
    b.append_view=[](void* p,void* v) noexcept -> int{
        auto& f=*static_cast<VehicleConfigFixture*>(p);assert(f.ctrl.busy&&v==f.vehicle&&f.appended<12);
        auto i=f.word(v,0x808);assert(i==f.appended);std::memset(f.record(i),0xaa,100);
        f.put(v,0x7fc,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.views)));
        f.put(v,0x808,++f.appended);return 1;
    };
    b.attachment_id=[](void* p,void* v,const char* name,int* out) noexcept -> int{
        auto& f=*static_cast<VehicleConfigFixture*>(p);assert(f.ctrl.busy&&v==f.vehicle);
        *out=!std::strcmp(name,"lean_more")?-1:static_cast<int>(f.attachments);
        ++f.attachments;return !f.fail;
    };
    assert(api.views(&f.ctrl,&b)==1&&f.appended==8&&f.attachments==8);
    assert(!std::strcmp(reinterpret_cast<char*>(f.record(0)),"passenger0_view")&&f.word(f.record(0),0x40)==0xffffffffu&&f.word(f.record(0),0x44)==0);
    assert(f.word(f.record(3),0x40)==1&&f.word(f.record(3),0x44)==1&&!f.record(3)[0x54]&&!f.record(3)[0x55]);
    assert(f.real(f.record(3),0x60)==-2.f&&f.real(f.record(5),0x60)==1.f); // first matching name only
    assert(f.word(f.record(4),0x4c)==2&&f.word(f.record(4),0x44)==1&&f.word(f.record(2),0x50)==0xfffffffcu&&!f.word(f.record(4),0x50));
    assert(f.word(f.record(6),0x40)==3&&f.record(6)[0x54]==1&&f.record(6)[0x55]==1&&f.record(6)[0x56]==0xaa);
    assert(f.word(f.record(7),0x40)==6&&f.word(f.record(7),0x48)==0xffffffffu&&f.word(f.record(7),0x50)==17&&f.word(f.record(7),0x58)==0xffffffffu);
    f.put(f.vehicle,0x86c,0x40000000);
    assert(api.damage(&f.ctrl,&b)==1&&f.real(f.vehicle,0x864)==0.75f&&f.real(f.vehicle,0x868)==0.25f&&f.real(f.vehicle,0x86c)==2.f);
    f.fail=true;assert(api.views(&f.ctrl,&b)==-1&&f.ctrl.blocked&&f.appended==9);
    assert(api.acknowledge(&f.ctrl)==1);
}
}
