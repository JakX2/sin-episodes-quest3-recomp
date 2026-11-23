#include "sin_compat/original_symbol_growth_phase644.hpp"
#include "sin_compat/original_symbol_slot_allocation_phase639.hpp"
#include "sin_compat/original_symbol_enumeration_phase643.hpp"
#include "sin_compat/original_symbol_tree_link_phase640.hpp"
#include "sin_compat/original_symbol_rotations_phase641.hpp"
#include "sin_compat/original_symbol_rebalance_phase642.hpp"
#include "sin_compat/original_symbol_forward_phase638.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace g=sin_compat::original_symbol_growth_phase644;
namespace p=sin_compat::original_symbol_slot_allocation_phase639;
namespace e=sin_compat::original_symbol_enumeration_phase643;
namespace l=sin_compat::original_symbol_tree_link_phase640;
namespace r=sin_compat::original_symbol_rotations_phase641;
namespace b=sin_compat::original_symbol_rebalance_phase642;
namespace f=sin_compat::original_symbol_forward_phase638;
namespace {
constexpr std::uint32_t table=0x2000u,owner=table+4u;
constexpr std::uint32_t context=0x1000u,record=0x6000u;
constexpr std::uint32_t comparator=0x4a4a4a4au;
struct Host final:g::IHost,p::IHost,e::IHost,l::IHost,r::IHost,f::IHost {
 std::map<std::uint32_t,std::uint8_t> bytes;
 std::map<std::uint32_t,std::uint32_t> allocations;
 std::vector<std::string> trace;
 std::vector<std::uint16_t> keys;
 std::uint32_t next_allocation=0x10000u;
 std::uint32_t last_alloc_size=0u,last_realloc_size=0u,last_freed=0u;
 std::uint32_t initial_allocations=0u,reallocations=0u,frees=0u;
 bool callback_failure=false,callback_throw=false,zero_allocation=false;
 bool reenter=false;
 unsigned fail_write=0u,write_count=0u;
 g::Code nested=g::Code::Completed;
 g::GrowArray* growth=nullptr;g::ReleaseArray* release_array=nullptr;
 p::RegisterSlot* registrar=nullptr;
 e::Enumerate* enumerate=nullptr;l::LinkNode* linker=nullptr;
 b::Rebalance* rebalance=nullptr;
 g::Result last_grow{};p::Result last_registration{};
 b::Result last_balance{};
 void set8(std::uint32_t a,std::uint8_t n){bytes[a]=n;}
 void set16(std::uint32_t a,std::uint16_t n){for(unsigned i=0;i<2u;++i)set8(a+i,static_cast<std::uint8_t>(n>>(8*i)));}
 void set32(std::uint32_t a,std::uint32_t n){for(unsigned i=0;i<4u;++i)set8(a+i,static_cast<std::uint8_t>(n>>(8*i)));}
 template<typename T> bool get(std::uint32_t a,T& result)const {
   result=0;for(unsigned i=0;i<sizeof(T);++i){const auto it=bytes.find(a+i);
     if(it==bytes.end())return false;
     result|=static_cast<T>(it->second)<<(8*i);}
   return true;
 }
 std::uint16_t u16(std::uint32_t a)const {std::uint16_t n=0u;assert(get(a,n));return n;}
 std::uint32_t u32(std::uint32_t a)const {std::uint32_t n=0u;assert(get(a,n));return n;}
 std::uint32_t slot(std::uint16_t i)const {return u32(owner)+16u*i;}
 std::uint16_t field(std::uint16_t i,std::uint32_t off)const {
   if(i==0xffffu)return off==6u?1u:0xffffu;
   return u16(slot(i)+off);
 }
 void reset(std::uint32_t step=0u){
   bytes.clear();allocations.clear();trace.clear();keys.clear();
   next_allocation=0x10000u;last_alloc_size=last_realloc_size=last_freed=0u;
   initial_allocations=reallocations=frees=0u;
   callback_failure=callback_throw=zero_allocation=reenter=false;
   fail_write=write_count=0u;nested=g::Code::Completed;
   last_grow={};last_registration={};last_balance={};
   set32(context,table);set32(table,comparator);set32(owner,0u);
   set32(owner+4u,0u);set32(owner+8u,step);
   set16(table+0x10u,0xffffu);set16(table+0x12u,0u);
   set16(table+0x14u,0xffffu);set16(table+0x16u,0u);
   set32(table+0x18u,0u);set32(e::kSentinelFlags,0u);
 }
 bool read_u8(std::uint32_t a,std::uint8_t& n)override {
   trace.push_back("r8:"+std::to_string(a));return get(a,n);
 }
 bool read_u16(std::uint32_t a,std::uint16_t& n)override {
   trace.push_back("r16:"+std::to_string(a));return get(a,n);
 }
 bool read_u32(std::uint32_t a,std::uint32_t& n)override {
   trace.push_back("r32:"+std::to_string(a));return get(a,n);
 }
 bool write_u16(std::uint32_t a,std::uint16_t n)override {
   trace.push_back("w16:"+std::to_string(a));++write_count;
   if(fail_write!=0u && write_count==fail_write)return false;
   set16(a,n);return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t n)override {
   trace.push_back("w32:"+std::to_string(a));++write_count;
   if(fail_write!=0u && write_count==fail_write)return false;
   set32(a,n);return true;
 }
 bool allocate_original_2226e390(std::uint32_t size,std::uint32_t& out)override {
   trace.push_back("ALLOC:"+std::to_string(size));++initial_allocations;
   last_alloc_size=size;
   if(reenter && growth)nested=growth->grow(owner,1u).code;
   if(callback_throw)throw std::runtime_error("allocator raised");
   if(callback_failure)return false;
   if(zero_allocation){out=0u;return true;}
   out=next_allocation;next_allocation+=0x10000u;
   allocations[out]=size;return true;
 }
 bool reallocate_original_2226e400(std::uint32_t previous,
                                    std::uint32_t size,std::uint32_t& out)override {
   trace.push_back("REALLOC:"+std::to_string(previous)+":"+std::to_string(size));
   ++reallocations;last_realloc_size=size;
   if(callback_throw)throw std::runtime_error("reallocator raised");
   if(callback_failure)return false;
   const auto existing=allocations.find(previous);
   if(existing==allocations.end())return false;
   if(zero_allocation){out=0u;return true;}
   const auto copy_size=std::min(existing->second,size);
   out=next_allocation;next_allocation+=0x10000u;
   const auto prior=bytes;
   for(const auto& [addr,value]:prior)
     if(addr>=previous && (addr-previous)<copy_size)
       set8(out+addr-previous,value);
   allocations.erase(previous);allocations[out]=size;
   return true;
 }
 bool free_original_2226e3e0(std::uint32_t previous)override {
   trace.push_back("FREE:"+std::to_string(previous));++frees;
   last_freed=previous;
   if(callback_throw)throw std::runtime_error("free raised");
   if(callback_failure)return false;
   allocations.erase(previous);return true;
 }
 bool grow_slot_array_22280dd0(std::uint32_t at,std::uint32_t n)override {
   assert(growth);last_grow=growth->grow(at,n);
   return last_grow.code==g::Code::Completed;
 }
 bool get_free_slot_next_22280d40(std::uint32_t ctx,std::uint16_t i,
                                   std::uint16_t& result)override {
   assert(ctx==table);return get(slot(i)+2u,result);
 }
 bool enumerate_22280fb0(std::uint32_t ctx,std::uint32_t key,
                           std::uint32_t& parent,std::uint32_t& side)override {
   assert(enumerate);return enumerate->search(ctx,key,parent,side).code==e::Code::Completed;
 }
 bool compare_original_22280fb0(std::uint32_t ctx,std::uint32_t cb,
         std::uint32_t key,std::uint32_t existing,std::uint8_t& al)override {
   assert(ctx==table && cb==comparator);
   al=u16(key)<u16(existing)?1u:0u;return true;
 }
 bool link_slot_22281f30(std::uint32_t ctx,std::uint16_t i,
                          std::uint32_t parent,std::uint32_t side)override {
   assert(linker);return linker->link(ctx,i,parent,side).code==l::Code::Linked;
 }
 bool rebalance_original_22281840(std::uint32_t ctx,std::uint32_t idx)override {
   assert(rebalance);last_balance=rebalance->insert(ctx,idx);
   return last_balance.code==b::Code::Completed;
 }
 bool lookup_original_tree_2228f7a0(std::uint32_t at,std::uint32_t& scratch,
                   std::uint32_t symbol,std::uint16_t& token)override {
   assert(at==table+0x1cu && scratch==symbol);
   token=static_cast<std::uint16_t>(symbol);return true;
 }
 bool allocate_original_slot_22282270(std::uint32_t ctx,
                    std::uint16_t& token,std::uint16_t& index)override {
   assert(registrar);set16(record,token);set32(record+4u,1000u+token);
   last_registration=registrar->register_record(ctx,record,0u);
   if(last_registration.code!=p::Code::Completed)return false;
   index=last_registration.handle;return true;
 }
 int validate(std::uint16_t index,std::uint16_t parent,int low,int high,
              std::set<std::uint16_t>& seen)const {
   if(index==0xffffu)return 1;
   assert(index<keys.size() && seen.insert(index).second);
   assert(field(index,4u)==parent);
   const int key=field(index,8u);
   assert(key>low && key<high && field(index,6u)<=1u);
   const auto left=field(index,0u),right=field(index,2u);
   if(field(index,6u)==0u){assert(field(left,6u)==1u);assert(field(right,6u)==1u);}
   const int ldepth=validate(left,index,low,key,seen);
   const int rdepth=validate(right,index,key,high,seen);
   assert(ldepth==rdepth);
   return ldepth+(field(index,6u)==1u?1:0);
 }
 void check_tree()const {
   const auto root=u16(table+0x10u);
   if(keys.empty()){assert(root==0xffffu);return;}
   assert(field(root,6u)==1u);
   std::set<std::uint16_t> seen;
   validate(root,0xffffu,-1,65536,seen);
   assert(seen.size()==keys.size());
   for(std::uint16_t i=0u;i<keys.size();++i){
     assert(field(i,8u)==keys[i]);
     assert(u32(slot(i)+12u)==1000u+keys[i]);
   }
 }
};
void test_growth_modes_and_callbacks(){
  Host h;h.reset();g::GrowArray grow(h);h.growth=&grow;
  auto res=grow.grow(owner,1u);
  assert(res.code==g::Code::Completed && res.initial_capacity==0u);
  assert(res.target_capacity_wrapped==1u && res.final_capacity==2u);
  assert(res.loop_iterations==1u && res.capacity_writes==1u);
  assert(res.performed_initial_allocate && !res.performed_reallocation);
  assert(res.requested_bytes_wrapped==32u && h.u32(owner+4u)==2u);
  assert(h.initial_allocations==1u && h.last_alloc_size==32u);
  const auto first=h.u32(owner);
  res=grow.grow(owner,1u);
  assert(res.code==g::Code::Completed && h.u32(owner+4u)==4u);
  assert(h.reallocations==1u && h.last_realloc_size==64u);
  assert(res.performed_reallocation && res.old_pointer==first);
  assert(res.capacity_writes==1u && res.loop_iterations==1u);
  res=grow.grow(owner,0u);
  assert(res.code==g::Code::Completed && res.capacity_writes==0u);
  assert(res.performed_reallocation && h.reallocations==2u);
  assert(h.last_realloc_size==64u); // native reallocates even with no growth
  h.reset(3u);
  res=grow.grow(owner,1u);assert(res.code==g::Code::Completed && h.u32(owner+4u)==2u);
  res=grow.grow(owner,5u);
  assert(res.code==g::Code::Completed && h.u32(owner+4u)==8u);
  assert(res.loop_iterations==2u && res.capacity_writes==2u);
  assert(h.last_realloc_size==128u);
  h.reset(0xffffffffu);
  res=grow.grow(owner,1u);
  assert(res.code==g::Code::Completed && res.skipped_disabled);
  assert(h.u32(owner+4u)==0u && h.u32(owner)==0u);
  assert(h.initial_allocations==0u && h.reallocations==0u);
  // Native compares capacities as SIGNED DWORD and wraps arithmetic in EDX.
  h.reset();h.set32(owner+4u,0x7fffffffu);
  res=grow.grow(owner,2u); // target wraps to 0x80000001 (negative)
  assert(res.code==g::Code::Completed && res.capacity_writes==0u);
  assert(h.last_alloc_size==0xfffffff0u); // SHL wraps, no overflow refusal
  h.reset();h.set32(owner+4u,5u);h.zero_allocation=true;
  res=grow.grow(owner,0u);
  assert(res.code==g::Code::Completed && res.new_pointer==0u);
  assert(h.u32(owner)==0u && h.u32(owner+4u)==5u);
}
void test_release_and_failures(){
  Host h;h.reset();g::GrowArray grow(h);g::ReleaseArray release(h);
  h.growth=&grow;h.release_array=&release;
  auto x=release.release(owner);
  assert(x.code==g::Code::Completed && !x.performed_free);
  assert(x.capacity_writes==1u && x.pointer_writes==0u);
  assert(grow.grow(owner,1u).code==g::Code::Completed);
  const auto prior=h.u32(owner);
  x=release.release(owner);
  assert(x.code==g::Code::Completed && x.performed_free);
  assert(h.last_freed==prior && h.frees==1u);
  assert(h.u32(owner)==0u && h.u32(owner+4u)==0u);
  h.reset(0xffffffffu);h.set32(owner,0xabcde000u);h.set32(owner+4u,19u);
  x=release.release(owner);
  assert(x.code==g::Code::Completed&&x.skipped_disabled);
  assert(h.u32(owner)==0xabcde000u && h.u32(owner+4u)==19u && h.frees==0u);
  h.reset();h.callback_failure=true;
  x=grow.grow(owner,1u);
  assert(x.code==g::Code::HostRejected && x.capacity_writes==1u);
  assert(h.u32(owner+4u)==2u && h.u32(owner)==0u);
  h.reset();h.callback_throw=true;
  x=grow.grow(owner,1u);
  assert(x.code==g::Code::HostRejected && x.capacity_writes==1u);
  h.reset();h.fail_write=1u;
  x=grow.grow(owner,1u);
  assert(x.code==g::Code::HostRejected && x.capacity_writes==0u);
  assert(h.initial_allocations==0u);
  h.reset();h.fail_write=2u;
  x=grow.grow(owner,1u);
  assert(x.code==g::Code::HostRejected && x.capacity_writes==1u);
  assert(x.performed_initial_allocate && h.initial_allocations==1u);
  assert(h.u32(owner)==0u && h.u32(owner+4u)==2u);
  h.reset();h.reenter=true;
  x=grow.grow(owner,1u);
  assert(x.code==g::Code::Completed && h.nested==g::Code::Reentrant);
  h.reset();h.set32(owner+4u,1u);h.set32(owner+8u,0x80000000u);
  x=grow.grow(owner,2u);
  assert(x.code==g::Code::GrowthLoopGuard);
  assert(x.loop_iterations==65536u && h.initial_allocations==0u);
  h.reset();x=grow.grow(0u,1u);assert(x.code==g::Code::NullOwnerHazard);
  x=grow.grow(0xfffffffau,1u);assert(x.code==g::Code::AddressOverflow);
  x=release.release(0u);assert(x.code==g::Code::NullOwnerHazard);
}
void test_full_pipeline_with_dynamic_memory(){
  Host h;h.reset();
  g::GrowArray growth(h);g::ReleaseArray release(h);
  e::Enumerate enumerate(h);p::AcquireSlot acquire(h);
  p::RegisterSlot registrar(h,acquire);l::LinkNode linker(h);
  r::SentinelSlotAccessor sentinel(h);r::RotateNode rotations(h,sentinel);
  b::Rebalance rebalance(h,sentinel,rotations);f::Forwarder forward(h);
  h.growth=&growth;h.release_array=&release;h.enumerate=&enumerate;
  h.registrar=&registrar;h.linker=&linker;h.rebalance=&rebalance;
  // First insert grows capacity 0->2; successive inserts double until
  // all values fit. This uses the compiled original Phase638..644 models,
  // not a fake 'growth succeeded' callback.
  const std::vector<std::uint16_t> items={60u,30u,90u,20u,40u,80u,
    100u,35u,45u,25u,75u,85u,95u,105u,15u,50u,55u,65u,
    10u,5u,110u,120u,115u,125u,130u,135u,140u,145u,150u,
    155u,160u,165u,170u,175u,180u,185u,190u,195u,200u};
  for(const auto key:items){
    const auto out=forward.write(context,key,1000u+key);
    assert(out.code==f::Code::Written);
    assert(out.slot_handle==h.keys.size());
    assert(h.last_registration.code==p::Code::Completed);
    assert(h.last_balance.code==b::Code::Completed);
    h.keys.push_back(key);h.check_tree();
    assert(h.u32(table+0x18u)==h.u32(owner));
  }
  assert(h.initial_allocations==1u);
  assert(h.reallocations>=5u); // 2->4->8->16->32->64
  assert(h.u32(owner+4u)==64u);
  assert(h.u16(table+0x12u)==items.size());
  assert(h.allocations.size()==1u);
  const auto previous=h.u32(owner);
  const auto cleaned=release.release(owner);
  assert(cleaned.code==g::Code::Completed && cleaned.performed_free);
  assert(h.last_freed==previous && h.u32(owner)==0u);
  assert(h.u32(owner+4u)==0u && h.allocations.empty());
}
}
int main(){test_growth_modes_and_callbacks();test_release_and_failures();
  test_full_pipeline_with_dynamic_memory();return 0;}
