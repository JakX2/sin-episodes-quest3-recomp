#pragma once
#include "native_client_flex_rule.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_client_flex_rule(const ClientFlexRuleApi& api){
    auto literal=[](float f){ClientFlexOp op{1,0};std::memcpy(&op.arg,&f,4);return op;};
    // (2 + 3) * 4 - 1 = 19; next rule reads prior descriptor and divides.
    ClientFlexOp a[]={{2,0},literal(3),{4,0},{2,1},{6,0},literal(1),{5,0}};
    ClientFlexOp b[]={{3,0},literal(2),{7,0}};
    ClientFlexOp c[]={literal(7),literal(-2),{7,0}};
    ClientFlexOp d[]={literal(7),literal(.0001f),{7,0}};
    ClientFlexOp e[]={literal(7),literal(.0002f),{7,0}};
    ClientFlexOp f[]={literal(3),{99,0},literal(8)}; // base of stack wins
    ClientFlexRule rules[]={{0,7,a},{1,3,b},{2,3,c},{3,3,d},{4,3,e},{5,3,f}};
    int map[]={17,8};float global[128]{};global[17]=2;global[8]=4;
    float out[8];for(auto& v:out)v=99;
    assert(api.evaluate(false,nullptr,0,nullptr,0,nullptr,out,8)&&out[0]==99);
    assert(api.evaluate(true,rules,6,map,2,global,out,8));
    assert(out[0]==19&&out[1]==9.5f&&out[2]==0&&out[3]==0&&std::fabs(out[4]-35000.f)<.01f&&out[5]==3&&out[6]==0&&out[7]==0);
    map[0]=128;out[0]=77;
    assert(!api.evaluate(true,rules,6,map,2,global,out,8)&&out[0]==77);
    map[0]=17;
    ClientFlexOp invalid[]={{4,0}};
    ClientFlexRule bad{0,1,invalid};
    assert(!api.evaluate(true,&bad,1,map,2,global,out,8)&&out[0]==77);
    assert(api.evaluate(true,nullptr,0,map,2,global,out,8));
    for(float v:out)assert(v==0);
}
}
