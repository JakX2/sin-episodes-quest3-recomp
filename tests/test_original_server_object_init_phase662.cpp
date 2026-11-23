#include "sin_compat/original_server_object_init_phase662.hpp"
#include "sin_compat/original_server_autogamesystem_phase660.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
namespace n=sin_compat::original_server_object_init_phase662;
namespace g=sin_compat::original_server_autogamesystem_phase660;
namespace {
constexpr std::uint32_t kOwner=0x4000u,kStorage=0x8000u;
constexpr std::uint32_t kVtable=0x2244aeccu;
struct Host final:n::IHost,g::IHost{
 std::map<std::uint32_t,std::uint32_t> memory;
 std::vector<std::uint32_t> alloc_addresses;
 std::vector<std::size_t> constructed;
 std::vector<std::uint32_t> init_objects;
 std::size_t allocations=0u;
 bool fail_allocator=false,fail_constructor=false,throw_constructor=false,reenter=false;
 bool fail_write=false;
 std::uint32_t reject_write=0u;
 n::InitAuxiliaryObjects* original=nullptr;
 n::Code nested_code=n::Code::Completed;
 std::uint8_t init_flag=0u;
 Host(){
  memory[g::kPendingHeadVA]=0u;memory[g::kArrayVA]=0xa000u;
  memory[g::kCapacityVA]=2u;memory[g::kCountVA]=0u;
  memory[g::kArrayCacheVA]=0xa000u;
  for(auto off:{0x08u,0x0cu,0x10u,0x14u,0x18u,0x1cu})
   memory[kOwner+off]=0u;
 }
 bool allocate_original_2226e790(std::uint32_t bytes,std::uint32_t& ptr)override{
  assert(bytes==n::kObjectBytes);
  if(fail_allocator)return false;
  const auto index=allocations++;
  ptr=index<alloc_addresses.size()?alloc_addresses.at(index):
                           kStorage+static_cast<std::uint32_t>(index)*0x100u;
  return true;
 }
 bool read_u32(std::uint32_t va,std::uint32_t& value)override{
  const auto it=memory.find(va);
  if(it==memory.end())return false;
  value=it->second;return true;
 }
 bool write_u32(std::uint32_t va,std::uint32_t value)override{
  if(fail_write||va==reject_write)return false;
  memory[va]=value;return true;
 }
 bool construct_original(const n::ObjectSpec& spec,std::uint32_t obj_this,
     std::uint32_t element_size,std::uint32_t zero1,std::uint32_t zero2)override{
  assert(element_size==64u&&zero1==0u&&zero2==0u);
  assert(obj_this>=n::kCtorThisOffset);
  assert(memory.at(obj_this-4u)==spec.original_vtable_va);
  std::size_t cnt=0u;const auto* all=n::original_objects(cnt);
  std::size_t found=cnt;
  for(std::size_t i=0;i<cnt;++i)
   if(all[i].original_constructor_function_va==spec.original_constructor_function_va)
    found=i;
  assert(found<cnt);
  constructed.push_back(found);
  if(reenter&&original){
   reenter=false;nested_code=original->run(kOwner).code;
  }
  if(throw_constructor)throw std::runtime_error("guest constructor callback");
  return !fail_constructor;
 }
 bool read_u8(std::uint32_t addr,std::uint8_t& value)override{
  if(addr!=g::kInitFlagVA)return false;
  value=init_flag;return true;
 }
 bool write_u8(std::uint32_t addr,std::uint8_t value)override{
  if(addr!=g::kInitFlagVA)return false;
  init_flag=value;return true;
 }
 bool grow_original_221f51a0(std::uint32_t,std::uint32_t)override{
  assert(false&&"one registered system cannot require growth");
  return false;
 }
 bool insert_original_221f54f0(std::uint32_t,std::uint32_t,std::uint32_t)override{
  assert(false&&"late-insert not in Phase662 integration");
  return false;
 }
 bool warn_duplicate_original(std::uint32_t)override{return false;}
 bool virtual_init_slot_zero(std::uint32_t obj,bool& success)override{
  init_objects.push_back(obj);
  assert(obj==kOwner);
  std::uint32_t vtable=0u,fn=0u;
  if(!read_u32(obj,vtable)||!read_u32(vtable,fn)||!original)return false;
  assert(vtable==kVtable&&fn==n::kOriginalVA);
  const auto r=original->run(obj);
  success=r.code==n::Code::Completed&&r.original_return_al;
  return true;
 }
};
void original_metadata(){
 std::size_t count=0;
 const auto* all=n::original_objects(count);
 assert(count==6u&&all!=nullptr);
 const std::uint32_t expected_fields[]={0x0cu,0x08u,0x10u,0x14u,0x18u,0x1cu};
 const std::uint32_t expected_vtables[]={
  0x2244b1e4u,0x2244b1d0u,0x2244b1bcu,
  0x2244b1a8u,0x2244b194u,0x2244b180u};
 const std::uint32_t expected_ctors[]={
  0x22225ba0u,0x22225f10u,0x22226280u,
  0x222265f0u,0x22226960u,0x22226cd0u};
 for(std::size_t i=0u;i<count;++i){
  assert(all[i].owner_field_offset==expected_fields[i]);
  assert(all[i].original_vtable_va==expected_vtables[i]);
  assert(all[i].original_constructor_function_va==expected_ctors[i]);
  assert(all[i].original_allocator_call_va>=n::kOriginalVA);
  assert(all[i].original_allocator_call_va<n::kOriginalVA+n::kOriginalBytes);
 }
}
void six_allocations_and_conditional_owner_stores(){
 Host h;n::InitAuxiliaryObjects original(h);h.original=&original;
 // Original allocation and construction occur EVEN if owner fields already
 // hold pointers. The final store into those fields is conditional.
 h.memory[kOwner+0x08u]=0xd00du;
 h.memory[kOwner+0x18u]=0xbeefu;
 const auto result=original.run(kOwner);
 assert(result.code==n::Code::Completed&&result.original_return_al);
 assert(result.objects_processed==6u&&result.allocation_calls==6u);
 assert(result.constructor_calls==6u&&result.successful_allocations==6u);
 assert(result.missing_allocations==0u&&result.preexisting_destinations==2u);
 assert(result.destination_writes==4u&&h.constructed.size()==6u);
 for(std::size_t i=0u;i<6u;++i)assert(h.constructed[i]==i);
 assert(h.memory.at(kOwner+0x08u)==0xd00du);
 assert(h.memory.at(kOwner+0x18u)==0xbeefu);
 assert(h.memory.at(kOwner+0x0cu)==0x8000u);
 assert(h.memory.at(kOwner+0x10u)==0x8200u);
 assert(h.memory.at(kOwner+0x14u)==0x8300u);
 assert(h.memory.at(kOwner+0x1cu)==0x8500u);
}
void original_malloc_failure_is_not_init_failure(){
 Host h;n::InitAuxiliaryObjects original(h);h.original=&original;
 h.alloc_addresses={0u,0u,0u,0u,0u,0u};
 const auto result=original.run(kOwner);
 assert(result.code==n::Code::Completed&&result.original_return_al);
 assert(result.missing_allocations==6u&&result.constructor_calls==0u);
 assert(result.destination_writes==6u&&result.objects_processed==6u);
 assert(h.constructed.empty());
 for(auto off:{0x08u,0x0cu,0x10u,0x14u,0x18u,0x1cu})
  assert(h.memory.at(kOwner+off)==0u);
}
void mixed_failure_preserves_order_and_preexisting_field(){
 Host h;n::InitAuxiliaryObjects original(h);h.original=&original;
 h.alloc_addresses={0x9000u,0u,0x9200u,0u,0x9400u,0u};
 h.memory[kOwner+0x14u]=0x11110000u;
 const auto r=original.run(kOwner);
 assert(r.code==n::Code::Completed&&r.allocation_calls==6u);
 assert(r.constructor_calls==3u&&r.missing_allocations==3u);
 assert(r.preexisting_destinations==1u);
 assert((h.constructed==std::vector<std::size_t>{0u,2u,4u}));
 assert(h.memory.at(kOwner+0x0cu)==0x9000u);
 assert(h.memory.at(kOwner+0x08u)==0u);
 assert(h.memory.at(kOwner+0x10u)==0x9200u);
 assert(h.memory.at(kOwner+0x14u)==0x11110000u);
 assert(h.memory.at(kOwner+0x18u)==0x9400u);
 assert(h.memory.at(kOwner+0x1cu)==0u);
}
void host_failure_and_reentrancy(){
 Host h;n::InitAuxiliaryObjects original(h);h.original=&original;
 assert(original.run(0u).code==n::Code::InvalidOwner);
 h.fail_allocator=true;
 assert(original.run(kOwner).code==n::Code::ExternalCallbackFailure);
 assert(h.allocations==0u);
 h.fail_allocator=false;h.fail_constructor=true;
 auto result=original.run(kOwner);
 assert(result.code==n::Code::ExternalCallbackFailure);
 assert(result.allocation_calls==1u&&result.constructor_calls==1u);
 assert(h.memory.at(kOwner+0x0cu)==0u); // constructor failed before target store
 h.fail_constructor=false;h.reenter=true;
 result=original.run(kOwner);
 assert(result.code==n::Code::Completed&&h.nested_code==n::Code::Reentrant);
 h.throw_constructor=true;
 result=original.run(kOwner);
 assert(result.code==n::Code::ExternalCallbackFailure);
 Host reject;n::InitAuxiliaryObjects missing(reject);
 reject.reject_write=kOwner+0x0cu;
 result=missing.run(kOwner);
 assert(result.code==n::Code::WriteFailure);
 assert(result.objects_processed==0u);
}
void compiled_phase660_integration(){
 Host h;n::InitAuxiliaryObjects original(h);h.original=&original;
 g::AutoGameSystems game(h);
 assert(game.register_system(kOwner).code==g::Code::Completed);
 h.memory[kOwner]=kVtable;
 h.memory[kVtable]=n::kOriginalVA;
 const auto result=game.initialize_all();
 assert(result.code==g::Code::Completed&&result.initialized_flag_set);
 assert(result.virtual_init_calls==1u&&result.appended==1u);
 assert(h.init_flag==1u&&h.memory.at(g::kCountVA)==1u);
 assert(h.memory.at(g::kPendingHeadVA)==0u);
 assert(h.allocations==6u&&h.constructed.size()==6u);
 assert((h.init_objects==std::vector<std::uint32_t>{kOwner}));
}
}
int main(){
 original_metadata();six_allocations_and_conditional_owner_stores();
 original_malloc_failure_is_not_init_failure();
 mixed_failure_preserves_order_and_preexisting_field();
 host_failure_and_reentrancy();compiled_phase660_integration();
}
