#pragma once
#include "native_server_flex_weight.h"
#include <cassert>
#include <cstring>
#include <cmath>
#include <limits>
namespace sin_port::native_weapon_tables::test {
inline void check_server_flex_weight(const ServerFlexWeightApi& api){
    alignas(4) unsigned char entity[server_flex_extent]{};
    auto put=[&](unsigned off,auto v){std::memcpy(entity+off,&v,sizeof(v));};
    auto raw=[&](unsigned i){float v=0;std::memcpy(&v,entity+server_flex_weight_offset+i*4,4);return v;};
    struct Context {unsigned calls{},flags{};bool reject{};} ctx;
    ServerFlexWeightBindings b{&ctx,[](void* p,void*,std::uint32_t token) noexcept -> int{
        auto& x=*static_cast<Context*>(p);assert(token==123);++x.calls;
        if(x.reject)return 0;x.flags|=1;return 1;
    }};
    ServerFlexRange rows[]={{-2,2},{3,3},{2,-2}};
    put(0x16c,123u);float value=99;
    assert(api.set(entity,sizeof(entity),rows,3,0,1,&b)&&raw(0)==.75f&&ctx.calls==1&&ctx.flags==1);
    assert(api.get(entity,sizeof(entity),rows,3,0,&value)&&value==1);
    assert(api.set(entity,sizeof(entity),rows,3,0,1,&b)&&ctx.calls==1);
    assert(api.set(entity,sizeof(entity),rows,3,0,99,&b)&&raw(0)==1&&ctx.calls==2);
    assert(api.set(entity,sizeof(entity),rows,3,0,-99,&b)&&raw(0)==0&&ctx.calls==3);
    assert(api.set(entity,sizeof(entity),rows,3,1,4.5f,&b)&&raw(1)==4.5f&&ctx.calls==4);
    assert(api.get(entity,sizeof(entity),rows,3,1,&value)&&value==4.5f);
    put(0x1f0,static_cast<unsigned char>(1));
    assert(api.set(entity,sizeof(entity),rows,3,2,1,&b)&&raw(2)==.25f&&ctx.calls==4&&entity[0x200]==1);
    assert(api.get(entity,sizeof(entity),rows,3,2,&value)&&value==1);
    assert(api.set(entity,sizeof(entity),rows,3,-1,1,&b)&&ctx.calls==4);
    assert(api.get(entity,sizeof(entity),rows,3,3,&value)&&value==0);
    put(0x1f0,static_cast<unsigned char>(0));ctx.reject=true;
    assert(!api.set(entity,sizeof(entity),rows,3,0,2,&b)&&raw(0)==0&&ctx.calls==5);
    ctx.reject=false;
    const float nan=std::numeric_limits<float>::quiet_NaN();
    assert(api.set(entity,sizeof(entity),rows,3,1,nan,&b)&&std::isnan(raw(1))&&ctx.calls==6);
    assert(api.set(entity,sizeof(entity),rows,3,1,nan,&b)&&ctx.calls==7);
    assert(api.get(entity,sizeof(entity),nullptr,0,0,&value)&&value==0);
}
}
