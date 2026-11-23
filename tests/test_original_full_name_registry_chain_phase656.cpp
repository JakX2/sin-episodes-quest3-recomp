#include "sin_compat/original_nested_insertion_phase651.hpp"
#include "sin_compat/original_string_lookup_phase650.hpp"
#include "sin_compat/original_nested_comparators_phase648.hpp"
#include "sin_compat/original_string_casefold_phase649.hpp"
#include "sin_compat/original_nested_tree_core_phase652.hpp"
#include "sin_compat/original_nested_slot_growth_phase653.hpp"
#include "sin_compat/original_nested_rebalance_phase654.hpp"
#include "sin_compat/original_nested_clear_phase655.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <functional>
namespace q=sin_compat::original_string_lookup_phase650;
namespace n=sin_compat::original_nested_insertion_phase651;
namespace c=sin_compat::original_nested_comparators_phase648;
namespace f=sin_compat::original_string_casefold_phase649;
namespace p=sin_compat::original_nested_tree_core_phase652;
namespace g=sin_compat::original_nested_slot_growth_phase653;
namespace r=sin_compat::original_nested_rebalance_phase654;
namespace z=sin_compat::original_nested_clear_phase655;
namespace {
constexpr std::uint32_t m=0x4000u,output=0x400u,ref=0x408u;
constexpr std::uint32_t slots=0x10000u,block_pointers=0x20000u;
struct Guest final:n::IHost,q::IHost,c::IHost,f::IHost,
                    p::IHost,g::IHost,r::IHost,z::IHost {
 std::map<std::uint32_t,std::uint8_t> data;
 c::Comparators* comparator=nullptr;
 f::Casefold* casefold=nullptr;
 q::LookupOrInsert* lookup=nullptr;
 n::ArrayGrowth* native_growth=nullptr;
 n::BlockArrayInsert* native_append=nullptr;
 n::TreeInsert* native_insertion=nullptr;
 p::FindParent* native_parent=nullptr;
 p::AcquireSlot* native_acquire=nullptr;
 p::LinkNode* native_link=nullptr;
 g::GrowSlots* native_slot_grow=nullptr;
 g::ResolveSlot* native_slot_resolve=nullptr;
 r::Rebalance* native_rebalance=nullptr;
 q::TreeSearch* native_search=nullptr;
 std::vector<std::uint32_t> freed_blocks;
 std::uint32_t slot_allocations=0u,slot_reallocations=0u;
 std::uint32_t pointer_array_allocations=0u,pointer_array_reallocations=0u;
 std::uint32_t pointer_array_address=block_pointers;
 std::uint32_t move_count=0u,find_parent_calls=0u,link_calls=0u;
 n::Result last_append_result{},last_insert_result{};
 q::Code reentered=q::Code::Completed;
 std::uint32_t alloc_base=0x30000u,actual_alloc_count=0u;
 std::uint32_t appends=0u,registered=0u,compare_calls=0u,casefold_calls=0u;
 std::uint32_t fail_address=0u;bool fail_alloc=false,fail_append=false,fail_insert=false;
 bool throw_compare=false,reenter_lookup=false;
 void put8(std::uint32_t a,std::uint8_t v){data[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){put8(a,static_cast<std::uint8_t>(v));put8(a+1u,static_cast<std::uint8_t>(v>>8));}
 void put32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4u;++i)put8(a+i,static_cast<std::uint8_t>(v>>(i*8u)));}
 std::uint8_t get8(std::uint32_t a)const {auto i=data.find(a);assert(i!=data.end());return i->second;}
 std::uint16_t get16(std::uint32_t a)const {return static_cast<std::uint16_t>(get8(a)|(get8(a+1u)<<8u));}
 std::uint32_t get32(std::uint32_t a)const {std::uint32_t v=0;for(unsigned i=0;i<4u;++i)v|=static_cast<std::uint32_t>(get8(a+i))<<(8u*i);return v;}
 bool read_u8(std::uint32_t va,std::uint8_t& v)override{
  if(fail_address==va)return false;
  auto i=data.find(va);if(i==data.end())return false;
  v=i->second;return true;
 }
 bool read_u16(std::uint32_t va,std::uint16_t& v)override{
  std::uint8_t a=0,b=0;if(!read_u8(va,a)||!read_u8(va+1u,b))return false;
  v=static_cast<std::uint16_t>(a|(static_cast<std::uint16_t>(b)<<8u));return true;
 }
 bool read_u32(std::uint32_t va,std::uint32_t& v)override{
  v=0;for(unsigned i=0;i<4u;++i){std::uint8_t b=0;if(!read_u8(va+i,b))return false;v|=static_cast<std::uint32_t>(b)<<(8u*i);}return true;
 }
 bool write_u8(std::uint32_t va,std::uint8_t v)override{
  if(fail_address==va)return false;
  put8(va,v);
  return true;
 }
 bool write_u16(std::uint32_t va,std::uint16_t v)override{
  if(fail_address==va)return false;
  put16(va,v);
  return true;
 }
 bool write_u32(std::uint32_t va,std::uint32_t v)override{
  if(fail_address==va)return false;
  put32(va,v);
  return true;
 }
 bool compare_original_223a5574(std::uint32_t left,std::uint32_t right,std::int32_t& eax)override{
  ++casefold_calls;assert(casefold);
  const auto r=casefold->compare(left,right);
  if(r.code!=f::Code::Completed)return false;
  eax=r.comparison;return true;
 }
 bool classify_original_223a898a(std::uint32_t,std::uint32_t,std::uint32_t& value)override{value=0u;return true;}
 bool locale_map_original_223ac9e7(const f::MapRequest&,f::MapAnswer&)override{return false;}
 bool compare_original_nested(std::uint32_t function,std::uint32_t key,
    std::uint32_t node,std::uint32_t context,bool& less)override{
  assert(function==c::kDelegatingVA && context==m && comparator);
  ++compare_calls;
  if(throw_compare)throw std::runtime_error("comparator exception");
  if(reenter_lookup&&lookup){
   reentered=lookup->run(m,output,0x5000u,ref).code;
  }
  const auto r=comparator->delegated(key,node,context);
  if(r.code!=c::Code::Completed)return false;
  less=r.eax!=0u;return true;
 }
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& ptr)override{
  if(fail_alloc)return false;
  if(size==36u||size==72u||size==144u||size==288u){
   ++slot_allocations;ptr=slots;
   for(std::uint32_t i=0u;i<size;++i)put8(ptr+i,0u);
   return true;
  }
  if(size < q::kDefaultBlockCapacity+11u){
   ++pointer_array_allocations;
   ptr=pointer_array_address;
   for(std::uint32_t i=0u;i<size;++i)put8(ptr+i,0u);
   return true;
  }
  ++actual_alloc_count;ptr=alloc_base;alloc_base+=0x10000u;
  return true;
 }
 bool reallocate_original_2226e400(std::uint32_t previous,
       std::uint32_t bytes,std::uint32_t& ptr)override {
  assert(bytes<=0x10000u);
  if(previous>=slots&&previous<block_pointers){
   ++slot_reallocations;
   ptr=slots+0x1000u*slot_reallocations;
  }else{
   ++pointer_array_reallocations;
   ptr=pointer_array_address+0x1000u*pointer_array_reallocations;
  }
  // Synthetic allocator copies its old DWORD array to newly reserved VA.
  for(std::uint32_t i=0u;i<bytes;++i){
   auto at=data.find(previous+i);
   put8(ptr+i,at==data.end()?0u:at->second);
  }
  return true;
 }
 bool move_original_223a5d10(std::uint32_t dst,std::uint32_t src,
                               std::uint32_t bytes)override {
  ++move_count;
  std::vector<std::uint8_t> saved;
  for(std::uint32_t i=0u;i<bytes;++i)saved.push_back(get8(src+i));
  for(std::uint32_t i=0u;i<bytes;++i)put8(dst+i,saved[i]);
  return true;
 }
 bool append_original_2228ed80(std::uint32_t owner,std::uint32_t block,
    std::uint32_t& index)override{
  assert(owner==m+0x1cu && native_append);
  if(fail_append)return false;
  const std::uint32_t scratch=0x6200u;
  put32(scratch,block);
  const auto old_count=get32(m+0x28u);
  last_append_result=native_append->insert(owner,old_count,scratch);
  if(last_append_result.code!=n::Code::Completed)return false;
  index=last_append_result.returned_index;
  ++appends;
  return true;
 }
 bool compare_original_indirect(std::uint32_t function,std::uint32_t key,
    std::uint32_t node,std::uint32_t context,std::uint8_t& al)override{
  bool less=false;
  if(!compare_original_nested(function,key,node,context,less))return false;
  al=less?1u:0u;return true;
 }
 bool grow_original_2228e200(std::uint32_t owner,std::uint32_t inc)override{
  assert(owner==m+4u&&native_slot_grow);
  return native_slot_grow->grow(owner,inc).code==g::Code::Completed;
 }
 bool resolve_original_2228e100(std::uint32_t map,std::uint16_t id,
     std::uint32_t& pointer)override{
  assert(map==m&&native_slot_resolve);
  const auto value=native_slot_resolve->resolve(map,id);
  if(value.code!=g::Code::Completed)return false;
  pointer=value.result_pointer;return true;
 }
 bool rebalance_original_2228edf0(std::uint32_t map,std::uint16_t id)override{
  assert(map==m&&native_rebalance);
  return native_rebalance->fixup(map,id).code==r::Code::Completed;
 }
 bool find_parent_original_2228e470(std::uint32_t map,
    std::uint32_t key_ref,std::uint32_t context,std::uint16_t& parent,
    std::uint8_t& branch)override{
  assert(map==m && key_ref==ref && context==m && native_parent);
  ++find_parent_calls;
  constexpr std::uint32_t kOutParent=0x470u,kOutBranch=0x474u;
  const auto result=native_parent->find(map,key_ref,kOutParent,kOutBranch,context);
  if(result.code!=p::Code::Completed)return false;
  parent=get16(kOutParent);branch=get8(kOutBranch);return true;
 }
 bool acquire_slot_original_2228e620(std::uint32_t map,
                                     std::uint16_t& handle)override {
  assert(map==m&&native_acquire);
  const auto result=native_acquire->acquire(map);
  if(result.code!=p::Code::Completed)return false;
  handle=result.handle;++registered;return true;
 }
 bool link_original_2228f460(std::uint32_t map,std::uint16_t handle,
    std::uint16_t parent,std::uint8_t branch)override{
  assert(map==m&&native_link);
  const auto result=native_link->link(map,handle,parent,branch);
  if(result.code!=p::Code::Completed)return false;
  ++link_calls;return true;
 }
 bool free_original_2226e3e0(std::uint32_t block)override {
  freed_blocks.push_back(block);return true;
 }
 bool find_original_2228eb40(std::uint32_t map,std::uint32_t key,
     std::uint32_t context,std::uint16_t& out)override{
  assert(native_search);
  const auto result=native_search->find(map,key,context);
  if(result.code!=q::Code::Completed)return false;
  out=result.handle;return true;
 }
 bool register_original_2228f710(std::uint32_t map,std::uint32_t string_ref,
    std::uint16_t& handle)override{
  assert(map==m&&string_ref==ref&&native_insertion);
  if(fail_insert)return false;
  last_insert_result=native_insertion->insert(map,string_ref,map);
  if(last_insert_result.code!=n::Code::Completed)return false;
  handle=last_insert_result.handle;
  return true;
 }
 static constexpr std::uint16_t kInvalid(){return q::kInvalid;}
 void string(std::uint32_t addr,const std::string& s){for(std::size_t i=0;i<s.size();++i)put8(addr+static_cast<std::uint32_t>(i),static_cast<std::uint8_t>(s[i]));put8(addr+static_cast<std::uint32_t>(s.size()),0u);}
 void init(){
  put32(m,c::kDelegatingVA);put32(m+4u,slots);put32(m+8u,32u);put32(m+0xcu,0u);
  put16(m+0x10u,q::kInvalid);put16(m+0x14u,q::kInvalid);
  put16(m+0x12u,0u);put16(m+0x16u,0u);
  put32(m+0x1cu,0u);put32(m+0x20u,0u);put32(m+0x24u,8u);put32(m+0x28u,0u);
  put32(m+0x30u,0xaaaaaaaau);put32(f::kModeGlobal,0u);
  put32(q::kSentinelStateVA,0u);put16(output,0x1234u);
 }
};
void test_complete_original_nested_registration_and_search(){
 Guest h;h.init();q::TreeSearch search(h);q::BlockFit fit(h);q::LookupOrInsert lookup(h,search,fit);
 c::Comparators cmp(h);f::Casefold casefold(h);h.comparator=&cmp;h.casefold=&casefold;h.lookup=&lookup;
 n::ArrayGrowth native_growth(h);n::BlockArrayInsert native_append(h,native_growth);
 n::TreeInsert native_insertion(h);
 h.native_growth=&native_growth;h.native_append=&native_append;
 h.native_insertion=&native_insertion;
 p::FindParent native_parent(h);p::AcquireSlot native_acquire(h);
 p::LinkNode native_link(h);g::GrowSlots native_slot_grow(h);
 g::ResolveSlot native_slot_resolve(h);r::Rebalance native_rebalance(h);
 h.native_parent=&native_parent;h.native_acquire=&native_acquire;
 h.native_link=&native_link;h.native_slot_grow=&native_slot_grow;
 h.native_slot_resolve=&native_slot_resolve;h.native_rebalance=&native_rebalance;
 h.native_search=&search;
 auto r=lookup.run(m,output,0u,ref);
 assert(r.code==q::Code::Completed&&r.original_null_name&&r.handle==q::kInvalid);
 assert(h.get16(output)==q::kInvalid && h.get32(m+0x30u)==0xaaaaaaaau);
 assert(h.registered==0u&&h.appends==0u);
 h.string(0x5000u,"Alpha");
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::Completed&&r.inserted&&r.handle==0u);
 assert(r.allocated_block&&r.bytes_required==6u&&r.bytes_copied==6u);
 assert(r.block_index==0u&&h.actual_alloc_count==1u&&h.appends==1u);
 assert(h.get16(output)==0u&&h.get16(m+0x10u)==0u);
 assert(h.get32(m+0x30u)==0x5000u);
 assert(h.get32(h.get32(block_pointers))==0x800u);
 assert(h.get32(h.get32(block_pointers)+4u)==6u);
 assert(h.get8(h.get32(block_pointers)+8u)=='A');
 assert(h.get16(slots+8u)==0u&&h.get16(slots+10u)==0u);
 assert(h.get32(q::kSentinelStateVA)==0u); // first empty lookup has no traversal
 h.string(0x5100u,"ALPHA");
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::Completed&&r.found_existing&&r.handle==0u);
 assert(!r.inserted&&r.bytes_copied==0u&&h.registered==1u&&h.appends==1u);
 assert(h.get32(m+0x30u)==0x5100u);
 assert(h.get32(h.get32(block_pointers)+4u)==6u);
 assert(h.get32(q::kSentinelStateVA)==0u); // equality does NOT initialize sentinel
 h.string(0x5200u,"Zulu");
 r=lookup.run(m,output,0x5200u,ref);
 assert(r.code==q::Code::Completed&&r.handle==1u&&r.inserted);
 assert(!r.allocated_block&&r.block_used_before==6u);
 assert(h.get16(slots+2u)==1u && h.get16(slots+12u+4u)==0u);
 assert(h.get32(q::kSentinelStateVA)==1u);
 assert(h.get16(q::kSentinelWordsVA)==q::kInvalid);
 assert(h.get16(q::kSentinelWordsVA+6u)==1u);
 h.string(0x5300u,"aardvark");
 r=lookup.run(m,output,0x5300u,ref);
 assert(r.code==q::Code::Completed&&r.handle==2u&&r.inserted);
 assert(h.get16(slots)==2u);
 assert(h.registered==3u&&h.appends==1u);
 h.string(0x5400u,"zulu");
 r=lookup.run(m,output,0x5400u,ref);
 assert(r.code==q::Code::Completed&&r.found_existing&&r.handle==1u);
 assert(h.registered==3u);
 // Deliberately fill current block; the next string goes to a new block.
 const auto first=h.get32(block_pointers);h.put32(first+4u,0x800u-2u);
 h.string(0x5500u,"Echo");
 r=lookup.run(m,output,0x5500u,ref);
 assert(r.code==q::Code::Completed&&r.inserted&&r.allocated_block);
 assert(r.block_index==1u&&r.handle==3u&&h.appends==2u);
 assert(h.get16(slots+3u*12u+8u)==1u);
 assert(h.get16(slots+3u*12u+10u)==0u);
 h.string(0x5600u,"ECHO");
 r=lookup.run(m,output,0x5600u,ref);
 assert(r.code==q::Code::Completed&&r.found_existing&&r.handle==3u);
 assert(h.registered==4u && h.compare_calls>0u && h.casefold_calls>0u);
 assert(h.pointer_array_allocations==1u && h.pointer_array_reallocations==0u);
 assert(h.get32(m+0x20u)==8u && h.get32(m+0x28u)==2u);
 assert(h.get32(m+0x2cu)==h.get32(m+0x1cu));
 assert(h.find_parent_calls==4u && h.link_calls==4u);
 assert(h.get16(m+0x12u)==4u);
 assert(h.last_insert_result.code==n::Code::Completed);
 assert(h.last_append_result.code==n::Code::Completed);
 // Force more than eight stored blocks. This invokes the *compiled* native
 // 0x2228E280 8->16 capacity growth, synthetic relocation callback and
 // original modeled block-array insertion, while future comparisons still
 // resolve all old strings through the relocated map+0x1C array pointer.
 for(std::uint32_t i=0u;i<7u;++i){
  const auto blocks=h.get32(m+0x1cu);
  const auto count=h.get32(m+0x28u);
  for(std::uint32_t k=0u;k<count;++k){
   const auto storage=h.get32(blocks+k*4u);
   h.put32(storage+4u,h.get32(storage)-1u);
  }
  const std::string key="NewUnique"+std::to_string(i);
  const auto key_va=0x5800u+i*0x80u;
  h.string(key_va,key);
  const auto appended=lookup.run(m,output,key_va,ref);
  assert(appended.code==q::Code::Completed&&appended.inserted);
  assert(appended.allocated_block);
  assert(appended.block_index==count);
  assert(h.get32(m+0x28u)==count+1u);
 }
 assert(h.get32(m+0x28u)==9u);
 assert(h.pointer_array_allocations==1u);
 assert(h.pointer_array_reallocations==1u);
 assert(h.get32(m+0x20u)==16u);
 assert(h.get32(m+0x2cu)==h.get32(m+0x1cu));
 assert(h.registered==11u&&h.get16(m+0x12u)==11u);
 h.string(0x5f00u,"ALPHA");
 const auto prior=lookup.run(m,output,0x5f00u,ref);
 assert(prior.code==q::Code::Completed&&prior.found_existing&&prior.handle==0u);
 assert(h.registered==11u);
}
void test_block_fit_and_error_edges(){
 Guest h;h.init();q::TreeSearch search(h);q::BlockFit fit(h);q::LookupOrInsert lookup(h,search,fit);
 c::Comparators cmp(h);f::Casefold casefold(h);h.comparator=&cmp;h.casefold=&casefold;h.lookup=&lookup;
 n::ArrayGrowth native_growth(h);n::BlockArrayInsert native_append(h,native_growth);
 n::TreeInsert native_insertion(h);
 h.native_growth=&native_growth;h.native_append=&native_append;
 h.native_insertion=&native_insertion;
 p::FindParent native_parent(h);p::AcquireSlot native_acquire(h);
 p::LinkNode native_link(h);g::GrowSlots native_slot_grow(h);
 g::ResolveSlot native_slot_resolve(h);r::Rebalance native_rebalance(h);
 h.native_parent=&native_parent;h.native_acquire=&native_acquire;
 h.native_link=&native_link;h.native_slot_grow=&native_slot_grow;
 h.native_slot_resolve=&native_slot_resolve;h.native_rebalance=&native_rebalance;
 h.native_search=&search;
 auto r=fit.find(m,6u);assert(r.code==q::Code::Completed&&r.block_index==0xffffffffu);
 assert(search.find(m,ref,m).code==q::Code::Completed);
 assert(lookup.run(0u,output,0x5000u,ref).code==q::Code::NullPointerHazard);
 assert(lookup.run(m,0u,0x5000u,ref).code==q::Code::NullPointerHazard);
 h.fail_alloc=true;h.string(0x5000u,"abc");
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::HostRejected&&r.stage==q::Stage::AllocateBlock);
 assert(h.get32(m+0x30u)==0x5000u); // committed before allocation fails
 assert(h.get16(ref)==q::kInvalid&&h.get16(ref+2u)==q::kInvalid);
 h.fail_alloc=false;h.fail_append=true;
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::HostRejected&&r.allocated_block);
 assert(r.stage==q::Stage::AppendBlockPointer);
 h.fail_append=false;
 r=lookup.run(m,output,0x5000u,ref);
 assert(r.code==q::Code::Completed&&r.inserted&&r.handle==0u);
 // When callback throws, no synthetic success is claimed.
 h.throw_compare=true;h.string(0x5100u,"beta");
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::HostRejected&&r.stage==q::Stage::CompareKeyNode);
 h.throw_compare=false;
 h.reenter_lookup=true;
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::Completed&&h.reentered==q::Code::Reentrant);
 h.reenter_lookup=false;
 h.fail_address=m+0x30u;
 r=lookup.run(m,output,0x5100u,ref);
 assert(r.code==q::Code::HostRejected&&r.stage==q::Stage::WriteFallback30);
 h.fail_address=0u;
 // Original BlockFit is signed; upper-bit count is non-positive.
 h.put32(m+0x28u,0xffffffffu);
 r=fit.find(m,1u);assert(r.code==q::Code::Completed&&r.block_index==0xffffffffu);
 h.put32(m+0x28u,1u);
 const auto block=h.get32(block_pointers);
 h.put32(block,0xffffffffu);h.put32(block+4u,0u);
 r=fit.find(m,5u);assert(r.code==q::Code::Completed&&r.block_index==0xffffffffu);
 h.put32(block,0x800u);h.put32(block+4u,0u);
 r=fit.find(m,5u);assert(r.code==q::Code::Completed&&r.block_index==0u);
 h.put16(m+0x10u,0u);h.put16(slots,0u);h.put16(slots+2u,0u);
 // Force a cycle, with key always greater than the sole node.
 h.string(0x5200u,"zzzz");
 h.put32(m+0x30u,0x5200u);
 h.put16(ref,q::kInvalid);h.put16(ref+2u,q::kInvalid);
 h.put16(slots+2u,0u);
 r=search.find(m,ref,m);
 assert(r.code==q::Code::CycleGuard && r.comparisons==131072u);
}

