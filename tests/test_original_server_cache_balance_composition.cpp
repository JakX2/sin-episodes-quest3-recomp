#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_server_cache_balance_composition.hpp"
#include "sin_compat/original_server_cache_tree_link.hpp"
#include "sin_compat/original_server_cache_key_comparators.hpp"

namespace b = sin_compat::original_server_cache_balance_composition;
namespace link = sin_compat::original_server_cache_tree_link;
namespace tree = sin_compat::original_server_cache_tree_lookup;
namespace compare = sin_compat::original_server_cache_key_comparators;
namespace rotate = sin_compat::original_server_cache_tree_rotations;

struct Host {
    tree::Tree& t;
    std::vector<b::Result> results{};
    void balance(const link::Config&, std::uint32_t index) {
        results.push_back(b::fixup(t,index));
    }
};
struct LookupHost {
    bool compare(std::uint32_t x,std::uint32_t y) {
        return compare::less_first_dword_unsigned({x},{y})!=0u;
    }
    void initialize_sentinel(std::uint32_t) {}
};
static int verify(const tree::Tree& t,std::uint16_t at,std::uint16_t p,
                  std::vector<bool>& visited,std::vector<std::uint32_t>& keys,
                  std::size_t depth) {
    if(at==b::kNil) return 1;
    assert(at<t.nodes.size() && depth<=t.nodes.size());
    assert(!visited[at]);
    visited[at]=true;
    const auto& n=t.nodes[at];
    assert(rotate::parent(n)==p);
    const auto color=rotate::color(n);
    assert(color==b::kRed || color==b::kBlack);
    if(color==b::kRed) {
        if(n.left_00!=b::kNil) assert(rotate::color(t.nodes[n.left_00])==b::kBlack);
        if(n.right_02!=b::kNil) assert(rotate::color(t.nodes[n.right_02])==b::kBlack);
    }
    const int left=verify(t,n.left_00,at,visited,keys,depth+1);
    keys.push_back(n.key_08);
    const int right=verify(t,n.right_02,at,visited,keys,depth+1);
    assert(left==right);
    return left+(color==b::kBlack?1:0);
}
static void check_tree(const tree::Tree& t,const std::vector<std::uint32_t>& expected) {
    assert(t.root!=b::kNil && rotate::color(t.nodes[t.root])==b::kBlack);
    std::vector<bool> visited(t.nodes.size(),false);
    std::vector<std::uint32_t> actual;
    (void)verify(t,t.root,b::kNil,visited,actual,0);
    assert(actual==expected);
    assert(std::count(visited.begin(),visited.end(),true)==static_cast<int>(expected.size()));
    for(std::size_t i=0;i<t.nodes.size();++i) {
        assert(t.nodes[i].payload_14==0xabcd0000u+i);
        assert(t.nodes[i].key_08!=0u);
    }
}
static b::Result insert(tree::Tree& t,Host& host,link::Config cfg,
                        std::uint16_t index,std::uint32_t key) {
    std::uint16_t cur=t.root;
    std::uint16_t p=b::kNil;
    bool left=false;
    while(cur!=b::kNil) {
        p=cur;
        left=key<t.nodes[cur].key_08;
        assert(key!=t.nodes[cur].key_08);
        cur=left?t.nodes[cur].left_00:t.nodes[cur].right_02;
    }
    auto& node=t.nodes[index];
    node.key_08=key;
    node.payload_14=0xabcd0000u+index;
    link::link(cfg,t,index,p,left?1u:0u,host);
    return host.results.back();
}
static void run_permutations(const link::Config cfg) {
    std::vector<std::uint32_t> values{10,20,30,40,50,60};
    std::size_t cases=0;
    do {
        tree::Tree t;t.nodes.resize(values.size());
        Host host{t,{}};
        std::vector<std::uint32_t> inserted;
        for(std::size_t i=0;i<values.size();++i) {
            const auto key=values[i];
            insert(t,host,cfg,static_cast<std::uint16_t>(i),key);
            inserted.push_back(key);
            std::sort(inserted.begin(),inserted.end());
            // Payloads of not-yet-inserted slots have their default values,
            // so verify only the active prefix using separate check below.
            std::vector<bool> visited(t.nodes.size(),false);
            std::vector<std::uint32_t> inorder;
            assert(t.root!=b::kNil && rotate::color(t.nodes[t.root])==b::kBlack);
            (void)verify(t,t.root,b::kNil,visited,inorder,0);
            assert(inorder==inserted);
            assert(std::count(visited.begin(),visited.end(),true)==static_cast<int>(i+1));
            for(std::size_t j=0;j<=i;++j) {
                assert(t.nodes[j].payload_14==0xabcd0000u+j);
                assert(t.nodes[j].key_08==values[j]);
            }
        }
        check_tree(t,inserted);
        LookupHost cb;tree::SentinelState sentinel;
        for(const auto value:inserted) {
            const auto index=tree::find(t,value,sentinel,tree::kSceneSentinelVA,cb);
            assert(index!=tree::kMissing);
            assert(t.nodes[index].key_08==value);
        }
        assert(tree::find(t,77u,sentinel,tree::kSceneSentinelVA,cb)==tree::kMissing);
        ++cases;
    }while(std::next_permutation(values.begin(),values.end()));
    assert(cases==720);
}
static void check_shape_cases(const link::Config cfg) {
    struct Scenario {std::vector<std::uint32_t> values;bool expect_recolor;bool expect_double;};
    const Scenario scenarios[]={
        {{30,20,10},false,false}, // LL
        {{10,20,30},false,false}, // RR
        {{30,10,20},false,true},  // LR
        {{10,30,20},false,true},  // RL
        {{20,10,30,5},true,false}, // red uncle
    };
    for(const auto& s:scenarios) {
        tree::Tree t;t.nodes.resize(s.values.size());Host host{t,{}};
        b::Result last{};
        for(std::size_t i=0;i<s.values.size();++i)
            last=insert(t,host,cfg,static_cast<std::uint16_t>(i),s.values[i]);
        if(s.expect_recolor) assert(last.recolor_steps>=1u);
        if(s.expect_double) assert(last.left_rotations==1u && last.right_rotations==1u);
        std::vector<std::uint32_t> expected=s.values;
        std::sort(expected.begin(),expected.end());
        check_tree(t,expected);
    }
}
static void check_fail_closed() {
    tree::Tree t;
    t.nodes.resize(3);
    t.root=0;
    t.nodes[0].unknown_04=0xffffu | (1u<<16);
    t.nodes[0].left_00=1;
    t.nodes[1].unknown_04=0u;
    t.nodes[1].left_00=2;
    t.nodes[2].unknown_04=1u;
    const auto before=t.nodes;
    const auto root=t.root;
    // Parent of #1 is the root but its child is red; grandparent of #2
    // is valid. Corrupt #2's parent into an out-of-range index.
    rotate::set_parent(t.nodes[2],99u);
    const auto invalid_before=t.nodes;
    bool refused=false;
    try {(void)b::fixup(t,2u);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
    assert(t.root==root);
    assert(std::memcmp(t.nodes.data(),invalid_before.data(),t.nodes.size()*sizeof(tree::NativeSlot32))==0);
    refused=false;
    try {(void)b::fixup(t,99u);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
    assert(t.root==root);
    (void)before;
}
int main() {
    for(auto cfg:{link::kScene,link::kModelSounds}) {
        check_shape_cases(cfg);
        run_permutations(cfg);
    }
    check_fail_closed();
    return 0;
}
