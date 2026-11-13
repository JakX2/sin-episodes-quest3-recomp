#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <vector>
#include "sin_compat/original_server_cache_balance_preflight.hpp"
#include "sin_compat/original_server_cache_tree_link.hpp"

namespace balance = sin_compat::original_server_cache_balance_preflight;
namespace link = sin_compat::original_server_cache_tree_link;
namespace tree = sin_compat::original_server_cache_tree_lookup;

struct Host {
    tree::Tree& nodes;
    std::vector<balance::Outcome> outcomes{};
    void balance(const link::Config&, std::uint32_t raw_index) {
        outcomes.push_back(balance::assess_nonrotating_case(nodes, raw_index));
    }
};
int main() {
    static_assert(balance::kSceneBalanceVA == 0x221A8F20u);
    static_assert(balance::kModelSoundsBalanceVA == 0x221429D0u);
    for (const auto cfg : {link::kScene, link::kModelSounds}) {
        tree::Tree t;
        t.nodes.resize(5);
        Host h{t, {}};
        link::link(cfg, t, 0u, link::kMissingIndex, 0u, h);
        assert(h.outcomes.back() == balance::Outcome::root_blackened);
        assert(t.root == 0u);
        assert(balance::parent(t.nodes[0]) == 0xffffu);
        assert(balance::color(t.nodes[0]) == balance::kBlack);
        link::link(cfg, t, 1u, 0u, 1u, h);
        assert(h.outcomes.back() == balance::Outcome::parent_already_black);
        assert(balance::parent(t.nodes[1]) == 0u);
        assert(balance::color(t.nodes[1]) == balance::kRed);
        assert(t.nodes[0].left_00 == 1u);
        // These do not require rotations while the root is black.
        link::link(cfg, t, 2u, 0u, 0u, h);
        assert(h.outcomes.back() == balance::Outcome::parent_already_black);
        assert(t.nodes[0].right_02 == 2u);
        // The native body would now enter its uncle-color/rotation logic.
        // Fail-closed preflight must not invent its effects or write flags.
        const auto root_before = t.nodes[0].unknown_04;
        const auto child_before = t.nodes[1].unknown_04;
        t.nodes[3].unknown_04 = 1u; // parent index 1, red parent
        const auto grandchild_before = t.nodes[3].unknown_04;
        assert(balance::assess_nonrotating_case(t, 3u)
            == balance::Outcome::requires_unimplemented_rotation_or_recolor);
        assert(t.nodes[0].unknown_04 == root_before);
        assert(t.nodes[1].unknown_04 == child_before);
        assert(t.nodes[3].unknown_04 == grandchild_before);
        // Parent colour uses the HIGH WORD of the original +4 DWORD.
        balance::set_color(t.nodes[1], balance::kBlack);
        assert(balance::parent(t.nodes[1]) == 0u);
        assert(balance::color(t.nodes[1]) == 1u);
        assert(balance::assess_nonrotating_case(t, 3u)
            == balance::Outcome::parent_already_black);
        assert(balance::color(t.nodes[0]) == balance::kBlack);
        assert(balance::assess_nonrotating_case(t, 0x10000u)
            == balance::Outcome::root_blackened); // original low-16 index
        assert(balance::assess_nonrotating_case(t, 9u)
            == balance::Outcome::invalid_synthetic_tree);
        t.nodes[4].unknown_04 = 0xffffu;
        assert(balance::assess_nonrotating_case(t, 4u)
            == balance::Outcome::invalid_synthetic_tree);
    }
    // The placeholder empty tree remains invalid, not implicitly blackened.
    tree::Tree empty;
    assert(balance::assess_nonrotating_case(empty, 0u)
        == balance::Outcome::invalid_synthetic_tree);
}
