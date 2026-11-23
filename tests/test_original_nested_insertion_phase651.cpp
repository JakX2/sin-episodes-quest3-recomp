#include "sin_compat/original_nested_insertion_phase651.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>
namespace n=sin_compat::original_nested_insertion_phase651;
namespace {
constexpr std::uint32_t kOwner=0x3000u,kSource=0x4000u;
constexpr std::uint32_t kMap=0x7000u,kKey=0x8000u,kSlots=0x9000u;
struct Host final:n::IHost {
 std::map<std::uint32_t,std::uint8_t> memory;
 std::uint32_t next_alloc=0x10000u,allocs=0,reallocs=0,moves=0,finds=0,acquires=0,links=0;
 std::uint32_t last_alloc_bytes=0,last_realloc_bytes=0,last_moved_bytes=0;
 std::uint16_t last_parent=n::kInvalid,last_handle=n::kInvalid;
 std::uint8_t last_branch=0;
 bool fail_alloc=false,fail_realloc=false,fail_move=false,fail_find=false,fail_acquire=false,
      fail_link=false,fail_read=false,fail_write=false,throw_alloc=false,reenter=false,
      return_null=false;
 n::ArrayGrowth* growth=nullptr;n::BlockArrayInsert* appender=nullptr;
 n::TreeInsert* inserter=nullptr;n::Code reentered=n::Code::Completed;
 std::uint16_t suggested_parent=n::kInvalid;
 std::uint8_t suggested_branch=0u;
 void put8(std::uint32_t a,std::uint8_t v){memory[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){put8(a,static_cast<std::uint8_t>(v));put8(a+1u,static_cast<std::uint8_t>(v>>8));}
 void put32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4u;++i)put8(a+i,static_cast<std::uint8_t>(v>>(i*8)));}
 std::uint8_t byte(std::uint32_t a)const{auto i=memory.find(a);assert(i!=memory.end());return i->second;}
 std::uint16_t word(std::uint32_t a)const{return static_cast<std::uint16_t>(byte(a)|(static_cast<std::uint16_t>(byte(a+1u))<<8));}
 std::uint32_t dword(std::uint32_t a)const{
  std::uint32_t v=0u;for(unsigned i=0;i<4u;++i)v|=static_cast<std::uint32_t>(byte(a+i))<<(i*8);return v;
 }
 bool read_u16(std::uint32_t a,std::uint16_t& v)override {
  if(fail_read||!memory.count(a)||!memory.count(a+1u))return false;
  v=word(a);return true;
 }
 bool read_u32(std::uint32_t a,std::uint32_t& v)override {
  if(fail_read)return false;
  for(unsigned i=0;i<4u;++i)if(!memory.count(a+i))return false;
  v=dword(a);return true;
 }
 bool write_u16(std::uint32_t a,std::uint16_t v)override {if(fail_write)return false;put16(a,v);return true;}
 bool write_u32(std::uint32_t a,std::uint32_t v)override {if(fail_write)return false;put32(a,v);return true;}
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& out)override{
  ++allocs;last_alloc_bytes=size;
  if(reenter&&growth)reentered=growth->grow(kOwner,1u).code;
  if(throw_alloc)throw std::runtime_error("allocation callback exception");
  if(fail_alloc)return false;
  if(return_null){out=0u;return true;}
  out=next_alloc;next_alloc+=0x10000u;
  for(std::uint32_t i=0;i<size&&i<1048576u;++i)put8(out+i,0u);
  return true;
 }
 bool reallocate_original_2226e400(std::uint32_t previous,std::uint32_t size,
                                     std::uint32_t& out)override{
  ++reallocs;last_realloc_bytes=size;
  if(fail_realloc)return false;
  if(return_null){out=0u;return true;}
  out=next_alloc;next_alloc+=0x10000u;
  for(std::uint32_t i=0;i<size&&i<1048576u;++i){
   auto old=memory.find(previous+i);
   put8(out+i,old==memory.end()?0u:old->second);
  }
  return true;
 }
 bool move_original_223a5d10(std::uint32_t dst,std::uint32_t src,
                               std::uint32_t bytes)override{
  ++moves;last_moved_bytes=bytes;
  if(fail_move)return false;
  std::vector<std::uint8_t> old;
  for(std::uint32_t i=0;i<bytes&&i<1048576u;++i){
   auto pos=memory.find(src+i);if(pos==memory.end())return false;
   old.push_back(pos->second);
  }
  for(std::uint32_t i=0;i<old.size();++i)put8(dst+i,old[i]);
  return true;
 }
 bool find_parent_original_2228e470(std::uint32_t map,std::uint32_t key,
       std::uint32_t context,std::uint16_t& parent,std::uint8_t& branch)override{
  assert(map==kMap&&key==kKey&&context==kMap);
  ++finds;if(fail_find)return false;
  parent=suggested_parent;branch=suggested_branch;
  return true;
 }
 bool acquire_slot_original_2228e620(std::uint32_t map,std::uint16_t& handle)override{
  assert(map==kMap);++acquires;if(fail_acquire)return false;
  handle=word(kMap+0x16u);
  put16(kMap+0x16u,static_cast<std::uint16_t>(handle+1u));
  return true;
 }
 bool link_original_2228f460(std::uint32_t map,std::uint16_t handle,
      std::uint16_t parent,std::uint8_t branch)override{
  assert(map==kMap);++links;if(fail_link)return false;
  last_parent=parent;last_branch=branch;last_handle=handle;
  const auto record=kSlots+static_cast<std::uint32_t>(handle)*12u;
  put16(record,n::kInvalid);put16(record+2u,n::kInvalid);
  put16(record+4u,parent);put16(record+6u,0u); // original link initializes RED
  if(parent==n::kInvalid)put16(kMap+0x10u,handle);
  else put16(kSlots+static_cast<std::uint32_t>(parent)*12u+(branch?0u:2u),handle);
  // Real 0x2228F460 also invokes 0x2228EDF0 red-black rebalance;
  // this synthetic callback does not pretend to implement that callee.
  return true;
 }
 void initialize(){
  put32(kOwner,0u);put32(kOwner+4u,0u);put32(kOwner+8u,0u);
  put32(kOwner+0xcu,0u);put32(kOwner+0x10u,0u);
  put32(kSource,0xdeadbeefu);
  put32(kMap+4u,kSlots);put16(kMap+0x10u,n::kInvalid);
  put16(kMap+0x12u,0u);put16(kMap+0x14u,n::kInvalid);
  put16(kMap+0x16u,0u);put32(kKey,0x12345678u);
 }
};
void test_growth_exact_original_modes(){
 Host h;h.initialize();n::ArrayGrowth growth(h);h.growth=&growth;
 auto r=growth.grow(kOwner,1u);
 assert(r.code==n::Code::Completed&&r.old_capacity==0u);
 assert(r.new_capacity==8u&&r.alloc_bytes==32u&&r.growth_rounds==1u);
 assert(h.dword(kOwner+4u)==8u&&h.allocs==1u&&h.reallocs==0u);
 const auto original=h.dword(kOwner);
 h.put32(original,0x87654321u);
 r=growth.grow(kOwner,0u);
 assert(r.code==n::Code::Completed&&r.growth_rounds==0u);
 assert(r.reallocation_called&&r.alloc_bytes==32u&&h.reallocs==1u);
 assert(h.dword(h.dword(kOwner))==0x87654321u);
 r=growth.grow(kOwner,9u); // cap8 -> cap32, 2 doublings
 assert(r.code==n::Code::Completed&&r.new_capacity==32u);
 assert(r.growth_rounds==2u&&r.alloc_bytes==128u);
 assert(h.dword(h.dword(kOwner))==0x87654321u);
 h.put32(kOwner+4u,2u);h.put32(kOwner+8u,3u);
 r=growth.grow(kOwner,6u);
 assert(r.code==n::Code::Completed&&r.new_capacity==8u&&r.growth_rounds==2u);
 h.put32(kOwner+8u,n::kDisabledStep);
 const auto calls=h.allocs+h.reallocs;
 r=growth.grow(kOwner,1u);
 assert(r.code==n::Code::Completed&&r.disabled_growth);
 assert(h.allocs+h.reallocs==calls);
 h.put32(kOwner+8u,0u);h.put32(kOwner+4u,0x80000000u);
 r=growth.grow(kOwner,0u);
 assert(r.code==n::Code::Completed&&r.new_capacity==0x80000000u);
 assert(r.growth_rounds==0u&&r.alloc_bytes==0u); // signed >= target
 h.put32(kOwner+4u,0u);h.put32(kOwner,0u);
 h.throw_alloc=true;
 r=growth.grow(kOwner,1u);
 assert(r.code==n::Code::HostRejected&&r.writes_committed==1u);
 assert(h.dword(kOwner+4u)==8u&&h.dword(kOwner)==0u);
 h.throw_alloc=false;h.reenter=true;
 r=growth.grow(kOwner,0u);
 assert(r.code==n::Code::Completed&&h.reentered==n::Code::Reentrant);
 assert(growth.grow(0u,1u).code==n::Code::NullOwnerHazard);
 assert(growth.grow(0xfffffffau,1u).code==n::Code::AddressOverflow);
}
void test_array_insert_and_overlapping_move(){
 Host h;h.initialize();n::ArrayGrowth growth(h);n::BlockArrayInsert insert(h,growth);
 h.growth=&growth;h.appender=&insert;
 auto r=insert.insert(kOwner,0u,kSource);
 assert(r.code==n::Code::Completed&&r.returned_index==0u);
 assert(r.growth_called&&r.allocation_called&&r.inserted_record);
 assert(h.dword(kOwner+0xcu)==1u&&h.dword(kOwner+4u)==8u);
 assert(h.dword(kOwner+0x10u)==h.dword(kOwner));
 assert(h.dword(h.dword(kOwner))==0xdeadbeefu);
 for(std::uint32_t i=1u;i<8u;++i){
  h.put32(kSource,0x1000u+i);
  r=insert.insert(kOwner,i,kSource);
  assert(r.code==n::Code::Completed&&!r.growth_called&&!r.move_called);
 }
 const auto old=h.dword(kOwner);
 assert(h.dword(old+12u)==0x1003u);
 // Full array insertion at index 2 causes reallocation and overlapping shift.
 h.put32(kSource,0xcafebabeu);
 r=insert.insert(kOwner,2u,kSource);
 assert(r.code==n::Code::Completed&&r.growth_called&&r.reallocation_called);
 assert(r.move_called&&r.moved_bytes==6u*4u);
 const auto now=h.dword(kOwner);
 assert(now!=old&&h.dword(kOwner+4u)==16u);
 assert(h.dword(now+8u)==0xcafebabeu);
 assert(h.dword(now+12u)==0x1002u);
 assert(h.dword(now+32u)==0x1007u);
 assert(h.dword(kOwner+0xcu)==9u&&h.dword(kOwner+0x10u)==now);
 h.fail_move=true;
 r=insert.insert(kOwner,1u,kSource);
 assert(r.code==n::Code::HostRejected&&r.stage==n::Stage::MoveOriginal5D10);
 assert(h.dword(kOwner+0xcu)==10u); // native count committed before move
 h.fail_move=false;
 assert(insert.insert(0u,0u,kSource).code==n::Code::NullOwnerHazard);
}
void test_original_tree_insert_ref_and_failure_order(){
 Host h;h.initialize();n::TreeInsert inserter(h);h.inserter=&inserter;
 auto r=inserter.insert(kMap,kKey,kMap);
 assert(r.code==n::Code::Completed&&r.handle==0u);
 assert(r.parent==n::kInvalid&&r.branch==0u&&r.inserted_record);
 assert(h.word(kMap+0x12u)==1u&&h.word(kMap+0x16u)==1u);
 assert(h.word(kMap+0x10u)==0u&&h.dword(kSlots+8u)==0x12345678u);
 assert(h.word(kSlots+6u)==0u); // red prior to synthetic rebalance
 h.suggested_parent=0u;h.suggested_branch=1u;
 h.put32(kKey,0xa1b2c3d4u);
 r=inserter.insert(kMap,kKey,kMap);
 assert(r.code==n::Code::Completed&&r.handle==1u&&r.parent==0u&&r.branch==1u);
 assert(h.word(kSlots)==1u&&h.word(kSlots+12u+4u)==0u);
 assert(h.word(kMap+0x12u)==2u&&h.dword(kSlots+12u+8u)==0xa1b2c3d4u);
 h.fail_link=true;
 r=inserter.insert(kMap,kKey,kMap);
 assert(r.code==n::Code::HostRejected&&r.acquired_node&&!r.linked_node);
 assert(h.word(kMap+0x12u)==2u); // slot acquired before later link refusal
 h.fail_link=false;h.fail_find=true;
 r=inserter.insert(kMap,kKey,kMap);
 assert(r.code==n::Code::HostRejected&&!r.acquired_node);
 h.fail_find=false;h.fail_acquire=true;
 r=inserter.insert(kMap,kKey,kMap);
 assert(r.code==n::Code::HostRejected&&r.found_parent&&!r.acquired_node);
 assert(inserter.insert(0u,kKey,kMap).code==n::Code::NullOwnerHazard);
}
}
int main(){test_growth_exact_original_modes();test_array_insert_and_overlapping_move();
 test_original_tree_insert_ref_and_failure_order();return 0;}
