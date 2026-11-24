#pragma once
#include "native_music_radio.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
struct MusicRadioFixture {
    alignas(4) unsigned char radio[music_radio_bytes]{},stations[112]{},tracks[204]{};
    HologramControl control{radio,sizeof(radio)};
    int speaker=1;
    unsigned emits=0,stops=0,schedules=0,handles=0,resolves=0,flags=99;
    float now=10,volume=0,when=0;
    char name[64]{};
    bool fail=false,external_live=false;
    void put(void* object,unsigned off,unsigned w){std::memcpy(static_cast<unsigned char*>(object)+off,&w,4);}
    unsigned word(void* object,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(object)+off,4);return w;}
    float number(unsigned off){float v=0;std::memcpy(&v,radio+off,4);return v;}
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
};
inline void check_music_radio(const MusicRadioApi& api){
    MusicRadioFixture f;MusicRadioBindings b{};b.context=&f;
    b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<MusicRadioFixture*>(p);assert(f.control.busy);*out=f.now;return 1;};
    b.schedule=[](void* p,void* radio,float when) noexcept -> int{auto& f=*static_cast<MusicRadioFixture*>(p);assert(f.control.busy&&radio==f.radio);++f.schedules;f.when=when;return 1;};
    b.resolve=[](void* p,std::uint32_t word,void** out) noexcept -> int{
        auto& f=*static_cast<MusicRadioFixture*>(p);assert(f.control.busy);++f.resolves;
        *out=word==0x1001?static_cast<void*>(f.radio):word==0x2001&&f.external_live?static_cast<void*>(&f.speaker):nullptr;return 1;
    };
    b.handle=[](void* p,void* radio,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<MusicRadioFixture*>(p);assert(f.control.busy&&radio==f.radio);++f.handles;*out=0x1001;return 1;};
    b.emit=[](void* p,void* radio,void* speaker,const char* name,float volume,unsigned channel,unsigned level,unsigned flags,float* duration) noexcept -> int {
        auto& f=*static_cast<MusicRadioFixture*>(p);assert(f.control.busy&&radio==f.radio&&(speaker==radio||speaker==&f.speaker)&&channel==5&&level==60);
        ++f.emits;f.volume=volume;f.flags=flags;std::strcpy(f.name,name);*duration=2;return !f.fail;
    };
    b.stop=[](void* p,void* speaker,unsigned channel,const char* name) noexcept -> int{
        auto& f=*static_cast<MusicRadioFixture*>(p);assert(f.control.busy&&speaker==f.radio&&channel==5&&!f.number(0x398));++f.stops;std::strcpy(f.name,name);return 1;
    };
    assert(api.initialize(&f.control,0x1234)==1&&f.word(f.radio,0x394)==0xffffffffu&&f.word(f.radio,0x39c)==0x1234);
    f.put(f.radio,0x3a8,f.ptr(f.stations));f.put(f.radio,0x3b4,2);
    f.put(f.stations,0x20,0xffffffffu);f.put(f.stations,0x24,f.ptr(f.tracks));f.put(f.stations,0x30,2);
    f.put(f.stations+56,0x20,0xffffffffu);f.put(f.stations+56,0x24,f.ptr(f.tracks+136));f.put(f.stations+56,0x30,1);
    std::strcpy(reinterpret_cast<char*>(f.tracks+4),"fixture.a");
    std::strcpy(reinterpret_cast<char*>(f.tracks+72),"fixture.b");
    std::strcpy(reinterpret_cast<char*>(f.tracks+140),"fixture.c");
    auto input=[&](MusicRadioInput op,int value=0){return api.input(&f.control,&b,op,value);};
    auto played=[&](const char* name){assert(!std::strcmp(f.name,name));};
    assert(input(MusicRadioInput::On)==1&&f.emits==1&&f.schedules==1&&f.handles==1&&f.resolves==1&&f.volume==1&&f.number(0x3a0)==12);played("fixture.a");
    assert(input(MusicRadioInput::On)==1&&f.emits==1);
    f.now=11.99f;assert(api.tick(&f.control,&b)==1&&f.emits==1&&f.schedules==2);
    f.now=12;assert(api.tick(&f.control,&b)==1&&f.emits==2&&f.schedules==3&&f.number(0x3a0)==14);played("fixture.b");
    assert(input(MusicRadioInput::Volume,250)==1&&f.volume==1&&f.flags==1&&f.number(0x398)==250&&f.number(0x3a0)==14);
    assert(input(MusicRadioInput::Volume,50)==1&&f.volume==0.5f&&f.flags==1);
    assert(input(MusicRadioInput::Cycle)==1&&f.word(f.radio,0x3a4)==1&&f.flags==0);played("fixture.c");
    assert(input(MusicRadioInput::Cycle)==1&&f.stops==1&&!f.number(0x398));played("fixture.c");
    assert(input(MusicRadioInput::Cycle)==1&&f.word(f.radio,0x3a4)==0&&f.number(0x398)==100);played("fixture.a");
    assert(input(MusicRadioInput::Previous)==1&&f.word(f.radio,0x3a4)==1);played("fixture.c");
    assert(input(MusicRadioInput::Next)==1&&f.word(f.radio,0x3a4)==0);played("fixture.b");
    assert(input(MusicRadioInput::Station,999)==1&&f.word(f.radio,0x3a4)==1);
    assert(input(MusicRadioInput::Station,-999)==1&&!f.word(f.radio,0x3a4));
    f.put(f.radio,0x394,0x2001);f.external_live=true;
    assert(input(MusicRadioInput::Volume,-5)==1&&f.volume==0&&f.number(0x398)==-5&&f.handles==1);
    const auto prior=f.emits;assert(input(MusicRadioInput::Cycle)==1&&f.emits==prior&&!f.word(f.radio,0x3a4));
    f.external_live=false;assert(input(MusicRadioInput::Volume,100)==1&&f.handles==2);
    f.fail=true;assert(input(MusicRadioInput::Next)==-1&&f.control.blocked);
    assert(api.acknowledge(&f.control)==1);f.fail=false;
    f.put(f.radio,0x3a8,0);assert(input(MusicRadioInput::Station,0)==-1&&f.control.blocked);
}
}
