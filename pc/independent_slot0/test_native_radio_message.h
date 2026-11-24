#pragma once
#include "native_radio_message.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct RadioFixture {
    alignas(4) unsigned char a[0x3d4]{},b[0x3d4]{};
    HologramControl ac{a,sizeof(a)},bc{b,sizeof(b)};
    RadioDispatcher state{};
    unsigned outputs=0,precache=0,clears=0,last_offset=0;
    int player=1,clients=1;
    const char* last=nullptr;
    unsigned word(void* entity,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<unsigned char*>(entity)+off,4);return v;}
};
inline void check_radio_message(const RadioMessageApi& api){
    RadioFixture f;RadioBindings b{};b.context=&f;
    b.string=[](void*,std::uint32_t word,const char** out) noexcept -> int {
        static const char* names[]={"","npc_jc","npc_jessica","npc_jc.Message","npc_jessica.Message",""};
        if(word>=6)return 0;*out=names[word];return 1;
    };
    b.convert_variant=[](void*,const RadioValue*,std::uint32_t* word) noexcept -> int{*word=4;return 1;};
    b.precache=[](void* p,const char* name) noexcept -> int{auto& f=*static_cast<RadioFixture*>(p);++f.precache;f.last=name;return 1;};
    b.resolve=[](void* p,std::uint32_t h,void** out,unsigned* bytes) noexcept -> int{auto& f=*static_cast<RadioFixture*>(p);*out=h==0x1001?f.a:h==0x1002?f.b:nullptr;*bytes=sizeof(f.a);return 1;};
    b.handle=[](void* p,void* e,std::uint32_t* h) noexcept -> int{auto& f=*static_cast<RadioFixture*>(p);assert(e==f.a||e==f.b);*h=e==f.a?0x1001:0x1002;return 1;};
    b.max_clients=[](void* p,int* out) noexcept -> int{*out=static_cast<RadioFixture*>(p)->clients;return 1;};
    b.single_player=[](void* p,void** out) noexcept -> int{*out=&static_cast<RadioFixture*>(p)->player;return 1;};
    b.output=[](void* p,void*,void* player,unsigned off) noexcept -> int{auto& f=*static_cast<RadioFixture*>(p);assert(player==&f.player);++f.outputs;f.last_offset=off;return 1;};
    b.clear_delivery=[](void* p) noexcept -> int{++static_cast<RadioFixture*>(p)->clears;return 1;};
    assert(api.initialize(&f.ac)&&api.initialize(&f.bc));
    assert(f.word(f.a,0x3a0)==0xffffffffu&&f.word(f.a,0x3b8)==0xffffffffu&&!f.word(f.a,0x3c4));
    auto input=[&](HologramControl& ctrl,RadioInput op,unsigned type,unsigned word){RadioValue v{type,word};return api.input(&ctrl,&f.state,&b,op,&v);};
    assert(input(f.ac,RadioInput::Speaker,2,1)==1&&f.word(f.a,0x3cc)==1);
    assert(input(f.bc,RadioInput::Speaker,2,2)==1);
    assert(input(f.bc,RadioInput::Priority,2,1)==1&&!f.word(f.b,0x3c4)); // SDK string declaration differs from DLL
    assert(input(f.bc,RadioInput::Priority,5,1)==1&&f.word(f.b,0x3c4)==1);
    assert(input(f.ac,RadioInput::Message,2,3)==1&&f.precache==1&&f.word(f.a,0x3c8)==3);
    assert(input(f.bc,RadioInput::Message,5,99)==1&&f.precache==2&&f.word(f.b,0x3c8)==4);
    assert(input(f.ac,RadioInput::Dispatch,2,0)==1&&f.state.pending==0x1001&&f.state.kind==1&&f.outputs==1&&f.last_offset==0x394&&f.precache==2);
    assert(input(f.bc,RadioInput::Dispatch,2,0)==1&&f.state.pending==0x1001&&f.state.kind==1&&f.outputs==2);
    RadioSound sound{};auto expect=[&](unsigned stage,const char* name){assert(api.sound(&f.state,&b,stage,&sound)==1&&sound.present&&!std::strcmp(sound.text,name));};
    expect(0,"policeradio.Alert.Objective");expect(1,"npc_jc.Notification.Objective");expect(2,"..");
    assert(api.promote(&f.state,&b)==1&&f.state.current==0x1001&&f.state.pending==0xffffffffu&&f.state.stage==4);
    expect(3,"npc_Blade.Acknowledgement.Objective.npc_jc");expect(4,"npc_jc.Message");assert(sound.speaker_word==1);
    assert(api.heard(&f.ac,&b,1)==1&&f.a[0x3d0]==1&&f.outputs==3&&f.last_offset==0x3ac);
    f.clients=2;assert(api.heard(&f.ac,&b,0)==1&&f.a[0x3d0]==0&&f.outputs==3);f.clients=1;
    assert(input(f.bc,RadioInput::Dispatch,2,5)==1&&f.state.pending==0x1002&&f.state.kind==2&&!f.word(f.b,0x3c8)&&f.precache==3);
    expect(0,"policeradio.Alert.Extraneous");expect(1,"npc_jessica.Notification.Extraneous");
    assert(input(f.ac,RadioInput::Dispatch,2,0)==1&&f.state.pending==0x1001&&f.state.kind==1); // objective overrides
    assert(api.input(&f.ac,&f.state,&b,RadioInput::Remove,nullptr)==1&&f.state.pending==0xffffffffu&&f.a[0x3d1]==1&&f.clears==1);
    assert(api.input(&f.ac,&f.state,&b,RadioInput::Remove,nullptr)==1&&f.state.current==0xffffffffu&&f.clears==2);
    assert(api.sound(&f.state,&b,4,&sound)==1&&!sound.present);
    f.state.other=0x1002;f.state.kind=3;assert(api.promote(&f.state,&b)==1&&f.state.current==0x1002&&f.state.stage==3);
    expect(3,"npc_Blade.Acknowledgement.Extraneous.npc_jessica");
    expect(4,"");
}

