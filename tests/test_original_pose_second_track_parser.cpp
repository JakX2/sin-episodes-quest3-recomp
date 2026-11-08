#include "sin_compat/original_pose_second_pass_state.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>
using namespace sin_compat::jessica;
void put(std::vector<std::uint8_t>& b,std::size_t x,std::uint32_t y){
 for(int i=0;i<4;i++)b.at(x+i)=std::uint8_t(y>>(8*i));
}
int main(){
    std::vector<std::uint8_t> bytes(0xe4*2,0);
    bytes[0xa0]=1;bytes[0xa1]=14;
    put(bytes,0xa4,0xfedcba98);put(bytes,0xa8,0xffffffff);
    bytes[0xe4+0xa0]=0;bytes[0xe4+0xa1]=1;
    put(bytes,0xe4+0xa4,0x12345678);put(bytes,0xe4+0xa8,3);
    auto r=parse_original_pose_second_track_state(bytes);
    assert(r && r->full_cap_mode && r->history_active==14);
    assert(r->history_accumulator_bits==0xfedcba98 && r->last_sequence==-1);
    r=parse_original_pose_second_track_state(bytes,0xe4);
    assert(r && !r->full_cap_mode && r->history_active==1);
    assert(r->history_accumulator_bits==0x12345678 && r->last_sequence==3);
    assert(!parse_original_pose_second_track_state(bytes,1+0xe4));
    assert(!parse_original_pose_second_track_state(bytes,bytes.size()+1));
    assert(!parse_original_pose_second_track_state(bytes,
        std::numeric_limits<std::size_t>::max()));
    std::cout<<"Original SiN second pass track metadata parser PASS\n";
}
