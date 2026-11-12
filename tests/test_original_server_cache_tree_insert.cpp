#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_server_cache_tree_insert.hpp"
namespace ins=sin_compat::original_server_cache_tree_insert;
namespace cmp=sin_compat::original_server_cache_key_comparators;
namespace tree=sin_compat::original_server_cache_tree_lookup;
struct Host {
    std::vector<std::string> events;
    std::uint32_t raw_index=1;
    std::uint32_t linked=0;
    void prepare(const ins::Config& cfg,const cmp::NativeKeyPrefix32& key,std::uint32_t auxiliary) {
        assert(cfg.native_entry_va == ins::kSceneInsertVA || cfg.native_entry_va == ins::kModelSoundsInsertVA);
        assert(auxiliary==17u && key.first_word_00==0x80000000u);
        events.emplace_back("prepare");
    }
    std::uint32_t claim(const ins::Config&) {
        events.emplace_back("claim");return raw_index;
    }
    void link(const ins::Config&,std::uint32_t index) {
        events.emplace_back("link");linked=index;
    }
};
struct LookupHost {
    bool compare(std::uint32_t a,std::uint32_t b) {
        return cmp::less_first_dword_unsigned({a},{b})!=0u;
    }
    void initialize_sentinel(std::uint32_t) {}
};
int main(){
    static_assert(sizeof(cmp::NativeKeyPrefix32)==16);
    static_assert(offsetof(tree::NativeSlot32,key_08)==8);
    static_assert(offsetof(tree::NativeSlot32,payload_14)==20);
    for (const auto cfg:{ins::kScene,ins::kModelSounds}) {
        Host host;
        ins::State state;
        state.slots.nodes.resize(3);
        const cmp::NativeKeyPrefix32 key{0x80000000u,0x11111111u,0x22222222u,0xffffffffu};
        const std::uint16_t index=ins::insert(cfg,state,key,17u,host);
        assert(index==1 && host.linked==1 && state.live_count==1);
        assert((host.events==std::vector<std::string>{"prepare","claim","link"}));
        const auto& node=state.slots.nodes[index];
        assert(node.key_08==key.first_word_00);
        std::uint32_t words[2]{};
        std::memcpy(words,node.unknown_0c,8);
        assert(words[0]==key.opaque_04 && words[1]==key.opaque_08);
        assert(node.payload_14==key.opaque_0c);
        // Original Phase190 callback subsequently publishes payload into +0x14.
        state.slots.nodes[index].payload_14=0xABCDEF01u;
        assert(state.slots.nodes[index].key_08==key.first_word_00);
        state.slots.root=index;tree::SentinelState sent;LookupHost lookup_host;
        assert(tree::find(state.slots,0x80000000u,sent,tree::kSceneSentinelVA,lookup_host)==index);
        assert(tree::find(state.slots,0x7fffffffu,sent,tree::kSceneSentinelVA,lookup_host)==tree::kMissing);
        host.events.clear();host.raw_index=0x10002u;
        state.live_count=0xffffu;
        assert(ins::insert(cfg,state,key,17u,host)==2u);
        assert(host.linked==0x10002u && state.live_count==0u); // native u16 wrapping
        host.raw_index=3;
        bool failed=false;
        try {(void)ins::insert(cfg,state,key,17u,host);}catch(const std::out_of_range&){failed=true;}
        assert(failed);
    }
}
