#pragma once
#include "native_vehicle_map.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct VehicleMapFixture {
    alignas(4) unsigned char vehicle[0x900]{},player[0x1230]{},radio[music_radio_bytes]{},views[100]{},doors[152]{},windows[152]{};
    HologramControl ctrl{vehicle,sizeof(vehicle)},player_ctrl{player,sizeof(player)},radio_ctrl{radio,sizeof(radio)};
    int camera=0;
    unsigned enters=0,exits=0,occupants=0,sounds=0,dirty=0,outputs=0,attachments=0,conversions=0,resolves=0,unlinks=0,radio_queries=0,schedules=0;
    bool camera_live=true,radio_live=true;
    int role=-1,last_index=-1;
    const char* group=nullptr;const char* action=nullptr;
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    void put(void* p,unsigned off,unsigned word){std::memcpy(static_cast<unsigned char*>(p)+off,&word,4);}
    unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
};
inline void check_vehicle_map(const VehicleMapApi& api,const MusicRadioApi& radio){
    VehicleMapFixture f;VehicleMapBindings b{};b.context=&f;
    const auto* schema=api.schema();
    assert(schema&&schema->field_count==40&&!std::strcmp(schema->classname,"prop_vehicle_sin"));
    assert(schema->fields[0].original_handler==0x2232dd00&&schema->fields[24].original_handler==0x2232f580&&schema->fields[39].offset==0x7e4);
    f.put(f.vehicle,0x810,f.ptr(f.doors));f.put(f.vehicle,0x81c,2);
    f.put(f.vehicle,0x824,f.ptr(f.windows));f.put(f.vehicle,0x830,2);
    f.put(f.vehicle,0x7fc,f.ptr(f.views));f.put(f.vehicle,0x808,1);
    f.views[0x55]=1;f.put(f.player,0x9b0,0xffffffffu);
    f.put(f.vehicle,0x16c,0x1001);f.put(f.vehicle,0x760,0x2001);f.put(f.vehicle,0x768,0x3001);
    b.enter=[](void* c,void* v,void* a,int role) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(f.ctrl.busy&&v==f.vehicle&&a==f.player);++f.enters;f.role=role;return 1;};
    b.occupant=[](void* c,void*,int role,void** out) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);++f.occupants;*out=role==0?f.player:nullptr;return 1;};
    b.exit=[](void* c,void*,void* occupant,unsigned argument) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(occupant==f.player&&argument==1);++f.exits;return 1;};
    b.sound=[](void* c,void*,const char* group,const char* action,int index) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(f.ctrl.busy);++f.sounds;f.group=group;f.action=action;f.last_index=index;return 1;};
    b.dirty_edict=[](void* c,void* v,std::uint32_t word) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(f.ctrl.busy&&v==f.vehicle&&word==0x1001);++f.dirty;return 1;};
    b.camera_value=[](void* c,unsigned type,std::uint32_t word,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);++f.conversions;*out=type==2?word:0;return 1;};
    b.resolve=[](void* c,std::uint32_t word,void** out) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);++f.resolves;*out=word==0x2001&&f.camera_live?static_cast<void*>(&f.camera):word==0x3001&&f.radio_live?f.radio:nullptr;return 1;};
    b.unlink_camera=[](void* c,void* camera) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(camera==&f.camera);++f.unlinks;return 1;};
    b.radio_control=[](void* c,void* r,HologramControl** out) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(r==f.radio);++f.radio_queries;*out=&f.radio_ctrl;return 1;};
    b.views.context=&f;
    b.views.cast_player=[](void* c,void* a,HologramControl** out) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(a==f.player);*out=&f.player_ctrl;return 1;};
    b.views.dirty_edict=[](void*,void*,std::uint32_t) noexcept -> int{return 1;};
    b.views.output=[](void* c,void*,void*,unsigned offset,int index) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(offset==0x7e4&&index==0&&f.player[0x9b4]);++f.outputs;return 1;};
    b.views.attachment=[](void* c,void*,int,float*,unsigned* found) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);++f.attachments;*found=0;return 1;};
    b.views.set_position=[](void*,void*,const float*) noexcept -> int{return 0;};
    b.radio.context=&f;
    b.radio.clock=[](void*,float* out) noexcept -> int{*out=10;return 1;};
    b.radio.schedule=[](void* c,void* r,float time) noexcept -> int{auto& f=*static_cast<VehicleMapFixture*>(c);assert(f.radio_ctrl.busy&&r==f.radio&&time==10);++f.schedules;return 1;};
    b.radio.resolve=b.resolve;
    b.radio.handle=[](void*,void*,std::uint32_t*) noexcept -> int{return 0;};
    b.radio.emit=[](void*,void*,void*,const char*,float,unsigned,unsigned,unsigned,float*) noexcept -> int{return 0;};
    b.radio.stop=[](void*,void*,unsigned,const char*) noexcept -> int{return 0;};
    assert(radio.initialize(&f.radio_ctrl,0)==1);
    auto run=[&](const char* name,unsigned type=0,unsigned word=0){return api.input(&f.ctrl,&b,name,type,word,f.player);};
    assert(api.key(&f.ctrl,"EnableUse",6,1)==1&&f.vehicle[0x74a]);
    assert(api.key(&f.ctrl,"DriverDoorLocked",6,1)==1&&f.vehicle[0x751]);
    assert(api.key(&f.ctrl,"Invunerable",6,1)==1&&f.vehicle[0x74d]);
    assert(api.key(&f.ctrl,"DamageTolerance",1,0x41a00000)==1&&f.word(f.vehicle,0x754)==0x41a00000);
    assert(api.key(&f.ctrl,"EnableUse",5,1)==0&&api.key(&f.ctrl,"UseBox",5,1)==0);
    assert(run("EnterVehicle",5,2)==1&&f.role==2&&f.enters==1);
    assert(run("EnterVehicle",2,99)==1&&f.role==0);
    assert(run("ExitVehicle",5,1)==1&&!f.exits&&run("ExitVehicle",5,0)==1&&f.exits==1);
    assert(run("EnableUse",2,1)==1&&!f.vehicle[0x74a]&&run("LockView",5,9)==1&&f.vehicle[0x74b]);
    assert(run("EnableHints",5,1)==1&&f.vehicle[0x764]&&run("EnableAmmo",5,1)==1&&f.vehicle[0x74c]);
    assert(run("SetView",5,0)==1&&f.outputs==1&&f.attachments==1&&f.player[0x9b4]);
    f.doors[0x45]=7;
    assert(run("OpenDoor",2,99)==1&&f.doors[0x44]&&!f.doors[0x45]&&f.sounds==1&&f.last_index==0);
    f.doors[0x45]=9;assert(run("OpenDoor",5,0)==1&&f.sounds==1&&!f.doors[0x45]);
    assert(run("OpenDoors")==1&&f.doors[76+0x44]&&f.sounds==2);
    assert(run("CloseDoors")==1&&f.sounds==4&&!f.doors[0x44]&&!f.doors[76+0x44]);
    assert(run("CloseDoor",5,9)==1&&f.sounds==4);
    f.windows[0x45]=5;
    assert(run("OpenWindow",5,0)==1&&f.windows[0x44]&&f.windows[0x45]==5&&f.sounds==5);
    assert(run("CloseWindow",5,0)==1&&!f.windows[0x44]&&f.sounds==6);
    assert(run("OpenWindows")==1&&f.sounds==8&&run("CloseWindows")==1&&f.sounds==10);
    assert(run("HeadlightOn")==1&&f.vehicle[0x748]&&f.dirty==1&&run("HeadlightOn")==1&&f.dirty==1);
    f.vehicle[0x1f0]=1;assert(run("HeadlightOff")==1&&!f.vehicle[0x748]&&f.vehicle[0x200]==1&&f.dirty==1);
    assert(run("WipersOn")==1&&f.vehicle[0x74e]&&run("WipersOff")==1&&!f.vehicle[0x74e]);
    assert(run("MonitorOn")==1&&f.vehicle[0x758]&&f.sounds==11&&run("MonitorOn")==1&&f.sounds==11);
    assert(run("MonitorOff")==1&&!f.vehicle[0x758]&&f.unlinks==1&&f.resolves==2&&f.word(f.vehicle,0x760)==0xffffffffu);
    f.put(f.vehicle,0x760,0x2001);f.camera_live=false;
    assert(run("SetCamera",2,123)==1&&f.word(f.vehicle,0x75c)==123&&f.word(f.vehicle,0x760)==0x2001&&f.unlinks==1);
    assert(run("SetCamera",5,456)==1&&!f.word(f.vehicle,0x75c));
    assert(run("RadioOn")==1&&f.schedules==1&&f.word(f.radio,0x398)==0x42c80000&&f.radio_queries==1);
    assert(run("RadioOff")==1&&!f.word(f.radio,0x398)&&f.radio_queries==2);
    assert(run("SetRadioStation",5,3)==1&&f.word(f.radio,0x3a4)==0xffffffffu&&f.radio_queries==3);
    assert(run("Unknown")==0&&!f.ctrl.blocked);
    f.radio_live=false;assert(run("RadioOn")==-1&&f.ctrl.blocked);
    assert(api.acknowledge(&f.ctrl)==1);
}
}
