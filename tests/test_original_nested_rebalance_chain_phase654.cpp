#include "sin_compat/original_nested_slot_growth_phase653.hpp"
#include "sin_compat/original_nested_tree_core_phase652.hpp"
#include "sin_compat/original_nested_rebalance_phase654.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
namespace n=sin_compat::original_nested_slot_growth_phase653;
namespace p=sin_compat::original_nested_tree_core_phase652;
namespace r=sin_compat::original_nested_rebalance_phase654;
namespace {
constexpr std::uint32_t kMap=0x4000u,kSlots=0x9000u;
constexpr std::uint32_t kKey=0x7100u,kParent=0x7200u,kBranch=0x7204u;
struct Host final:n::IHost,p::IHost,r::IHost{
 std::map<std::uint32_t,std::uint8_t> memory;
 n::GrowSlots* grow_model=nullptr;
 n::ResolveSlot* resolve_model=nullptr;
 r::Rebalance* rebalance_model=nullptr;
 std::uint32_t next_alloc=kSlots,allocs=0,reallocs=0,grows=0,resolves=0,rebalances=0,compares=0;
 void put8(std::uint32_t a,std::uint8_t v){memory[a]=v;}
 void put16(std::uint32_t a,std::uint16_t v){put8(a,static_cast<std::uint8_t>(v));put8(a+1u,static_cast<std::uint8_t>(v>>8));}
 void put32(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4u;++i)put8(a+i,static_cast<std::uint8_t>(v>>(8*i)));}
 std::uint8_t byte(std::uint32_t a)const{auto it=memory.find(a);assert(it!=memory.end());return it->second;}
 std::uint16_t word(std::uint32_t a)const{return static_cast<std::uint16_t>(byte(a)|(static_cast<std::uint16_t>(byte(a+1u))<<8));}
 std::uint32_t dword(std::uint32_t a)const{
  std::uint32_t v=0;for(unsigned i=0;i<4u;++i)v|=static_cast<std::uint32_t>(byte(a+i))<<(8u*i);return v;
 }
 bool read_u8(std::uint32_t a,std::uint8_t& v)override{auto it=memory.find(a);if(it==memory.end())return false;v=it->second;return true;}
 bool read_u16(std::uint32_t a,std::uint16_t& v)override{
  if(!memory.count(a)||!memory.count(a+1u))return false;
  v=word(a);return true;
 }
 bool read_u32(std::uint32_t a,std::uint32_t& v)override{
  for(unsigned i=0;i<4u;++i)if(!memory.count(a+i))return false;
  v=dword(a);return true;
 }
 bool write_u8(std::uint32_t a,std::uint8_t v)override{put8(a,v);return true;}
 bool write_u16(std::uint32_t a,std::uint16_t v)override{put16(a,v);return true;}
 bool write_u32(std::uint32_t a,std::uint32_t v)override{put32(a,v);return true;}
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& out)override{
  ++allocs;out=next_alloc;next_alloc+=0x1000u;
  for(unsigned i=0;i<size&&i<4096u;++i)put8(out+i,0u);
  return true;
 }
 bool reallocate_original_2226e400(std::uint32_t prior,std::uint32_t size,std::uint32_t& out)override{
  ++reallocs;out=next_alloc;next_alloc+=0x1000u;
  for(unsigned i=0;i<size&&i<4096u;++i){auto it=memory.find(prior+i);put8(out+i,it==memory.end()?0u:it->second);}
  return true;
 }
 bool compare_original_indirect(std::uint32_t cmp,std::uint32_t key,
     std::uint32_t node,std::uint32_t context,std::uint8_t& al)override{
  assert(cmp==0x2228e9b0u&&context==kMap);++compares;
  al=static_cast<std::uint8_t>(word(key)<word(node));return true;
 }
 bool grow_original_2228e200(std::uint32_t owner,std::uint32_t increment)override{
  assert(owner==kMap+4u && grow_model);++grows;
  const auto res=grow_model->grow(owner,increment);
  return res.code==n::Code::Completed;
 }
 bool resolve_original_2228e100(std::uint32_t map,std::uint16_t slot,
     std::uint32_t& node)override{
  assert(resolve_model);++resolves;
  const auto res=resolve_model->resolve(map,slot);
  if(res.code!=n::Code::Completed)return false;
  node=res.result_pointer;return true;
 }
 bool rebalance_original_2228edf0(std::uint32_t map,std::uint16_t slot)override{
  assert(map==kMap && rebalance_model);++rebalances;
  return rebalance_model->fixup(map,slot).code==r::Code::Completed;
 }
 void reset(){memory.clear();next_alloc=kSlots;
  grows=allocs=reallocs=resolves=rebalances=compares=0u;
  put32(kMap,0x2228e9b0u);put32(kMap+4u,0u);
  put32(kMap+8u,0u);put32(kMap+0xcu,0u);
  put16(kMap+0x10u,p::kInvalid);put16(kMap+0x12u,0u);
  put16(kMap+0x14u,p::kInvalid);put16(kMap+0x16u,0u);
  put32(kMap+0x18u,0u);put32(n::kSentinelFlagVA,0u);
  put16(kKey,0u);put16(kParent,0u);put8(kBranch,0u);
 }
};
void test_actual_phase652_acquire_link_find_with_phase653_growth(){
 Host h;h.reset();n::GrowSlots grow(h);n::ResolveSlot resolver(h);
 r::Rebalance rebalance(h);
 p::AcquireSlot acquire(h);p::LinkNode link(h);p::FindParent find(h);
 h.grow_model=&grow;h.resolve_model=&resolver;h.rebalance_model=&rebalance;
 const auto add=[&](std::uint16_t key,std::uint16_t parent,std::uint8_t left)->std::uint16_t{
  auto slot=acquire.acquire(kMap);assert(slot.code==p::Code::Completed);
  const auto linked=link.link(kMap,slot.handle,parent,left);
  assert(linked.code==p::Code::Completed);
  h.put16(h.dword(kMap+4u)+slot.handle*12u+8u,key);
  h.put16(kMap+0x12u,static_cast<std::uint16_t>(h.word(kMap+0x12u)+1u));
  return slot.handle;
 };
 assert(add(20u,p::kInvalid,0u)==0u);
 assert(h.allocs==1u&&h.grows==1u&&h.dword(kMap+8u)==3u);
 assert(h.dword(kMap+4u)==kSlots&&h.word(kSlots+6u)==1u);
 assert(add(10u,0u,1u)==1u);
 assert(add(30u,0u,0u)==2u);
 assert(h.rebalances==3u&&h.dword(kMap+8u)==3u);
 assert(h.word(kSlots)==1u&&h.word(kSlots+2u)==2u);
 h.put16(kKey,15u);
 auto search=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(search.code==p::Code::Completed&&search.parent==1u);
 assert(search.branch==0u&&search.comparisons==2u);
 assert(h.dword(n::kSentinelFlagVA)==1u);
 const auto fourth=acquire.acquire(kMap);
 assert(fourth.code==p::Code::Completed&&fourth.handle==3u&&fourth.grow_called);
 assert(h.grows==2u&&h.reallocs==1u&&h.dword(kMap+8u)==6u);
 const auto relocated=h.dword(kMap+4u);
 assert(relocated==kSlots+0x1000u&&h.dword(kMap+0x18u)==relocated);
 assert(h.word(relocated+8u)==20u);
 assert(h.word(relocated+12u+8u)==10u);
 assert(h.word(relocated+24u+8u)==30u);
 search=find.find(kMap,kKey,kParent,kBranch,kMap);
 assert(search.code==p::Code::Completed&&search.parent==1u);
 // Simulate the original free-list structure and exercise *compiled*
 // 0x2228E100 from the Phase652 AcquireSlot recycler.
 h.put16(kMap+0x14u,2u);
 h.put16(relocated+2u*12u+2u,p::kInvalid);
 const auto before=h.word(kMap+0x16u);
 const auto recycled=acquire.acquire(kMap);
 assert(recycled.code==p::Code::Completed&&recycled.used_free_list);
 assert(recycled.handle==2u&&h.resolves==1u);
 assert(h.word(kMap+0x14u)==p::kInvalid);
 assert(h.word(kMap+0x16u)==before);
 assert(h.dword(kMap+0x18u)==relocated);
 // The resolver also exposes the original static FFFF sentinel address.
 const auto sentinel=resolver.resolve(kMap,p::kInvalid);
 assert(sentinel.code==n::Code::Completed&&sentinel.sentinel_returned);
 assert(sentinel.result_pointer==n::kSentinelVA);
}

