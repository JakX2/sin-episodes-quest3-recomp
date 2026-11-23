#include "sin_compat/original_nested_tree_core_phase652.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
namespace p=sin_compat::original_nested_tree_core_phase652;
namespace {
constexpr std::uint32_t kMap=0x4000u,kSlots=0x6000u,kKey=0x8000u;
constexpr std::uint32_t kParent=0x8100u,kBranch=0x8200u;
struct Host final:p::IHost {
 std::map<std::uint32_t,std::uint8_t> mem;
 p::AcquireSlot* acquirer=nullptr;p::LinkNode* linker=nullptr;p::FindParent* finder=nullptr;
 p::Code nested=p::Code::Completed;
 unsigned comparisons=0,grows=0,resolves=0,rebalances=0,writes=0;
 unsigned fail_write=0;bool fail_compare=false,fail_grow=false,fail_rebalance=false;
 bool throw_compare=false,reenter=false;
 std::vector<std::uint32_t> compared_values;
 void put8(std::uint32_t a,std::uint8_t v){mem[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){put8(a,static_cast<std::uint8_t>(v));put8(a+1u,static_cast<std::uint8_t>(v>>8));}
 void put32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4;i++)put8(a+i,static_cast<std::uint8_t>(v>>(8u*i)));}
 std::uint8_t get8(std::uint32_t a)const{auto i=mem.find(a);assert(i!=mem.end());return i->second;}
 std::uint16_t get16(std::uint32_t a)const{return static_cast<std::uint16_t>(get8(a)|(static_cast<std::uint16_t>(get8(a+1u))<<8));}
 std::uint32_t get32(std::uint32_t a)const{std::uint32_t v=0;for(unsigned i=0;i<4;i++)v|=static_cast<std::uint32_t>(get8(a+i))<<(8u*i);return v;}
 bool read_u8(std::uint32_t a,std::uint8_t& v)override{auto i=mem.find(a);if(i==mem.end())return false;v=i->second;return true;}
 bool read_u16(std::uint32_t a,std::uint16_t& v)override{if(!mem.count(a)||!mem.count(a+1u))return false;v=get16(a);return true;}
 bool read_u32(std::uint32_t a,std::uint32_t& v)override{for(unsigned i=0;i<4;i++)if(!mem.count(a+i))return false;v=get32(a);return true;}
 bool write_u8(std::uint32_t a,std::uint8_t v)override{if(++writes==fail_write)return false;put8(a,v);return true;}
 bool write_u16(std::uint32_t a,std::uint16_t v)override{if(++writes==fail_write)return false;put16(a,v);return true;}
 bool write_u32(std::uint32_t a,std::uint32_t v)override{if(++writes==fail_write)return false;put32(a,v);return true;}
 bool compare_original_indirect(std::uint32_t cmp,std::uint32_t key,std::uint32_t node,
   std::uint32_t context,std::uint8_t& al)override{
  assert(cmp==0x2228e9b0u&&context==kMap);++comparisons;
  if(reenter&&finder)nested=finder->find(kMap,kKey,kParent,kBranch,kMap).code;
  if(throw_compare)throw std::runtime_error("comparator failure");
  if(fail_compare)return false;
  auto l=get16(key),r=get16(node);compared_values.push_back(r);
  al=static_cast<std::uint8_t>(l<r?1:0);return true;
 }
 bool grow_original_2228e200(std::uint32_t owner,std::uint32_t additional)override{
  assert(owner==kMap+4u&&additional==1u);++grows;
  if(fail_grow)return false;
  auto cap=get32(kMap+8u);
  put32(kMap+8u,cap?cap*2u:8u);
  put32(kMap+4u,kSlots);
  return true;
 }
 bool resolve_original_2228e100(std::uint32_t map,std::uint16_t slot,
   std::uint32_t& node)override{
  assert(map==kMap);++resolves;
  node=kSlots+static_cast<std::uint32_t>(slot)*p::kStride;return true;
 }
 bool rebalance_original_2228edf0(std::uint32_t map,std::uint16_t handle)override{
  assert(map==kMap);++rebalances;
  if(reenter&&linker)nested=linker->link(kMap,handle,p::kInvalid,0u).code;
  if(fail_rebalance)return false;
  // The original 1367-byte rebalance is NOT implemented. The stub can
  // ensure a singleton root is black to support one-node synthetic use.
  if(get16(kMap+0x10u)==handle)put16(kSlots+handle*p::kStride+6u,1u);
  return true;
 }
 void reset(){mem.clear();comparisons=grows=resolves=rebalances=writes=0u;
  fail_write=0u;fail_compare=fail_grow=fail_rebalance=throw_compare=reenter=false;
  compared_values.clear();nested=p::Code::Completed;
  put32(kMap,0x2228e9b0u);put32(kMap+4u,kSlots);
  put32(kMap+8u,8u);put16(kMap+0x10u,p::kInvalid);
  put16(kMap+0x12u,0u);put16(kMap+0x14u,p::kInvalid);
  put16(kMap+0x16u,0u);put32(kMap+0x18u,kSlots);
  put32(p::kSentinelFlagVA,0u);
  for(unsigned i=0;i<8u;i++){
   auto a=kSlots+i*p::kStride;
   put16(a,p::kInvalid);put16(a+2u,p::kInvalid);
   put16(a+4u,p::kInvalid);put16(a+6u,0u);put32(a+8u,0u);
  }
  put16(kKey,0u);put16(kParent,0u);put8(kBranch,0u);
 }
};
void test_empty_parent_and_first_branch(){
 Host h;h.reset();p::FindParent find(h);h.finder=&find;
 auto r=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(r.code==p::Code::Completed&&r.visited==0u);
 assert(h.get16(kParent)==p::kInvalid&&h.get8(kBranch)==0u);
 assert(h.get32(p::kSentinelFlagVA)==0u);
 h.put16(kMap+0x10u,0u);h.put16(kSlots,1u);h.put16(kSlots+2u,2u);
 h.put16(kSlots+8u,30u);h.put16(kKey,20u);
 h.put16(kSlots+1u*p::kStride+0u,p::kInvalid);
 h.put16(kSlots+1u*p::kStride+8u,10u);
 r=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(r.code==p::Code::Completed&&r.visited==2u);
 assert(r.parent==1u&&r.branch==0u);
 assert(h.get16(kParent)==1u&&h.get8(kBranch)==0u);
 assert(h.get16(p::kSentinelVA)==p::kInvalid);
 assert(h.get16(p::kSentinelVA+2u)==p::kInvalid);
 assert(h.get16(p::kSentinelVA+4u)==p::kInvalid);
 assert(h.get16(p::kSentinelVA+6u)==1u);
 assert(h.get32(p::kSentinelFlagVA)==1u&&r.sentinel_initialized);
 h.put16(kKey,40u);h.put16(kSlots+2u,p::kInvalid);
 r=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(r.code==p::Code::Completed&&r.visited==1u&&r.parent==0u);
 assert(r.branch==0u&&!r.sentinel_initialized);
}
void test_fresh_and_recycled_slot(){
 Host h;h.reset();p::AcquireSlot acquire(h);h.acquirer=&acquire;
 auto r=acquire.acquire(kMap);
 assert(r.code==p::Code::Completed&&r.handle==0u&&!r.grow_called);
 assert(h.get16(kMap+0x16u)==1u&&h.get32(kMap+0x18u)==kSlots);
 h.put16(kMap+0x16u,8u);
 r=acquire.acquire(kMap);
 assert(r.code==p::Code::Completed&&r.grow_called&&h.grows==1u);
 assert(r.handle==8u&&h.get16(kMap+0x16u)==9u);
 h.put16(kMap+0x14u,4u);
 h.put16(kSlots+4u*p::kStride+2u,7u);
 r=acquire.acquire(kMap);
 assert(r.code==p::Code::Completed&&r.used_free_list&&r.handle==4u);
 assert(h.resolves==1u&&h.get16(kMap+0x14u)==7u);
 assert(h.get16(kMap+0x16u)==9u);
 h.put16(kMap+0x14u,p::kInvalid);h.put16(kMap+0x16u,16u);
 h.fail_grow=true;r=acquire.acquire(kMap);
 assert(r.code==p::Code::HostRejected&&r.grow_called);
}
void test_root_and_child_link_and_rebalance(){
 Host h;h.reset();p::LinkNode link(h);h.linker=&link;
 auto r=link.link(kMap,0u,p::kInvalid,0u);
 assert(r.code==p::Code::Completed&&r.rebalance_called&&r.linked);
 assert(h.get16(kMap+0x10u)==0u&&h.get16(kSlots+4u)==p::kInvalid);
 assert(h.get16(kSlots+0u)==p::kInvalid&&h.get16(kSlots+2u)==p::kInvalid);
 assert(h.get16(kSlots+6u)==1u&&h.rebalances==1u);
 r=link.link(kMap,1u,0u,1u);
 assert(r.code==p::Code::Completed&&h.get16(kSlots+0u)==1u);
 assert(h.get16(kSlots+p::kStride+6u)==0u);
 r=link.link(kMap,2u,0u,0u);
 assert(r.code==p::Code::Completed&&h.get16(kSlots+2u)==2u);
 assert(h.get16(kSlots+2u*p::kStride+4u)==0u);
 assert(h.rebalances==3u);
 h.fail_rebalance=true;
 r=link.link(kMap,3u,2u,0u);
 assert(r.code==p::Code::HostRejected&&r.linked&&r.rebalance_called);
 assert(h.get16(kSlots+2u*p::kStride+2u)==3u);
}
void test_guards_and_reentrance(){
 Host h;h.reset();p::FindParent find(h);p::AcquireSlot acquire(h);
 p::LinkNode link(h);h.finder=&find;h.linker=&link;
 assert(find.find(0u,kKey,kParent,kBranch,kMap).code==p::Code::NullPointerHazard);
 assert(acquire.acquire(0u).code==p::Code::NullPointerHazard);
 assert(link.link(0u,0u,0u,0u).code==p::Code::NullPointerHazard);
 h.put16(kMap+0x10u,0u);h.put16(kSlots+8u,7u);h.put16(kKey,6u);
 h.reenter=true;
 auto r=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(r.code==p::Code::Completed&&h.nested==p::Code::Reentrant);
 h.reenter=false;h.throw_compare=true;
 r=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(r.code==p::Code::HostRejected&&r.stage==p::Stage::CompareCallback);
 h.throw_compare=false;h.fail_write=h.writes+1;
 r=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(r.code==p::Code::HostRejected);
 h.reset();h.reenter=true;
 r=link.link(kMap,0u,p::kInvalid,0u);
 assert(r.code==p::Code::Completed&&h.nested==p::Code::Reentrant);
 h.reset();h.put16(kMap+0x10u,0u);h.put16(kSlots+8u,10u);
 h.put16(kKey,9u);h.put16(kSlots,0u);
 // Deliberately corrupted self-cycle in the left-child word.
 r=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(r.code==p::Code::TraversalCycle&&r.visited==65537u);
}
}
int main(){test_empty_parent_and_first_branch();test_fresh_and_recycled_slot();
 test_root_and_child_link_and_rebalance();test_guards_and_reentrance();return 0;}
