#include "sin_compat/original_cache_io_hud_lifecycle.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::phase92;
int main(){
    static_assert(cache_contracts.size()==2 && hud_panel_lifecycle.size()==8);
    assert(cache_contracts[0].float_prefix && !cache_contracts[1].float_prefix);
    assert(cache_contracts[0].count_byte_offset==20);
    assert(cache_contracts[1].count_byte_offset==16);
    assert(cache_contracts[0].entries_byte_offset==8);
    assert(cache_contracts[1].entries_byte_offset==4);
    std::vector<std::string> calls;
    for(const auto& cache:cache_contracts){
        calls.clear();
        describe_original_write(cache,0x3f800000u,2,
            [&](std::uint32_t bits){assert(bits==0x3f800000u);calls.emplace_back("float");},
            [&](std::uint16_t n){assert(n==2);calls.emplace_back("count");},
            [&](std::uint32_t i){calls.emplace_back("lookup"+std::to_string(i));return i+3;},
            [&](std::uint32_t token){calls.emplace_back("write"+std::to_string(token));});
        if(cache.float_prefix)
            assert((calls==std::vector<std::string>{"float","count","lookup0","write3","lookup1","write4"}));
        else
            assert((calls==std::vector<std::string>{"count","lookup0","write3","lookup1","write4"}));
        calls.clear();
        describe_original_read(cache,
            [&](){calls.emplace_back("float");},
            [&](){calls.emplace_back("count");return std::uint16_t(3);},
            [&](std::size_t limit){assert(limit==512);calls.emplace_back("read");return int(calls.size());},
            [&](int marker){calls.emplace_back("lookup");return marker==3?unresolved_token:std::uint32_t(marker);},
            [&](std::uint32_t){calls.emplace_back("append");});
        assert(calls[0]==(cache.float_prefix?"float":"count"));
        assert(calls[cache.float_prefix?1:0]=="count");
    }
    int a=11,b=12;
    for(const auto& panel:hud_panel_lifecycle) {
        PanelPointers state{&a,&b};calls.clear();
        describe_original_hud_shutdown(panel,state,
            [&](void* ptr,int arg,int off){
                assert(ptr && arg==0 && off==0x98);calls.emplace_back("unbind");},
            [&](void* ptr,int arg,int off){
                assert(ptr && arg==1 && off==0x74);calls.emplace_back("release");});
        const bool extra=panel.shape==PanelReleaseShape::DoubleLoadingDisc;
        assert(calls.size()==(extra?4u:2u));
        assert(calls[0]=="unbind" && calls[1]=="release");
        assert((state.offset4==nullptr)==(panel.shape==PanelReleaseShape::SingleNulling));
        assert(state.offset8==&b);
    }
    PanelPointers state{&a,nullptr};calls.clear();
    describe_original_hud_shutdown(hud_panel_lifecycle[0],state,
        [&](void*,int,int){calls.emplace_back("unbind");state.offset4=nullptr;},
        [&](void*,int,int){assert(false && "second call must re-read pointer");});
    assert((calls==std::vector<std::string>{"unbind"}));
    PanelPointers absent{};
    describe_original_hud_shutdown(hud_panel_lifecycle[4],absent,
        [](void*,int,int){assert(false);},[](void*,int,int){assert(false);});
}