void test_full_original_652_653_654_compiled_chain(){
 Host h;h.reset();
 n::GrowSlots grow(h);n::ResolveSlot resolver(h);r::Rebalance rebalance(h);
 p::AcquireSlot acquire(h);p::FindParent find(h);p::LinkNode link(h);
 h.grow_model=&grow;h.resolve_model=&resolver;h.rebalance_model=&rebalance;
 const std::uint16_t keys[]={20u,10u,30u,5u,15u,1u,25u,40u,50u};
 std::uint16_t count=0u;
 for(std::uint16_t key:keys){
  h.put16(kKey,key);
  auto found=find.find(kMap,kKey,kParent,kBranch,kMap);
  assert(found.code==p::Code::Completed);
  auto acquired=acquire.acquire(kMap);
  assert(acquired.code==p::Code::Completed && acquired.handle==count);
  const auto slot=h.dword(kMap+4u)+static_cast<std::uint32_t>(acquired.handle)*12u;
  h.put16(slot+8u,key);
  auto linked=link.link(kMap,acquired.handle,found.parent,found.branch);
  assert(linked.code==p::Code::Completed && linked.rebalance_called);
  ++count;
  const auto array=h.dword(kMap+4u);
  const auto root=h.word(kMap+0x10u);
  assert(root!=p::kInvalid);
  assert(h.word(array+static_cast<std::uint32_t>(root)*12u+6u)==1u);
  std::vector<bool> seen(count,false);
  const auto walk=[&](auto&& self,std::uint16_t id,std::uint16_t parent,
                      std::uint32_t lo,std::uint32_t hi)->unsigned{
   if(id==p::kInvalid)return 1u;
   assert(id<count && !seen.at(id));seen[id]=true;
   const auto node=array+static_cast<std::uint32_t>(id)*12u;
   assert(h.word(node+4u)==parent);
   const auto value=h.word(node+8u),color=h.word(node+6u);
   assert(lo<value && value<hi && color<=1u);
   const auto left=h.word(node),right=h.word(node+2u);
   if(color==0u){
    if(left!=p::kInvalid)assert(h.word(array+left*12u+6u)==1u);
    if(right!=p::kInvalid)assert(h.word(array+right*12u+6u)==1u);
   }
   const auto a=self(self,left,id,lo,value);
   const auto b=self(self,right,id,value,hi);
   assert(a==b);
   return a+(color==1u?1u:0u);
  };
  assert(walk(walk,root,p::kInvalid,0u,65536u)>0u);
  for(bool was_seen:seen)assert(was_seen);
 }
 assert(count==9u && h.rebalances==9u);
 assert(h.grows==3u && h.allocs==1u && h.reallocs==2u);
 assert(h.dword(kMap+8u)==12u);
 assert(h.dword(n::kSentinelFlagVA)&1u);
}
}
int main(){test_actual_phase652_acquire_link_find_with_phase653_growth();test_full_original_652_653_654_compiled_chain();return 0;}
