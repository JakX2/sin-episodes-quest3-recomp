#include "sin_compat/original_server_system_init_phase661.hpp"
#include "sin_compat/original_server_autogamesystem_phase660.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>
namespace n=sin_compat::original_server_system_init_phase661;
namespace g=sin_compat::original_server_autogamesystem_phase660;
namespace {
constexpr std::uint32_t kNestedOwner=0x2000u,kNestedSub=0x2630u;
constexpr std::uint32_t kNamesSub=0x5000u,kDecalSub=0x6004u;
constexpr std::uint32_t kPartitionSub=0x7000u;
constexpr std::uint32_t kArray=0xc000u;
struct Guest final:n::IHost,g::IHost{
 std::map<std::uint32_t,std::uint32_t> words;
 std::vector<std::tuple<std::string,std::uint32_t,std::uint32_t>> actions;
 std::vector<std::uint32_t> virtual_systems;
 std::uint8_t init_flag=0u;
 bool fail_external=false,throw_external=false,reenter=false;
 std::size_t fail_named_call=static_cast<std::size_t>(-1);
 std::size_t named_calls=0u;
 n::Code nested_code=n::Code::Completed;
 n::Initializers* init=nullptr;
 std::uint32_t saved_partition_arg=0xffffffffu;
 std::uint32_t saved_primary_result=0u;
 Guest(){
  words[g::kPendingHeadVA]=0u;
  words[g::kArrayVA]=kArray;
  words[g::kCountVA]=0u;
  words[g::kCapacityVA]=8u;
  words[g::kArrayCacheVA]=kArray;
  words[kNestedOwner]=0x9000u; // original enclosing-object primary vtable
  words[0x9018u]=0x9a00u;    // original vtable byte offset +0x18
  words[n::kSpatialPartitionGlobalVA]=0x9200u;
  words[0x9200u]=0x9300u;   // external engine interface vtable
  words[0x9328u]=0x9400u;   // original virtual byte offset +0x28
 }
 bool read_u32(std::uint32_t address,std::uint32_t& value)override{
  const auto found=words.find(address);
  if(found==words.end())return false;
  value=found->second;return true;
 }
 bool write_u32(std::uint32_t address,std::uint32_t value)override{
  words[address]=value;return true;
 }
 bool read_u8(std::uint32_t address,std::uint8_t& value)override{
  if(address!=g::kInitFlagVA)return false;
  value=init_flag;return true;
 }
 bool write_u8(std::uint32_t address,std::uint8_t value)override{
  if(address!=g::kInitFlagVA)return false;
  init_flag=value;return true;
 }
 bool grow_original_221f51a0(std::uint32_t,std::uint32_t)override{
  assert(false&&"8 slots suffice; unexpected Phase660 guest grow");
  return false;
 }
 bool insert_original_221f54f0(std::uint32_t,std::uint32_t,std::uint32_t)override{
  assert(false&&"no late registration in Phase661 integration");
  return false;
 }
 bool warn_duplicate_original(std::uint32_t)override{return false;}
 bool invoke_primary_slot_six(std::uint32_t fn,std::uint32_t owner,
                             std::uint32_t& value)override{
  assert(fn==0x9a00u&&owner==kNestedOwner);
  actions.emplace_back("primary_slot6",fn,owner);
  if(fail_external)return false;
  value=0xabcd1234u;
  return true;
 }
 bool invoke_nested_helper(std::uint32_t owner,std::uint32_t value)override{
  actions.emplace_back("nested_helper",owner,value);
  saved_primary_result=value;
  return !fail_external;
 }
 bool invoke_named_helper(std::uint32_t object,std::uint32_t zero,
                           const n::NameSpec& name)override{
  assert(object==kNamesSub&&zero==0u);
  ++named_calls;
  actions.emplace_back(name.text,name.original_literal_va,zero);
  if(reenter&&init){
   reenter=false;
   nested_code=init->run(n::kNamedRegistrationVA,object).code;
  }
  if(throw_external)throw 17;
  return !fail_external&&named_calls!=fail_named_call;
 }
 bool invoke_decal_helper(std::uint32_t owner,const n::NameSpec& name)override{
  assert(owner==kDecalSub-4u);
  assert(std::string(name.text)=="scripts/decals_subrect.txt");
  actions.emplace_back(name.text,owner,name.original_literal_va);
  return !fail_external;
 }
 bool invoke_partition_slot_ten(std::uint32_t fn,std::uint32_t interface_this,
                                std::uint32_t argument)override{
  assert(fn==0x9400u&&interface_this==0x9200u);
  saved_partition_arg=argument;
  actions.emplace_back("partition_vslot10",interface_this,argument);
  return !fail_external;
 }
 bool virtual_init_slot_zero(std::uint32_t object,bool& al)override{
  virtual_systems.push_back(object);
  std::uint32_t vt=0u,fn=0u;
  if(!read_u32(object,vt)||!read_u32(vt,fn)||!init)return false;
  const auto r=init->run(fn,object);
  al=(r.code==n::Code::Completed&&r.original_return_al);
  return true;
 }
 void register_all(g::AutoGameSystems& game){
  auto a=game.register_system(kNestedSub);
  auto b=game.register_system(kNamesSub);
  auto c=game.register_system(kDecalSub);
  auto d=game.register_system(kPartitionSub);
  assert(a.code==g::Code::Completed&&b.code==g::Code::Completed);
  assert(c.code==g::Code::Completed&&d.code==g::Code::Completed);
  // Derived class constructors replace base AutoGameSystem's vtable.
  words[kNestedSub]=0xa000u;words[0xa000u]=n::kSubobjectDelegationVA;
  words[kNamesSub]=0xa004u;words[0xa004u]=n::kNamedRegistrationVA;
  words[kDecalSub]=0xa008u;words[0xa008u]=n::kDecalScriptVA;
  words[kPartitionSub]=0xa00cu;words[0xa00cu]=n::kSpatialPartitionVA;
 }
};
void test_exact_method_shapes(){
 std::size_t count=0;
 const auto* spec=n::original_name_specs(count);
 assert(spec&&count==5u);
 const char* expected[]={"phonemes","phonemes_weak","phonemes_strong",
                         "random","randomAlert"};
 const std::uint32_t addresses[]={
  n::kPhonemesLiteralVA,n::kPhonemesWeakLiteralVA,
  n::kPhonemesStrongLiteralVA,n::kRandomLiteralVA,
  n::kRandomAlertLiteralVA};
 for(std::size_t i=0;i<count;++i){
  assert(std::string(spec[i].text)==expected[i]);
  assert(spec[i].original_literal_va==addresses[i]);
 }
 Guest host;n::Initializers original(host);host.init=&original;
 auto nested=original.run(n::kSubobjectDelegationVA,kNestedSub);
 assert(nested.code==n::Code::Completed&&nested.external_calls==2u);
 assert(nested.original_return_al&&nested.final_object_va==kNestedOwner);
 assert(host.saved_primary_result==0xabcd1234u);
 assert(host.actions.size()==2u);
 assert(std::get<0>(host.actions[0])=="primary_slot6");
 assert(std::get<0>(host.actions[1])=="nested_helper");
 host.actions.clear();
 const auto names=original.run(n::kNamedRegistrationVA,kNamesSub);
 assert(names.code==n::Code::Completed&&names.external_calls==5u);
 assert(host.actions.size()==5u&&host.named_calls==5u);
 for(std::size_t i=0;i<count;++i){
  assert(std::get<0>(host.actions[i])==expected[i]);
  assert(std::get<1>(host.actions[i])==addresses[i]);
  assert(std::get<2>(host.actions[i])==0u);
 }
 host.actions.clear();
 const auto decal=original.run(n::kDecalScriptVA,kDecalSub);
 assert(decal.code==n::Code::Completed&&decal.external_calls==1u);
 assert(decal.final_object_va==kDecalSub-4u);
 assert(std::get<0>(host.actions[0])=="scripts/decals_subrect.txt");
 assert(std::get<1>(host.actions[0])==kDecalSub-4u);
 assert(std::get<2>(host.actions[0])==n::kDecalLiteralVA);
 host.actions.clear();
 const auto nonnull=original.run(n::kSpatialPartitionVA,kPartitionSub);
 assert(nonnull.code==n::Code::Completed&&nonnull.external_calls==1u);
 assert(host.saved_partition_arg==kPartitionSub+8u);
 const auto null_this=original.run(n::kSpatialPartitionVA,0u);
 assert(null_this.code==n::Code::Completed&&null_this.original_return_al);
 assert(host.saved_partition_arg==0u);
 assert(host.actions.size()==2u);
}
void test_original_return_value_and_fault_boundaries(){
 Guest host;n::Initializers original(host);host.init=&original;
 assert(original.run(0x22222222u,kNamesSub).code==n::Code::Unsupported);
 assert(original.run(n::kNamedRegistrationVA,0u).code==n::Code::InvalidObject);
 assert(original.run(n::kDecalScriptVA,3u).code==n::Code::InvalidPointer);
 assert(original.run(n::kSubobjectDelegationVA,0x620u).code==n::Code::InvalidPointer);
 assert(original.run(n::kSpatialPartitionVA,0xfffffffcu).code==n::Code::InvalidPointer);
 host.words.erase(0x9018u);
 auto a=original.run(n::kSubobjectDelegationVA,kNestedSub);
 assert(a.code==n::Code::GuestReadFailure&&a.stage==n::Stage::ReadPrimarySlotSix);
 host.words[0x9018u]=0x9a00u;
 host.words.erase(n::kSpatialPartitionGlobalVA);
 a=original.run(n::kSpatialPartitionVA,kPartitionSub);
 assert(a.code==n::Code::GuestReadFailure&&a.external_calls==0u);
 host.words[n::kSpatialPartitionGlobalVA]=0x9200u;
 host.fail_external=true;
 a=original.run(n::kSpatialPartitionVA,kPartitionSub);
 assert(a.code==n::Code::ExternalCallbackFailure&&!a.original_return_al);
 host.fail_external=false;
 host.fail_named_call=3u;
 a=original.run(n::kNamedRegistrationVA,kNamesSub);
 assert(a.code==n::Code::ExternalCallbackFailure&&a.external_calls==3u);
 assert(host.named_calls==3u);
 host.fail_named_call=static_cast<std::size_t>(-1);
 host.reenter=true;
 a=original.run(n::kNamedRegistrationVA,kNamesSub);
 assert(a.code==n::Code::Completed&&host.nested_code==n::Code::Reentrant);
 host.throw_external=true;
 a=original.run(n::kNamedRegistrationVA,kNamesSub);
 assert(a.code==n::Code::ExternalCallbackFailure);
}
void test_compiled_phase660_to_661_integration(){
 Guest host;n::Initializers original(host);
 host.init=&original;
 g::AutoGameSystems systems(host);
 host.register_all(systems);
 const auto r=systems.initialize_all();
 assert(r.code==g::Code::Completed&&r.initialized_flag_set);
 assert(r.appended==4u&&r.virtual_init_calls==4u);
 assert(host.init_flag==1u);
 assert(host.words.at(g::kPendingHeadVA)==0u);
 assert(host.words.at(g::kCountVA)==4u);
 assert((host.virtual_systems==std::vector<std::uint32_t>{
   kPartitionSub,kDecalSub,kNamesSub,kNestedSub}));
 assert(host.saved_partition_arg==kPartitionSub+8u);
 assert(host.named_calls==5u);
 assert(host.saved_primary_result==0xabcd1234u);
 assert(host.actions.size()==9u);
 assert(std::get<0>(host.actions.front())=="partition_vslot10");
 assert(std::get<0>(host.actions[1])=="scripts/decals_subrect.txt");
 assert(std::get<0>(host.actions.back())=="nested_helper");
}
}
int main(){
 test_exact_method_shapes();
 test_original_return_value_and_fault_boundaries();
 test_compiled_phase660_to_661_integration();
}