void test_complete_real_string_storage_growth_clear_and_reuse(){
 Guest h;h.init();
 // Native zero-capacity map: Phase653 must allocate 3 nodes, then
 // reallocate to 6 and 12, while distinct Phase651 block array grows 0→8.
 h.put32(m+4u,0u);h.put32(m+8u,0u);
 q::TreeSearch search(h);q::BlockFit fit(h);
 q::LookupOrInsert lookup(h,search,fit);
 c::Comparators cmp(h);f::Casefold casefold(h);
 n::ArrayGrowth arrgrow(h);n::BlockArrayInsert append(h,arrgrow);
 n::TreeInsert insert(h);
 p::FindParent parent(h);p::AcquireSlot acquire(h);p::LinkNode link(h);
 g::GrowSlots slotgrow(h);g::ResolveSlot resolver(h);
 r::Rebalance rebalance(h);
 z::ClearSlots clear(h);z::ReleaseBlocks release(h,clear);
 z::FindNameView view(h);
 h.comparator=&cmp;h.casefold=&casefold;h.lookup=&lookup;
 h.native_growth=&arrgrow;h.native_append=&append;
 h.native_insertion=&insert;
 h.native_parent=&parent;h.native_acquire=&acquire;
 h.native_link=&link;h.native_slot_grow=&slotgrow;
 h.native_slot_resolve=&resolver;h.native_rebalance=&rebalance;
 h.native_search=&search;
 const std::vector<std::string> names={
   "Mango","Apple","Peach","Banana","Grape","Lemon",
   "Orange","Cherry","Fig","Kiwi","Plum"};
 for(std::size_t i=0u;i<names.size();++i){
  const auto addr=0x7000u+static_cast<std::uint32_t>(i)*0x80u;
  h.string(addr,names[i]);
  const auto added=lookup.run(m,output,addr,ref);
  assert(added.code==q::Code::Completed&&added.inserted);
  assert(added.handle==static_cast<std::uint16_t>(i));
  assert(h.get16(m+0x12u)==i+1u);
 }
 assert(h.slot_allocations==1u&&h.slot_reallocations==2u);
 assert(h.get32(m+8u)==12u&&h.get16(m+0x16u)==11u);
 assert(h.pointer_array_allocations==1u);
 assert(h.get32(m+0x28u)==1u);
 const auto final_array=h.get32(m+4u);
 assert(final_array==slots+0x2000u);
 // Verify complete red-black invariants in the relocated 12-byte array,
 // including parent backlinks, sorted inorder strings and black-height.
 std::vector<bool> visited(names.size(),false);
 std::vector<std::string> ordered;
 const auto read_string=[&](std::uint16_t id)->std::string{
  const auto base=final_array+static_cast<std::uint32_t>(id)*12u;
  const auto block_id=h.get16(base+8u),offset=h.get16(base+10u);
  const auto blocklist=h.get32(m+0x1cu);
  const auto block=h.get32(blocklist+block_id*4u);
  std::string text;
  for(unsigned j=0u;j<80u;++j){
   const auto ch=h.get8(block+8u+offset+j);
   if(!ch)break;
   const auto lowered=(ch>='A'&&ch<='Z')?ch-'A'+'a':ch;
   text.push_back(static_cast<char>(lowered));
  }
  return text;
 };
 const auto walk=[&](auto&& self,std::uint16_t id,std::uint16_t father)->unsigned{
  if(id==0xffffu)return 1u;
  assert(id<names.size()&&!visited.at(id));
  visited[id]=true;
  const auto base=final_array+static_cast<std::uint32_t>(id)*12u;
  assert(h.get16(base+4u)==father);
  const auto left=h.get16(base),right=h.get16(base+2u),color=h.get16(base+6u);
  assert(color<=1u);
  if(color==0u){
   if(left!=0xffffu)assert(h.get16(final_array+left*12u+6u)==1u);
   if(right!=0xffffu)assert(h.get16(final_array+right*12u+6u)==1u);
  }
  const auto lhs=self(self,left,id);
  ordered.push_back(read_string(id));
  const auto rhs=self(self,right,id);
  assert(lhs==rhs);
  return lhs+(color==1u?1u:0u);
 };
 const auto root=h.get16(m+0x10u);
 assert(root!=0xffffu&&h.get16(final_array+root*12u+6u)==1u);
 assert(walk(walk,root,0xffffu)>=2u);
 for(bool v:visited)assert(v);
 assert(ordered.size()==names.size());
 for(std::size_t i=1u;i<ordered.size();++i)assert(ordered[i-1u]<ordered[i]);
 // Phase648/649 original delegated case folding must return the
 // existing 1-based Apple slot, with no added node or string storage.
 constexpr std::uint32_t kCase=0x7e00u,kViewScratch=0x7f00u,kViewOut=0x7f10u;
 h.string(kCase,"aPpLe");h.put32(kViewScratch,0u);
 const auto prior=h.get16(m+0x16u);
 const auto found=view.run(m,kViewOut,kCase,kViewScratch);
 assert(found.code==z::Code::Completed&&found.handle==1u);
 assert(h.get16(kViewOut)==1u&&h.get16(m+0x16u)==prior);
 assert(h.get16(kViewScratch)==0xffffu&&h.get16(kViewScratch+2u)==0xffffu);
 assert(h.compare_calls>0u&&h.casefold_calls>0u);
 const auto original_blocks=h.get32(m+0x1cu);
 const auto original_string_block=h.get32(original_blocks);
 const auto old_growth=h.slot_reallocations;
 const auto cleaned=release.run(m);
 assert(cleaned.code==z::Code::Completed);
 assert(cleaned.slots_recycled==11u&&cleaned.block_free_calls==1u);
 assert(h.freed_blocks.size()==1u&&h.freed_blocks.front()==original_string_block);
 assert(h.get16(m+0x10u)==0xffffu&&h.get16(m+0x12u)==0u);
 assert(h.get16(m+0x14u)==0u&&h.get16(m+0x16u)==11u);
 assert(h.get32(m+4u)==final_array&&h.get32(m+8u)==12u);
 assert(h.get32(m+0x1cu)==original_blocks&&h.get32(m+0x28u)==0u);
 for(unsigned i=0u;i<11u;++i){
  const auto node=final_array+i*12u;
  assert(h.get16(node)==i);
  assert(h.get16(node+2u)==(i==10u?0xffffu:i+1u));
 }
 // Re-register after teardown, reusing slot zero; no new slot reallocation.
 h.string(0x7d00u,"Reborn");
 const auto reused=lookup.run(m,output,0x7d00u,ref);
 assert(reused.code==q::Code::Completed&&reused.inserted&&reused.handle==0u);
 assert(h.get16(m+0x14u)==1u&&h.get16(m+0x16u)==11u);
 assert(h.slot_reallocations==old_growth);
 assert(h.get32(m+4u)==final_array);
 assert(h.get16(m+0x10u)==0u&&h.get16(final_array+6u)==1u);
 assert(h.get32(m+0x28u)==1u);
 // An old record from the released tree is no longer discoverable.
 h.string(0x7c00u,"MANGO");
 const auto missing=view.run(m,kViewOut,0x7c00u,kViewScratch);
 assert(missing.code==z::Code::Completed&&missing.handle==0xffffu);
 assert(h.get16(kViewOut)==0xffffu);
}
} // namespace
int main(){test_complete_original_nested_registration_and_search();
 test_block_fit_and_error_edges();test_complete_real_string_storage_growth_clear_and_reuse();return 0;}
