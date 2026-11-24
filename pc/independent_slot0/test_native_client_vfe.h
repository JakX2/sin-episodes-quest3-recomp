#pragma once
#include "native_client_vfe.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
inline void check_client_vfe(const ClientVfeApi& api){
    unsigned char generic[256]{},over[256]{};
    auto make=[](unsigned char* b,const char* name,float weight){
        auto put=[&](unsigned o,int v){std::memcpy(b+o,&v,4);};
        std::memcpy(b,"EFV",4);
        put(0x48,256);put(0x4c,1);put(0x50,108);put(0x58,2);put(0x5c,144);
        put(0x60,1);put(0x64,152);put(0x68,156);
        put(108,52);put(116,1);put(128,24);
        put(132,0);std::memcpy(b+136,&weight,4);
        put(144,-1);put(148,0);put(152,180);put(156,-1);
        std::strcpy(reinterpret_cast<char*>(b+160),name);
        std::strcpy(reinterpret_cast<char*>(b+180),"jaw");
    };
    make(generic,"phoneme",.25f);make(over,"PHONEME",.75f);
    auto map=[](void*,const char* n) noexcept -> int {return std::strcmp(n,"jaw")==0?9:-1;};
    ClientPhonemeContribution rows[2]{};unsigned count=99;
    ClientVfeView g{generic,sizeof(generic)},o{over,sizeof(over)};
    assert(api.resolve(g,{},1,nullptr,map,rows,2,&count)==1&&count==1&&rows[0].global==9&&rows[0].weight==.25f);
    assert(api.resolve(g,o,1,nullptr,map,rows,2,&count)==1&&count==1&&rows[0].weight==.75f);
    assert(api.resolve(g,o,0,nullptr,map,rows,2,&count)==0&&count==0);
    std::strcpy(reinterpret_cast<char*>(over+160),"different");
    assert(api.resolve(g,o,1,nullptr,map,rows,2,&count)==1&&rows[0].weight==.25f);
    assert(api.resolve(g,o,1,nullptr,map,rows,0,&count)==-1&&count==1);
    int bad=99;std::memcpy(generic+132,&bad,4);
    assert(api.resolve(g,o,1,nullptr,map,rows,2,&count)==-1&&rows[0].weight==.25f&&count==1);
    assert(api.resolve({generic,100},o,1,nullptr,map,rows,2,&count)==-1);
    int zero=0,one=1;std::memcpy(generic+132,&zero,4);std::memcpy(generic+112,&one,4);
    // Self-referential selected alias must be rejected rather than loop.
    assert(api.resolve(g,{},1,nullptr,map,rows,2,&count)==-1);
}
}
