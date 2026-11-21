#pragma once
// Source-grounded 2006 client.dll 12-byte string-token RB-tree record and
// comparator-driven lookup/insert-slot control flow. Original engine bytes,
// x86 pointers and original native comparator are NOT embedded/called.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_compat::original_client_string_tokens {
inline constexpr std::uint16_t kMissing=0xFFFF;

struct Node final {
    std::uint16_t smaller=kMissing;       // original +0x00
    std::uint16_t larger=kMissing;        // original +0x02
    std::uint16_t parent=kMissing;        // original +0x04
    std::uint16_t color=0;                // original +0x06: newly linked 0, final root 1
    std::uint32_t payload_native32=0;    // original +0x08: no native pointer dereference!
};
static_assert(sizeof(Node)==12,"original node stride");
static_assert(offsetof(Node,smaller)==0);
static_assert(offsetof(Node,larger)==2);
static_assert(offsetof(Node,parent)==4);
static_assert(offsetof(Node,color)==6);
static_assert(offsetof(Node,payload_native32)==8);

enum class Status : std::uint8_t { Found, NotFound, InsertPosition, InvalidHostTree };
struct Result final {
    Status status=Status::InvalidHostTree;
    std::uint16_t index=kMissing;
    bool smaller=false;
};

// The original +0x3E20 lookup invokes the native comparator twice (query,
// node) then (node,query). A host callback deliberately supplies semantics;
// an integer order in a synthetic test does NOT imply the native callback's
// byte/case/locale semantics are recovered.
template<class Less>
Result find_token(const std::vector<Node>& nodes,std::uint16_t root,
                  std::uint32_t query,Less less) {
    std::uint16_t idx=root;
    for(std::size_t iter=0;iter<=nodes.size();++iter){
        if(idx==kMissing)return {Status::NotFound,kMissing,false};
        if(static_cast<std::size_t>(idx)>=nodes.size())return {};
        const auto& n=nodes[idx];
        if(less(query,n.payload_native32))idx=n.smaller;
        else if(less(n.payload_native32,query))idx=n.larger;
        else return {Status::Found,idx,false};
    }
    return {};
}

// The original +0x3750 insertion search invokes the comparator only once
// per visited node, then outputs parent index and branch side. Equality goes
// along the larger-child path; the caller must reject duplicates beforehand.
template<class Less>
Result find_insertion_position(const std::vector<Node>& nodes,std::uint16_t root,
                               std::uint32_t query,Less less) {
    std::uint16_t idx=root,parent=kMissing;
    bool smaller=false;
    for(std::size_t iter=0;iter<=nodes.size();++iter){
        if(idx==kMissing)return {Status::InsertPosition,parent,smaller};
        if(static_cast<std::size_t>(idx)>=nodes.size())return {};
        parent=idx;
        smaller=less(query,nodes[idx].payload_native32);
        idx=smaller?nodes[idx].smaller:nodes[idx].larger;
    }
    return {};
}
} // namespace sin_compat::original_client_string_tokens