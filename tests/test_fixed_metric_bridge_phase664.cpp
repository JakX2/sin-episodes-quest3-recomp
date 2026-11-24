// Phase664: tested binding of the previously reconstructed fixed Easy/
// Normal/Hard policy to the ORIGINAL SHA-pinned SiN server metric layout.
// The memory and Source ConVars are controlled by a synthetic engine host.
// No original executable bytes or original Windows game process in tests.
#include "sin_compat/fixed_metric_bridge_phase664.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace p=sin_compat::difficulty::fixed_phase663;
namespace b=sin_compat::difficulty::fixed_phase664;
using sin_compat::difficulty::RuntimeMetrics;
using sin_compat::difficulty::original_initial_metric_values;

namespace {
constexpr std::uint32_t kBase=0x40000000u;
std::uint32_t bits(float value) noexcept {
 std::uint32_t result=0u;std::memcpy(&result,&value,sizeof value);return result;
}
float float_from_bits(std::uint32_t value) noexcept {
 float result=0.f;std::memcpy(&result,&value,sizeof value);return result;
}
struct Host final:b::IOriginalGameHost {
 std::map<std::uint32_t,std::uint32_t> words;
 std::map<std::uint32_t,std::uint8_t> bytes;
 std::map<std::string,int> convars{{"dd_enabled",1},{"skill",0}};
 std::vector<std::string> events;
 std::uint32_t base=kBase;
 std::size_t total_writes=0u;
 std::size_t fail_write_index=14u;
 std::size_t fail_readback_index=14u;
 std::size_t corrupt_readback_index=14u;
 std::size_t writes_before=0u;
 bool throw_write=false,stuck_dd=false,fail_cvar_read=false;
 b::OriginalMetricBridge* active=nullptr;
 b::BindCode reentrant_result=b::BindCode::Completed;
 bool reenter=false;
 explicit Host(std::uint32_t location=kBase):base(location){
  const auto original=original_initial_metric_values();
  for(std::size_t i=0u;i<14u;++i) {
    const auto text=sin_compat::difficulty::original_metrics[i].name;
    // Invented synthetic mapped name VAs inside 0x628000 PE32 image.
    const auto name_addr=base+0x00600000u+static_cast<std::uint32_t>(i)*64u;
    words[b::name_pointer_va(base,i)]=name_addr;
    for(std::size_t j=0u;j<text.size();++j)
      bytes[name_addr+static_cast<std::uint32_t>(j)]=
          static_cast<std::uint8_t>(text[j]);
    bytes[name_addr+static_cast<std::uint32_t>(text.size())]=0u;
    words[b::metric_va(base,i)]=bits(original[i]);
  }
 }
 bool read_u32(std::uint32_t address,std::uint32_t& value)override {
  const auto it=words.find(address);
  if(it==words.end())return false;
  value=it->second;
  return true;
 }
 bool read_u8(std::uint32_t address,std::uint8_t& value)override{
  const auto it=bytes.find(address);
  if(it==bytes.end())return false;
  value=it->second;
  return true;
 }
 bool read_f32(std::uint32_t address,float& value)override{
  const auto it=words.find(address);
  if(it==words.end())return false;
  if(total_writes>writes_before) {
   for(std::size_t i=0u;i<14u;++i)if(address==b::metric_va(base,i)) {
     if(i==fail_readback_index)return false;
     if(i==corrupt_readback_index){
       corrupt_readback_index=14u; // only corrupt one verification
       value=float_from_bits(it->second^0x00800000u);
       return true;
     }
   }
  }
  value=float_from_bits(it->second);
  return true;
 }
 bool write_f32(std::uint32_t address,float value)override {
  ++total_writes;
  for(std::size_t i=0u;i<14u;++i)if(address==b::metric_va(base,i)){
   if(i==fail_write_index)return false;
   if(throw_write){
     throw_write=false;
     throw std::runtime_error("synthetic guest-memory fault");
   }
   words[address]=bits(value);
   events.emplace_back("metric:"+std::to_string(i));
   return true;
  }
  return false;
 }
 bool get_convar_int(std::string_view name,int& out)override{
  if(fail_cvar_read)return false;
  const auto it=convars.find(std::string(name));
  if(it==convars.end())return false;
  out=it->second;
  events.emplace_back("get:"+it->first);
  return true;
 }
 bool set_convar_int(std::string_view name,int value)override{
  const auto key=std::string(name);
  auto it=convars.find(key);
  if(it==convars.end())return false;
  if(!(stuck_dd&&key=="dd_enabled"))
    it->second=value;
  events.emplace_back("set:"+key);
  if(reenter&&active) {
   reenter=false;
   reentrant_result=active->apply_skill(3).code;
  }
  return true;
 }
 RuntimeMetrics current_metrics() const {
  RuntimeMetrics values{};
  for(std::size_t i=0u;i<14u;++i)
   values[i]=float_from_bits(words.at(b::metric_va(base,i)));
  return values;
 }
};
void original_strides_and_relocation(){
 static_assert(b::kOriginalMetricEntries==14u);
 static_assert(b::kOriginalRecordStride==28u);
 static_assert(b::kOriginalMetricTableRva==0x523478u);
 static_assert(b::kOriginalNameTableRva==0x45f04cu);
 for(const auto base:{0x10000000u,0x40000000u,0x70000000u}){
  for(std::size_t i=0u;i<14u;++i){
   assert(b::metric_va(base,i)==base+0x523478u+28u*i);
   assert(b::name_pointer_va(base,i)==base+0x45f04cu+4u*i);
  }
  assert(b::metric_va(base,14u)==0u);
  assert(b::name_pointer_va(base,100u)==0u);
 }
 assert(b::metric_va(0u,0u)==0u);
 assert(b::metric_va(1u,0u)==0u);
 assert(b::metric_va(0xfff00000u,0u)==0u);
}
void original_names_and_profiles_apply(){
 Host original{};
 p::FixedDifficultyRuntime policy;
 b::OriginalMetricBridge bridge(original,policy,kBase);
 const auto result=bridge.apply_skill(3);
 assert(result.code==b::BindCode::Completed&&result.published);
 assert(result.attempted_writes==14u&&result.confirmed_writes==14u);
 assert(result.old_adaptive_enabled&&result.old_engine_skill==0);
 assert(result.adaptive_disabled&&result.skill_selected);
 assert(!result.recovery_required);
 assert(original.convars.at("dd_enabled")==0);
 assert(original.convars.at("skill")==3);
 assert(policy.skill()==3&&policy.revision()==1u);
 assert(original.current_metrics()==p::make_profile(p::Level::Hard).metrics);
 assert(original.events.at(0)=="get:dd_enabled");
 assert(original.events.at(1)=="set:dd_enabled");
 assert(original.events.at(2)=="get:dd_enabled");
 assert(original.events.at(3)=="get:skill");
 assert(original.events.at(4)=="set:skill");
 assert(original.events.at(5)=="get:skill");
 assert(original.events.at(6)=="metric:0");
 assert(original.events.back()=="metric:13");
 // Save-restore may mutate the original 14 slots independently.
 for(std::size_t i=0u;i<14u;++i)
  original.words[b::metric_va(kBase,i)]=bits(999.f);
 const auto again=bridge.apply_skill(3);
 assert(again.code==b::BindCode::Completed&&policy.skill()==3);
 assert(policy.revision()==2u&&again.confirmed_writes==14u);
 assert(original.current_metrics()==p::make_profile(p::Level::Hard).metrics);
 assert(bridge.apply_skill(1).published);
 assert(original.current_metrics()==p::make_profile(p::Level::Easy).metrics);
 assert(policy.skill()==1&&policy.revision()==3u);
}
void no_writes_before_original_name_preflight(){
 Host host;
 p::FixedDifficultyRuntime policy;
 b::OriginalMetricBridge bridge(host,policy,kBase);
 assert(bridge.apply_skill(0).code==b::BindCode::InvalidSkill);
 assert(host.events.empty()&&policy.revision()==0u);
 const auto ptr_va=b::name_pointer_va(kBase,5u);
 const auto valid=host.words.at(ptr_va);
 host.words[ptr_va]=0u;
 auto result=bridge.apply_skill(1);
 assert(result.code==b::BindCode::NamePointerOutsideImage);
 assert(result.failed_metric==5u&&host.events.empty());
 host.words[ptr_va]=0xfffffffc;
 result=bridge.apply_skill(1);
 assert(result.code==b::BindCode::NamePointerOutsideImage);
 assert(host.events.empty());
 host.words[ptr_va]=valid;
 host.bytes[valid]=static_cast<std::uint8_t>('X');
 result=bridge.apply_skill(1);
 assert(result.code==b::BindCode::NameMismatch);
 assert(host.events.empty()&&policy.skill()==2);
 host.bytes[valid]=static_cast<std::uint8_t>('P');
 // A bad pointer table slot is a different failure from bad string bytes.
 host.words.erase(ptr_va);
 result=bridge.apply_skill(1);
 assert(result.code==b::BindCode::NamePointerMissing);
 assert(host.convars["dd_enabled"]==1);
}
void missing_metric_and_cvar_fail_closed(){
 Host host;
 p::FixedDifficultyRuntime policy;
 b::OriginalMetricBridge bridge(host,policy,kBase);
 host.words.erase(b::metric_va(kBase,8u));
 auto result=bridge.apply_skill(1);
 assert(result.code==b::BindCode::CurrentValueUnreadable);
 assert(result.failed_metric==8u&&host.events.empty());
 assert(host.convars["dd_enabled"]==1&&policy.skill()==2);
 Host disabled;
 b::OriginalMetricBridge bridge2(disabled,policy,kBase);
 disabled.fail_cvar_read=true;
 result=bridge2.apply_skill(1);
 assert(result.code==b::BindCode::ConvarReadFailure);
 assert(disabled.convars["dd_enabled"]==1);
 disabled.fail_cvar_read=false;
 disabled.stuck_dd=true;
 result=bridge2.apply_skill(1);
 assert(result.code==b::BindCode::ConvarReadbackMismatch);
 assert(result.recovery_required&&!result.adaptive_disabled);
 assert(result.attempted_writes==0u);
 assert(disabled.convars["dd_enabled"]==1);
 assert(policy.skill()==2);
}
void partial_write_rolls_back_in_reverse(){
 Host host;
 p::FixedDifficultyRuntime policy;
 b::OriginalMetricBridge bridge(host,policy,kBase);
 const auto before=host.current_metrics();
 host.fail_write_index=5u;
 auto result=bridge.apply_skill(3);
 assert(result.code==b::BindCode::MetricWriteFailure);
 assert(result.failed_metric==5u&&result.confirmed_writes==5u);
 assert(result.attempted_writes==6u&&result.rollback_attempted==6u);
 assert(result.rollback_verified==5u); // failing slot also rejects rollback
 assert(result.recovery_required&&!result.published);
 assert(host.current_metrics()==before);
 assert(host.convars["dd_enabled"]==0&&host.convars["skill"]==3);
 assert(policy.skill()==2&&policy.revision()==0u);
 // ConVars may already be changed while the profile is partially written.
 // The engine must abort startup even when earlier floats were restored.
 Host corruption;
 b::OriginalMetricBridge bridge2(corruption,policy,kBase);
 const auto original=corruption.current_metrics();
 corruption.corrupt_readback_index=2u;
 const auto mismatch=bridge2.apply_skill(1);
 assert(mismatch.code==b::BindCode::MetricReadbackMismatch);
 assert(mismatch.rollback_attempted==3u);
 assert(corruption.current_metrics()==original);
 assert(policy.skill()==2);
}
void reentry_fault_and_invalid_loaded_image(){
 Host host;
 p::FixedDifficultyRuntime policy;
 b::OriginalMetricBridge bridge(host,policy,kBase);
 host.active=&bridge;
 host.reenter=true;
 const auto r=bridge.apply_skill(1);
 assert(r.published&&r.code==b::BindCode::Completed);
 assert(host.reentrant_result==b::BindCode::Reentrant);
 Host bad;
 b::OriginalMetricBridge invalid(bad,policy,0xfffff000u);
 auto report=invalid.apply_skill(1);
 assert(report.code==b::BindCode::InvalidImageBase);
 assert(bad.events.empty());
 Host thrown;
 b::OriginalMetricBridge throwbridge(thrown,policy,kBase);
 thrown.throw_write=true;
 report=throwbridge.apply_skill(3);
 assert(report.code==b::BindCode::HostException);
 assert(report.rollback_attempted==1u);
 assert(report.recovery_required&&!report.published);
 assert(thrown.current_metrics()==original_initial_metric_values());
}
}
int main(){
 original_strides_and_relocation();
 original_names_and_profiles_apply();
 no_writes_before_original_name_preflight();
 missing_metric_and_cvar_fail_closed();
 partial_write_rolls_back_in_reverse();
 reentry_fault_and_invalid_loaded_image();
}
