#include "sin_compat/original_parent_bone_merge.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace sin_compat::jessica;
void put32(std::vector<std::uint8_t>& b,std::size_t p,std::uint32_t v){
    for(int i=0;i<4;i++)b.at(p+i)=static_cast<std::uint8_t>(v>>(8*i));
}
std::vector<std::uint8_t> fixture(){
    constexpr std::size_t bone_start=0x200,sorted_start=0x190;
    std::vector<std::uint8_t> b(0x200+3*0xd8+0x30);
    put32(b,0x9c,3);
    put32(b,0xa0,bone_start);
    put32(b,0x16c,sorted_start);
    b[sorted_start]=1;b[sorted_start+1]=2;b[sorted_start+2]=0;
    const char* names[]={"Pelvis","arm","Head"};
    for(std::size_t i=0;i<3;i++){
        const auto off=bone_start+i*0xd8;
        put32(b,off,16);
        put32(b,off+4,i?0:0xffffffffu);
        const char* p=names[i];
        for(std::size_t j=0;p[j];j++)b[off+16+j]=static_cast<std::uint8_t>(p[j]);
    }
    return b;
}
void test(){
    auto b=fixture();
    auto s=parse_original_studio_bones(b);
    assert(s && s->bones.size()==3);
    assert(s->bones[0].name=="Pelvis" && s->bones[0].parent_index==-1);
    assert(s->bones[1].name=="arm" && s->bones[1].parent_index==0);
    assert(find_original_bone_by_name(*s,"HEAD")==2);
    assert(find_original_bone_by_name(*s,"pElViS")==0);
    assert(!find_original_bone_by_name(*s,"missing"));
    b=fixture();b.resize(0x100);
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0x9c,257);
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0x9c,0xffffffffu);
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0xa0,0xffffffffu);
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0x16c,0xffffff00u);
    assert(!parse_original_studio_bones(b));
    b=fixture();b[0x190]=0xff;
    assert(!parse_original_studio_bones(b));
    b=fixture();b[0x190]=2;b[0x191]=1;
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0x200+0xd8+4,10);
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0x200,0x7fffffff);
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0x200,0xffffffffu);
    assert(!parse_original_studio_bones(b));
    b=fixture();put32(b,0x200+2*0xd8,0x1000);
    assert(!parse_original_studio_bones(b));
    b=fixture();b[0x200+16]=0;
    assert(!parse_original_studio_bones(b));
}
int main(){test();std::cout<<"Original SiN studiohdr 216-byte bone record parser PASS\n";}
