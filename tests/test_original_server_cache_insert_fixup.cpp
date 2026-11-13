#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>
#include "sin_compat/original_server_cache_insert_fixup.hpp"
#include "sin_compat/original_server_cache_tree_link.hpp"

namespace fix=sin_compat::original_server_cache_insert_fixup;
namespace link=sin_compat::original_server_cache_tree_link;
namespace tree=sin_compat::original_server_cache_tree_lookup;
namespace bal=sin_compat::original_server_cache_balance_preflight;

struct Check {
    const tree::Tree& t;
    std::vector<std::uint8_t> seen;
    std::size_t visited{};
    explicit Check(const tree::Tree& input):t(input),seen(input.nodes.size()){}
    unsigned branch(std::uint16_t idx,std::uint16_t expected_parent,
                    std::int64_t lower,std::int64_t upper) {
        if(idx==fix::kNil)return 1u;
        assert(idx<t.nodes.size() && seen[idx]==0u);
        seen[idx]=1u;++visited;
        const auto& n=t.nodes[idx];
        assert(fix::parent(n)==expected_parent);
        assert(static_cast<std::int64_t>(n.key_08)>lower);
        assert(static_cast<std::int64_t>(n.key_08)<upper);
        assert(fix::color(n)==bal::kRed||fix::color(n)==bal::kBlack);
        if(fix::color(n)==bal::kRed){
            if(n.left_00!=fix::kNil)assert(fix::color(t.nodes[n.left_00])==bal::kBlack);
            if(n.right_02!=fix::kNil)assert(fix::color(t.nodes[n.right_02])==bal::kBlack);
        }
        const auto left=branch(n.left_00,idx,lower,n.key_08);
        const auto right=branch(n.right_02,idx,n.key_08,upper);
        assert(left==right);
        return left+(fix::color(n)==bal::kBlack?1u:0u);
    }
};
struct Host {
    tree::Tree& nodes;
    fix::Config cfg;
    fix::Result last{fix::Status::malformed_host_tree,{}};
    void balance(const link::Config&,std::uint32_t new_idx){
        last=fix::apply(nodes,new_idx,cfg);
        assert(last.status==fix::Status::completed);
    }
};
struct Trace { std::uint32_t left{},right{},recolor{}; };

static Trace run(const std::vector<std::uint32_t>& keys, bool scene) {
    const auto cfg=scene?fix::kScene:fix::kModelSounds;
    const auto link_cfg=scene?link::kScene:link::kModelSounds;
    assert(cfg.original_balance_va==link_cfg.original_balance_va);
    tree::Tree t;
    t.nodes.resize(keys.size());
    Host host{t,cfg,{fix::Status::malformed_host_tree,{}}};
    Trace trace{};
    for(std::size_t i=0;i<keys.size();++i){
        auto& n=t.nodes[i];
        n.key_08=keys[i];
        n.payload_14=0xa0000000u+static_cast<std::uint32_t>(i);
        std::memset(n.unknown_0c,static_cast<int>(i+1u),8u);
        std::uint16_t p=fix::kNil;
        std::uint16_t curr=t.root;
        std::uint8_t side=0;
        while(curr!=fix::kNil){
            p=curr;
            assert(curr<t.nodes.size() && keys[i]!=t.nodes[curr].key_08);
            side=static_cast<std::uint8_t>(keys[i]<t.nodes[curr].key_08);
            curr=side?t.nodes[curr].left_00:t.nodes[curr].right_02;
        }
        link::link(link_cfg,t,static_cast<std::uint32_t>(i),p,side,host);
        trace.left+=host.last.stats.left_rotations;
        trace.right+=host.last.stats.right_rotations;
        trace.recolor+=host.last.stats.red_uncle_recolor_rounds;
        assert(fix::color(t.nodes[t.root])==bal::kBlack);
        Check validate{t};
        validate.branch(t.root,fix::kNil,-1,0x100000000ll);
        assert(validate.visited==i+1u);
        for(std::size_t j=0;j<=i;++j){
            assert(t.nodes[j].key_08==keys[j]);
            assert(t.nodes[j].payload_14==0xa0000000u+static_cast<std::uint32_t>(j));
            for(auto x:t.nodes[j].unknown_0c)assert(x==static_cast<std::uint8_t>(j+1u));
        }
    }
    return trace;
}
static void targeted_cases(bool scene) {
    auto a=run({30u,20u,10u},scene);
    assert(a.left==0u&&a.right==1u);
    a=run({10u,20u,30u},scene);
    assert(a.left==1u&&a.right==0u);
    a=run({30u,10u,20u},scene);
    assert(a.left==1u&&a.right==1u);
    a=run({10u,30u,20u},scene);
    assert(a.left==1u&&a.right==1u);
    a=run({40u,20u,60u,10u},scene);
    assert(a.recolor==1u);
    (void)run({0x80000000u,0x7fffffffu,0xffffffffu,0u,42u},scene);
    // Exhaustive six-key insertion permutations; validate after every key.
    std::vector<std::uint32_t> keys{1u,2u,3u,4u,5u,6u};
    do {(void)run(keys,scene);}while(std::next_permutation(keys.begin(),keys.end()));
    std::mt19937 rng(0x5153494eu);
    for(unsigned k=0;k<12u;++k){
        std::vector<std::uint32_t> long_keys(64u);
        for(std::uint32_t i=0;i<64u;++i)long_keys[i]=i*0x01010101u;
        std::shuffle(long_keys.begin(),long_keys.end(),rng);
        (void)run(long_keys,scene);
    }
}
static void failed_input_is_transactional(const fix::Config& cfg) {
    tree::Tree t;
    t.nodes.resize(3);
    t.root=0;
    t.nodes[0].unknown_04=0xffff0000u; // malformed root
    const auto old=t;
    assert(fix::apply(t,0u,cfg).status==fix::Status::malformed_host_tree);
    assert(t.root==old.root);
    assert(std::memcmp(t.nodes.data(),old.nodes.data(),t.nodes.size()*sizeof(tree::NativeSlot32))==0);
    t.nodes[0].unknown_04=0x0001ffffu;
    t.nodes[1].unknown_04=0u; // orphan inserted slot
    const auto old2=t;
    assert(fix::apply(t,1u,cfg).status==fix::Status::malformed_host_tree);
    assert(std::memcmp(t.nodes.data(),old2.nodes.data(),t.nodes.size()*sizeof(tree::NativeSlot32))==0);
    t.root=fix::kNil;
    assert(fix::apply(t,0u,cfg).status==fix::Status::malformed_host_tree);
}
int main() {
    static_assert(fix::kScene.original_balance_va==0x221A8F20u);
    static_assert(fix::kModelSounds.original_balance_va==0x221429D0u);
    targeted_cases(true);
    targeted_cases(false);
    failed_input_is_transactional(fix::kScene);
    failed_input_is_transactional(fix::kModelSounds);
    return 0;
}
