#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <random>
// Simulate Windows legacy keywords even in the portable CI.
#define near
#define far
#include "../pc/independent_slot0/choreography_indexed_rb.h"
#undef far
#undef near

template<class I,unsigned Stride> void trial(){
    using Tree=sin_port::choreography_index::Tree<I,Stride>;
    std::mt19937 rng(938);
    std::array<I,64> order{};
    for(unsigned i=0;i<order.size();++i)order[i]=static_cast<I>(i);
    for(unsigned round=0;round<1000;++round){
        std::array<unsigned char,64*Stride> bytes{};
        Tree t{bytes.data(),64,Tree::nil};
        std::shuffle(order.begin(),order.end(),rng);
        unsigned live=0;
        auto less=[](I a,I b){return a<b;};
        for(I z:order){
            I parent=Tree::nil,x=t.root;unsigned side=0;
            while(x!=Tree::nil){parent=x;side=z<x?0u:1u;x=t.get(x,side);}
            t.link(z,parent,side);
            const auto marker=static_cast<std::uint32_t>(0xabc00000u+z);
            std::memcpy(bytes.data()+static_cast<unsigned>(z)*Stride+4*sizeof(I),&marker,sizeof(marker));
            assert(t.valid(++live,less));
        }
        std::shuffle(order.begin(),order.end(),rng);
        for(I z:order){
            t.erase(z);
            assert(t.valid(--live,less));
            // No surviving node payload was moved to a different index.
            for(unsigned i=0;i<64;++i){
                std::uint32_t marker=0;
                std::memcpy(&marker,bytes.data()+i*Stride+4*sizeof(I),sizeof(marker));
                assert(marker==0xabc00000u+i);
            }
        }
        assert(t.root==Tree::nil);
    }
}
int main(){trial<std::uint32_t,24>();trial<std::uint16_t,16>();}
