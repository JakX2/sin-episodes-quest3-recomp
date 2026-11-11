#include "sin_compat/original_server_registry_tree_search.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace r=sin_compat::original_server_registry_tree_search;
namespace {
struct Host {
    std::vector<std::string> log;
    std::map<std::uint16_t,bool> insert_cmp,first_cmp,second_cmp;
    std::map<std::uint16_t,std::uint16_t> insert_left,insert_right,symbol_left,symbol_right;
    std::uint16_t slot=7;
    unsigned init_count=0;
    void initialize_sentinel_words(std::uint16_t a,std::uint16_t b,std::uint16_t c,std::uint16_t d){
        assert(a==r::kInvalidIndex&&b==a&&c==a&&d==1);
        log.emplace_back("sentinel");++init_count;
    }
    bool compare_insertion_candidate(std::uint16_t i){log.emplace_back("insert-compare:"+std::to_string(i));return insert_cmp.at(i);}
    std::uint16_t read_insertion_child(std::uint16_t i,unsigned offset){
        log.emplace_back("insert-child:"+std::to_string(offset));
        auto& m=offset==0?insert_left:insert_right;
        return m.count(i)?m.at(i):r::kInvalidIndex;
    }
    bool compare_symbol_first(std::uint16_t i){log.emplace_back("symbol-first:"+std::to_string(i));return first_cmp.at(i);}
    bool compare_symbol_second(std::uint16_t i){log.emplace_back("symbol-second:"+std::to_string(i));return second_cmp.at(i);}
    std::uint16_t read_symbol_child(std::uint16_t i,unsigned offset){
        log.emplace_back("symbol-child:"+std::to_string(offset));
        auto& m=offset==0?symbol_left:symbol_right;
        return m.count(i)?m.at(i):r::kInvalidIndex;
    }
    void prepare_record_inputs(std::uint32_t v){assert(v==0x88776655u);log.emplace_back("prepare");}
    std::uint16_t allocate_symbol_slot(){log.emplace_back("allocate");return slot;}
    void link_symbol_slot(std::uint16_t i){assert(i==slot);log.emplace_back("link");}
    void write_record_word(std::uint16_t i,std::uint32_t w,std::uint16_t count){
        assert(i==slot&&w==0x88776655u);assert(count==0||count==1||count==0xffffu);
        log.emplace_back("write");
    }
    void release_allocation(std::uint32_t x){assert(x==0x11223344u);log.emplace_back("free");}
};
void test_empty_paths_no_lazy_init(){
    Host h;r::LazySentinel s{};
    auto a=r::original_prepare_insertion(r::kInvalidIndex,s,h);
    assert(a.last_examined==r::kInvalidIndex&&!a.last_direction);
    assert(r::original_find_symbol(r::kInvalidIndex,s,h)==r::kInvalidIndex);
    assert(h.log.empty()&&!s.initialized);
}
void test_native_insertion_search_two_link_offsets(){
    Host h; r::LazySentinel state{};
    h.insert_cmp={{1,true},{2,false}};h.insert_left[1]=2;
    auto result=r::original_prepare_insertion(1,state,h);
    assert(result.last_examined==2&&!result.last_direction);
    assert(state.initialized&&h.init_count==1);
    assert((h.log==std::vector<std::string>{"insert-compare:1","sentinel","insert-child:0","insert-compare:2","insert-child:2"}));
    h.log.clear();h.insert_cmp[2]=true;
    auto next=r::original_prepare_insertion(1,state,h);
    assert(next.last_examined==2&&next.last_direction);
    assert(h.init_count==1);assert(h.log.size()==4);
}
void test_symbol_lookup_comparator_order_and_match(){
    Host h;r::LazySentinel state{};
    h.first_cmp={{3,true},{4,false},{6,false}};
    h.second_cmp={{4,true},{6,false}};
    h.symbol_left[3]=4;h.symbol_right[4]=6;
    const auto idx=r::original_find_symbol(3,state,h);
    assert(idx==6&&h.init_count==1);
    assert((h.log==std::vector<std::string>{"symbol-first:3","sentinel","symbol-child:0","symbol-first:4","symbol-second:4","symbol-child:2","symbol-first:6","symbol-second:6"}));
    h.log.clear();assert(r::original_find_symbol(6,state,h)==6);
    assert((h.log==std::vector<std::string>{"symbol-first:6","symbol-second:6"}));
    assert(h.init_count==1);
}
void test_symbol_no_match_and_sentinel_init(){
    Host h;r::LazySentinel state{};
    h.first_cmp={{10,false},{11,false}};
    h.second_cmp={{10,true},{11,true}};h.symbol_right[10]=11;
    assert(r::original_find_symbol(10,state,h)==r::kInvalidIndex);
    assert(h.init_count==1);
}
void test_symbol_second_false_return_has_no_lazy_init(){
    Host h;r::LazySentinel state{};h.first_cmp[5]=false;h.second_cmp[5]=false;
    assert(r::original_find_symbol(5,state,h)==5);
    assert(!state.initialized&&h.init_count==0);
}
void test_symbol_record_insertion_and_count_wrap(){
    Host h;r::SymbolRecords rec{};
    assert(r::original_insert_symbol_record(rec,0x88776655u,h)==7);
    assert(rec.registered_count==1);
    assert((h.log==std::vector<std::string>{"prepare","allocate","link","write"}));
    h.log.clear();rec.registered_count=0xffffu;
    assert(r::original_insert_symbol_record(rec,0x88776655u,h)==7);
    assert(rec.registered_count==0u);
}
void test_guarded_release(){
    Host h;r::OwnedIndexedStorage s{0x11223344u,45u,0xffffffffu};
    r::original_release_indexed_storage(s,h);assert(h.log.empty());
    assert(s.allocation==0x11223344u&&s.auxiliary==45u);
    s.guard=0u;r::original_release_indexed_storage(s,h);
    assert((h.log==std::vector<std::string>{"free"}));
    assert(s.allocation==0u&&s.auxiliary==0u&&s.guard==0u);
    h.log.clear();s.auxiliary=67;
    r::original_release_indexed_storage(s,h);
    assert(h.log.empty()&&s.auxiliary==0);
}
}
int main(){
    test_empty_paths_no_lazy_init();test_native_insertion_search_two_link_offsets();
    test_symbol_lookup_comparator_order_and_match();
    test_symbol_no_match_and_sentinel_init();
    test_symbol_second_false_return_has_no_lazy_init();
    test_symbol_record_insertion_and_count_wrap();test_guarded_release();
}