#include "sin_compat/original_server_symbol_growth_traversal.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
namespace n=sin_compat::original_server_symbol_growth_traversal;
namespace {
struct MemoryHost {
    std::vector<std::string> trace{};
    std::uint32_t last_bytes=0;
    std::uintptr_t allocate_2226E390(std::uint32_t bytes){trace.push_back("allocate");last_bytes=bytes;return 0x1000;}
    std::uintptr_t reallocate_2226E400(std::uintptr_t old,std::uint32_t bytes){
        assert(old==0x1000||old==0x2000);trace.push_back("reallocate");last_bytes=bytes;return 0x2000;
    }
    void free_2226E3E0(std::uintptr_t old){assert(old==0x1000);trace.push_back("free");}
};
void test_growth_width_initial_values_and_existing_allocations(){
    MemoryHost h{};n::ArrayGrowthState a{};
    n::original_grow_symbol_nodes_2228E200(a,1,h);
    assert(a.capacity_4==3 && a.buffer_0==0x1000 && h.last_bytes==36);
    n::original_grow_symbol_nodes_2228E200(a,1,h);
    assert(a.capacity_4==6 && a.buffer_0==0x2000 && h.last_bytes==72);
    assert((h.trace==std::vector<std::string>{"allocate","reallocate"}));
    MemoryHost d{};n::ArrayGrowthState b{};
    n::original_grow_dword_slots_2228E280(b,1,d);
    assert(b.capacity_4==8 && d.last_bytes==32);
    n::original_grow_dword_slots_2228E280(b,9,d);
    assert(b.capacity_4==32 && d.last_bytes==128 && b.buffer_0==0x2000);
}
void test_signed_target_and_no_op_still_allocates(){
    MemoryHost h{};n::ArrayGrowthState s{};
    s.capacity_4=20;s.buffer_0=0x1000;s.growth_8=5;
    n::original_grow_dword_slots_2228E280(s,9,h);
    assert(s.capacity_4==30 && h.last_bytes==120);
    n::original_grow_dword_slots_2228E280(s,0,h);
    assert(s.capacity_4==30 && h.trace.size()==2);
    n::ArrayGrowthState skip{};skip.growth_8=n::kNoAllocator;
    n::original_grow_symbol_nodes_2228E200(skip,50,h);
    assert(skip.capacity_4==0 && skip.buffer_0==0 && h.trace.size()==2);
    assert(n::signed_word(0xffffffffu)==-1);
    assert(n::signed_word(0x80000000u)==-2147483648ll);
    n::ArrayGrowthState negative{};negative.capacity_4=0x7fffffffu;
    n::original_grow_symbol_nodes_2228E200(negative,3u,h);
    assert(negative.capacity_4==0x7fffffffu); // JGE on signed wrapped target
}
void test_quantum_and_uninitialized_notifier(){
    MemoryHost h{};n::ArrayGrowthState a{};
    a.capacity_4=8;a.growth_8=3;
    n::original_grow_dword_slots_2228E280(a,8,h);
    assert(a.capacity_4==17 && h.last_bytes==68);
}
struct CompareHost {
    std::vector<std::string> trace{};
    bool compare_key_to_node(std::uint32_t key,std::uint32_t node,
                             std::uint32_t context){
        assert(context==0xaau);trace.push_back("compare"+std::to_string(node));
        return key<node;
    }
    void on_sentinel_initialized(){trace.push_back("sentinel");}
};
void test_empty_tree_and_laziness(){
    n::InsertionSearchState s{};n::NativeSentinel sentinel{};CompareHost h{};
    auto r=n::original_prepare_symbol_tree_position_2228E470(s,1,0xaa,sentinel,h);
    assert(r.last_index==n::kEmptyIndex && r.comparator_nonzero==0);
    assert(!sentinel.guard_initialized && h.trace.empty());
}
void test_tree_left_right_output_and_sentinel_once(){
    n::InsertionSearchState s{};s.root_10=0;
    s.records.resize(3);
    s.records[0].key_8=20;s.records[0].link_0=1;s.records[0].link_2=2;
    s.records[1].key_8=5;s.records[2].key_8=35;
    n::NativeSentinel sentinel{};CompareHost h{};
    auto r=n::original_prepare_symbol_tree_position_2228E470(s,3,0xaa,sentinel,h);
    assert(r.last_index==1 && r.comparator_nonzero==1);
    assert(sentinel.guard_initialized && sentinel.node.link_0==0xffff && sentinel.node.meta_6==1);
    assert((h.trace==std::vector<std::string>{"compare20","sentinel","compare5"}));
    h.trace.clear();
    r=n::original_prepare_symbol_tree_position_2228E470(s,40,0xaa,sentinel,h);
    assert(r.last_index==2 && r.comparator_nonzero==0);
    assert((h.trace==std::vector<std::string>{"compare20","compare35"}));
}
void test_tree_invalid_index_protected_host_side(){
    n::InsertionSearchState s{};s.root_10=3;s.records.resize(1);
    n::NativeSentinel sentinel{};CompareHost h{};bool caught=false;
    try{(void)n::original_prepare_symbol_tree_position_2228E470(s,5,0xaa,sentinel,h);}
    catch(const std::out_of_range&){caught=true;}
    assert(caught && !sentinel.guard_initialized);
}
void test_cleanup_guard_and_event_order(){
    n::SymbolCleanupState s{};MemoryHost h{};
    s.allocation_0=0x1000;s.capacity_4=22;s.guard_8=n::kNoAllocator;s.pending_0c=99;
    n::original_reset_symbol_storage_2228E5B0(s,h);
    assert(s.pending_0c==0 && s.allocation_0==0x1000 && s.capacity_4==22);
    assert(s.cached_pointer_10==0x1000 && h.trace.empty());
    s.guard_8=0;s.pending_0c=123;
    n::original_reset_symbol_storage_2228E5B0(s,h);
    assert(s.pending_0c==0 && s.allocation_0==0 && s.capacity_4==0 && s.cached_pointer_10==0);
    assert((h.trace==std::vector<std::string>{"free"}));
    s.capacity_4=33;s.cached_pointer_10=1;
    n::original_reset_symbol_storage_2228E5B0(s,h);
    assert(s.capacity_4==0 && s.cached_pointer_10==0 && h.trace.size()==1);
}
}
int main(){
    test_growth_width_initial_values_and_existing_allocations();
    test_signed_target_and_no_op_still_allocates();
    test_quantum_and_uninitialized_notifier();
    test_empty_tree_and_laziness();
    test_tree_left_right_output_and_sentinel_once();
    test_tree_invalid_index_protected_host_side();
    test_cleanup_guard_and_event_order();
}