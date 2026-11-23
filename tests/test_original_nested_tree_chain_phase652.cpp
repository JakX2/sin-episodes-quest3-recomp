#include "sin_compat/original_nested_tree_core_phase652.hpp"
#include "sin_compat/original_nested_insertion_phase651.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
namespace n=sin_compat::original_nested_tree_core_phase652;
namespace prev=sin_compat::original_nested_insertion_phase651;
namespace {
constexpr std::uint32_t kMap=0x4000u,kSlots=0x9000u,kKey=0x8100u;
constexpr std::uint32_t kOutParent=0x8200u,kOutBranch=0x8204u;
struct Host final:n::IHost,prev::IHost {
 std::map<std::uint32_t,std::uint8_t> memory;
 n::FindParent* parent_model=nullptr;
 n::AcquireSlot* slot_model=nullptr;
 n::LinkNode* link_model=nullptr;
 std::vector<char> order;
 std::uint32_t grows=0,compares=0,rebalances=0;
 void put8(std::uint32_t a,std::uint8_t v){memory[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){put8(a,static_cast<std::uint8_t>(v));put8(a+1u,static_cast<std::uint8_t>(v>>8));}
 void put32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4u;++i)put8(a+i,static_cast<std::uint8_t>(v>>(8u*i)));}
 std::uint8_t get8(std::uint32_t a)const{auto i=memory.find(a);assert(i!=memory.end());return i->second;}
 std::uint16_t get16(std::uint32_t a)const{return static_cast<std::uint16_t>(get8(a)|(static_cast<std::uint16_t>(get8(a+1u))<<8));}
 std::uint32_t get32(std::uint32_t a)const{std::uint32_t v=0;for(unsigned i=0;i<4u;++i)v|=static_cast<std::uint32_t>(get8(a+i))<<(8u*i);return v;}
 bool read_u8(std::uint32_t a,std::uint8_t& v)override{auto i=memory.find(a);if(i==memory.end())return false;v=i->second;return true;}
 bool read_u16(std::uint32_t a,std::uint16_t& v)override{
  if(!memory.count(a)||!memory.count(a+1u))return false;
  v=get16(a);return true;
 }
 bool read_u32(std::uint32_t a,std::uint32_t& v)override{
  for(unsigned i=0;i<4u;++i)if(!memory.count(a+i))return false;
  v=get32(a);return true;
 }
 bool write_u8(std::uint32_t a,std::uint8_t v)override{put8(a,v);return true;}
 bool write_u16(std::uint32_t a,std::uint16_t v)override{put16(a,v);return true;}
 bool write_u32(std::uint32_t a,std::uint32_t v)override{put32(a,v);return true;}
 bool compare_original_indirect(std::uint32_t method,std::uint32_t key,
     std::uint32_t node,std::uint32_t context,std::uint8_t& al)override{
  assert(method==0x2228e9b0u&&context==kMap);++compares;
  al=static_cast<std::uint8_t>(get16(key)<get16(node));return true;
 }
 bool grow_original_2228e200(std::uint32_t owner,std::uint32_t increment)override{
  assert(owner==kMap+4u&&increment==1u);++grows;
  const auto old=get32(kMap+8u);
  put32(kMap+8u,old?old*2u:8u);put32(kMap+4u,kSlots);
  for(unsigned i=0;i<8u;++i){
   const auto a=kSlots+i*n::kStride;
   put16(a,n::kInvalid);put16(a+2u,n::kInvalid);
   put16(a+4u,n::kInvalid);put16(a+6u,0u);put32(a+8u,0u);
  }
  return true;
 }
 bool resolve_original_2228e100(std::uint32_t map,std::uint16_t slot,
     std::uint32_t& address)override{
  assert(map==kMap);address=kSlots+static_cast<std::uint32_t>(slot)*n::kStride;
  return true;
 }
 bool rebalance_original_2228edf0(std::uint32_t map,std::uint16_t)override{
  assert(map==kMap);++rebalances;order.push_back('R');
  // Synthetic callback deliberately does only the singleton-root blackening.
  // Full 1367-byte native 0x2228EDF0 rebalancing is NOT implemented.
  const auto root=get16(kMap+0x10u);
  if(root!=n::kInvalid)put16(kSlots+static_cast<std::uint32_t>(root)*n::kStride+6u,1u);
  return true;
 }
 bool allocate_original_2226e390(std::uint32_t,std::uint32_t&)override{return false;}
 bool reallocate_original_2226e400(std::uint32_t,std::uint32_t,std::uint32_t&)override{return false;}
 bool move_original_223a5d10(std::uint32_t,std::uint32_t,std::uint32_t)override{return false;}
 bool find_parent_original_2228e470(std::uint32_t map,std::uint32_t key,
     std::uint32_t context,std::uint16_t& parent,std::uint8_t& branch)override{
  order.push_back('F');assert(parent_model);
  const auto result=parent_model->find(map,key,kOutParent,kOutBranch,context);
  if(result.code!=n::Code::Completed)return false;
  parent=get16(kOutParent);branch=get8(kOutBranch);return true;
 }
 bool acquire_slot_original_2228e620(std::uint32_t map,std::uint16_t& handle)override{
  order.push_back('A');assert(slot_model);
  const auto result=slot_model->acquire(map);
  if(result.code!=n::Code::Completed)return false;
  handle=result.handle;return true;
 }
 bool link_original_2228f460(std::uint32_t map,std::uint16_t handle,
     std::uint16_t parent,std::uint8_t branch)override{
  order.push_back('L');assert(link_model);
  const auto result=link_model->link(map,handle,parent,branch);
  return result.code==n::Code::Completed;
 }
 void reset(){
  memory.clear();order.clear();grows=compares=rebalances=0;
  put32(kMap,0x2228e9b0u);put32(kMap+4u,0u);
  put32(kMap+8u,0u);put16(kMap+0x10u,n::kInvalid);
  put16(kMap+0x12u,0u);put16(kMap+0x14u,n::kInvalid);
  put16(kMap+0x16u,0u);put32(kMap+0x18u,0u);
  put32(n::kSentinelFlagVA,0u);put32(kKey,0u);
  put16(kOutParent,0u);put8(kOutBranch,0u);
 }
};
void test_original_insertion_chain(){
 Host h;h.reset();n::FindParent parent(h);n::AcquireSlot slot(h);
 n::LinkNode link(h);prev::TreeInsert original_phase651_insert(h);
 h.parent_model=&parent;h.slot_model=&slot;h.link_model=&link;
 const auto insert=[&](std::uint16_t id){
  h.put32(kKey,id);
  auto result=original_phase651_insert.insert(kMap,kKey,kMap);
  assert(result.code==prev::Code::Completed);
  assert(result.handle==static_cast<std::uint16_t>(h.get16(kMap+0x12u)-1u));
  assert(h.get16(kSlots+static_cast<std::uint32_t>(result.handle)*12u+8u)==id);
  return result.handle;
 };
 assert(insert(20u)==0u);
 assert(h.order==std::vector<char>({'F','A','L','R'}));
 assert(h.grows==1u&&h.compares==0u&&h.get16(kMap+0x10u)==0u);
 assert(h.get16(kSlots+6u)==1u);
 assert(insert(10u)==1u);
 assert(h.compares==1u&&h.get16(kSlots)==1u);
 assert(h.get16(kSlots+12u+4u)==0u&&h.get16(kSlots+12u+6u)==0u);
 assert(insert(30u)==2u);
 assert(h.compares==2u&&h.get16(kSlots+2u)==2u);
 assert(h.get16(kSlots+24u+4u)==0u&&h.get16(kSlots+24u+6u)==0u);
 assert(h.get16(kMap+0x12u)==3u&&h.get16(kMap+0x16u)==3u);
 assert(h.rebalances==3u&&h.get32(kMap+0x18u)==kSlots);
 assert(h.get32(n::kSentinelFlagVA)==1u);
 assert(h.order==std::vector<char>({'F','A','L','R','F','A','L','R','F','A','L','R'}));
}
}
int main(){test_original_insertion_chain();return 0;}
