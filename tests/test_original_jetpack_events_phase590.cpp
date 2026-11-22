#include "sin_compat/original_jetpack_events_phase590.hpp"
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
using namespace sin_compat::jetpack_events_phase590;

struct Mock final:Host {
 std::vector<std::string> calls;
 bool filter_result=false;
 void native_reset(State& state,uint32_t n) override {
  assert(n==0);calls.push_back("reset:"+std::to_string(state.raw_fec));
 }
 void native_timed_action(float time,uint32_t mode) override {
  assert(mode==0);assert(time==0.05f);calls.push_back("time:0.05,0");
 }
 void native_jetpack_a5_handler() override {calls.push_back("jetpack:A5");}
 void original_debug_event(uint32_t event) override {calls.push_back("debug:"+std::to_string(event));}
 bool native_event_filter(uint32_t id) override {calls.push_back("filter:"+std::to_string(id));return filter_result;}
 void native_parent_grunt_event(uint32_t id) override {calls.push_back("parent:"+std::to_string(id));}
};
static void test_exact_original_142_entry_classification(){
 int special=0,base=0;
 for(uint32_t id=kNativeFirstEvent;id<=kNativeLastEvent;id++){
  if(classify(id)==DispatchClass::OriginalFallback)base++;else special++;
 }
 assert(kNativeOriginalTableEntries==0xA6-0x19+1);
 assert(base==133&&special==9&&base==kNativeDefaultInRangeEntries);
 assert(classify(0x18)==DispatchClass::OriginalFallback);
 assert(classify(0xA7)==DispatchClass::OriginalFallback);
 assert(classify(0x54)==DispatchClass::OriginalFallback);
 assert(classify(0x38)==classify(0x3C));
 assert(classify(0x19)==DispatchClass::JetpackNavigation19);
 assert(classify(0x9F)==DispatchClass::JetpackState9F);
 assert(classify(0xA0)==DispatchClass::JetpackStateA0);
 assert(classify(0xA3)==DispatchClass::JetpackDebugA3);
 assert(classify(0xA4)==DispatchClass::JetpackTimerA4);
 assert(classify(0xA5)==DispatchClass::JetpackResetA5);
 assert(classify(0xA6)==DispatchClass::JetpackTaskA6);
}
static void test_original_state_updates_before_reset(){
 State s;Mock h;
 assert(dispatch(s,0x9F,h)==Status::ModeledNativeBranch);
 assert(s.raw_fec==0 && h.calls==std::vector<std::string>{"reset:0"});
 h.calls.clear();
 assert(dispatch(s,0xA0,h)==Status::ModeledNativeBranch);
 assert(s.raw_fec==2&&h.calls==std::vector<std::string>{"reset:2"});
}
static void test_original_timer_and_debug(){
 State s;Mock h;
 assert(dispatch(s,0xA4,h)==Status::ModeledNativeBranch);
 assert((h.calls==std::vector<std::string>{"time:0.05,0"}));
 h.calls.clear();
 assert(dispatch(s,0xA3,h,false)==Status::ModeledNativeBranch);
 assert(h.calls.empty());
 assert(dispatch(s,0xA3,h,true)==Status::ModeledNativeBranch);
 assert((h.calls==std::vector<std::string>{"debug:163"}));
}
static void test_original_A5_call_order(){
 State s;Mock h;
 assert(dispatch(s,0xA5,h)==Status::ModeledNativeBranch);
 assert((h.calls==std::vector<std::string>{"jetpack:A5","reset:4294967295"}));
}
static void test_fallback_and_original_base_dispatch(){
 State s;Mock h;
 assert(dispatch(s,0x54,h)==Status::DelegatedToBase);
 assert((h.calls==std::vector<std::string>{"filter:84","parent:84"}));
 h.calls.clear();h.filter_result=true;
 assert(dispatch(s,0xA7,h)==Status::ConsumedByOriginalFilter);
 assert((h.calls==std::vector<std::string>{"filter:167"}));
 h.calls.clear();h.filter_result=false;
 assert(dispatch(s,0x18,h)==Status::DelegatedToBase);
 assert((h.calls==std::vector<std::string>{"filter:24","parent:24"}));
}
static void test_opaque_native_branches_remain_explicit(){
 State s;Mock h;
 for(uint32_t id:{0x19u,0x38u,0x3Cu,0xA6u}){
  assert(dispatch(s,id,h)==Status::RequiresOriginalJetpackBranch);
  assert(h.calls.empty());
 }
}
int main(){
 test_exact_original_142_entry_classification();test_original_state_updates_before_reset();
 test_original_timer_and_debug();test_original_A5_call_order();
 test_fallback_and_original_base_dispatch();test_opaque_native_branches_remain_explicit();
}