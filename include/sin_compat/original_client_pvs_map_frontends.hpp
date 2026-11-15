#pragma once
// Phase236 independent C++17 host reference for original 2006 SiN client.dll
// PVS map frontend methods (insert/search/release). NOT engine-owned classes.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_pvs_map_frontends {

inline constexpr std::uint32_t kInsertVA=0x24108750u;
inline constexpr std::uint32_t kLookupVA=0x24106B20u;
inline constexpr std::uint32_t kReleaseVariantAVA=0x241087C0u;
inline constexpr std::uint32_t kReleaseVariantBVA=0x24108840u;
inline constexpr std::size_t kNewOriginalMethods=4u;
inline constexpr std::size_t kNewOriginalBytes=473u;
inline constexpr std::uint16_t kAbsent=0xFFFFu;
inline constexpr std::size_t kRecordStride=16u;
inline constexpr std::size_t kLeftWordOffset=0u;
inline constexpr std::size_t kRightWordOffset=2u;
inline constexpr std::size_t kPayloadOffset=8u;
inline constexpr std::size_t kManagerStoragePointerDword=4u;
inline constexpr std::size_t kManagerRootWord=0x10u;
inline constexpr std::size_t kManagerCountWord=0x12u;
inline constexpr std::size_t kManagerFreeHeadWord=0x14u;
using Payload=std::array<std::uint8_t,8u>;

// The original insertion writes only two payload DWORDs (+8,+C) after
// delegating 16-byte node header, tree and free-list maintenance to helpers.
// These fields are host reference representations, not original object ABI.
struct Record {
    std::uint16_t left{kAbsent};        // original header+0
    std::uint16_t right{kAbsent};       // original header+2
    std::uint32_t opaque_header{};      // original header+4..+7
    Payload payload{};                  // original header+8..+15
};
static_assert(sizeof(Record)==kRecordStride);
static_assert(offsetof(Record,left)==kLeftWordOffset);
static_assert(offsetof(Record,right)==kRightWordOffset);
static_assert(offsetof(Record,payload)==kPayloadOffset);

struct Map {
    std::vector<Record> nodes;
    std::uint16_t root{kAbsent};
    std::uint16_t live_count{};
    std::uint16_t free_head{kAbsent};
    explicit Map(std::size_t capacity):nodes(capacity) {
        if(capacity>=kAbsent)
            throw std::out_of_range("host map exceeds original 16-bit index domain");
    }
};
inline Record& checked(Map& m,std::uint16_t index) {
    if(index>=m.nodes.size())
        throw std::out_of_range("PVS map index outside synthetic records");
    return m.nodes[index];
}
inline const Record& checked(const Map& m,std::uint16_t index) {
    if(index>=m.nodes.size())
        throw std::out_of_range("PVS map index outside synthetic records");
    return m.nodes[index];
}
struct Comparator {
    virtual ~Comparator()=default;
    // Exact native call order: less(key,node_payload) first; if false,
    // less(node_payload,key) second. Original function pointer is [this+0].
    virtual bool less(const Payload& lhs,const Payload& rhs) const=0;
};

inline std::uint16_t lookup(const Map& m,const Payload& key,
                            const Comparator& comparator) {
    auto index=m.root; // original manager root WORD at +0x10
    std::size_t visited=0u;
    while(index!=kAbsent) {
        // Host guard: original native code does not defend against cycles.
        if(++visited>m.nodes.size())
            throw std::runtime_error("cyclic synthetic PVS map tree");
        const Record& rec=checked(m,index);
        if(comparator.less(key,rec.payload))
            index=rec.left;
        else if(comparator.less(rec.payload,key))
            index=rec.right;
        else
            return index;
    }
    return kAbsent;
}

// The original insertion calls opaque find (0x24106440), node allocation
// (0x24106700), and tree attach (0x241076E0) IN THIS ORDER. Those functions
// must not be fabricated here: an injected host implements their callbacks.
struct Placement {
    std::uint16_t parent{kAbsent};
    bool as_left{};
};
struct InsertionBackend {
    virtual ~InsertionBackend()=default;
    virtual Placement find_placement(Map& map,const Payload& key)=0;
    virtual std::uint16_t allocate_index(Map& map)=0;
    virtual void attach(Map& map,std::uint16_t index,Placement placement)=0;
};
inline std::uint16_t insert(Map& m,const Payload& payload,
                            InsertionBackend& backend) {
    const auto placement=backend.find_placement(m,payload);
    const auto index=backend.allocate_index(m);
    (void)checked(m,index); // host safety before mutation
    backend.attach(m,index,placement);
    m.live_count=static_cast<std::uint16_t>(m.live_count+1u);
    checked(m,index).payload=payload; // native two DWORD copies (+8 and +C)
    return index;
}

struct DeletionBackend {
    virtual ~DeletionBackend()=default;
    // Original opaque tree remover at 0x241083C0.
    virtual void erase_tree_node(Map& map,std::uint16_t index)=0;
};
// Both original wrappers have the same observed effect but distinct native
// x86 VAs, complete code bytes and SHA fingerprints.
inline bool release_index(Map& m,std::uint16_t index,
                          DeletionBackend& backend) {
    if(index==kAbsent)return false; // native fast path skips helper and writes
    (void)checked(m,index);
    backend.erase_tree_node(m,index);
    Record& node=checked(m,index);
    node.left=index;              // original node +0 WORD self/free marker
    node.right=m.free_head;       // original node +2 WORD previous free head
    m.live_count=static_cast<std::uint16_t>(m.live_count-1u);
    m.free_head=index;
    return true;
}
inline bool release_variant_a(Map& m,std::uint16_t index,
                              DeletionBackend& backend) {
    return release_index(m,index,backend);
}
inline bool release_variant_b(Map& m,std::uint16_t index,
                              DeletionBackend& backend) {
    return release_index(m,index,backend);
}
} // namespace sin_compat::original_client_pvs_map_frontends
