// Phase629: synthetic host checks exact original event43 0->1->2->3->0
// weapon-state transitions and separate 2006 global-clock reads.
// No Source engine objects or proprietary game code are linked.
#include "sin_compat/original_elite_event43_chaingun_gate_phase629.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>
namespace gate=sin_compat::original_elite_event43_chaingun_gate_phase629;

class Host final:public gate::IHost {
public:
  std::vector<float> times;
  std::vector<std::string> calls;
  std::size_t index=0;
  std::size_t fail_time_at=999;
  int fail_command=0;
  bool throw_command=false;
  gate::Reference* reenter=nullptr;
  gate::State* other_state=nullptr;
  gate::Code reenter_code=gate::Code::Allowed;

  bool native_virtual_3c4(std::uint32_t cmd,std::uint32_t zero)override {
    assert(zero==0u && (cmd==13u || cmd==14u));
    calls.push_back(cmd==13u?"3c4-13":"3c4-14");
    if(throw_command)throw std::runtime_error("synthetic host throw");
    if(reenter&&other_state)
      reenter_code=reenter->gate(*other_state,true).code;
    return fail_command!=static_cast<int>(cmd);
  }
  bool native_virtual_3c8(std::uint32_t cmd)override {
    assert(cmd==13u);
    calls.push_back("3c8-13");
    if(throw_command)throw std::runtime_error("synthetic host throw");
    return fail_command!=113;
  }
  bool read_original_engine_time(float& out)override {
    calls.push_back("time");
    if(index==fail_time_at)return false;
    assert(index<times.size());
    out=times[index++];
    return true;
  }
  void reset_times(std::initializer_list<float> samples) {
    times.assign(samples.begin(),samples.end());
    index=0;
    calls.clear();
  }
};
void test_full_cycle_and_exact_gate() {
  Host h;gate::Reference model(h);
  gate::State state{};
  h.reset_times({0.125f,0.375f});
  auto r=model.gate(state,true);
  assert(r.code==gate::Code::Blocked);
  assert(state.original_phase_594==1u);
  assert(state.original_time_590==0.125f);
  assert(state.original_time_58c==0.375f);
  assert((h.calls==std::vector<std::string>{"3c4-13","time","time"}));

  h.reset_times({1.124f});
  assert(model.update(state,true).code==gate::Code::Unchanged);
  assert(state.original_phase_594==1u);
  h.reset_times({1.125f,1.300f,1.500f});
  assert(model.update(state,true).code==gate::Code::Advanced);
  assert(state.original_phase_594==2u);
  assert(state.original_time_590==1.300f);
  assert(state.original_time_58c==1.500f);
  assert((h.calls==std::vector<std::string>{"time","time","time"}));
  h.calls.clear();
  assert(model.gate(state,true).code==gate::Code::Allowed);
  assert(h.calls.empty());

  h.reset_times({2.499f});
  assert(model.update(state,true).code==gate::Code::Unchanged);
  h.reset_times({2.500f});
  assert(model.update(state,true).code==gate::Code::Advanced);
  assert(state.original_phase_594==3u);
  h.calls.clear();
  assert(model.gate(state,true).code==gate::Code::Blocked);
  assert(h.calls.empty());

  h.calls.clear();
  assert(model.update(state,true).code==gate::Code::Advanced);
  assert(state.original_phase_594==0u);
  assert((h.calls==std::vector<std::string>{"3c8-13","3c4-14"}));
}
void test_other_states_never_call_host() {
  Host h;gate::Reference model(h);
  gate::State state{};
  for(std::uint32_t phase:{1u,3u,4u,0xffffffffu,300u}) {
    state.original_phase_594=phase;
    assert(model.gate(state,true).code==gate::Code::Blocked);
    assert(state.original_phase_594==phase);
    assert(h.calls.empty());
  }
  for(std::uint32_t phase:{0u,4u,0xffffffffu,300u}) {
    state.original_phase_594=phase;
    assert(model.update(state,true).code==gate::Code::Unchanged);
    assert(state.original_phase_594==phase);
    assert(h.calls.empty());
  }
  // The original reads [ECX+0x594] unconditionally: a null pointer is NOT
  // a valid native target. This model refuses rather than dereferencing.
  state.original_phase_594=2u;
  assert(model.gate(state,false).code==gate::Code::RejectedNullTarget);
  assert(model.update(state,false).code==gate::Code::RejectedNullTarget);
  assert(h.calls.empty());
}
void test_timed_x87_unordered_and_boundary() {
  Host h;gate::Reference model(h);
  gate::State s{};
  s.original_phase_594=1u;
  s.original_time_590=2.f;
  h.reset_times({std::numeric_limits<float>::quiet_NaN()});
  assert(model.update(s,true).code==gate::Code::Unchanged);
  h.reset_times({std::numeric_limits<float>::infinity(),3.f,3.f});
  assert(model.update(s,true).code==gate::Code::Advanced);
  assert(s.original_phase_594==2u);
  h.reset_times({std::numeric_limits<float>::quiet_NaN()});
  assert(model.update(s,true).code==gate::Code::Unchanged);
}
void test_rejection_stage_and_partial_mutation() {
  {
    Host h;gate::Reference model(h);gate::State s{};
    h.fail_command=13;
    assert(model.gate(s,true).failed_stage==
           gate::Stage::GateVirtual3C4Command13);
    assert(s.original_phase_594==0u);
  }
  {
    Host h;gate::Reference model(h);gate::State s{};
    h.reset_times({1.f,2.f});h.fail_time_at=0;
    auto r=model.gate(s,true);
    assert(r.code==gate::Code::HostRejected &&
           r.failed_stage==gate::Stage::GateTime590);
    assert(s.original_phase_594==1u);
  }
  {
    Host h;gate::Reference model(h);gate::State s{};
    h.reset_times({1.f,2.f});h.fail_time_at=1;
    auto r=model.gate(s,true);
    assert(r.code==gate::Code::HostRejected &&
           r.failed_stage==gate::Stage::GateTime58C);
    assert(s.original_phase_594==1u&&s.original_time_590==1.f);
  }
  {
    Host h;gate::Reference model(h);gate::State s{};
    s.original_phase_594=1;s.original_time_590=0.f;
    h.reset_times({1.f,1.1f,1.2f});h.fail_time_at=0;
    auto r=model.update(s,true);
    assert(r.code==gate::Code::HostRejected &&
           r.failed_stage==gate::Stage::UpdateReadTime);
    assert(s.original_phase_594==1u);
    h.fail_time_at=1;
    r=model.update(s,true);
    assert(r.code==gate::Code::HostRejected &&
           r.failed_stage==gate::Stage::UpdateTime590);
    assert(s.original_phase_594==2u);
  }
  {
    Host h;gate::Reference model(h);gate::State s{};
    s.original_phase_594=1;s.original_time_590=0.f;
    h.reset_times({1.f,1.1f,1.2f});h.fail_time_at=2;
    auto r=model.update(s,true);
    assert(r.code==gate::Code::HostRejected &&
           r.failed_stage==gate::Stage::UpdateTime58C);
    assert(s.original_phase_594==2u&&s.original_time_590==1.1f);
  }
  {
    Host h;gate::Reference model(h);gate::State s{};
    s.original_phase_594=3;h.fail_command=113;
    auto r=model.update(s,true);
    assert(r.failed_stage==gate::Stage::UpdateVirtual3C8Command13);
    assert(s.original_phase_594==3u);
    h.fail_command=14;
    r=model.update(s,true);
    assert(r.failed_stage==gate::Stage::UpdateVirtual3C4Command14);
    assert(s.original_phase_594==3u);
    assert(h.calls.back()=="3c4-14");
  }
  {
    Host h;gate::Reference model(h);gate::State s{};
    h.throw_command=true;
    assert(model.gate(s,true).code==gate::Code::HostRejected);
  }
}
void test_reentrance() {
  Host h;gate::Reference model(h);gate::State s{};
  h.reenter=&model;h.other_state=&s;
  h.reset_times({4.f,4.f});
  assert(model.gate(s,true).code==gate::Code::Blocked);
  assert(h.reenter_code==gate::Code::Reentrant);
}
int main() {
  test_full_cycle_and_exact_gate();
  test_other_states_never_call_host();
  test_timed_x87_unordered_and_boundary();
  test_rejection_stage_and_partial_mutation();
  test_reentrance();
  std::puts("PASS Phase629 authenticated original chaingun event43 gate and 1-second state-update synthetic host");
}
