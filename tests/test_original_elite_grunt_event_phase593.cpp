#include "sin_compat/original_elite_grunt_event_phase593.hpp"
#include "sin_compat/original_grunt_event_parent_phase592.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
using namespace sin_compat::elite_grunt_event_phase593;
struct Mock final :Host {
 std::vector<std::string> called;
 bool precondition47=false,gate43=false,target03=false,positive120=false,rejected300=false;
 std::uint32_t resolved=0;float now=10.f;
 void inherited_grunt_event(const Event&e)override {called.push_back("parent");assert(e.id!=0x47u);}
 bool elite47_precondition_zero()override{called.push_back("pre47");return precondition47;}
 std::uint32_t elite47_resolve()override{called.push_back("resolve47");return 0x123u;}
 void elite47_apply(std::uint32_t id)override{assert(id==0x123u);called.push_back("apply47");}
 void original_timed_action(float time,unsigned mode)override{
  assert(mode==0 && (time==1.f || time==2.f));called.push_back(time==1.f?"timer1":"timer2");
 }
 std::uint32_t resolved_entity_or_zero(std::uint32_t handle)override{
  assert(handle!=0xffffffffu);called.push_back("resolve43");return resolved;
 }
 bool original_gate43(std::uint32_t entity)override{
  assert(entity==resolved||entity==0);called.push_back("gate43");return gate43;
 }
 bool original_event03_target()override{called.push_back("target03");return target03;}
 bool original_event03_target_virtual_120()override{called.push_back("positive120");return positive120;}
 bool original_event03_target_virtual_300_owner()override{called.push_back("negative300");return rejected300;}
 float original_engine_now()override{called.push_back("clock");return now;}
 void native_event03_voice_queue(int arg,unsigned mode)override{
  assert(arg==2&&mode==2);called.push_back("voice03");
 }
 void native_event03_numeric_request(unsigned request)override{
  assert(request==1);called.push_back("request1");
 }
 void native_reset(unsigned mode)override{assert(mode==0);called.push_back("reset0");}
};
int main(){
 static_assert(sin_compat::grunt_event_parent_phase592::original_case_index(0x54)==24);
 {
  Mock h;State s;
  assert(dispatch(s,{0x54,1.0f},h)==Status::ForwardedToInheritedGrunt);
  assert((h.called==std::vector<std::string>{"parent"}));
 }
 {
  Mock h;State s;
  assert(dispatch(s,{0x47,0},h)==Status::EliteEvent47Handled);
  assert((h.called==std::vector<std::string>{"pre47","timer1"}));
  h.called.clear();h.precondition47=true;
  assert(dispatch(s,{0x47,0},h)==Status::EliteEvent47Handled);
  assert((h.called==std::vector<std::string>{"pre47","resolve47","apply47","timer1"}));
 }
 {
  Mock h;State s;
  assert(dispatch(s,{0x43,1.f},h)==Status::EliteEvent43Blocked);
  assert(s.predicate_fd9==0);
  assert((h.called==std::vector<std::string>{"gate43"}));
  h.called.clear();h.gate43=true;
  assert(dispatch(s,{0x43,1.f},h)==Status::ForwardedToInheritedGrunt);
  assert(s.predicate_fd9==1);
  assert((h.called==std::vector<std::string>{"gate43","parent"}));
  h.called.clear();s.serial_handle_fdc=0x12300005u;h.resolved=0x15;
  assert(dispatch(s,{0x43,1.f},h)==Status::ForwardedToInheritedGrunt);
  assert((h.called==std::vector<std::string>{"resolve43","gate43","parent"}));
 }
 {
  Mock h;State s;
  assert(dispatch(s,{0x03,0.5f},h)==Status::EliteEvent03ParamNotOne);
  assert(h.called.empty());
  assert(dispatch(s,{0x03,1.8f},h)==Status::EliteEvent03Reset);
  assert((h.called==std::vector<std::string>{"target03","reset0"}));
  h.called.clear();h.target03=true;h.positive120=true;
  h.now=3.f;s.previous_voice_time_99c=0.f;
  assert(dispatch(s,{0x03,1.f},h)==Status::EliteEvent03Reset);
  assert(s.previous_voice_time_99c==0.f);
  assert((h.called==std::vector<std::string>{"target03","positive120","negative300","clock","reset0"}));
  h.called.clear();h.now=3.01f;
  assert(dispatch(s,{0x03,1.999f},h)==Status::EliteEvent03VoiceAction);
  assert(s.previous_voice_time_99c==3.01f);
  assert((h.called==std::vector<std::string>{"target03","positive120","negative300","clock","voice03","timer2","request1"}));
 }
 {
  Mock h;State s;
  assert(dispatch(s,{0x03,std::numeric_limits<float>::quiet_NaN()},h)==Status::UnsupportedOriginalX87);
  assert(h.called.empty());
  assert(dispatch(s,{0x03,2147483648.0f},h)==Status::UnsupportedOriginalX87);
  assert(h.called.empty());
 }
}