#include "sin_compat/original_client_token_lifecycle.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string_view>
#include <vector>
using namespace sin_compat::original_client_string_tokens;
using namespace sin_compat::original_client_string_tokens::lifecycle;
int main(){
    static_assert(kGlobalTokenObjectNativeBytes==52);
    static_assert(kGlobalNodeArrayNativeBytes==32*12);
    assert(NativeTableDefault::GlobalTokenCaseSensitive!=NativeTableDefault::HudConverterCaseFolded);
    std::vector<Node> nodes(4);
    for(unsigned i=0;i<4;++i){
        nodes[i].parent=static_cast<std::uint16_t>(i+10);
        nodes[i].color=1;
        nodes[i].payload_native32=0x10000u+i;
    }
    ResetState st{};
    assert(rebuild_original_free_chain(nodes,3,st));
    assert(st.root==kMissing&&st.active_count==0&&st.highwater_count==3);
    assert(st.free_head==0);
    assert(nodes[0].smaller==0&&nodes[0].larger==1);
    assert(nodes[1].smaller==1&&nodes[1].larger==2);
    assert(nodes[2].smaller==2&&nodes[2].larger==kMissing);
    assert(nodes[3].smaller==kMissing&&nodes[3].larger==kMissing);
    for(unsigned i=0;i<4;++i){
        assert(nodes[i].parent==i+10 && nodes[i].color==1);
        assert(nodes[i].payload_native32==0x10000u+i);
    }
    assert(rebuild_original_free_chain(nodes,0,st));
    assert(st.root==kMissing&&st.active_count==0&&st.free_head==kMissing);
    assert(rebuild_original_free_chain(nodes,1,st));
    assert(st.free_head==0 && nodes[0].larger==kMissing);
    assert(!rebuild_original_free_chain(nodes,5,st));
    assert(!rebuild_original_free_chain(nodes,0xFFFF,st));

    std::vector<std::vector<std::uint8_t>> pages(2);
    pages[0]={0,0,0,0,0,0,0,0,'H','e','l','l','o',0,'X',0};
    pages[1]={0,0,0,0,0,0,0,0,'W','o','r','l','d',0};
    nodes[0].payload_native32=0; // page=0, offset=0
    nodes[1].payload_native32=6u<<16; // page=0, offset=6
    nodes[2].payload_native32=1; // page=1, offset=0
    assert(string_from_token(nodes,pages,kMissing).value()=="");
    assert(string_from_token(nodes,pages,0).value()=="Hello");
    assert(string_from_token(nodes,pages,1).value()=="X");
    assert(string_from_token(nodes,pages,2).value()=="World");
    assert(!string_from_token(nodes,pages,40).has_value());
    nodes[3].payload_native32=5; // bad page index
    assert(!string_from_token(nodes,pages,3).has_value());
    nodes[3].payload_native32=100u<<16; // bad byte offset
    assert(!string_from_token(nodes,pages,3).has_value());
    pages[0][13]='Y'; // string deliberately unterminated to page end for token=0
    pages[0][15]='Z';
    assert(!string_from_token(nodes,pages,1).has_value());
    std::puts("PASS original global 32-node token initializer, free-list reset, token descriptor -> page+8+C-string and host-only validity guards");
}