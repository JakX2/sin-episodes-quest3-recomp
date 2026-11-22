#include "sin_compat/original_flying_event_filter_phase591.hpp"
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>
using namespace sin_compat::flying_event_filter_phase591;
struct MockHost final:Host {
 std::vector<std::pair<uint32_t,EventCase>> events;
 void native_special_event(uint32_t id,EventCase c) override {events.emplace_back(id,c);}
};
static void test_original_52_entry_dispatch(){
 unsigned special=0,otherwise=0;
 for(uint32_t event=kFirstNativeEvent;event<=kLastNativeEvent;event++){
  if(classify(event)==EventCase::NoNativeCase)otherwise++;else special++;
 }
 assert(kNativeTableEntries==0x3C-0x09+1);
 assert(special==6 && otherwise==46);
 assert(classify(0x08)==EventCase::NoNativeCase);
 assert(classify(0x3D)==EventCase::NoNativeCase);
 assert(classify(0x09)==EventCase::Native09Or0A);
 assert(classify(0x0A)==EventCase::Native09Or0A);
 assert(classify(0x28)==EventCase::Native28);
 assert(classify(0x2B)==EventCase::Native2B);
 assert(classify(0x38)==EventCase::Native38Or3C);
 assert(classify(0x3C)==EventCase::Native38Or3C);
 assert(classify(0x54)==EventCase::NoNativeCase);
}
static void test_exact_source_bound_return_groups(){
 MockHost host;
 for(uint32_t event:{0x09u,0x0Au,0x28u,0x2Bu}){
  auto result=dispatch(event,true,host);
  assert(result.native_boolean && result.native_effects_unmodeled && !result.needs_original_owner);
 }
 for(uint32_t event:{0x38u,0x3Cu}){
  auto result=dispatch(event,true,host);
  assert(!result.native_boolean && result.native_effects_unmodeled && !result.needs_original_owner);
 }
 assert(host.events.size()==6);
 assert(host.events.at(0).first==0x09 && host.events.at(1).first==0x0A);
 assert(host.events.at(4).second==EventCase::Native38Or3C);
}
static void test_event54_and_other_defaults_forward_without_owner(){
 MockHost host;
 for(uint32_t event:{0x08u,0x0Bu,0x27u,0x29u,0x39u,0x3Du,0x54u,0xA6u,0xFFFFFFFFu}){
  auto result=dispatch(event,false,host);
  assert(!result.native_boolean&&!result.native_effects_unmodeled&&!result.needs_original_owner);
 }
 assert(host.events.empty());
 assert(original_event_0x54_cannot_be_consumed());
}
static void test_special_native_invalid_handle_host_guard(){
 MockHost host;
 for(uint32_t event:{0x09u,0x28u,0x2Bu,0x38u,0x3Cu}){
  auto result=dispatch(event,false,host);
  assert(!result.native_boolean && result.native_effects_unmodeled && result.needs_original_owner);
 }
 assert(host.events.empty());
}
int main(){
 test_original_52_entry_dispatch();test_exact_source_bound_return_groups();
 test_event54_and_other_defaults_forward_without_owner();test_special_native_invalid_handle_host_guard();
}