#include "sin_compat/original_server_registry_intern_insert.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace s=sin_compat::original_server_registry_intern_insert;
namespace {
struct Record {
  std::uint16_t value_plus8=0;
  std::uint16_t next_recycled=0xffffu;
  std::uint32_t value_plus12=0;
};
struct SlotHost {
    s::SlotState state{};
    std::vector<Record> records{};
    std::vector<std::string> log{};
    std::uint32_t previous_argument=0;
    bool insert_callback_called=false;
    s::PreparedInsert prepare_table_insertion(const s::RegistryKeyWords& key,
        std::uint32_t arg){
        log.emplace_back("prepare"); assert(key.first_word==0x27u);
        previous_argument=arg;return {0x234u,0x345u};
    }
    void expand_registry_capacity(s::SlotState& st,std::uint32_t how_many){
        log.emplace_back("grow");assert(how_many==1u);
        assert(&st==&state);
        st.capacity_08+=std::max<std::uint32_t>(how_many,2u);
        records.resize(st.capacity_08);
    }
    std::uint16_t next_recycled_slot(std::uint16_t slot){
        log.emplace_back("reuse");assert(slot<records.size());
        return records[slot].next_recycled;
    }
    void reset_record_word_at_plus8(std::uint16_t slot,std::uint16_t value){
        log.emplace_back("reset");assert(slot<records.size());
        records[slot].value_plus8=value;
    }
    void capture_slot_storage_base(){log.emplace_back("cache_storage_base");}
    std::uint16_t allocate_new_slot(){return s::original_allocate_registry_slot(state,*this);}
    void link_new_registry_slot(std::uint16_t slot,std::uint16_t first,
                                std::uint16_t second){
        log.emplace_back("link");insert_callback_called=true;
        assert(slot<records.size()&&first==0x234u&&second==0x345u);
    }
    void write_key_to_record_plus8(std::uint16_t slot,std::uint16_t first,
                                   std::uint32_t plus4){
        log.emplace_back("write");assert(slot<records.size());
        records[slot].value_plus8=first;
        records[slot].value_plus12=plus4;
    }
};
// Model 0x2228B0A0 using its own original 0x2228A3B0 slot helper rather
// than fabricating reuse/find logic inside the outer routine.
// The outer original model requires native allocate slot callback via
// original_allocate_registry_slot. This adapter exposes that spelling.
struct CompleteInsertHost: SlotHost {
    std::uint16_t allocate_registry_slot(){return allocate_new_slot();}
};
void test_insert_fresh_and_recycled(){
    CompleteInsertHost h{};
    const s::RegistryKeyWords key{0x27u,0x1234abcd};
    const auto first=s::original_insert_registry_record(h.state,key,44u,h);
    assert(first==0u&&h.state.used_count_16==1&&h.state.insertions_12==1);
    assert(h.records[0].value_plus8==key.first_word);
    assert(h.records[0].value_plus12==key.word_at_plus4);
    assert(h.previous_argument==44u&&h.insert_callback_called);
    assert((h.log==std::vector<std::string>{"prepare","grow","reset",
        "cache_storage_base","link","write"}));
    h.log.clear();h.state.free_head_14=0u;
    h.records[0].next_recycled=0xffffu;
    const auto second=s::original_insert_registry_record(h.state,key,45u,h);
    assert(second==0u&&h.state.used_count_16==1u&&h.state.insertions_12==2u);
    assert(h.state.free_head_14==s::kMissingSlot);
    assert((h.log==std::vector<std::string>{"prepare","reuse","reset",
        "cache_storage_base","link","write"}));
    // When capacity remains sufficient, no 0x22288390 call.
    h.log.clear();
    const auto third=s::original_insert_registry_record(h.state,key,46u,h);
    assert(third==1u&&h.state.used_count_16==2u);
    assert((h.log==std::vector<std::string>{"prepare","reset",
        "cache_storage_base","link","write"}));
}
void test_free_list_chain_and_wrap(){
    CompleteInsertHost h{};h.state.capacity_08=4u;h.state.used_count_16=4u;
    h.records.resize(4);
    h.state.free_head_14=2u;h.records[2].next_recycled=1u;
    h.records[1].next_recycled=0xffffu;
    assert(s::original_allocate_registry_slot(h.state,h)==2u);
    assert(h.state.free_head_14==1u&&h.state.used_count_16==4u);
    assert(s::original_allocate_registry_slot(h.state,h)==1u);
    assert(h.state.free_head_14==s::kMissingSlot);
    assert((h.log==std::vector<std::string>{"reuse","reset",
        "cache_storage_base","reuse","reset","cache_storage_base"}));
    // Original 16-bit counter wraps (not a valid runtime precondition).
    h.state.used_count_16=0xffffu;h.state.capacity_08=0xffffu;
    h.records.resize(65538);
    assert(s::original_allocate_registry_slot(h.state,h)==0xffffu);
    assert(h.state.used_count_16==0u);
}
struct PoolHost {
    std::vector<s::StringPoolChunk> chunks{};
    std::vector<std::vector<unsigned char>> bytes{};
    std::vector<std::string> log{};
    std::uint16_t existing=s::kMissingSlot;
    std::uint32_t allocation_cap=0,allocation_raw=0;
    std::uint16_t last_chunk=0,last_offset=0;
    std::string last_pending{};
    void set_pending_intern_name(const char* name){
        log.emplace_back("pending");last_pending=name;
    }
    std::uint16_t find_interned_name(const char* name){
        log.emplace_back("probe");assert(std::string(name)==last_pending);
        return existing;
    }
    std::int32_t first_pool_with_space(std::uint32_t count){
        log.emplace_back("find_chunk");
        return s::original_first_pool_with_space(chunks,count);
    }
    std::int32_t allocate_pool_with_header(std::uint32_t cap,std::uint32_t raw){
        log.emplace_back("allocate_pool");
        assert(raw==cap+s::kNativeChunkAllocationOverhead);
        allocation_cap=cap;allocation_raw=raw;
        chunks.push_back({cap,0});bytes.emplace_back(cap,0xEEu);
        return static_cast<std::int32_t>(chunks.size()-1);
    }
    std::uint32_t pool_bytes_used(std::uint32_t index){
        log.emplace_back("pool_used");return chunks.at(index).used;
    }
    void copy_name_into_pool(std::uint32_t index,std::uint32_t offset,
                             const char* name,std::uint32_t count){
        log.emplace_back("copy_name");
        auto& chunk=bytes.at(index);
        assert(offset+count<=chunk.size());
        std::memcpy(chunk.data()+offset,name,count);
        assert(chunk[offset+count-1u]==0u);
    }
    void advance_pool_used(std::uint32_t index,std::uint32_t count){
        log.emplace_back("advance_used");chunks.at(index).used+=count;
    }
    std::uint16_t register_interned_locator(std::uint16_t index,
                                           std::uint16_t offset){
        log.emplace_back("register_locator");last_chunk=index;last_offset=offset;
        return static_cast<std::uint16_t>(0x100u+index);
    }
};
void test_intern_null_and_existing(){
    PoolHost host{};
    assert(s::original_intern_classname<PoolHost>(nullptr,host)==0xffffu);
    assert(host.log.empty());
    host.existing=0x44u;
    assert(s::original_intern_classname("flak",host)==0x44u);
    assert((host.log==std::vector<std::string>{"pending","probe"}));
}
void test_intern_existing_pool_first_fit_and_nul_copy(){
    PoolHost h{};
    h.chunks={{10u,10u},{20u,3u},{4096u,0u}};
    h.bytes={{std::vector<unsigned char>(10,0xEEu)},
             std::vector<unsigned char>(20,0xEEu),
             std::vector<unsigned char>(4096,0xEEu)};
    const auto id=s::original_intern_classname("flak",h);
    assert(id==0x101u&&h.last_chunk==1u&&h.last_offset==3u);
    assert(h.chunks[1].used==8u);
    assert((h.bytes[1][3]=='f'&&h.bytes[1][7]==0));
    assert((h.log==std::vector<std::string>{"pending","probe","find_chunk",
        "pool_used","copy_name","advance_used","register_locator"}));
}
void test_intern_new_pool_min_and_large(){
    PoolHost h{};
    const auto empty_id=s::original_intern_classname("",h);
    assert(empty_id==0x100u&&h.allocation_cap==2048u&&h.allocation_raw==2059u);
    assert(h.chunks[0].used==1u&&h.bytes[0][0]==0u);
    assert((h.log==std::vector<std::string>{"pending","probe","find_chunk",
        "allocate_pool","pool_used","copy_name","advance_used","register_locator"}));
    PoolHost bigger{};
    const std::string huge(2050,'X');
    const auto result=s::original_intern_classname(huge.c_str(),bigger);
    assert(result==0x100u);
    assert(bigger.allocation_cap==2051u&&bigger.allocation_raw==2062u);
    assert(bigger.chunks[0].used==2051u);
}
void test_chunk_first_fit_signed_and_offset_truncation(){
    std::vector<s::StringPoolChunk> chunks{{2u,2u},{10u,4u},{99u,0u}};
    assert(s::original_first_pool_with_space(chunks,1u)==1);
    assert(s::original_first_pool_with_space(chunks,7u)==2);
    assert(s::original_first_pool_with_space(chunks,100u)==-1);
    chunks={{0u,0xffffffffu}};
    assert(s::original_first_pool_with_space(chunks,1u)==0);
    chunks={{10u,20u}};
    assert(s::original_first_pool_with_space(chunks,1u)==-1);
    // Existing used-offset's low WORD, not absolute 32-bit offset, goes
    // into 0x2228F710 locator. Allocate sufficient host memory for copy.
    PoolHost h{};
    h.chunks={{70000u,65542u}};
    h.bytes={std::vector<unsigned char>(70000u,0)};
    assert(s::original_intern_classname("xy",h)==0x100u);
    assert(h.last_offset==6u&&h.chunks[0].used==65545u);
}
}
int main(){
 test_insert_fresh_and_recycled();
 test_free_list_chain_and_wrap();
 test_intern_null_and_existing();
 test_intern_existing_pool_first_fit_and_nul_copy();
 test_intern_new_pool_min_and_large();
 test_chunk_first_fit_signed_and_offset_truncation();
}