#include "sin_compat/original_client_converter_tree.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <vector>
using namespace sin_compat::original_client_converter;
int main() {
    assert(!less_than_16(0,0));
    assert(less_than_16(0,65535));
    assert(!less_than_16(65535,0));
    assert(sizeof(NodeRecord)==24);
    // Index 0 is root 0x4000, with two 24-byte children; compare query
    // below node first and reverse node/query second, as original x86 does.
    std::vector<NodeRecord> nodes(3);
    nodes[0].token=0x4000;
    nodes[0].smaller_child=1;
    nodes[0].larger_child=2;
    nodes[1].token=0x0100;
    nodes[2].token=0xF000;
    auto got=find_original_token_tree(nodes,0,0x4000);
    assert(got.status==LookupStatus::Found&&got.node_index==0);
    got=find_original_token_tree(nodes,0,0x0100);
    assert(got.status==LookupStatus::Found&&got.node_index==1);
    got=find_original_token_tree(nodes,0,0xF000);
    assert(got.status==LookupStatus::Found&&got.node_index==2);
    got=find_original_token_tree(nodes,0,0x0500);
    assert(got.status==LookupStatus::NotFound);
    got=find_original_token_tree(nodes,0,0xFFFF);
    assert(got.status==LookupStatus::NotFound);
    got=find_original_token_tree(nodes,-1,0);
    assert(got.status==LookupStatus::NotFound);
    got=find_original_token_tree(nodes,-2,0);
    assert(got.status==LookupStatus::InvalidHostTree);
    got=find_original_token_tree(nodes,12,0);
    assert(got.status==LookupStatus::InvalidHostTree);
    nodes[0].smaller_child=0; // host-only cycle guard
    got=find_original_token_tree(nodes,0,0x0010);
    assert(got.status==LookupStatus::InvalidHostTree);
    std::puts("PASS original SiN converter 16-bit token order, 24-byte entry tree, missing and malformed host tree");
}