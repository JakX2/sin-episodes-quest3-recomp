#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_server_cache_tree_lookup.hpp"
namespace lookup=sin_compat::original_server_cache_tree_lookup;

struct Host {
    std::vector<std::string> calls;
    bool compare(std::uint32_t a,std::uint32_t b) {
        calls.push_back("compare:"+std::to_string(a)+":"+std::to_string(b));
        return a<b;
    }
    void initialize_sentinel(std::uint32_t addr) {
        calls.push_back("sentinel:"+std::to_string(addr));
    }
};
int main() {
    static_assert(lookup::kSceneLookupVA==0x221A7FB0u);
    static_assert(lookup::kModelSoundsLookupVA==0x22142670u);
    for (std::uint32_t sentinel_va:{lookup::kSceneSentinelVA,lookup::kModelSoundsSentinelVA}) {
        lookup::SentinelState sentinel;
        Host h;
        lookup::Tree empty;
        assert(lookup::find(empty,5,sentinel,sentinel_va,h)==lookup::kMissing);
        assert(h.calls.empty() && !sentinel.ready);
        lookup::Tree t;
        t.nodes.resize(3);
        t.root=0;
        t.nodes[0].key_08=20;
        t.nodes[0].left_00=1;
        t.nodes[0].right_02=2;
        t.nodes[1].key_08=10;
        t.nodes[2].key_08=30;
        assert(lookup::find(t,20,sentinel,sentinel_va,h)==0);
        assert(h.calls.size()==2 && !sentinel.ready);
        h.calls.clear();
        assert(lookup::find(t,10,sentinel,sentinel_va,h)==1);
        assert(h.calls.size()==4);
        assert(h.calls[0]=="compare:10:20");
        assert(h.calls[1]=="sentinel:"+std::to_string(sentinel_va));
        assert(h.calls[2]=="compare:10:10" && h.calls[3]=="compare:10:10");
        assert(sentinel.ready);
        h.calls.clear();
        assert(lookup::find(t,30,sentinel,sentinel_va,h)==2);
        assert(h.calls.size()==4);
        assert(h.calls[0]=="compare:30:20" && h.calls[1]=="compare:20:30");
        assert(h.calls[2]=="compare:30:30" && h.calls[3]=="compare:30:30");
        h.calls.clear();
        assert(lookup::find(t,40,sentinel,sentinel_va,h)==lookup::kMissing);
        assert(h.calls.size()==4 && h.calls[0]=="compare:40:20");
        h.calls.clear();
        assert(lookup::find(t,5,sentinel,sentinel_va,h)==lookup::kMissing);
        assert(h.calls.size()==2 && h.calls[0]=="compare:5:20");
        h.calls.clear();
        t.nodes[1].left_00=1;
        bool cycle=false;
        try { (void)lookup::find(t,5,sentinel,sentinel_va,h); }
        catch(const std::length_error&) { cycle=true; }
        assert(cycle);
        t.nodes[1].left_00=10;
        bool bad_index=false;
        try { (void)lookup::find(t,5,sentinel,sentinel_va,h); }
        catch(const std::out_of_range&) { bad_index=true; }
        assert(bad_index);
    }
}
