// Phase546: demonstrate source-grounded native token-tree callback dispatch.
// Test payload_native32 contains synthetic string-pool indices, not 2006 ptrs.
#include "sin_compat/original_client_string_token_tree.hpp"
#include "sin_compat/original_client_string_token_compare.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdio>
#include <string_view>
#include <vector>
namespace tree=sin_compat::original_client_string_tokens;
namespace cmp=sin_compat::original_client_token_compare;
int main() {
    const std::array<std::string_view,4> string_pool{"m", "Alpha", "zulu", "ALPHA"};
    std::vector<tree::Node> nodes(3);
    nodes[0].payload_native32=0; nodes[0].smaller=1; nodes[0].larger=2;
    nodes[1].payload_native32=1; nodes[1].parent=0;
    nodes[2].payload_native32=2; nodes[2].parent=0;
    auto ordinal=[&](std::uint32_t a,std::uint32_t b){
        return cmp::less_unsigned_c_string(string_pool.at(a),string_pool.at(b));
    };
    auto ascii=[&](std::uint32_t a,std::uint32_t b){
        return cmp::less_ascii_folded_c_string(string_pool.at(a),string_pool.at(b));
    };
    const auto q_ordinal=tree::find_token(nodes,0,3,ordinal);
    const auto q_folded=tree::find_token(nodes,0,3,ascii);
    assert(q_ordinal.status==tree::Status::NotFound); // A vs a is different
    assert(q_folded.status==tree::Status::Found&&q_folded.index==1);
    assert(tree::find_token(nodes,0,1,ordinal).status==tree::Status::Found);
    assert(tree::find_token(nodes,0,1,ascii).status==tree::Status::Found);
    // The original's constructor-mode flag is independent of this test data;
    // the test proves callback choice has observable lookup semantics.
    assert(cmp::constructor_selection(true)==cmp::ComparatorKind::CaseFolded);
    std::puts("PASS original-free token tree callback modes and case-sensitive/folded lookup distinction");
}