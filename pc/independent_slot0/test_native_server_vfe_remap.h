#pragma once
#include "native_server_vfe_remap.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
inline void check_server_vfe_remap(const ServerVfeRemapApi& api){
    alignas(4) unsigned char bytes[256]{},other[256]{};
    auto put=[&](unsigned off,int value){std::memcpy(bytes+off,&value,4);};
    std::memcpy(bytes,"EFV",4);put(0x48,256);put(0x60,3);put(0x64,108);put(0x68,120);
    put(108,160);put(112,180);put(116,200);
    std::strcpy(reinterpret_cast<char*>(bytes+160),"JAW");
    std::strcpy(reinterpret_cast<char*>(bytes+180),"not_present");
    std::strcpy(reinterpret_cast<char*>(bytes+200),"BROW");
    std::memcpy(other,bytes,sizeof(bytes));
    const char* names[]={"mouth","jaw","brow","JAW"};
    const char* alternate[]={"brow","mouth","jaw","JAW"};
    int out=-1;
    assert(api.find(names,4,"JaW",&out)&&out==1);
    assert(api.find(names,4,"missing",&out)&&out==0);
    assert(api.find(nullptr,0,"jaw",&out)&&out==0);
    ServerVfeRemapEntry entries[2]{};ServerVfeRemapCache cache{entries,0,2};
    ServerExpressionBank a{bytes,sizeof(bytes)},b{other,sizeof(other)};
    assert(api.lookup(&cache,a,names,4,0,&out)&&out==1&&cache.count==1&&entries[0].keys==3);
    assert(api.lookup(&cache,a,alternate,4,2,&out)&&out==2&&cache.count==1);
    assert(api.lookup(&cache,a,nullptr,0,1,&out)&&out==0&&cache.count==1);
    assert(api.lookup(&cache,b,alternate,4,0,&out)&&out==2&&cache.count==2);
    assert(api.lookup(&cache,b,alternate,4,2,&out)&&out==0);
    out=99;assert(!api.lookup(&cache,a,names,4,3,&out)&&out==99);
    assert(api.clear(&cache)&&cache.count==0);
    assert(api.lookup(&cache,a,alternate,4,0,&out)&&out==2&&cache.count==1);
    assert(api.clear(&cache));put(116,257);
    out=99;assert(!api.lookup(&cache,a,names,4,0,&out)&&out==99&&cache.count==0);
}
}
