#pragma once
#include "native_server_facial_records.h"
#include "native_server_npc_random_expression.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
inline void check_server_facial_records(const ServerFacialRecordApi& api,const ActorEventRecordApi& records){
    struct Fixture {
        alignas(4) unsigned char actor[server_npc_random_expression_extent]{},scene[0x220]{},rows[0xc0]{},events[3][0x3c0]{};
        unsigned fallback{},diagnostics{};float time=10;
    } f;
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    auto get=[](void* p,unsigned off){float v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;};
    auto word=[](void* p){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));};
    const auto scene=word(f.scene);
    put(f.actor,0x5b8,word(f.rows));put(f.actor,0x5bc,3);
    put(f.events[0],0,10u);put(f.events[1],0,16u);put(f.events[2],0,16u);
    for(auto& e:f.events)put(e,0x2b4,99u);
    std::strcpy(reinterpret_cast<char*>(f.events[1]+0x84),"ai_randomfaceflex");
    std::strcpy(reinterpret_cast<char*>(f.events[2]+0x84),"AI_RANDOMHEADFLEX");
    ServerFacialRecordBindings cfg{};cfg.base_flex=cfg.npc_random=true;cfg.host.context=&f;
    cfg.host.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        auto match=[&](void* q,unsigned size){if(token!=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(q)))return false;*out=q;*n=size;return true;};
        if(match(f.rows,sizeof(f.rows))||match(f.scene,sizeof(f.scene)))return 1;
        for(auto& e:f.events)if(match(e,sizeof(e)))return 1;
        return 0;
    };
    cfg.host.grow=[](void*,void*,unsigned) noexcept -> int{return 0;};
    cfg.host.handle=[](void*,void*,std::uint32_t* out) noexcept -> int{*out=0xffffffffu;return 1;};
    cfg.host.diagnostic=[](void* p,ActorEventDiagnostic,std::uint32_t) noexcept -> int{++static_cast<Fixture*>(p)->diagnostics;return 1;};
    cfg.host.begin=[](void* p,void*,void*,std::uint32_t,void*,std::uint32_t,void*,unsigned* out) noexcept -> int{
        ++static_cast<Fixture*>(p)->fallback;*out=0;return 1;
    };
    cfg.extent=[](void*,void*,unsigned* out) noexcept -> int{*out=server_npc_random_expression_extent;return 1;};
    cfg.now=[](void* p,float* out) noexcept -> int{*out=static_cast<Fixture*>(p)->time;return 1;};
    ActorEventRecordBindings b{};assert(api.adapt(&cfg,&b));
    unsigned ok=0;
    assert(records.add(f.actor,sizeof(f.actor),scene,f.events[0],sizeof(f.events[0]),nullptr,&b,&ok)==1&&ok);
    assert(get(f.rows,0x20)==1&&get(f.actor,0x604)==10);
    f.scene[0x218]=1;put(f.actor,0x28,9.9f);put(f.actor,0x2c,10.1f);f.time=10.1f;
    assert(records.add(f.actor,sizeof(f.actor),scene,f.events[1],sizeof(f.events[1]),nullptr,&b,&ok)==1&&ok);
    unsigned subtype=0;std::memcpy(&subtype,f.rows+0x78,4);
    assert(subtype==6&&get(f.rows,0x60)==0&&get(f.rows,0x7c)==0&&f.fallback==0);
    assert(records.add(f.actor,sizeof(f.actor),scene,f.events[2],sizeof(f.events[2]),nullptr,&b,&ok)==1&&!ok);
    assert(f.fallback==1&&f.diagnostics==1);
    assert(records.check(f.actor,sizeof(f.actor),0,scene,f.events[1],&b,&ok)==1&&ok);
    assert(records.stop(f.actor,sizeof(f.actor),scene,f.events[0],0,&b,&ok)==1&&ok);
    std::memcpy(&subtype,f.rows+0x38,4);assert(subtype==6);
    assert(records.stop(f.actor,sizeof(f.actor),scene,f.events[1],0,&b,&ok)==1&&ok);
    unsigned count=99;std::memcpy(&count,f.actor+0x5c4,4);assert(count==0);
}
}
