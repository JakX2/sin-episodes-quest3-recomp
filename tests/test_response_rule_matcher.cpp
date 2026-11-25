#include "../pc/independent_slot0/response_rule_matcher.h"
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::response_rule;
static bool enum_value(void*,const char* key,float* v,bool* found) noexcept {
    *found=!std::strcmp(key,"[NPCState::Combat]");*v=*found?3.f:0.f;return true;
}
int main(){
    Enumeration e{nullptr,enum_value};
    unsigned char m[matcher_bytes]{};
    auto check=[&](const char* expr,const char* actual,bool expected){
        bool result=!expected;assert(compile(expr,e,m,sizeof(m)));
        assert(match(m,sizeof(m),actual,e,result)&&result==expected);
    };
    check("ACT_IDLE","act_idle",true);check("ACT_IDLE","ACT_RUN",false);
    check("!none","weapon_magnum",true);check("!none","NONE",false);
    check("0","0.0",true);check("0","",false);check("!0","",false);
    check("0.0","0",false);check("1suffix","1",true);
    check(">2,<5","2",false);check(">2,<5","3",true);check(">2,<5","5",false);
    check(">=2,<=5","2",true);check(">=2,<=5","5",true);
    check(">=2,<=5","6",false);check(">=2,<=5","nan",true);
    check("[NPCState::Combat]","[NPCState::Combat]",true);
    check("[NPCState::Combat]","[unknown]",false);
    check("0","[unknown]",true);check("!3","[unknown]",true);
    check("","",false);check(nullptr,"any",false);
    check("!=none","NONE",false);
    assert(compile(">1,<9",e,m,sizeof(m))&&m[0]&&m[1]&&m[3]&&m[0xc]);
    assert(read_float(m,8)==1.f&&read_float(m,0x10)==9.f);
    const std::string big(128,'x');assert(!compile(big.c_str(),e,m,sizeof(m)));
    bool result=false;assert(!match(m,1,"1",e,result));
}
