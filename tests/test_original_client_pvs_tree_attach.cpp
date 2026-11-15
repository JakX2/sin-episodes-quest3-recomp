#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_pvs_tree_attach.hpp"
#include "sin_compat/original_client_pvs_map_node_growth.hpp"

namespace tree = sin_compat::original_client_pvs_tree_attach;
namespace map = tree::map;
namespace idx = tree::index;
namespace growth = sin_compat::original_client_pvs_storage_growth;
namespace nodes = sin_compat::original_client_pvs_map_node_growth;

static map::Payload key(std::uint32_t n) {
    return map::Payload{{
        static_cast<std::uint8_t>(n),
        static_cast<std::uint8_t>(n >> 8),
        static_cast<std::uint8_t>(n >> 16),
        static_cast<std::uint8_t>(n >> 24),0,0,0,0
    }};
}
static std::uint32_t key_number(const map::Payload& p) {
    return static_cast<std::uint32_t>(p[0]) |
        (static_cast<std::uint32_t>(p[1]) << 8) |
        (static_cast<std::uint32_t>(p[2]) << 16) |
        (static_cast<std::uint32_t>(p[3]) << 24);
}
struct Less final : map::Comparator {
    bool less(const map::Payload& a, const map::Payload& b) const override {
        return key_number(a) < key_number(b);
    }
};
struct Memory final : growth::IMemoryBackend {
    std::vector<std::size_t> requested_bytes;
    bool request(growth::MemoryAction, std::size_t bytes) override {
        requested_bytes.push_back(bytes);
        return true;
    }
};
struct Observed {
    std::uint16_t index{};
    std::uint16_t parent{};
    std::uint16_t left{};
    std::uint16_t right{};
    std::uint16_t state{};
    std::uint16_t root{};
    std::uint32_t payload_before_frontend_copy{};
};
struct BalanceSpy final : tree::BalancingBackend {
    std::vector<Observed> calls;
    void balance_after_attach(map::Map& manager, std::uint16_t index) override {
        const auto& r = map::checked(manager,index);
        calls.push_back({
            index,
            static_cast<std::uint16_t>(r.opaque_header & 0xFFFFu),
            r.left,r.right,
            static_cast<std::uint16_t>(r.opaque_header >> 16),
            manager.root,
            key_number(r.payload)
        });
    }
};
struct LeafErase final : map::DeletionBackend {
    void erase_tree_node(map::Map& manager, std::uint16_t index) override {
        const auto& node = map::checked(manager,index);
        if(node.left != map::kAbsent || node.right != map::kAbsent)
            throw std::logic_error("synthetic leaf-only erase");
        if(manager.root == index) {
            manager.root = map::kAbsent;
            return;
        }
        for(auto& r : manager.nodes) {
            if(r.left == index) {
                r.left = map::kAbsent;
                return;
            }
            if(r.right == index) {
                r.right = map::kAbsent;
                return;
            }
        }
        throw std::logic_error("synthetic leaf not linked");
    }
};

