#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <random>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_server_cache_balance_composition.hpp"
#include "sin_compat/original_server_cache_tree_link.hpp"
#include "sin_compat/original_server_cache_key_comparators.hpp"

namespace balance=sin_compat::original_server_cache_balance_composition;
namespace tree=sin_compat::original_server_cache_tree_lookup;
namespace link=sin_compat::original_server_cache_tree_link;
namespace rotate=sin_compat::original_server_cache_tree_rotations;
namespace comparator=sin_compat::original_server_cache_key_comparators;

struct Host {
    tree::Tree& nodes;
    balance::Result last{};
    void balance(const link::Config&, std::uint32_t raw) {
        last=balance::fixup(nodes,raw);
    }
};
static unsigned verify(const tree::Tree& t, std::uint16_t at,
                       std::uint16_t expected_parent,
                       std::int64_t low, std::int64_t high,
                       std::vector<std::uint8_t>& visited) {
    if(at==tree::kMissing)return 1u;
    assert(at<t.nodes.size() && visited[at]==0u);
    visited[at]=1u;
    const auto& n=t.nodes[at];
    assert(rotate::parent(n)==expected_parent);
    const auto key=static_cast<std::int64_t>(n.key_08);
    assert(key>low && key<high);
    const auto c=rotate::color(n);
    assert(c==0u || c==1u);
    if(c==0u){
        if(n.left_00!=tree::kMissing)assert(rotate::color(t.nodes[n.left_00])==1u);
        if(n.right_02!=tree::kMissing)assert(rotate::color(t.nodes[n.right_02])==1u);
    }
    const auto lhs=verify(t,n.left_00,at,low,key,visited);
    const auto rhs=verify(t,n.right_02,at,key,high,visited);
    assert(lhs==rhs);
    return lhs+(c==1u?1u:0u);
}
static void test_order(const link::Config& cfg,
                       const std::vector<std::uint32_t>& keys,
                       bool test_wide_index) {
    tree::Tree t;t.nodes.resize(keys.size());Host host{t,{}};
    std::uint32_t rotations=0u,recolors=0u;
    for(std::size_t i=0;i<keys.size();++i) {
        const auto key=keys[i];
        auto& n=t.nodes[i];
        n.key_08=key;
        n.payload_14=0xc0de0000u+static_cast<std::uint32_t>(i);
        std::memset(n.unknown_0c,static_cast<int>(i+1u),8);
        std::uint16_t p=tree::kMissing;
        std::uint16_t cur=t.root;
        std::uint8_t side=0u;
        while(cur!=tree::kMissing){
            p=cur;
            assert(cur<t.nodes.size() && key!=t.nodes[cur].key_08);
            const bool first=comparator::less_first_dword_unsigned(
                {key},{t.nodes[cur].key_08})!=0u;
            side=first?1u:0u;
            cur=first?t.nodes[cur].left_00:t.nodes[cur].right_02;
        }
        // Original link wrapper passes the full raw index while actual
        // indexed native slots truncate to the low 16 bits.
        const auto raw=static_cast<std::uint32_t>(i)
                       +(test_wide_index?0x10000u:0u);
        link::link(cfg,t,raw,p,side,host);
        rotations+=host.last.left_rotations+host.last.right_rotations;
        recolors+=host.last.recolor_steps;
        assert(t.root!=tree::kMissing && rotate::color(t.nodes[t.root])==1u);
        std::vector<std::uint8_t> visited(t.nodes.size(),0u);
        (void)verify(t,t.root,tree::kMissing,-1,0x100000000ll,visited);
        assert(std::count(visited.begin(),visited.end(),1u)
               ==static_cast<std::ptrdiff_t>(i+1u));
        for(std::size_t j=0;j<=i;++j){
            assert(t.nodes[j].key_08==keys[j]);
            assert(t.nodes[j].payload_14==0xc0de0000u+static_cast<std::uint32_t>(j));
            for(auto byte:t.nodes[j].unknown_0c)
                assert(byte==static_cast<std::uint8_t>(j+1u));
        }
    }
    if(keys.size()>=20u){
        assert(rotations>0u);
        assert(recolors>0u);
    }
}
static void test_transactional_recolor_then_failure() {
    // 0 -> 1 -> 3 -> 5, with red sibling 4 of node 3. Recolor
    // (3,4,1), ascend to node 1, then encounter invalid parent=99.
    // The original x86 has no such bounds guard; host input stays intact.
    tree::Tree t;t.nodes.resize(6u);t.root=0u;
    t.nodes[0].unknown_04=0x0001ffffu;
    t.nodes[0].left_00=1u;
    t.nodes[1].unknown_04=99u; // deliberately malformed upward parent
    t.nodes[1].left_00=3u;t.nodes[1].right_02=4u;
    t.nodes[3].unknown_04=1u;t.nodes[3].left_00=5u;
    t.nodes[4].unknown_04=1u;
    t.nodes[5].unknown_04=3u;
    auto original=t;
    bool refused=false;
    try {(void)balance::fixup(t,5u);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused && t.root==original.root);
    assert(std::memcmp(t.nodes.data(),original.nodes.data(),
                       t.nodes.size()*sizeof(tree::NativeSlot32))==0);
}
int main() {
    // Independent extension to Phase199's permutation suite: unsigned zero,
    // bit-31 edges, 64-node fuzz, raw32 truncation and opaque payload bytes.
    std::mt19937 rng(0x53494e45u);
    for(const auto cfg:{link::kScene,link::kModelSounds}) {
        test_order(cfg,{0u,0x7fffffffu,0x80000000u,0xffffffffu,1u,
                        0x40000000u,0xc0000000u,0x01010101u},true);
        for(unsigned run=0;run<20u;++run) {
            std::vector<std::uint32_t> keys;
            keys.reserve(64);
            // Odd multiplication is bijective modulo 2^32, giving 64
            // distinct keys that span both sides of bit31.
            for(std::uint32_t i=0;i<64u;++i)
                keys.push_back((i+run*64u)*0x9e3779b9u);
            std::shuffle(keys.begin(),keys.end(),rng);
            test_order(cfg,keys,(run&1u)!=0u);
        }
    }
    test_transactional_recolor_then_failure();
}
