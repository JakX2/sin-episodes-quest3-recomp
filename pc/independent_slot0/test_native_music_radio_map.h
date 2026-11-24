#pragma once
#include "native_music_radio_map.h"
#include "native_vehicle_actions.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct MusicMapFixture {
    alignas(4) unsigned char radio[music_radio_bytes]{},vehicle[0x900]{},actor[0x1230]{};
    alignas(4) unsigned char stations[4*56]{};
    MusicRadioTrack tracks[4][12]{};
    HologramControl ctrl{radio,sizeof(radio)},vehicle_ctrl{vehicle,sizeof(vehicle)};
    const MusicRadioApi* api=nullptr;
    const MusicRadioMapApi* map=nullptr;
    MusicRadioMapBindings b{};
    unsigned opens=0,closes=0,precaches=0,spawns=0,activates=0,thinks=0,schedules=0,emits=0,stops=0,outputs=0;
    bool missing=false,fail_precache=false;
    float now=10,when=0;
    char last[64]{};
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    unsigned word(void* p,unsigned o){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+o,4);return w;}
    void put(void* p,unsigned o,unsigned w){std::memcpy(static_cast<unsigned char*>(p)+o,&w,4);}
    float number(unsigned off){float v=0;std::memcpy(&v,radio+off,4);return v;}
    unsigned char* station(int i){return stations+i*56;}
};
inline void check_music_radio_map(const MusicRadioMapApi& map,const MusicRadioApi& api,const VehicleActionsApi& vehicle){
    MusicMapFixture f;f.api=&api;f.map=&map;auto& b=f.b;
    auto& p=b.playback;p.context=&f;
    const auto* schema=map.schema();
    assert(schema&&schema->object_bytes==0x3bc&&schema->count==12&&!std::strcmp(schema->classname,"env_radio"));
    assert(schema->fields[1].type==1&&schema->fields[1].offset==0x398&&schema->fields[2].offset==0x39c&&!std::strcmp(schema->fields[5].name,"RadioThink"));
    p.clock=[](void* c,float* out) noexcept -> int{*out=static_cast<MusicMapFixture*>(c)->now;return 1;};
    p.schedule=[](void* c,void* r,float when) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&r==f.radio);++f.schedules;f.when=when;return 1;};
    p.resolve=[](void* c,std::uint32_t word,void** out) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(word==0x2001);*out=f.radio;return 1;};
    p.handle=[](void* c,void* r,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(r==f.radio);*out=0x2001;return 1;};
    p.emit=[](void* c,void* r,void* speaker,const char* name,float vol,unsigned channel,unsigned level,unsigned flags,float* duration) noexcept -> int{
        auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&r==f.radio&&speaker==f.radio&&channel==5&&level==60&&!flags&&vol>=0&&vol<=1);
        ++f.emits;std::strcpy(f.last,name);*duration=2;return 1;
    };
    p.stop=[](void* c,void* speaker,unsigned channel,const char* name) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&speaker==f.radio&&channel==5);++f.stops;std::strcpy(f.last,name);return 1;};
    b.open_script=[](void* c,std::uint32_t word,void** out) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&word==9);++f.opens;*out=f.missing?nullptr:&f;return 1;};
    b.entry=[](void* c,void* token,unsigned i,unsigned* present,const char** station,const char** sound) noexcept -> int{
        auto& f=*static_cast<MusicMapFixture*>(c);assert(token==&f&&f.ctrl.busy&&i<=4);
        static const char* names[]={"Scanner","sCaNnEr","Music","SCANNER"};
        static const char* sounds[]={"fixture.a","fixture.b","fixture.c","fixture.d"};
        *present=i<4;if(*present){*station=names[i];*sound=sounds[i];}return 1;
    };
    b.close_script=[](void* c,void* token) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(token==&f&&f.ctrl.busy);++f.closes;return 1;};
    b.append_station=[](void* c,void* r) noexcept -> int{
        auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&r==f.radio);
        auto n=f.word(r,0x3b4);assert(n<4);std::memset(f.station(n),0xaa,56);
        f.put(r,0x3a8,f.ptr(f.stations));f.put(r,0x3b4,n+1);return 1;
    };
    b.append_track=[](void* c,void* r,int station,const MusicRadioTrack* t) noexcept -> int{
        auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&r==f.radio&&station>=0&&station<4&&t->station==station);
        auto* s=f.station(station);auto count=f.word(s,0x30);assert(count<12);f.tracks[station][count]=*t;
        f.put(s,0x24,f.ptr(f.tracks[station]));f.put(s,0x30,count+1);return 1;
    };
    b.precache=[](void* c,const char* name) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&!std::strncmp(name,"fixture.",8));++f.precaches;return !f.fail_precache;};
    b.base_spawn=[](void* c,void* r) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&r==f.radio&&!f.opens);++f.spawns;return 1;};
    b.base_activate=[](void* c,void* r) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&r==f.radio&&f.spawns==1&&f.precaches==4);++f.activates;return 1;};
    b.install_think=[](void* c,void* r,const char* name) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.ctrl.busy&&r==f.radio&&!std::strcmp(name,"RadioThink")&&f.opens==f.closes);++f.thinks;return 1;};
    assert(api.initialize(&f.ctrl,8)==1);
    assert(map.key(&f.ctrl,"ScriptFile",2,9)==1&&map.key(&f.ctrl,"Volume",1,0x42480000)==1);
    assert(map.key(&f.ctrl,"RadioOn",5,1)==0&&map.key(&f.ctrl,"Volume",5,50)==0);
    assert(map.spawn(&f.ctrl,&b)==1&&f.spawns==1&&f.thinks==1&&f.schedules==1&&f.when==10.2f);
    assert(f.opens==1&&f.closes==1&&f.precaches==4&&f.word(f.radio,0x3b4)==2);
    assert(!std::strcmp(reinterpret_cast<char*>(f.station(0)),"Scanner")&&!std::strcmp(reinterpret_cast<char*>(f.station(1)),"Music"));
    assert(f.word(f.station(0),0x20)==0xffffffffu&&f.word(f.station(0),0x30)==3&&f.word(f.station(1),0x30)==1);
    assert(!std::strcmp(f.tracks[0][0].sound,"fixture.a")&&!std::strcmp(f.tracks[0][1].sound,"fixture.b")&&!std::strcmp(f.tracks[0][2].sound,"fixture.d"));
    f.put(f.radio,0x3a0,0x42c80000);f.now=11;
    assert(map.activate(&f.ctrl,&b)==1&&f.activates==1&&f.thinks==2&&f.schedules==2&&f.when==11.2f&&f.number(0x3a0)==0);
    assert(f.word(f.station(0),0x30)==6&&f.word(f.station(1),0x30)==2&&f.precaches==8);
    assert(api.tick(&f.ctrl,&p)==1&&f.emits==1&&!std::strcmp(f.last,"fixture.a"));
    assert(map.input(&f.ctrl,&b,"SetScript",2,9)==0);
    assert(map.input(&f.ctrl,&b,"SetStation",2,99)==1&&!f.word(f.radio,0x3a4)&&!std::strcmp(f.last,"fixture.b"));
    // Actual vehicle action branch invokes the loaded station cycle,
    // not a fixture-side imitation of the radio behavior.
    f.put(f.vehicle,0x768,0x2001);
    VehicleActionBindings v{};v.context=&f;
    v.output=[](void* c,void* v,void* actor,unsigned off,int part) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(f.vehicle_ctrl.busy&&v==f.vehicle&&actor==f.actor&&off==0x7cc&&part==2);++f.outputs;return 1;};
    v.resolve=p.resolve;
    v.participant=[](void*,void*,unsigned*) noexcept -> int{return 0;};
    v.occupied_seat=[](void*,void*,void*,int*) noexcept -> int{return 0;};v.candidate_seat=v.occupied_seat;
    v.seat_type=[](void*,void*,int,int*) noexcept -> int{return 0;};
    v.passenger_enter=[](void*,void*,void*,int) noexcept -> int{return 0;};
    v.passenger_exit=[](void*,void*,void*,unsigned) noexcept -> int{return 0;};
    v.sound=[](void*,void*,const char*,const char*,int) noexcept -> int{return 0;};
    v.sequence=[](void*,void*,const char*) noexcept -> int{return 0;};
    v.unimplemented=[](void*,int) noexcept -> int{return 0;};
    v.radio_next=[](void* c,void* r) noexcept -> int{auto& f=*static_cast<MusicMapFixture*>(c);assert(r==f.radio);return f.api->input(&f.ctrl,&f.b.playback,MusicRadioInput::Cycle,0)==1;};
    auto button=[&](){return vehicle.apply(&f.vehicle_ctrl,&v,f.actor,sizeof(f.actor),2);};
    assert(button()==1&&f.outputs==1&&f.word(f.radio,0x3a4)==1&&!std::strcmp(f.last,"fixture.c"));
    assert(button()==1&&f.outputs==2&&f.stops==1&&!f.number(0x398));
    assert(button()==1&&f.outputs==3&&f.number(0x398)==100&&!f.word(f.radio,0x3a4)&&!std::strcmp(f.last,"fixture.d"));
    f.missing=true;assert(map.precache(&f.ctrl,&b)==1&&f.opens==3&&f.closes==2&&f.precaches==8);
    f.missing=false;f.fail_precache=true;assert(map.precache(&f.ctrl,&b)==-1&&f.ctrl.blocked&&f.opens==4&&f.closes==3&&f.precaches==9);
    assert(api.acknowledge(&f.ctrl)==1);
}
}
