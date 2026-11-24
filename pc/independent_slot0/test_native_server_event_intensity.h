#pragma once
#include "native_server_event_intensity.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_server_event_intensity(const ServerEventIntensityApi& api){
    float out=-1;
    assert(api.ramp(nullptr,0,1,.5f,&out)&&out==1);
    ServerIntensitySample rows[]={{.5f,.5f,0}};
    assert(api.ramp(rows,1,1,.5f,&out)&&out==.5f);
    assert(api.ramp(rows,1,1,0,&out)&&out==0);
    assert(api.ramp(rows,1,1,1,&out)&&out==0);
    assert(api.ramp(rows,1,1,.25f,&out)&&out>0&&out<.5f);
    struct Fixture {
        alignas(4) unsigned char scene[0x220]{},event[0x3c0]{},tail[0x3c0]{};
        std::uint32_t words[2]{3,4};
        ServerIntensitySample event_rows[1]{{.5f,.5f,0}},scene_rows[1]{{.4f,1.5f,0}};
    } f;
    auto put=[](void* p,unsigned o,auto v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));};
    put(f.event,0x184,1.f);put(f.event,0x188,2.f);put(f.event,0x19c,5u);put(f.event,0x1a8,1);
    put(f.event,0x330,1u);put(f.tail,0x184,3.f);put(f.tail,0x188,-1.f);
    put(f.scene,0,2u);put(f.scene,0xc,2);put(f.scene,0x12c,6u);put(f.scene,0x138,1);
    auto storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        switch(token){
        case 1:*out=f.scene;*n=sizeof(f.scene);break;
        case 2:*out=f.words;*n=sizeof(f.words);break;
        case 3:*out=f.event;*n=sizeof(f.event);break;
        case 4:*out=f.tail;*n=sizeof(f.tail);break;
        case 5:*out=f.event_rows;*n=sizeof(f.event_rows);break;
        case 6:*out=f.scene_rows;*n=sizeof(f.scene_rows);break;
        default:return 0;
        }return 1;
    };
    assert(api.event(f.event,sizeof(f.event),1.5f,&f,storage,&out)&&std::fabs(out-.2f)<.00001f);
    put(f.event,0x330,0u);
    assert(api.event(f.event,sizeof(f.event),1.5f,&f,storage,&out)&&out==.5f);
    put(f.event,0x188,-1.f);
    assert(api.event(f.event,sizeof(f.event),1.5f,&f,storage,&out)&&out==0);
    put(f.event,0x188,2.f);put(f.event,0x1a8,0);
    assert(api.event(f.event,sizeof(f.event),1.5f,&f,storage,&out)&&out==1);
    put(f.event,0x1a8,1);put(f.event,0x19c,99u);out=77;
    assert(!api.event(f.event,sizeof(f.event),1.5f,&f,storage,&out)&&out==77);
}
}
