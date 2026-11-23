#include "sin_compat/original_server_autogamesystem_phase660.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <stdexcept>
#include <utility>

namespace n=sin_compat::original_server_autogamesystem_phase660;
namespace {
constexpr std::uint32_t kExisting=0x1000u,kAlpha=0x2000u;
constexpr std::uint32_t kBravo=0x3000u,kLate=0x4000u;
constexpr std::uint32_t kInitialArray=0xa000u,kRelocatedArray=0xb000u;
struct Host final:n::IHost {
 std::map<std::uint32_t,std::uint32_t> words;
 std::uint8_t init_flag=0u;
 std::vector<std::uint32_t> writes;
 std::vector<std::uint32_t> virtual_calls,duplicates;
 std::vector<std::pair<std::uint32_t,std::uint32_t>> late_inserts;
 n::AutoGameSystems* model=nullptr;
 n::Code nested=n::Code::Completed;
 bool reenter=false,fail_warn=false,fail_grow=false;
 bool fail_insert=false,fail_virtual_callback=false,throw_on_virtual=false;
 std::uint32_t fail_virtual_object=0u,reject_write=0u;
 unsigned native_grows=0u;
 Host(){
  words[n::kPendingHeadVA]=0u;words[n::kArrayVA]=kInitialArray;
  words[n::kCountVA]=0u;words[n::kCapacityVA]=2u;
  words[n::kArrayCacheVA]=kInitialArray;
 }
 bool read_u8(std::uint32_t a,std::uint8_t& v)override{
  if(a!=n::kInitFlagVA)return false;
  v=init_flag;return true;
 }
 bool read_u32(std::uint32_t a,std::uint32_t& v)override{
  const auto it=words.find(a);
  if(it==words.end())return false;
  v=it->second;return true;
 }
 bool write_u8(std::uint32_t a,std::uint8_t v)override{
  if(a!=n::kInitFlagVA||reject_write==a)return false;
  init_flag=v;writes.push_back(a);return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t v)override{
  if(a==reject_write)return false;
  words[a]=v;writes.push_back(a);return true;
 }
 bool grow_original_221f51a0(std::uint32_t owner,std::uint32_t requested)override{
  assert(owner==n::kArrayVA&&requested==1u);
  ++native_grows;
  if(fail_grow)return false;
  const auto from=words.at(n::kArrayVA),count=words.at(n::kCountVA);
  const auto to=kRelocatedArray+(native_grows-1u)*0x1000u;
  for(std::uint32_t i=0u;i<count;++i)
    words[to+i*4u]=words.at(from+i*4u);
  words[n::kArrayVA]=to;words[n::kCapacityVA]=words.at(n::kCapacityVA)*2u;
  return true;
 }
 bool insert_original_221f54f0(std::uint32_t owner,std::uint32_t index,
                                std::uint32_t object)override{
  late_inserts.push_back({index,object});
  assert(owner==n::kArrayVA);
  if(fail_insert)return false;
  const auto cap=words.at(n::kCapacityVA);
  if(index==cap){
   if(!grow_original_221f51a0(owner,1u))return false;
  }
  words[words.at(n::kArrayVA)+index*4u]=object;
  words[n::kCountVA]=index+1u;
  words[n::kArrayCacheVA]=words.at(n::kArrayVA);
  return true;
 }
 bool warn_duplicate_original(std::uint32_t object)override{
  duplicates.push_back(object);
  return !fail_warn;
 }
 bool virtual_init_slot_zero(std::uint32_t object,bool& ok)override{
  virtual_calls.push_back(object);
  if(throw_on_virtual)throw std::runtime_error("guest virtual init");
  if(reenter&&model){reenter=false;nested=model->initialize_all().code;}
  if(fail_virtual_callback)return false;
  ok=(object!=fail_virtual_object);
  return true;
 }
};
void deferred_registration_and_duplicate(){
 Host h;n::AutoGameSystems reg(h);
 h.words[n::kCountVA]=1u;h.words[kInitialArray]=kExisting;
 for(std::uint32_t object:{kAlpha,kExisting,kBravo}){
  const auto r=reg.register_system(object);
  assert(r.code==n::Code::Completed);
  assert(r.writes_committed==3u&&!r.native_late_insert_called);
  assert(h.words.at(object)==n::kAutoSystemVtableVA);
 }
 assert(h.words.at(n::kPendingHeadVA)==kBravo);
 assert(h.words.at(kBravo+4u)==kExisting);
 assert(h.words.at(kExisting+4u)==kAlpha);
 assert(h.words.at(kAlpha+4u)==0u);
 const auto r=reg.initialize_all();
 assert(r.code==n::Code::Completed);
 assert(r.processed_pending==3u&&r.appended==2u&&r.duplicates==1u);
 assert(r.virtual_init_calls==3u&&r.initialized_flag_set);
 assert(r.native_grow_called&&h.native_grows==1u);
 assert(h.words.at(n::kPendingHeadVA)==0u&&h.init_flag==1u);
 assert(h.words.at(n::kCountVA)==3u);
 assert(h.words.at(n::kCapacityVA)==4u);
 assert(h.words.at(n::kArrayVA)==kRelocatedArray);
 assert(h.words.at(n::kArrayCacheVA)==kRelocatedArray);
 assert(h.words.at(kRelocatedArray)==kExisting);
 assert(h.words.at(kRelocatedArray+4u)==kBravo);
 assert(h.words.at(kRelocatedArray+8u)==kAlpha);
 assert((h.virtual_calls==std::vector<std::uint32_t>{kExisting,kBravo,kAlpha}));
 assert((h.duplicates==std::vector<std::uint32_t>{kExisting}));
 assert(h.writes[h.writes.size()-2u]==n::kPendingHeadVA);
 assert(h.writes.back()==n::kInitFlagVA);
}
void empty_boot_then_late_registration(){
 Host h;n::AutoGameSystems reg(h);
 const auto first=reg.initialize_all();
 assert(first.code==n::Code::Completed&&first.processed_pending==0u);
 assert(first.virtual_init_calls==0u&&first.initialized_flag_set);
 assert(h.init_flag==1u&&h.words.at(n::kPendingHeadVA)==0u);
 const auto late=reg.register_system(kLate);
 assert(late.code==n::Code::Completed&&late.native_late_insert_called);
 assert(late.writes_committed==1u);
 assert(h.words.at(kLate)==n::kAutoSystemVtableVA);
 assert((h.late_inserts==std::vector<std::pair<std::uint32_t,std::uint32_t>>{{0u,kLate}}));
 assert(h.words.at(n::kCountVA)==1u);
 assert(h.words.at(kInitialArray)==kLate);
 assert(h.words.at(n::kPendingHeadVA)==0u);
 const auto repeated=reg.initialize_all();
 assert(repeated.code==n::Code::Completed&&repeated.virtual_init_calls==1u);
 assert(h.virtual_calls.back()==kLate);
}
void virtual_failure_is_not_rolled_back(){
 Host h;n::AutoGameSystems reg(h);
 assert(reg.register_system(kAlpha).code==n::Code::Completed);
 assert(reg.register_system(kBravo).code==n::Code::Completed);
 h.fail_virtual_object=kAlpha;
 const auto fail=reg.initialize_all();
 assert(fail.code==n::Code::VirtualInitFailed&&fail.initialized_flag_set);
 assert(fail.virtual_init_calls==2u&&fail.failed_system_va==kAlpha);
 assert(h.init_flag==1u&&h.words.at(n::kPendingHeadVA)==0u);
 assert(h.words.at(n::kCountVA)==2u);
 assert(h.words.at(kInitialArray)==kBravo);
 assert(h.words.at(kInitialArray+4u)==kAlpha);
 h.fail_virtual_object=0u;
 const auto retry=reg.initialize_all();
 assert(retry.code==n::Code::Completed&&retry.virtual_init_calls==2u);
}
void partial_write_and_host_error(){
 Host h;n::AutoGameSystems reg(h);
 assert(reg.register_system(0u).code==n::Code::InvalidObject);
 h.reject_write=kAlpha;
 const auto r=reg.register_system(kAlpha);
 assert(r.code==n::Code::HostRejected&&r.writes_committed==0u);
 h.reject_write=0u;
 assert(reg.register_system(kAlpha).code==n::Code::Completed);
 h.reject_write=n::kInitFlagVA;
 const auto fail=reg.initialize_all();
 assert(fail.code==n::Code::HostRejected);
 assert(h.words.at(n::kPendingHeadVA)==0u);
 assert(h.words.at(n::kCountVA)==1u);
 assert(h.init_flag==0u); // native has no rollback of committed count/head.
 h.reject_write=0u;
 h.throw_on_virtual=true;
 const auto thrown=reg.initialize_all();
 assert(thrown.code==n::Code::HostRejected);
 assert(h.init_flag==1u);
 h.throw_on_virtual=false;
 h.reenter=true;h.model=&reg;
 const auto ok=reg.initialize_all();
 assert(ok.code==n::Code::Completed&&h.nested==n::Code::Reentrant);
 Host missing;n::AutoGameSystems check(missing);
 assert(check.register_system(kAlpha).code==n::Code::Completed);
 missing.words[n::kArrayVA]=0u;
 assert(check.initialize_all().code==n::Code::MissingArray);
}
void pending_cycle_guard(){
 Host h;n::AutoGameSystems reg(h);
 assert(reg.register_system(kAlpha).code==n::Code::Completed);
 h.words[kAlpha+4u]=kAlpha;
 const auto broken=reg.initialize_all();
 assert(broken.code==n::Code::CycleGuard);
 assert(broken.processed_pending==65536u);
 assert(h.words.at(n::kPendingHeadVA)==kAlpha);
 assert(h.init_flag==0u);
}
}
int main(){
 deferred_registration_and_duplicate();empty_boot_then_late_registration();
 virtual_failure_is_not_rolled_back();partial_write_and_host_error();
 pending_cycle_guard();
}
