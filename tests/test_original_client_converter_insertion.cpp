#include "sin_compat/original_client_converter_tree.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <vector>
using namespace sin_compat::original_client_converter;
int main(){
    static_assert(sizeof(NodeRecord)==24);
    static_assert(offsetof(NodeRecord,parent_index)==0x08);
    static_assert(offsetof(NodeRecord,color)==0x0C);
    NodeRecord n{};
    assert(n.parent_index==-1 && n.color==0);
    std::vector<NodeRecord> nodes(3);
    nodes[0].token=0x4000;
    nodes[0].smaller_child=1;
    nodes[0].larger_child=2;
    nodes[0].color=1; // root is forced to one by original fixup
    nodes[1].token=0x0100;nodes[1].parent_index=0;
    nodes[2].token=0xF000;nodes[2].parent_index=0;
    auto s=locate_original_converter_insert_slot(nodes,-1,0x9999);
    assert(s.status==InsertSlotStatus::Position&&s.parent_index==-1&&!s.smaller_child);
    s=locate_original_converter_insert_slot(nodes,0,0x0010);
    assert(s.status==InsertSlotStatus::Position&&s.parent_index==1&&s.smaller_child);
    s=locate_original_converter_insert_slot(nodes,0,0x2000);
    assert(s.status==InsertSlotStatus::Position&&s.parent_index==1&&!s.smaller_child);
    s=locate_original_converter_insert_slot(nodes,0,0xFFFF);
    assert(s.status==InsertSlotStatus::Position&&s.parent_index==2&&!s.smaller_child);
    s=locate_original_converter_insert_slot(nodes,0,0xC000);
    assert(s.status==InsertSlotStatus::Position&&s.parent_index==2&&s.smaller_child);
    // The recovered insertion helper does not independently reject duplicate keys.
    s=locate_original_converter_insert_slot(nodes,0,0x4000);
    assert(s.status==InsertSlotStatus::Position&&s.parent_index==2&&s.smaller_child);
    s=locate_original_converter_insert_slot(nodes,-2,0x4000);
    assert(s.status==InsertSlotStatus::InvalidHostTree);
    s=locate_original_converter_insert_slot(nodes,900,0x4000);
    assert(s.status==InsertSlotStatus::InvalidHostTree);
    nodes[1].smaller_child=1; // synthetic cycle rejected by host-only guard
    s=locate_original_converter_insert_slot(nodes,0,0x0010);
    assert(s.status==InsertSlotStatus::InvalidHostTree);
    std::puts("PASS original converter insert parent/side, duplicate-right direction, 24-byte parent/color fields, safety guards");
}