#include "sin_compat/original_ordinary_bone.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace sin_compat::jessica;
void put(std::vector<std::uint8_t>& b,std::size_t p,std::uint32_t v){
    for(int i=0;i<4;++i)b.at(p+i)=static_cast<std::uint8_t>(v>>(8*i));
}
int main(){
    std::vector<std::uint8_t> b(0x200+0xd8*3,0);
    put(b,0x9c,3);put(b,0xa0,0x200);
    put(b,0x200+0xa0,0x100);
    put(b,0x200+0xd8+0xa0,0x200);
    put(b,0x200+2*0xd8+0xa0,0x300);
    auto r=parse_original_studio_bone_flags(b);
    assert(r&&r->size()==3&&(*r)[0]==0x100&&(*r)[1]==0x200&&(*r)[2]==0x300);
    put(b,0x9c,0xffffffff);
    assert(!parse_original_studio_bone_flags(b));
    put(b,0x9c,3);put(b,0xa0,0xffffffff);
    assert(!parse_original_studio_bone_flags(b));
    put(b,0xa0,0x300);
    assert(!parse_original_studio_bone_flags(b));
    put(b,0xa0,0x200);put(b,0x9c,257);
    assert(!parse_original_studio_bone_flags(b));
    assert(!parse_original_studio_bone_flags(std::vector<std::uint8_t>(0x20)));
    b.resize(0x200+0xd8*3-1);put(b,0x9c,3);
    assert(!parse_original_studio_bone_flags(b));
    std::cout<<"Original SiN studio bone-mask metadata parser tests PASS\n";
}
