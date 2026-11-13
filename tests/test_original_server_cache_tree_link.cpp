#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_server_cache_tree_link.hpp"
#include "sin_compat/original_server_cache_tree_insert.hpp"
#include "sin_compat/original_server_cache_key_comparators.hpp"

namespace link = sin_compat::original_server_cache_tree_link;
namespace ins = sin_compat::original_server_cache_tree_insert;
namespace tree = sin_compat::original_server_cache_tree_lookup;
namespace key = sin_compat::original_server_cache_key_comparators;

struct LinkHost {
    std::vector<std::uint32_t> raw_balance_indices;
    std::vector<std::uint32_t> balance_functions;
    void balance(const link::Config& cfg, std::uint32_t raw) {
        raw_balance_indices.push_back(raw);
        balance_functions.push_back(cfg.original_balance_va);
    }
};

struct InsertHost {
    tree::Tree& target;
    LinkHost& balances;
    std::uint32_t next{0u};
    std::uint16_t parent{link::kMissingIndex};
    std::uint8_t first_child{0u};
    void prepare(const ins::Config&, const key::NativeKeyPrefix32&, std::uint32_t) {}
    std::uint32_t claim(const ins::Config&) { return next; }
    void link(const ins::Config& cfg, std::uint32_t raw) {
        const auto matching = cfg.native_entry_va == ins::kScene.native_entry_va
                                ? link::kScene : link::kModelSounds;
        link::link(matching, target, raw, parent, first_child, balances);
    }
};
struct CompareHost {
    bool compare(std::uint32_t a, std::uint32_t b) {
        return key::less_first_dword_unsigned({a}, {b}) != 0u;
    }
    void initialize_sentinel(std::uint32_t) {}
};

int main() {
    static_assert(link::kSceneLinkVA == 0x221AA890u);
    static_assert(link::kModelSoundsLinkVA == 0x221441B0u);
    for (const auto cfg : {link::kScene, link::kModelSounds}) {
        tree::Tree t;
        t.nodes.resize(4u);
        LinkHost h;
        t.nodes[0].key_08 = 0x80000000u;
        t.nodes[0].payload_14 = 0xabcdefu;
        link::link(cfg, t, 0u, link::kMissingIndex, 0u, h);
        assert(t.root == 0u && t.nodes[0].unknown_04 == 0xffffu);
        assert(t.nodes[0].left_00 == 0xffffu);
        assert(t.nodes[0].right_02 == 0xffffu);
        assert(t.nodes[0].key_08 == 0x80000000u);
        assert(t.nodes[0].payload_14 == 0xabcdefu);
        assert(h.raw_balance_indices.size() == 1u);
        assert(h.balance_functions.back() == cfg.original_balance_va);

        t.nodes[1].key_08 = 0x7fffffffu;
        link::link(cfg, t, 1u, 0u, 1u, h);
        assert(t.nodes[0].left_00 == 1u && t.nodes[1].unknown_04 == 0u);
        t.nodes[2].key_08 = 0xffffffffu;
        link::link(cfg, t, 2u, 0u, 0u, h);
        assert(t.nodes[0].right_02 == 2u && t.nodes[2].unknown_04 == 0u);
        // Original stores low 16 bits in slots but passes raw uint32 to balance.
        link::link(cfg, t, 0x10003u, 1u, 9u, h);
        assert(t.nodes[1].left_00 == 3u);
        assert(t.nodes[3].unknown_04 == 1u);
        assert(h.raw_balance_indices.back() == 0x10003u);
        assert(h.raw_balance_indices.size() == 4u);
        // Host-only bound guard triggers before writes and rebalance callback.
        bool rejected = false;
        try { link::link(cfg, t, 5u, 0u, 1u, h); }
        catch (const std::out_of_range&) { rejected = true; }
        assert(rejected && h.raw_balance_indices.size() == 4u);
        rejected = false;
        try { link::link(cfg, t, 3u, 5u, 1u, h); }
        catch (const std::out_of_range&) { rejected = true; }
        assert(rejected && h.raw_balance_indices.size() == 4u);
        // Recovered Phase191 lookup + Phase192 unsigned comparator can
        // traverse slots initialized by the actual Phase196 parent/child links.
        tree::SentinelState sentinel;
        CompareHost comparator;
        assert(tree::find(t, 0x80000000u, sentinel,
                          tree::kSceneSentinelVA, comparator) == 0u);
        assert(tree::find(t, 0x7fffffffu, sentinel,
                          tree::kSceneSentinelVA, comparator) == 1u);
        assert(tree::find(t, 0xffffffffu, sentinel,
                          tree::kSceneSentinelVA, comparator) == 2u);
    }

    for (const auto cfg : {ins::kScene, ins::kModelSounds}) {
        ins::State state;
        state.slots.nodes.resize(2u);
        LinkHost balances;
        InsertHost host{state.slots, balances, 0u, 0xffffu, 0u};
        auto index = ins::insert(cfg, state, {100u, 1u, 2u, 3u}, 0u, host);
        assert(index == 0u && state.slots.root == 0u);
        assert(state.live_count == 1u);
        host.next = 1u;
        host.parent = 0u;
        host.first_child = 1u;
        index = ins::insert(cfg, state, {50u, 4u, 5u, 6u}, 0u, host);
        assert(index == 1u && state.slots.nodes[0].left_00 == 1u);
        assert(state.slots.nodes[1].unknown_04 == 0u);
        assert(state.slots.nodes[1].key_08 == 50u);
        assert(state.live_count == 2u);
        assert(balances.raw_balance_indices.size() == 2u);
    }
    return 0;
}
