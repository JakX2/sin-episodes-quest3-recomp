#pragma once
#include "native_radio_transport.h"
#include "native_radio_entity.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct RadioTransportFixture {
    alignas(4) unsigned char entity[radio_entity_bytes]{},hud_obj[0xc0]{};
    HologramControl entity_control{entity,sizeof(entity)},hud_control{hud_obj,sizeof(hud_obj)};
    RadioDispatcher dispatcher{};
    RadioPlaybackControl playback{&dispatcher};
    RadioDeliveryBindings delivery{};
    RadioClientRouteBindings route{};
    const RadioDeliveryApi* delivery_api=nullptr;
    const RadioClientRouteApi* route_api=nullptr;
    int client_count=1,player=0,unknown=0;
    bool edict_present=true,unknown_present=true,player_present=true,hud_present=true;
    unsigned flags=0,registrations=0,edict_queries=0,player_queries=0,sends=0,finds=0,heard=0,dispatched=0,plays=0;
    float now=10;
    char stored[2048]{},sound[256]{};
    const char* animation=nullptr;
};
inline void check_radio_transport(const RadioMessageApi& message,const RadioEntityApi& entity,const RadioPlaybackApi& playback,
        const RadioDeliveryApi& delivery,const RadioHudApi& hud,const RadioClientRouteApi& route){
    RadioTransportFixture f;f.delivery_api=&delivery;f.route_api=&route;
    auto& d=f.delivery;d.context=&f;
    d.max_clients=[](void* p,int* out) noexcept -> int{*out=static_cast<RadioTransportFixture*>(p)->client_count;return 1;};
    d.edict=[](void* p,unsigned index,RadioEdictObservation* out) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(index==1);++f.edict_queries;
        *out={f.edict_present?1u:0u,f.flags,f.unknown_present?&f.unknown:nullptr};return 1;
    };
    d.player_from_unknown=[](void* p,void* unknown,void** out) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(unknown==&f.unknown);++f.player_queries;
        *out=f.player_present?&f.player:nullptr;return 1;
    };
    d.register_message=[](void* p,const char* name,int bytes) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(bytes==-1&&!std::strcmp(name,"PoliceRadioMessage"));++f.registrations;return 1;
    };
    d.send=[](void* p,void* player,unsigned reliable,const char* name,const unsigned char* data,unsigned bytes) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(player==&f.player&&reliable==1&&!std::strcmp(name,"PoliceRadioMessage"));++f.sends;
        return f.route_api->receive(&f.route,data,bytes)==1;
    };
    auto& c=f.route;c.context=&f;c.register_message=d.register_message;
    c.find_hud=[](void* p,const char* name,HologramControl** out) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(!std::strcmp(name,"CHudPoliceRadio"));++f.finds;
        *out=f.hud_present?&f.hud_control:nullptr;return 1;
    };
    c.hud.context=&f;
    c.hud.store_speaker=[](void* p,const char* name,const char** out) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(f.hud_control.busy);std::strcpy(f.stored,name);*out=f.stored;return 1;
    };
    c.hud.panel_flag=[](void* p,void* panel,unsigned off,unsigned active) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(f.hud_control.busy&&panel==f.hud_obj+0x14&&(off==0x78||off==0xfc)&&active==f.hud_obj[0xb6]);return 1;
    };
    c.hud.animation=[](void* p,const char* name) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(f.hud_control.busy);f.animation=name;return 1;
    };
    assert(delivery.register_message(&d)==1&&route.register_message(&c)==1&&f.registrations==2);
    assert(hud.initialize(&f.hud_control)==1);
    f.client_count=2;assert(delivery.notify(&d,1,1,"npc_jessica")==1&&!f.edict_queries&&!f.sends);
    f.client_count=0;assert(delivery.notify(&d,1,1,"npc_jessica")==1&&!f.edict_queries);
    f.client_count=1;f.edict_present=false;assert(delivery.notify(&d,1,1,"npc_jessica")==1&&f.edict_queries==1&&!f.player_queries);
    f.edict_present=true;f.flags=2;assert(delivery.notify(&d,1,1,"npc_jessica")==1&&f.edict_queries==2&&!f.player_queries);
    f.flags=0;f.unknown_present=false;assert(delivery.notify(&d,1,1,"npc_jessica")==1&&!f.player_queries);
    f.unknown_present=true;f.player_present=false;assert(delivery.notify(&d,1,1,"npc_jessica")==1&&f.player_queries==1&&!f.sends);
    f.player_present=true;f.hud_present=false;assert(delivery.notify(&d,1,1,"npc_jessica")==1&&f.sends==1&&f.finds==1&&!f.hud_obj[0xb6]);
    f.hud_present=true;
    RadioEntityBindings e{};auto& m=e.message;m.context=&f;
    m.string=[](void*,std::uint32_t word,const char** out) noexcept -> int{
        static const char* names[]={"","npc_jessica","test.RadioScene"};if(word>2)return 0;*out=names[word];return 1;
    };
    m.precache=[](void*,const char*) noexcept -> int{return 1;};
    m.resolve=[](void* p,std::uint32_t handle,void** out,unsigned* bytes) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);*out=handle==0x1001?f.entity:nullptr;*bytes=sizeof(f.entity);return 1;
    };
    m.handle=[](void* p,void* obj,std::uint32_t* out) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(obj==f.entity);*out=0x1001;return 1;
    };
    m.max_clients=d.max_clients;
    m.single_player=[](void* p,void** out) noexcept -> int{*out=&static_cast<RadioTransportFixture*>(p)->player;return 1;};
    m.output=[](void* p,void* obj,void* player,unsigned offset) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(obj==f.entity&&player==&f.player);
        if(offset==0x394)++f.dispatched;else{assert(offset==0x3ac);++f.heard;}return 1;
    };
    m.clear_delivery=[](void* p) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);return f.delivery_api->notify(&f.delivery,0,0,nullptr)==1;
    };
    e.base_construct=[](void*,void*,unsigned bytes) noexcept -> int{return bytes==radio_entity_bytes;};
    e.class_init=[](void*,void*,const char* name) noexcept -> int{return !std::strcmp(name,"env_radio_message");};
    e.spawn_helper=[](void*,void*,unsigned,unsigned,unsigned) noexcept -> int{return 1;};
    e.base_activate=[](void*,void*) noexcept -> int{return 1;};
    assert(entity.construct(&f.entity_control,&e)==1);
    RadioValue speaker{2,1},sound{0x13,2},priority{5,1},empty{2,0};
    assert(entity.key(&f.entity_control,"speaker",&speaker)==1&&entity.key(&f.entity_control,"message",&sound)==1&&entity.key(&f.entity_control,"priority",&priority)==1);
    assert(entity.spawn(&f.entity_control,&e)==1&&entity.activate(&f.entity_control,&e)==1);
    assert(entity.input(&f.entity_control,&f.dispatcher,&e,"Dispatch",&empty)==1&&f.dispatched==1);
    RadioPlaybackBindings b{};b.message=m;
    b.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<RadioTransportFixture*>(p)->now;return 1;};
    b.participant=[](void*,void*,unsigned* out) noexcept -> int{*out=1;return 1;};
    b.emit=[](void* p,void* player,const char* name,int channel,float volume,float filter,float* duration) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);assert(f.playback.busy&&player==&f.player&&channel==4&&volume==1&&filter==0.8f);
        ++f.plays;std::strcpy(f.sound,name);*duration=2;return 1;
    };
    b.notify=[](void* p,unsigned active,int priority,const char* speaker) noexcept -> int{
        auto& f=*static_cast<RadioTransportFixture*>(p);return f.delivery_api->notify(&f.delivery,active,priority,speaker)==1;
    };
    auto tick=[&](){return playback.tick(&f.playback,&b);};
    assert(tick()==1&&f.dispatcher.stage==0&&tick()==1&&f.dispatcher.stage==1);
    assert(f.sends==2&&f.hud_obj[0xb6]==1&&!std::strcmp(f.animation,"PoliceRadioShowExtraneous"));
    RadioIconRequest icon{};assert(hud.icon(&f.hud_control,&icon)==1&&icon.present&&!std::strcmp(icon.name,"police_radio_npc_jessica"));
    f.now=12;assert(tick()==1&&f.dispatcher.stage==6&&!std::strcmp(f.sound,"npc_jessica.Notification.Extraneous"));
    assert(playback.request(&f.playback)==1);f.now=14;
    assert(tick()==1&&f.dispatcher.stage==2&&tick()==1&&f.dispatcher.stage==3&&tick()==1&&f.dispatcher.stage==4);
    f.now=16;assert(tick()==1&&f.dispatcher.stage==5&&f.heard==1&&!std::strcmp(f.sound,"test.RadioScene"));
    f.now=18;assert(tick()==1&&f.dispatcher.stage==6&&f.sends==3&&!f.hud_obj[0xb6]&&!std::strcmp(f.animation,"PoliceRadioHide"));
    assert(hud.icon(&f.hud_control,&icon)==1&&!icon.present);
    assert(f.plays==4&&f.finds==3);
    (void)message;
}
}
