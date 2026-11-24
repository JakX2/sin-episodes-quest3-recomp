#pragma once
#include "native_radio_entity.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct RadioEntityFixture {
    alignas(4) unsigned char entity[radio_entity_bytes]{};
    HologramControl control{entity,sizeof(entity)};
    RadioDispatcher state{};
    unsigned events[64]{},count=0,precaches=0;
    const char* last=nullptr;
    int player=1;
    bool fail=false;
    void push(unsigned n){assert(control.busy&&count<64);events[count++]=n;}
    unsigned word(unsigned offset){unsigned v=0;std::memcpy(&v,entity+offset,4);return v;}
};
inline void check_radio_entity(const RadioEntityApi& api,const RadioMessageApi& messages){
    RadioEntityFixture f;RadioEntityBindings b{};auto& m=b.message;m.context=&f;
    const auto* schema=api.schema();
    assert(schema&&schema->object_bytes==0x414&&schema->field_count==10&&!std::strcmp(schema->classname,"env_radio_message"));
    assert(schema->fields[0].field_type==0x13&&schema->fields[5].field_type==5&&schema->fields[9].offset==0x3ac);
    b.base_construct=[](void* p,void* obj,unsigned bytes) noexcept -> int{
        auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity&&bytes==0x414);f.push(1);std::memset(obj,0xaa,bytes);return 1;
    };
    b.class_init=[](void* p,void* obj,const char* name) noexcept -> int{
        auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity&&!std::strcmp(name,"env_radio_message")&&f.word(0x3c8)==0&&f.word(0x3a0)==0xffffffffu);f.push(2);return 1;
    };
    b.spawn_helper=[](void* p,void* obj,unsigned leaf,unsigned a,unsigned b) noexcept -> int{
        auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity&&!a&&!b&&f.precaches==11);
        f.push(leaf==0x22201ac0?4:5);assert(leaf==0x22201ac0||leaf==0x22143610);return 1;
    };
    b.base_activate=[](void* p,void* obj) noexcept -> int{auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity);f.push(6);return 1;};
    b.destroy_outputs=[](void* p,void* obj,unsigned offset) noexcept -> int{auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity&&(offset==0x3ac||offset==0x394));f.push(offset==0x3ac?8:9);return 1;};
    b.base_destroy=[](void* p,void* obj) noexcept -> int{auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity);f.push(10);return 1;};
    m.string=[](void*,std::uint32_t word,const char** out) noexcept -> int{static const char* names[]={"","npc_jessica","test.Message",""};if(word>3)return 0;*out=names[word];return 1;};
    m.precache=[](void* p,const char* name) noexcept -> int {
        auto& f=*static_cast<RadioEntityFixture*>(p);assert(f.control.busy);
        const char* expected[]={"policeradio.Alert.Objective","policeradio.Alert.Extraneous","npc_jessica.Notification.Objective","npc_jessica.Notification.Extraneous","npc_jc.Notification.Objective","npc_jc.Notification.Extraneous","npc_Blade.Acknowledgement.Objective.npc_jessica","npc_Blade.Acknowledgement.Extraneous.npc_jessica","npc_Blade.Acknowledgement.Objective.npc_jc","npc_Blade.Acknowledgement.Extraneous.npc_jc","test.Message"};
        if(f.precaches<11)assert(!std::strcmp(name,expected[f.precaches]));
        ++f.precaches;f.last=name;f.push(3);return !f.fail;
    };
    m.resolve=[](void* p,std::uint32_t handle,void** out,unsigned* bytes) noexcept -> int{auto& f=*static_cast<RadioEntityFixture*>(p);*out=handle==0x1234?f.entity:nullptr;*bytes=sizeof(f.entity);return 1;};
    m.handle=[](void* p,void* obj,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity);*out=0x1234;return 1;};
    m.max_clients=[](void*,int* out) noexcept -> int{*out=1;return 1;};
    m.single_player=[](void* p,void** out) noexcept -> int{*out=&static_cast<RadioEntityFixture*>(p)->player;return 1;};
    m.output=[](void* p,void* obj,void* player,unsigned offset) noexcept -> int{auto& f=*static_cast<RadioEntityFixture*>(p);assert(obj==f.entity&&player==&f.player&&offset==0x394);f.push(7);return 1;};
    m.clear_delivery=[](void*) noexcept -> int{return 1;};
    assert(api.construct(&f.control,&b)==1&&f.count==2&&f.events[0]==1&&f.events[1]==2&&f.entity[0x3d3]==0xaa);
    auto key=[&](const char* name,unsigned type,unsigned word){RadioValue v{type,word};return api.key(&f.control,name,&v);};
    assert(key("speaker",2,1)&&key("message",0x13,2)&&key("priority",5,1));
    assert(key("priority",2,1)==0&&key("OnHeard",2,1)==0&&f.word(0x3c4)==1);
    assert(api.spawn(&f.control,&b)==1&&f.precaches==11&&f.events[13]==4&&f.events[14]==5);
    assert(api.activate(&f.control,&b)==1&&f.precaches==12&&f.events[15]==6&&!std::strcmp(f.last,"test.Message"));
    auto input=[&](const char* name,unsigned type,unsigned word){RadioValue v{type,word};return api.input(&f.control,&f.state,&b,name,&v);};
    assert(input("Dispatch",2,0)==1&&f.state.pending==0x1234&&f.state.kind==2&&f.events[17]==7);
    assert(input("SetPriority",5,0)==1&&!f.word(0x3c4));
    assert(input("unknown",2,0)==0);
    assert(key("message",2,3)&&api.activate(&f.control,&b)==1&&!f.word(0x3c8)&&!*f.last);
    f.fail=true;assert(input("SetMessage",2,2)==-1&&f.control.blocked);
    assert(messages.acknowledge(&f.control)==1);f.fail=false;
    assert(api.input(&f.control,&f.state,&b,"Remove",nullptr)==1&&f.state.pending==0xffffffffu);
    const auto count=f.count;
    assert(api.destroy(&f.control,&b)==1&&!f.control.object&&!f.control.bytes);
    assert(f.events[count]==8&&f.events[count+1]==9&&f.events[count+2]==10);
    assert(api.spawn(&f.control,&b)==0);
}
}
