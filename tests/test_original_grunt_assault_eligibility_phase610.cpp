// Phase610 host-only tests: bounded finite-time Assault +0x18 native logic.
#include "sin_compat/original_grunt_assault_eligibility_phase610.hpp"
#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace a = sin_compat::grunt_assault_eligibility_phase610;
struct Host final : a::Host {
  std::vector<std::string> calls;
  bool gate=true,condition3b=false,owner154=false;
  bool present1=true,present2=true;
  bool resolve_first=true,resolve_second=true;
  int resolve_count=0;
  std::uint8_t primary=1u;
  bool owner_gate_0x450() override {calls.emplace_back("gate");return gate;}
  bool owner_has_condition_0x3b() override {
    calls.emplace_back("condition3b");return condition3b;
  }
  bool owner_virtual_0x154_nonnull() override {
    calls.emplace_back("owner154");return owner154;
  }
  bool serial_checked_entity_present(std::uint32_t index,
                                     std::uint32_t serial) override {
    calls.emplace_back("present");
    assert((index==0x123u && serial==0xabcdeu) ||
           (index==0x001u && serial==0x54321u));
    return index==0x123u?present1:present2;
  }
  bool resolve_primary_target_field_0x3b9(std::uint32_t handle,
                                           std::uint8_t& value) override {
    calls.emplace_back("resolve");
    assert(handle==0xabcde123u);
    ++resolve_count;
    value=primary;
    return resolve_count==1?resolve_first:resolve_second;
  }
  void signal_condition_0x18() override {calls.emplace_back("signal18");}
};
static a::State ready() {
  a::State s;
  s.handle_plus_0x14=0xabcde123u;
  s.handle_plus_0x18=0x54321001u;
  s.raw.raw_plus_0x1c=1u;
  s.raw.raw_plus_0x24=7u;
  s.raw.raw_plus_0x25=6u;
  s.deadline_plus_0x2c=2.f;
  return s;
}
static void test_early_gates_without_mutation() {
  {Host h;auto s=ready();h.gate=false;
    assert(!a::eligible(h,s,3.f));assert(h.calls.size()==1u);
    assert(s.raw.raw_plus_0x1c==1u);}
  {Host h;auto s=ready();h.condition3b=true;
    assert(!a::eligible(h,s,3.f));assert(h.calls.size()==2u);}
  {Host h;auto s=ready();s.deadline_plus_0x2c=3.01f;
    assert(!a::eligible(h,s,3.f));assert(h.calls.size()==2u);}
  {Host h;auto s=ready();s.raw.raw_plus_0x1c=0u;
    assert(!a::eligible(h,s,3.f));assert(h.calls.size()==2u);}
}
static void test_target_guards_reset_three_fields() {
  {Host h;auto s=ready();h.present1=false;
    assert(!a::eligible(h,s,3.f));assert(s.raw.raw_plus_0x1c==0u);
    assert(s.raw.raw_plus_0x24==0u && s.raw.raw_plus_0x25==0u);
    assert(h.resolve_count==0);}
  {Host h;auto s=ready();h.present2=false;
    assert(!a::eligible(h,s,3.f));assert(s.raw.raw_plus_0x1c==0u);
    assert(h.resolve_count==0);}
}
static void test_two_primary_handle_resolutions() {
  {Host h;auto s=ready();h.resolve_first=false;
    assert(a::eligible(h,s,3.f));assert(h.resolve_count==1);}
  {Host h;auto s=ready();h.resolve_first=false;s.byte_plus_0x26=1u;
    assert(!a::eligible(h,s,3.f));assert(s.byte_plus_0x26==1u);}
  {Host h;auto s=ready();h.primary=0u;
    assert(a::eligible(h,s,3.f));assert(h.resolve_count==2);}
  {Host h;auto s=ready();h.primary=0u;s.byte_plus_0x26=1u;
    assert(!a::eligible(h,s,3.f));assert(s.byte_plus_0x26==1u);}
  {Host h;auto s=ready();h.resolve_second=false;
    assert(!a::eligible(h,s,3.f));assert(h.resolve_count==2);}
}
static void test_exact_four_second_gate() {
  {Host h;auto s=ready();assert(a::eligible(h,s,3.999f));
    assert(s.byte_plus_0x26==0u);}
  {Host h;auto s=ready();assert(!a::eligible(h,s,4.f));
    assert(s.byte_plus_0x26==1u);}
  {Host h;auto s=ready();assert(!a::eligible(h,s,4.01f));
    assert(s.byte_plus_0x26==1u);}
}
static void test_condition_signalling_and_state_clearing() {
  {Host h;auto s=ready();s.byte_plus_0x26=1u;s.time_plus_0x28=1.f;
    assert(a::eligible(h,s,20.f));assert(s.byte_plus_0x26==0u);
    assert(h.calls.back()=="signal18");}
  {Host h;auto s=ready();s.byte_plus_0x26=1u;h.owner154=true;
    assert(a::eligible(h,s,20.f));assert(s.time_plus_0x28==20.f);
    assert(s.byte_plus_0x26==0u);assert(h.calls.back()=="signal18");}
  {Host h;auto s=ready();s.time_plus_0x28=2.f;
    assert(a::eligible(h,s,8.f));assert(h.calls.back()!="signal18");}
}
static void test_exact_deadline_and_nonfinite_refusal() {
  {Host h;auto s=ready();assert(a::eligible(h,s,2.f));}
  {Host h;auto s=ready();s.time_plus_0x28=
        std::numeric_limits<float>::quiet_NaN();
    assert(!a::eligible(h,s,5.f));assert(h.calls.empty());}
}
int main() {
  test_early_gates_without_mutation();
  test_target_guards_reset_three_fields();
  test_two_primary_handle_resolutions();
  test_exact_four_second_gate();
  test_condition_signalling_and_state_clearing();
  test_exact_deadline_and_nonfinite_refusal();
}
