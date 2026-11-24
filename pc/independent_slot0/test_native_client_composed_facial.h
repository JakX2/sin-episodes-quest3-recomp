#pragma once
#include "native_client_composed_facial.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_client_composed_facial(const ClientComposedFacialApi& api){
    alignas(4) unsigned char entity[client_facial_object_bytes]{};
    auto put=[&](unsigned off,auto v){std::memcpy(entity+off,&v,sizeof(v));};
    auto get=[&](unsigned off){unsigned v=0;std::memcpy(&v,entity+off,4);return v;};
    put(0x860,.25f);put(0xac0,1u);put(0xacc,-1);
    ClientFacialController ctrl{5,0,2};
    ClientFacialFrameBindings blink{};
    blink.lookup=[](void*,const char*,int* out) noexcept -> int {*out=9;return 1;};
    ClientVfePhonemeBindings speech{};
    speech.resolve=[](void*,std::uint32_t,ClientPhonemeSentence*) noexcept -> int {return 0;};
    speech.map=[](void*,const char*) noexcept -> int {return 5;};
    ClientFlexOp op{2,0};ClientFlexRule rule{0,1,&op};
    float previous[1]={1};
    ClientComposedFacialInput in{};
    in.studio_present=true;in.rules_enabled=true;in.smoothing_enabled=true;
    in.now=10;in.blink_duration=.5f;in.window=.1f;in.dt=.033f;
    in.controllers=&ctrl;in.controller_count=1;in.blink=&blink;in.speech=&speech;
    in.rules=&rule;in.rule_count=1;in.descriptor_count=1;in.previous=previous;
    ClientComposedFacialOutput out{};
    assert(api.run(entity,sizeof(entity),&in,&out)==1&&out.descriptors_valid&&out.smoothed_valid);
    assert(out.global[5]==.5f&&out.current[0]==.5f&&std::fabs(out.smoothed[0]-.9f)<.0001f);
    assert(get(0xac8)==1&&get(0xacc)==9&&previous[0]==1);
    put(0xac0,0u);op.arg=7;out.global[5]=77;
    assert(!api.run(entity,sizeof(entity),&in,&out)&&out.global[5]==77&&get(0xac8)==1);
    op.arg=0;in.rules_enabled=false;
    assert(api.run(entity,sizeof(entity),&in,&out)==1&&!out.descriptors_valid&&!out.smoothed_valid&&get(0xac8)==0);
    in.studio_present=false;out.global[5]=99;
    assert(api.run(entity,sizeof(entity),&in,&out)==2&&out.global[5]==99);
    in.studio_present=true;in.controller_count=0;
    assert(api.run(entity,sizeof(entity),&in,&out)==1&&out.global[5]==0&&!out.descriptors_valid);
}
}
