#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "sin_compat/original_server_cache_tree_rotations.hpp"
#include "sin_compat/original_server_cache_balance_preflight.hpp"

namespace r = sin_compat::original_server_cache_tree_rotations;
namespace t = sin_compat::original_server_cache_tree_lookup;
namespace b = sin_compat::original_server_cache_balance_preflight;

static_assert(sizeof(t::NativeSlot32) == 24u);
static_assert(std::is_trivially_copyable<t::NativeSlot32>::value);

static void assign(t::Tree& tree, std::uint16_t index, std::uint32_t key,
                   std::uint16_t parent, std::uint16_t color) {
    auto& n = tree.nodes[index];
    n.key_08 = key;
    n.unknown_04 = static_cast<std::uint32_t>(parent)
                 | (static_cast<std::uint32_t>(color) << 16u);
    n.payload_14 = 0xbeef0000u + index;
}
static void gather(const t::Tree& tree, std::uint16_t index,
                   std::vector<std::uint32_t>& out, std::size_t fuel) {
    if (index == r::kNil) return;
    assert(fuel > 0u && index < tree.nodes.size());
    const auto& n = tree.nodes[index];
    gather(tree, n.left_00, out, fuel-1u);
    out.push_back(n.key_08);
    gather(tree, n.right_02, out, fuel-1u);
}
static std::vector<std::uint32_t> sorted_keys(const t::Tree& tree) {
    std::vector<std::uint32_t> out;
    gather(tree, tree.root, out, tree.nodes.size()+1u);
    return out;
}
static void verify_original_fields_unchanged(const t::Tree& before,
                                             const t::Tree& after) {
    assert(before.nodes.size() == after.nodes.size());
    for (std::size_t i = 0; i < before.nodes.size(); ++i) {
        const auto& a = before.nodes[i]; const auto& z = after.nodes[i];
        assert(a.key_08 == z.key_08 && a.payload_14 == z.payload_14);
        assert(r::color(a) == r::color(z));
        assert(std::memcmp(a.unknown_0c,z.unknown_0c,8)==0);
    }
}
static void expect_reject_unchanged(t::Tree tree, std::uint32_t index,
                                    const r::Config& cfg) {
    const auto initial_root = tree.root;
    const auto original_nodes = tree.nodes;
    bool rejected = false;
    try { r::rotate(tree,index,cfg); }
    catch (const std::out_of_range&) { rejected = true; }
    assert(rejected);
    assert(tree.root == initial_root);
    assert(tree.nodes.size() == original_nodes.size());
    assert(std::memcmp(tree.nodes.data(),original_nodes.data(),
                       original_nodes.size()*sizeof(t::NativeSlot32)) == 0);
}
static void root_rotation(const r::Config& cfg) {
    const bool left = cfg.direction == r::Direction::left;
    t::Tree tree;
    tree.nodes.resize(3u);
    tree.root = 0u;
    assign(tree,0u,20u,r::kNil,b::kBlack);
    assign(tree,1u,left ? 30u : 10u,0u,b::kRed);
    assign(tree,2u,20u,r::kNil,b::kBlack);
    // Distinct sorted three-key tree with middle beta node.
    tree.nodes[0].key_08 = left ? 10u : 30u;
    tree.nodes[2].key_08 = 20u;
    if(left) {
        tree.nodes[0].right_02=1u;
        tree.nodes[1].left_00=2u;
    } else {
        tree.nodes[0].left_00=1u;
        tree.nodes[1].right_02=2u;
    }
    r::set_parent(tree.nodes[2],1u);
    const auto before = tree;
    const auto keys = sorted_keys(tree);
    assert((keys==std::vector<std::uint32_t>{10u,20u,30u}));
    r::rotate(tree,0x10000u,cfg); // original routine narrows to low 16 bits
    assert(tree.root==1u);
    assert(r::parent(tree.nodes[1])==r::kNil);
    assert(r::parent(tree.nodes[0])==1u);
    assert(r::parent(tree.nodes[2])==0u);
    if(left) {
        assert(tree.nodes[1].left_00==0u);
        assert(tree.nodes[0].right_02==2u);
    } else {
        assert(tree.nodes[1].right_02==0u);
        assert(tree.nodes[0].left_00==2u);
    }
    assert(sorted_keys(tree)==keys);
    verify_original_fields_unchanged(before,tree);
    assert(r::color(tree.nodes[0])==b::kBlack);
    assert(r::color(tree.nodes[1])==b::kRed);
    // A reverse rotation restores the exact original relationships.
    r::rotate(tree,1u,left ? r::Direction::right : r::Direction::left);
    assert(tree.root==before.root);
    assert(std::memcmp(tree.nodes.data(),before.nodes.data(),
                       before.nodes.size()*sizeof(t::NativeSlot32))==0);
}
static void non_root_rotation(const r::Config& cfg) {
    const bool left=cfg.direction==r::Direction::left;
    t::Tree tree;
    tree.nodes.resize(4);
    tree.root=0u;
    assign(tree,0u,left ? 50u : 10u,r::kNil,b::kBlack);
    assign(tree,1u,left ? 20u : 40u,0u,b::kRed);
    assign(tree,2u,left ? 30u : 30u,1u,b::kBlack);
    assign(tree,3u,left ? 25u : 35u,2u,b::kRed);
    if(left) {
        tree.nodes[0].left_00=1u;
        tree.nodes[1].right_02=2u;
        tree.nodes[2].left_00=3u;
    } else {
        tree.nodes[0].right_02=1u;
        tree.nodes[1].left_00=2u;
        tree.nodes[2].right_02=3u;
    }
    auto before=tree;auto keys=sorted_keys(tree);
    r::rotate(tree,1u,cfg);
    assert(tree.root==0u);
    assert(r::parent(tree.nodes[2])==0u);
    assert(r::parent(tree.nodes[1])==2u);
    assert(r::parent(tree.nodes[3])==1u);
    if(left) {
        assert(tree.nodes[0].left_00==2u);
        assert(tree.nodes[2].left_00==1u);
        assert(tree.nodes[1].right_02==3u);
    } else {
        assert(tree.nodes[0].right_02==2u);
        assert(tree.nodes[2].right_02==1u);
        assert(tree.nodes[1].left_00==3u);
    }
    assert(sorted_keys(tree)==keys);
    verify_original_fields_unchanged(before,tree);
}
static void reject_invalid_cases(const r::Config& cfg) {
    t::Tree t0; t0.nodes.resize(4);
    t0.root=0u;
    assign(t0,0u,10u,r::kNil,b::kBlack);
    assign(t0,1u,20u,0u,b::kRed);
    assign(t0,2u,15u,1u,b::kBlack);
    assign(t0,3u,25u,r::kNil,b::kBlack);
    const bool left=cfg.direction==r::Direction::left;
    auto& target=left ? t0.nodes[0].right_02 : t0.nodes[0].left_00;
    target=1u;
    auto& beta=left ? t0.nodes[1].left_00 : t0.nodes[1].right_02;
    beta=2u;
    expect_reject_unchanged(t0,0xffffu,cfg); // no real node at sentinel
    expect_reject_unchanged(t0,99u,cfg);
    auto t1=t0;beta=2u; // beta reference still refers to t0
    if(left) t1.nodes[0].right_02=0xffffu;
    else t1.nodes[0].left_00=0xffffu;
    expect_reject_unchanged(t1,0u,cfg);
    t1=t0; r::set_parent(t1.nodes[1],3u); // promotion parent wrong
    expect_reject_unchanged(t1,0u,cfg);
    t1=t0; r::set_parent(t1.nodes[2],0u); // beta parent wrong
    expect_reject_unchanged(t1,0u,cfg);
    t1=t0;
    if(left)t1.nodes[1].left_00=9u;
    else t1.nodes[1].right_02=9u;
    expect_reject_unchanged(t1,0u,cfg);
    t1=t0; r::set_parent(t1.nodes[0],3u); // root topology broken
    expect_reject_unchanged(t1,0u,cfg);
}
int main() {
    static_assert(r::kSceneLeft.native_entry_va==0x221A6BB0u);
    static_assert(r::kSceneRight.native_entry_va==0x221A6CA0u);
    static_assert(r::kModelSoundsLeft.native_entry_va==0x221409B0u);
    static_assert(r::kModelSoundsRight.native_entry_va==0x22140AA0u);
    for(const auto cfg : {r::kSceneLeft,r::kSceneRight,
                          r::kModelSoundsLeft,r::kModelSoundsRight}) {
        root_rotation(cfg);
        non_root_rotation(cfg);
        reject_invalid_cases(cfg);
    }
    return 0;
}
