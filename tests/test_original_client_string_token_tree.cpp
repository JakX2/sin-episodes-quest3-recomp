#include "sin_compat/original_client_string_token_tree.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <vector>
using namespace sin_compat::original_client_string_tokens;
int main() {
    static_assert(sizeof(Node)==12);
    Node native{};
    assert(native.smaller==0xFFFF&&native.larger==0xFFFF);
    assert(native.parent==0xFFFF&&native.color==0);
    std::vector<Node> nodes(3);
    nodes[0].payload_native32=10;nodes[0].color=1;
    nodes[0].smaller=1;nodes[0].larger=2;
    nodes[1].payload_native32=4;nodes[1].parent=0;
    nodes[2].payload_native32=20;nodes[2].parent=0;
    auto less=[](std::uint32_t a,std::uint32_t b){return a<b;};
    for(const auto i:{0u,1u,2u}){
        const auto result=find_token(nodes,0,nodes[i].payload_native32,less);
        assert(result.status==Status::Found&&result.index==i);
    }
    auto result=find_token(nodes,0,16,less);
    assert(result.status==Status::NotFound);
    result=find_token(nodes,kMissing,9,less);
    assert(result.status==Status::NotFound);
    result=find_insertion_position(nodes,kMissing,9,less);
    assert(result.status==Status::InsertPosition&&result.index==kMissing&&!result.smaller);
    result=find_insertion_position(nodes,0,3,less);
    assert(result.status==Status::InsertPosition&&result.index==1&&result.smaller);
    result=find_insertion_position(nodes,0,8,less);
    assert(result.status==Status::InsertPosition&&result.index==1&&!result.smaller);
    result=find_insertion_position(nodes,0,19,less);
    assert(result.status==Status::InsertPosition&&result.index==2&&result.smaller);
    // Duplicate 10 follows the larger branch in native insert-position code.
    result=find_insertion_position(nodes,0,10,less);
    assert(result.status==Status::InsertPosition&&result.index==2&&result.smaller);
    result=find_token(nodes,900,1,less);
    assert(result.status==Status::InvalidHostTree);
    nodes[1].smaller=1; // only independent host safety rejects this cycle
    result=find_token(nodes,0,3,less);
    assert(result.status==Status::InvalidHostTree);
    std::puts("PASS original 12-byte token node, ushort parent/color, two-comparison lookup, single-comparison insertion, host-only safeguards");
}