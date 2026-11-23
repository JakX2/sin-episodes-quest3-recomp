#include "sin_compat/original_nested_map_phase647.hpp"
#include "sin_compat/original_symbol_comparator_phase646.hpp"
#include "sin_compat/original_memory_wrappers_phase645.hpp"
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
namespace m=sin_compat::original_memory_wrappers_phase645;
namespace t=sin_compat::original_symbol_comparator_phase646;
namespace n=sin_compat::original_nested_map_phase647;
namespace p=sin_compat::original_symbol_slot_allocation_phase639;
namespace e=sin_compat::original_symbol_enumeration_phase643;
namespace l=sin_compat::original_symbol_tree_link_phase640;
namespace r=sin_compat::original_symbol_rotations_phase641;
namespace b=sin_compat::original_symbol_rebalance_phase642;
namespace f=sin_compat::original_symbol_forward_phase638;
namespace {
constexpr std::uint32_t table=0x2000u,owner=table+4u;
constexpr std::uint32_t context=0x1000u,record=0x6000u;
constexpr std::uint32_t comparator=t::kCompareVA;
struct Host final:n::IHost,t::IHost,m::IHost,g::IHost,p::IHost,e::IHost,l::IHost,r::IHost,f::IHost {
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
 m::Code nested=m::Code::Completed;
 g::GrowArray* growth=nullptr;g::ReleaseArray* release_array=nullptr;
 m::AllocatorWrappers* wrappers=nullptr;
 t::CompareKey* real_comparator=nullptr;
 t::InitializeTable* table_init=nullptr;
 n::MapConstructor* nested_map_ctor=nullptr;
 n::Result last_nested_ctor{};
 t::Code compare_nested=t::Code::Completed,construct_nested=t::Code::Completed;
 t::CompareResult last_compare{};
 std::uint32_t compare_callbacks=0u,subobject_callbacks=0u;
 std::uint32_t table_allocations=0u;
 bool table_allocation_zero=false,table_init_reenter=false;
 bool table_nested_failure=false;
 m::Result last_wrapper{};
 bool tag_probe=false,fail_win_import=false,fail_manager_call=false;
 std::uint32_t win_heap_calls=0,win_virtual_calls=0,win_module_calls=0;
 std::uint32_t tag_probe_calls=0;
 std::vector<std::uint32_t> dispatched_slots;
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
   fail_write=write_count=0u;nested=m::Code::Completed;
   last_grow={};last_registration={};last_balance={};last_wrapper={};
   compare_callbacks=subobject_callbacks=table_allocations=0;
   table_allocation_zero=table_init_reenter=table_nested_failure=false;
   compare_nested=construct_nested=t::Code::Completed;last_compare={};
   last_nested_ctor={};
   tag_probe=false;fail_win_import=fail_manager_call=false;
   win_heap_calls=win_virtual_calls=win_module_calls=tag_probe_calls=0;dispatched_slots.clear();
   set32(context,table);set32(table,comparator);set32(owner,0u);
   set32(m::kMemoryManagerGlobal,0x8000u);set32(0x8000u,0x8010u);
   set32(0x8010u,0x8100u);
   for(const auto off:{0u,4u,8u,12u,20u,92u})set32(0x8100u+off,0x9000u+off);
   set32(m::kAllocateFlag,0u);set32(m::kAllocateTag,0u);
   set32(m::kReallocateFlag,0u);set32(m::kReallocateTag,0u);
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
   assert(wrappers);last_wrapper=wrappers->allocate(size);
   if(last_wrapper.code!=m::Code::Completed)return false;
   out=last_wrapper.result_pointer;return true;
 }
 bool reallocate_original_2226e400(std::uint32_t previous,
                                    std::uint32_t size,std::uint32_t& out)override {
   assert(wrappers);last_wrapper=wrappers->reallocate(previous,size);
   if(last_wrapper.code!=m::Code::Completed)return false;
   out=last_wrapper.result_pointer;return true;
 }
 bool free_original_2226e3e0(std::uint32_t previous)override {
   assert(wrappers);last_wrapper=wrappers->free(previous);
   return last_wrapper.code==m::Code::Completed;
 }
 bool win_get_process_heap(std::uint32_t& heap)override {
   if(fail_win_import)return false;
   heap=0xabcdu;return true;
 }
 bool win_heap_alloc(std::uint32_t heap,std::uint32_t flags,
                     std::uint32_t size,std::uint32_t& buffer)override {
   assert(heap==0xabcdu && flags==0u && size==260u);
   ++win_heap_calls;
   if(fail_win_import)return false;
   buffer=0x60000000u;return true;
 }
 bool win_virtual_query(std::uint32_t address,std::uint32_t length,
                        std::uint32_t& base)override {
   assert(address==m::kTagProbeAddress && length==28u);
   ++win_virtual_calls;
   if(fail_win_import)return false;
   base=0x22000000u;return true;
 }
 bool win_get_module_file_name_a(std::uint32_t module,
      std::uint32_t buffer,std::uint32_t count,std::uint32_t& result)override {
   assert(module==0x22000000u && buffer==0x60000000u && count==260u);
   ++win_module_calls;
   if(fail_win_import)return false;
   result=16u;return true;
 }
 bool call_memory_manager(std::uint32_t instance,std::uint32_t method,
      std::uint32_t slotid,const std::array<std::uint32_t,4>& args,
      std::uint32_t count,std::uint32_t& eax_result)override {
   assert(instance==0x8010u && method==0x9000u+slotid);
   dispatched_slots.push_back(slotid);
   if(fail_manager_call)return false;
   if(slotid==m::kSlotTagProbe){++tag_probe_calls;assert(count==0u);
     eax_result=tag_probe?0x101u:0x100u;return true;}
   if(slotid==m::kSlotFree){assert(count==1u);++frees;
     last_freed=args[0];allocations.erase(args[0]);eax_result=0u;return true;}
   const bool is_realloc=slotid==m::kSlotTaggedReallocate||slotid==m::kSlotUntaggedReallocate;
   const bool tagged=slotid==m::kSlotTaggedAllocate||slotid==m::kSlotTaggedReallocate;
   assert(slotid==m::kSlotTaggedAllocate||slotid==m::kSlotUntaggedAllocate||
          slotid==m::kSlotTaggedReallocate||slotid==m::kSlotUntaggedReallocate);
   assert(count==(tagged?(is_realloc?4u:3u):(is_realloc?2u:1u)));
   const auto size=args[is_realloc?1u:0u];
   if(tagged){assert(args[is_realloc?2u:1u]==0x60000000u);
     assert(args[is_realloc?3u:2u]==0u);}
   if(is_realloc){++reallocations;last_realloc_size=size;}
   else {++initial_allocations;last_alloc_size=size;}
   if(reenter && wrappers)nested=wrappers->allocate(1u).code;
   if(callback_throw)throw std::runtime_error("allocator raised");
   if(callback_failure)return false;
   if(zero_allocation){eax_result=0u;return true;}
   const auto out=next_allocation;next_allocation+=0x10000u;
   if(is_realloc){
     const auto old=args[0];auto it=allocations.find(old);
     if(it==allocations.end())return false;
     const auto copy_size=std::min(it->second,size);
     const auto prior=bytes;
     for(const auto& [addr,value]:prior)
       if(addr>=old && (addr-old)<copy_size)set8(out+addr-old,value);
     allocations.erase(old);
   }
   allocations[out]=size;eax_result=out;return true;
 }
 bool allocate_original_2226e790(std::uint32_t size,std::uint32_t& pointer)override {
   assert(size==t::kTableSize && wrappers);
   ++table_allocations;
   if(table_allocation_zero){pointer=0u;return true;}
   const auto ret=wrappers->allocate(size);
   if(ret.code!=m::Code::Completed)return false;
   pointer=ret.result_pointer;
   return true;
 }
 bool initialize_original_2228f360(std::uint32_t nested_va,
                         std::uint32_t a,std::uint32_t b,std::uint32_t c)override {
   ++subobject_callbacks;
   assert(a==0u&&b==0u&&c==1u);
   assert(nested_va>=0x1cu && nested_va!=0u);
   if(table_init_reenter && table_init)construct_nested=table_init->construct(nested_va-0x1cu).code;
   if(table_nested_failure)return false;
   if(nested_map_ctor){
     last_nested_ctor=nested_map_ctor->construct(nested_va,a,b,c);
     return last_nested_ctor.code==n::Code::Completed;
   }
   return true;
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
   assert(ctx==table && cb==comparator && real_comparator);
   ++compare_callbacks;
   last_compare=real_comparator->compare(key,existing);
   if(last_compare.code!=t::Code::Completed)return false;
   al=static_cast<std::uint8_t>(last_compare.eax);return true;
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
  Host h;h.reset();g::GrowArray grow(h);m::AllocatorWrappers wrappers(h);h.wrappers=&wrappers;h.growth=&grow;
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
  m::AllocatorWrappers wrappers(h);h.wrappers=&wrappers;
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
  h.reset();h.fail_write=4u;
  x=grow.grow(owner,1u);
  assert(x.code==g::Code::HostRejected && x.capacity_writes==1u);
  assert(x.performed_initial_allocate && h.initial_allocations==1u);
  assert(h.u32(owner)==0u && h.u32(owner+4u)==2u);
  h.reset();h.reenter=true;
  x=grow.grow(owner,1u);
  assert(x.code==g::Code::Completed && h.nested==m::Code::Reentrant);
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
  m::AllocatorWrappers wrappers(h);h.wrappers=&wrappers;
  e::Enumerate enumerate(h);p::AcquireSlot acquire(h);
  p::RegisterSlot registrar(h,acquire);l::LinkNode linker(h);
  r::SentinelSlotAccessor sentinel(h);r::RotateNode rotations(h,sentinel);
  b::Rebalance rebalance(h,sentinel,rotations);f::Forwarder forward(h);
  h.growth=&growth;h.release_array=&release;h.enumerate=&enumerate;
  h.registrar=&registrar;h.linker=&linker;h.rebalance=&rebalance;
  t::CompareKey cmp(h);t::InitializeTable init(h);
  n::ArrayConstructor nested_array(h);
  n::MapConstructor nested_map(h,nested_array);
  n::ArrayRelease nested_release(h);
  h.real_comparator=&cmp;h.table_init=&init;
  h.nested_map_ctor=&nested_map;
  // This is NOT a preseeded fake comparator or tree-state fixture:
  // the original 63-byte table constructor writes the actual comparator.
  h.set32(table,0xdeadbeefu);
  h.set16(table+0x10u,0x1234u);
  auto constructed=init.construct(table);
  assert(constructed.code==t::Code::Completed && constructed.nested_init_invoked);
  assert(h.u32(table)==comparator && h.u16(table+0x10u)==0xffffu);
  assert(h.u32(table+4u)==0u && h.u32(table+8u)==0u);
  assert(h.u32(table+0xcu)==0u && h.subobject_callbacks==1u);
  // Original Phase646 constructor now calls the actual compiled
  // Phase647 0x2228F360 -> 0x2228EAB0 nested map initializer.
  assert(h.last_nested_ctor.code==n::Code::Completed);
  assert(h.last_nested_ctor.nested_array_completed);
  assert(h.last_nested_ctor.selected_comparator==n::kStringComparatorVA);
  assert(h.u32(table+0x1cu)==n::kStringComparatorVA);
  assert(h.u32(table+0x1cu+0x24u)==8u);
  assert(h.u32(table+0x1cu+8u)==0u);
  assert(h.u16(table+0x1cu+0x10u)==0xffffu);
  assert(h.u16(table+0x1cu+0x14u)==0xffffu);
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
  assert(h.compare_callbacks>items.size());
  assert(h.last_compare.code==t::Code::Completed);
  assert(h.reallocations>=5u); // 2->4->8->16->32->64
  assert(h.u32(owner+4u)==64u);
  assert(h.u16(table+0x12u)==items.size());
  assert(h.allocations.size()==1u);
  const auto previous=h.u32(owner);
  const auto cleaned=release.release(owner);
  assert(cleaned.code==g::Code::Completed && cleaned.performed_free);
  assert(h.last_freed==previous && h.u32(owner)==0u);
  assert(h.u32(owner+4u)==0u && h.allocations.empty());
  const auto inner_release=nested_release.release(table+0x1cu);
  assert(inner_release.code==n::Code::Completed);
  assert(!inner_release.release_called && inner_release.writes_committed==1u);
  assert(h.u32(table+0x1cu+8u)==0u);
}
void test_uninitialized_untagged_dispatch(){
  Host h;h.reset();m::AllocatorWrappers wrap(h);h.wrappers=&wrap;
  h.set32(m::kAllocateFlag,0x1000u);
  h.set32(m::kReallocateFlag,0x2000u);
  auto x=wrap.allocate(48u);
  assert(x.code==m::Code::Completed && !x.tag_enabled);
  assert(x.tag_discovery_calls==1u && x.writes_committed==2u);
  assert(x.dispatch_slot==m::kSlotUntaggedAllocate);
  assert(h.u32(m::kAllocateFlag)==0x1001u);
  assert(h.u32(m::kAllocateTag)==0u);
  assert(h.tag_probe_calls==1u && h.win_heap_calls==0u);
  const auto p=x.result_pointer;
  x=wrap.allocate(64u);
  assert(x.code==m::Code::Completed && x.cache_was_initially_set);
  assert(x.tag_discovery_calls==0u && x.writes_committed==0u);
  assert(h.tag_probe_calls==1u);
  x=wrap.reallocate(p,128u);
  assert(x.code==m::Code::Completed && !x.tag_enabled);
  assert(x.dispatch_slot==m::kSlotUntaggedReallocate);
  assert(x.tag_discovery_calls==1u && h.tag_probe_calls==2u);
  assert(h.u32(m::kReallocateFlag)==0x2001u);
  x=wrap.reallocate(x.result_pointer,144u);
  assert(x.code==m::Code::Completed && x.cache_was_initially_set);
  assert(h.tag_probe_calls==2u);
  auto fr=wrap.free(x.result_pointer);
  assert(fr.code==m::Code::Completed && fr.dispatch_slot==m::kSlotFree);
  assert(fr.tag_discovery_calls==0u && h.frees==1u);
  assert(h.dispatched_slots.front()==m::kSlotTagProbe);
  assert(h.dispatched_slots.back()==m::kSlotFree);
}
void test_tagged_dispatch_and_separate_caches(){
  Host h;h.reset();m::AllocatorWrappers wrap(h);h.wrappers=&wrap;
  h.tag_probe=true;
  auto x=wrap.allocate(32u);
  assert(x.code==m::Code::Completed && x.tag_enabled);
  assert(x.tag_pointer==0x60000000u);
  assert(x.dispatch_slot==m::kSlotTaggedAllocate);
  assert(h.win_heap_calls==1u && h.win_virtual_calls==1u && h.win_module_calls==1u);
  assert(h.u32(m::kAllocateTag)==0x60000000u);
  const auto p=x.result_pointer;
  x=wrap.reallocate(p,96u);
  assert(x.code==m::Code::Completed && x.tag_enabled);
  assert(x.dispatch_slot==m::kSlotTaggedReallocate);
  assert(h.tag_probe_calls==2u && h.win_heap_calls==2u);
  assert(h.u32(m::kReallocateTag)==0x60000000u);
  x=wrap.allocate(1u);
  assert(x.code==m::Code::Completed && x.cache_was_initially_set);
  assert(x.tag_discovery_calls==0u && h.tag_probe_calls==2u);
  assert(h.win_heap_calls==2u);
  auto f=wrap.free(x.result_pointer);
  assert(f.code==m::Code::Completed);
  assert(h.tag_probe_calls==2u);
}
void test_tag_discovery_and_failure_order(){
  Host h;h.reset();m::AllocatorWrappers wrap(h);h.wrappers=&wrap;
  auto r=wrap.discover_tag();
  assert(r.code==m::Code::Completed && r.result_pointer==0u);
  assert(r.tag_discovery_calls==1u && h.tag_probe_calls==1u);
  assert(h.win_heap_calls==0u);
  h.tag_probe=true;
  r=wrap.discover_tag();
  assert(r.code==m::Code::Completed && r.result_pointer==0x60000000u);
  assert(h.win_heap_calls==1u && h.win_virtual_calls==1u && h.win_module_calls==1u);
  h.reset();h.tag_probe=true;h.fail_win_import=true;
  r=wrap.allocate(16u);
  assert(r.code==m::Code::HostRejected);
  assert(r.stage==m::Stage::GetProcessHeap);
  assert(r.writes_committed==1u && h.u32(m::kAllocateFlag)==1u);
  assert(h.u32(m::kAllocateTag)==0u && h.initial_allocations==0u);
  h.fail_win_import=false;
  r=wrap.allocate(16u);
  // Original flag was already set before failure; no rediscovery.
  assert(r.code==m::Code::Completed && !r.tag_enabled);
  assert(r.cache_was_initially_set && r.tag_discovery_calls==0u);
  assert(r.dispatch_slot==m::kSlotUntaggedAllocate);
  h.reset();h.fail_write=1u;
  r=wrap.allocate(16u);
  assert(r.code==m::Code::HostRejected);
  assert(r.stage==m::Stage::WriteCacheFlagDword && r.writes_committed==0u);
  assert(h.tag_probe_calls==0u);
  h.reset();h.fail_write=2u;
  r=wrap.allocate(16u);
  assert(r.code==m::Code::HostRejected);
  assert(r.stage==m::Stage::WriteCachedTag && r.writes_committed==1u);
  assert(h.u32(m::kAllocateFlag)==1u && h.tag_probe_calls==1u);
  h.reset();h.fail_manager_call=true;
  r=wrap.allocate(16u);
  assert(r.code==m::Code::HostRejected);
  assert(r.stage==m::Stage::ProbeManagerTag && r.writes_committed==1u);
  h.reset();h.set32(m::kAllocateFlag,1u);h.fail_manager_call=true;
  r=wrap.allocate(16u);
  assert(r.code==m::Code::HostRejected);
  assert(r.stage==m::Stage::DispatchUntaggedAllocate);
  assert(r.writes_committed==0u);
  h.reset();h.set32(m::kMemoryManagerGlobal,0u);
  r=wrap.free(123u);
  assert(r.code==m::Code::NullManagerHazard && r.stage==m::Stage::ReadManagerGlobal);
  h.reset();h.set32(0x8010u,0xfffffff0u);
  r=wrap.free(123u);
  assert(r.code==m::Code::AddressOverflow);
  h.reset();h.reenter=true;
  r=wrap.allocate(16u);
  assert(r.code==m::Code::Completed && h.nested==m::Code::Reentrant);
  h.reset();h.callback_throw=true;
  r=wrap.allocate(16u);
  assert(r.code==m::Code::HostRejected && r.writes_committed==2u);
}

void test_original_unsigned_comparator(){
 Host h;h.reset();t::CompareKey cmp(h);h.real_comparator=&cmp;
 const auto read=[&](std::uint16_t lhs,std::uint16_t rhs)->std::uint32_t{
   h.set16(0x9100u,lhs);h.set16(0x9200u,rhs);
   const auto out=cmp.compare(0x9100u,0x9200u);
   assert(out.code==t::Code::Completed);
   assert(out.key==lhs&&out.existing==rhs);
   return out.eax;
 };
 assert(read(0u,0u)==0u);
 assert(read(0u,1u)==1u);
 assert(read(1u,0u)==0u);
 assert(read(0xffffu,0xffffu)==0u);
 assert(read(0xfffeu,0xffffu)==1u);
 assert(read(0x8000u,0x7fffu)==0u);
 assert(read(0x7fffu,0x8000u)==1u);
 assert(read(0u,0xffffu)==1u);
 assert(read(0xffffu,0u)==0u);
 assert(cmp.compare(0u,0x9200u).code==t::Code::NullPointerHazard);
 assert(cmp.compare(0x9100u,0u).code==t::Code::NullPointerHazard);
 assert(cmp.compare(0xabcdefu,0x9200u).code==t::Code::HostRejected);
}
void test_original_table_constructor(){
 Host h;h.reset();t::InitializeTable init(h);h.table_init=&init;
 h.set32(table,0x98765432u);
 h.set32(owner,0xffffffffu);
 h.set32(table+8u,111u);
 h.set32(table+0xcu,0xffffffffu);
 h.set16(table+0x10u,3u);h.set16(table+0x12u,3u);
 h.set16(table+0x14u,3u);h.set16(table+0x16u,3u);
 h.set32(table+0x18u,0x22222222u);
 const auto result=init.construct(table);
 assert(result.code==t::Code::Completed && result.table_va==table);
 assert(result.writes_committed==9u && result.nested_init_invoked);
 assert(h.u32(table)==comparator);
 assert(h.u32(owner)==0u && h.u32(table+8u)==0u);
 assert(h.u32(table+0xcu)==0u);
 assert(h.u16(table+0x10u)==0xffffu && h.u16(table+0x14u)==0xffffu);
 assert(h.u16(table+0x12u)==0u && h.u16(table+0x16u)==0u);
 assert(h.u32(table+0x18u)==0u && h.subobject_callbacks==1u);
 assert(init.construct(0u).code==t::Code::NullPointerHazard);
 assert(init.construct(0xfffffff0u).code==t::Code::AddressOverflow);
}
void test_original_factory_and_failures(){
 Host h;h.reset();m::AllocatorWrappers wrappers(h);h.wrappers=&wrappers;
 t::InitializeTable init(h);h.table_init=&init;
 constexpr std::uint32_t factory_owner=0x7500u;
 h.set32(factory_owner,0xdeadbeefu);
 auto result=init.factory(factory_owner);
 assert(result.code==t::Code::Completed && result.alloc_attempted);
 assert(result.owner_pointer_written && !result.null_allocation);
 assert(result.table_va!=0u && h.u32(factory_owner)==result.table_va);
 assert(h.u32(result.table_va)==comparator);
 assert(h.u16(result.table_va+0x10u)==0xffffu);
 assert(h.u32(result.table_va+0x0cu)==0u);
 assert(result.writes_committed==10u);
 assert(h.initial_allocations==1u && h.last_alloc_size==0x50u);
 assert(h.table_allocations==1u && h.subobject_callbacks==1u);
 h.reset();h.set32(factory_owner,0xdeadbeefu);h.table_allocation_zero=true;
 result=init.factory(factory_owner);
 assert(result.code==t::Code::Completed && result.null_allocation);
 assert(result.owner_pointer_written && result.table_va==0u);
 assert(h.u32(factory_owner)==0u && h.subobject_callbacks==0u);
 assert(result.writes_committed==1u);
 h.reset();h.set32(factory_owner,0xdeadbeefu);h.table_nested_failure=true;
 result=init.factory(factory_owner);
 assert(result.code==t::Code::HostRejected);
 assert(result.nested_init_invoked && !result.owner_pointer_written);
 assert(h.u32(factory_owner)==0xdeadbeefu);
 assert(h.u32(result.table_va)==comparator);
 h.reset();h.set32(factory_owner,0xdeadbeefu);h.fail_write=1u;
 result=init.factory(factory_owner);
 assert(result.code==t::Code::HostRejected && !result.owner_pointer_written);
 assert(h.u32(factory_owner)==0xdeadbeefu);
 assert(result.writes_committed==0u);
 h.reset();h.set32(factory_owner,0xdeadbeefu);h.table_init_reenter=true;
 result=init.factory(factory_owner);
 assert(result.code==t::Code::Completed);
 assert(h.construct_nested==t::Code::Reentrant);
 h.reset();h.set32(factory_owner,0xdeadbeefu);h.callback_failure=true;
 result=init.factory(factory_owner);
 assert(result.code==t::Code::HostRejected);
 assert(!result.owner_pointer_written && h.u32(factory_owner)==0xdeadbeefu);
}

void test_nested_constructor_through_original_manager(){
  Host h;h.reset();
  m::AllocatorWrappers wrappers(h);h.wrappers=&wrappers;
  n::ArrayConstructor inner(h);
  n::MapConstructor outer(h,inner);
  n::ArrayRelease cleanup(h);
  const auto object=table+0x1cu;
  const auto built=outer.construct(object,2u,3u,0x101u);
  assert(built.code==n::Code::Completed);
  assert(built.allocation_called && built.allocation_bytes==36u);
  assert(built.selected_comparator==n::kStringComparatorVA);
  assert(h.u32(object)==n::kStringComparatorVA);
  assert(h.u32(object+8u)==3u && h.u32(object+0xcu)==2u);
  assert(h.u32(object+4u)==h.u32(object+0x18u));
  assert(h.initial_allocations==1u && h.last_alloc_size==36u);
  assert(h.allocations.size()==1u);
  const auto original_allocation=h.u32(object+4u);
  const auto released=cleanup.release(object);
  assert(released.code==n::Code::Completed && released.release_called);
  assert(h.last_freed==original_allocation && h.frees==1u);
  assert(h.u32(object+4u)==0u && h.u32(object+8u)==0u);
  assert(h.allocations.empty());
  // Re-init with low selector byte zero chooses the other comparator.
  const auto zero=outer.construct(object,0u,0u,0x100u);
  assert(zero.code==n::Code::Completed);
  assert(zero.selected_comparator==n::kBytewiseLessComparatorVA);
  assert(h.u32(object)==n::kBytewiseLessComparatorVA);
}
}
int main(){test_nested_constructor_through_original_manager();test_original_unsigned_comparator();test_original_table_constructor();
 test_original_factory_and_failures();test_growth_modes_and_callbacks();test_release_and_failures();
 test_uninitialized_untagged_dispatch();test_tagged_dispatch_and_separate_caches();
 test_tag_discovery_and_failure_order();test_full_pipeline_with_dynamic_memory();return 0;}