struct RadioPlaybackFixture {
    RadioFixture f;
    RadioPlaybackControl control{&f.state};
    float now=10,duration=2;
    unsigned played=0,notified=0;
    bool eligible=true,fail=false;
    char sound[256]{},speaker[64]{};
};
inline void check_radio_playback(const RadioMessageApi& messages,const RadioPlaybackApi& api){
    RadioPlaybackFixture x;auto& f=x.f;RadioPlaybackBindings b{};auto& m=b.message;m.context=&x;
    m.string=[](void*,std::uint32_t word,const char** out) noexcept -> int {
        static const char* names[]={"","npc_jc","npc_jessica","test.JC","test.Jessica"};
        if(word>=5)return 0;*out=names[word];return 1;
    };
    m.precache=[](void*,const char*) noexcept -> int{return 1;};
    m.resolve=[](void* p,std::uint32_t h,void** out,unsigned* bytes) noexcept -> int{
        auto& f=static_cast<RadioPlaybackFixture*>(p)->f;*out=h==0x1001?f.a:h==0x1002?f.b:nullptr;*bytes=sizeof(f.a);return 1;
    };
    m.handle=[](void* p,void* entity,std::uint32_t* out) noexcept -> int{
        auto& f=static_cast<RadioPlaybackFixture*>(p)->f;*out=entity==f.a?0x1001:0x1002;return 1;
    };
    m.max_clients=[](void* p,int* out) noexcept -> int{*out=static_cast<RadioPlaybackFixture*>(p)->f.clients;return 1;};
    m.single_player=[](void* p,void** out) noexcept -> int{*out=&static_cast<RadioPlaybackFixture*>(p)->f.player;return 1;};
    m.output=[](void* p,void*,void* player,unsigned off) noexcept -> int{
        auto& f=static_cast<RadioPlaybackFixture*>(p)->f;assert(player==&f.player);++f.outputs;f.last_offset=off;return 1;
    };
    m.clear_delivery=[](void* p) noexcept -> int{++static_cast<RadioPlaybackFixture*>(p)->f.clears;return 1;};
    b.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<RadioPlaybackFixture*>(p)->now;return 1;};
    b.participant=[](void* p,void* player,unsigned* out) noexcept -> int{
        auto& x=*static_cast<RadioPlaybackFixture*>(p);assert(player==&x.f.player);*out=x.eligible;return 1;
    };
    b.emit=[](void* p,void* player,const char* name,int channel,float volume,float filter,float* duration) noexcept -> int {
        auto& x=*static_cast<RadioPlaybackFixture*>(p);assert(x.control.busy&&player==&x.f.player&&channel==4&&volume==1&&filter==0.8f);
        ++x.played;std::strcpy(x.sound,name);*duration=x.duration;return !x.fail;
    };
    b.notify=[](void* p,unsigned active,int priority,const char* speaker) noexcept -> int {
        auto& x=*static_cast<RadioPlaybackFixture*>(p);assert(x.control.busy&&active==1&&priority>=0&&priority<=1);
        ++x.notified;std::strcpy(x.speaker,speaker);return 1;
    };
    auto input=[&](HologramControl& c,RadioInput op,unsigned type,unsigned word){RadioValue v{type,word};return messages.input(&c,&f.state,&m,op,&v);};
    assert(messages.initialize(&f.ac)&&messages.initialize(&f.bc));
    assert(input(f.ac,RadioInput::Speaker,2,1)&&input(f.ac,RadioInput::Message,2,3));
    assert(input(f.bc,RadioInput::Speaker,2,2)&&input(f.bc,RadioInput::Message,2,4)&&input(f.bc,RadioInput::Priority,5,1));
    auto tick=[&](){return api.tick(&x.control,&b);};
    auto sound=[&](const char* n){assert(!std::strcmp(x.sound,n));};
    assert(input(f.ac,RadioInput::Dispatch,2,0)==1&&f.state.kind==1);
    assert(tick()==1&&f.state.stage==0&&!x.played);
    assert(tick()==1&&f.state.stage==2&&f.state.deadline==12&&x.played==1&&x.notified==1&&!std::strcmp(x.speaker,"npc_jc"));sound("policeradio.Alert.Objective");
    x.now=11.99f;assert(tick()==1&&f.state.stage==2);
    x.now=12;assert(tick()==1&&f.state.stage==4&&f.state.current==0x1001&&!f.a[0x3d0]&&x.played==1);
    assert(tick()==1&&f.state.stage==5&&f.state.deadline==14&&f.state.other==0x1001&&f.state.current==0xffffffffu&&f.a[0x3d0]&&f.last_offset==0x3ac);sound("test.JC");
    x.now=14;assert(tick()==1&&f.state.stage==6&&f.clears==1);
    assert(input(f.bc,RadioInput::Dispatch,2,0)==1&&tick()==1&&f.state.stage==0);
    assert(tick()==1&&f.state.stage==1&&f.state.deadline==16);sound("policeradio.Alert.Extraneous");
    x.now=16;assert(tick()==1&&f.state.stage==6&&f.state.kind==0&&f.state.pending==0x1002&&f.state.deadline==18);sound("npc_jessica.Notification.Extraneous");
    assert(api.player_input(&x.control,0,0x08000000,0)==1&&x.control.input_latch==1&&f.state.kind==3);
    assert(api.player_input(&x.control,0,0,0x08000000)==1&&!x.control.input_latch);
    x.now=18;assert(tick()==1&&f.state.stage==2);
    assert(tick()==1&&f.state.stage==3&&f.state.current==0x1002);
    assert(tick()==1&&f.state.stage==4&&f.state.deadline==20);sound("npc_Blade.Acknowledgement.Extraneous.npc_jessica");
    x.now=20;assert(tick()==1&&f.state.stage==5&&f.b[0x3d0]&&f.state.other==0x1001);sound("test.Jessica");
    x.now=22;assert(tick()==1&&f.state.stage==6&&f.clears==2);
    assert(input(f.ac,RadioInput::Dispatch,2,0)&&tick()==1&&f.state.stage==0);
    x.fail=true;assert(tick()==-1&&x.control.blocked);assert(tick()==0&&api.acknowledge(&x.control));x.fail=false;
    assert(tick()==1&&f.state.stage==2);
}

}