static void test_native_layout_and_entrypoint_evidence() {
    static_assert(tree::kOriginalAttachVA == 0x241076E0u);
    static_assert(tree::kOpaqueOriginalBalancerVA == 0x24106E30u);
    static_assert(tree::kWholeOriginalMethodBytes == 106u);
    static_assert(tree::kOriginalNativeRET12Paths == 3u);
    static_assert(tree::kNodeStride == 16u);
    static_assert(tree::kNodeLeftWORD == 0u);
    static_assert(tree::kNodeRightWORD == 2u);
    static_assert(tree::kNodeParentWORD == 4u);
    static_assert(tree::kNodeStateWORD == 6u);
    static_assert(tree::kMapStorageDWORD == 4u);
    static_assert(tree::kMapRootWORD == 0x10u);
    static_assert(offsetof(map::Record, opaque_header) == 4u);
    static_assert(sizeof(map::Record) == 16u);
}
static void test_native_root_case_preserves_unwritten_payload() {
    map::Map manager(4u);
    manager.nodes[2].payload = key(77u);
    manager.nodes[2].opaque_header = 0xBAADF00Du;
    BalanceSpy balancing;
    tree::NativeTreeAttach attach(balancing);
    attach.attach(manager,2u,{map::kAbsent,true});
    assert(manager.root == 2u);
    assert(manager.nodes[2].left == map::kAbsent);
    assert(manager.nodes[2].right == map::kAbsent);
    assert(manager.nodes[2].opaque_header == 0x0000FFFFu);
    assert(manager.nodes[2].payload == key(77u));
    assert(balancing.calls.size() == 1u);
    const auto& event=balancing.calls[0];
    assert(event.index == 2u && event.root == 2u);
    assert(event.parent == map::kAbsent);
    assert(event.state == 0u);
    assert(event.payload_before_frontend_copy == 77u);
}
static void test_native_left_and_right_cases() {
    map::Map manager(4u);
    manager.root = 0u;
    manager.nodes[0].payload = key(20u);
    BalanceSpy balancing;
    tree::NativeTreeAttach attach(balancing);
    attach.attach(manager,1u,{0u,true});
    assert(manager.root == 0u);
    assert(manager.nodes[0].left == 1u);
    assert(manager.nodes[0].right == map::kAbsent);
    assert(manager.nodes[1].opaque_header == 0u);
    attach.attach(manager,2u,{0u,false});
    assert(manager.nodes[0].right == 2u);
    assert(manager.nodes[2].opaque_header == 0u);
    assert(balancing.calls.size() == 2u);
    assert(balancing.calls[0].index == 1u);
    assert(balancing.calls[1].index == 2u);
    assert(balancing.calls[0].root == 0u);
    assert(balancing.calls[1].root == 0u);
}
static void test_host_rejects_invalid_inputs_without_balancing() {
    map::Map manager(3u);
    BalanceSpy balancing;
    tree::NativeTreeAttach attach(balancing);
    bool rejected=false;
    try {attach.attach(manager,map::kAbsent,{map::kAbsent,false});}
    catch(const std::out_of_range&) {rejected=true;}
    assert(rejected);
    rejected=false;
    try {attach.attach(manager,3u,{map::kAbsent,false});}
    catch(const std::out_of_range&) {rejected=true;}
    assert(rejected);
    rejected=false;
    try {attach.attach(manager,1u,{99u,false});}
    catch(const std::out_of_range&) {rejected=true;}
    assert(rejected);
    rejected=false;
    try {attach.attach(manager,1u,{1u,true});}
    catch(const std::out_of_range&) {rejected=true;}
    assert(rejected);
    assert(manager.root == map::kAbsent && balancing.calls.empty());
    assert(manager.nodes[1].opaque_header == 0u);
}
static void test_integrated_growth_insert_lookup_release_reinsert() {
    map::Map manager(0u);
    Memory memory;
    nodes::ResolverState resolver;
    nodes::NativeIndexCallbacks allocation(manager,memory,resolver);
    idx::IndexState indices;
    Less comparator;
    BalanceSpy balancing;
    tree::NativeTreeAttach attach(balancing);
    idx::InsertBridge bridge(indices,allocation,comparator,attach);
    assert(map::insert(manager,key(20u),bridge) == 0u);
    assert(map::insert(manager,key(10u),bridge) == 1u);
    assert(map::insert(manager,key(30u),bridge) == 2u);
    assert((memory.requested_bytes == std::vector<std::size_t>{32u,64u}));
    assert(manager.root == 0u);
    assert(manager.nodes[0].left == 1u);
    assert(manager.nodes[0].right == 2u);
    assert(manager.nodes[0].opaque_header == 0x0000FFFFu);
    assert(manager.nodes[1].opaque_header == 0u);
    assert(manager.nodes[2].opaque_header == 0u);
    assert(manager.live_count == 3u);
    assert(map::lookup(manager,key(30u),comparator) == 2u);
    assert(balancing.calls.size() == 3u);
    assert(balancing.calls[0].payload_before_frontend_copy == 0u);
    assert(balancing.calls[1].payload_before_frontend_copy == 0u);
    assert(balancing.calls[2].payload_before_frontend_copy == 0u);
    assert(balancing.calls[0].parent == map::kAbsent);
    assert(balancing.calls[1].parent == 0u);
    assert(balancing.calls[2].parent == 0u);
    LeafErase deletion;
    assert(map::release_variant_b(manager,2u,deletion));
    assert(manager.free_head == 2u);
    assert(manager.nodes[2].left == 2u);
    // Reuse the released record; the native attach callback must execute
    // BEFORE the Phase236 payload copy overwrites the previous key 30.
    assert(map::insert(manager,key(25u),bridge) == 2u);
    assert(balancing.calls.size() == 4u);
    assert(balancing.calls[3].index == 2u);
    assert(balancing.calls[3].parent == 0u);
    assert(balancing.calls[3].payload_before_frontend_copy == 30u);
    assert(manager.nodes[2].payload == key(25u));
    assert(manager.nodes[2].left == map::kAbsent);
    assert(manager.nodes[2].right == map::kAbsent);
    assert(manager.nodes[2].opaque_header == 0u);
    assert(manager.root == 0u && manager.nodes[0].right == 2u);
    assert(map::lookup(manager,key(25u),comparator) == 2u);
    assert(manager.live_count == 3u && manager.free_head == map::kAbsent);
    assert(allocation.recycle_resolutions == 1u);
    assert(indices.next_unused == 3u && indices.cached_base_refreshes == 4u);
    assert(resolver.initializations == 1u);
    assert(memory.requested_bytes.size() == 2u);
}
int main() {
    test_native_layout_and_entrypoint_evidence();
    test_native_root_case_preserves_unwritten_payload();
    test_native_left_and_right_cases();
    test_host_rejects_invalid_inputs_without_balancing();
    test_integrated_growth_insert_lookup_release_reinsert();
    return 0;
}
