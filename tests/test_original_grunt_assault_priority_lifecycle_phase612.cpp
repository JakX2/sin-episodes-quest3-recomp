// Phase612: end-to-end original-free Grunt Assault host lifecycle regression.
// Combines the separately source-verified Phase606 priority selector,
// Phase610 Assault finite-time eligibility and Phase611 Assault +0x20 cleanup.
// This is NOT a live engine-hosted NPC or a complete game server.dll.
#include "sin_compat/original_grunt_behavior_priority_phase606.hpp"
#include "sin_compat/original_grunt_assault_eligibility_phase610.hpp"
#include "sin_compat/original_grunt_assault_deactivate_phase611.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace selection=sin_compat::grunt_behavior_priority_phase606;
namespace eligibility=sin_compat::grunt_assault_eligibility_phase610;
namespace cleanup=sin_compat::grunt_assault_deactivate_phase611;

struct Host final : selection::Host, eligibility::Host, cleanup::Host {
  eligibility::State assault{};
  bool rappel=false,owner_gate=true,condition3b=false,owner154=false;
  std::uint8_t primary_byte=1u;
  float now=10.f;
  int current=-1,notification_count=0,cleanup_count=0,link_writes=0;
  std::uint32_t link_encoded=0x22222009u;
  std::vector<std::string> calls;

  Host() {
    assault.handle_plus_0x14=0xabcde123u;
    assault.handle_plus_0x18=0x54321001u;
    assault.raw.raw_plus_0x1c=1u;
    assault.raw.raw_plus_0x24=6u;
    assault.raw.raw_plus_0x25=7u;
    assault.time_plus_0x28=1.f;
    assault.deadline_plus_0x2c=1.f;
  }

  // The original registry contains eight behavior objects in Phase535 order.
  // Only Rappel and Assault are enabled in these focused host-only scenarios.
  bool eligible(std::size_t i) override {
    calls.push_back("eligible"+std::to_string(i));
    if(i==0)return rappel;
    if(i==5)return eligibility::eligible(*this,assault,now);
    return false;
  }
  // Original CNPC_MobGrunt owner virtual+0x77C returns true.
  bool owner_accepts(std::size_t) override {
    calls.push_back("accept");return true;
  }

  void retire_old(int old) {
    if(old!=5)return;
    calls.push_back("old20");
    ++cleanup_count;
    cleanup::State state{assault.handle_plus_0x18,
                         assault.raw.raw_plus_0x24,
                         assault.raw.raw_plus_0x25};
    cleanup::deactivate(*this,state,9u);
    assault.raw.raw_plus_0x24=state.byte_plus_0x24;
    assault.raw.raw_plus_0x25=state.byte_plus_0x25;
  }
  // The original Phase604 transition first sets the new active pointer.
  // When changed, it invokes new+0x1C/+0x24 then old+0x20 then notifies.
  void transition_to(std::size_t i) override {
    int next=static_cast<int>(i);
    if(current==next)return;
    int old=current;
    current=next;
    calls.push_back("new1c");calls.push_back("new24");
    retire_old(old);
    calls.push_back("notify");++notification_count;
  }
  void clear_current_and_notify_if_needed() override {
    if(current<0)return;
    int old=current;current=-1;
    retire_old(old);
    calls.push_back("notify");++notification_count;
  }

  bool owner_gate_0x450() override {return owner_gate;}
  bool owner_has_condition_0x3b() override {return condition3b;}
  bool owner_virtual_0x154_nonnull() override {return owner154;}
  bool serial_checked_entity_present(std::uint32_t index,
                                     std::uint32_t serial) override {
    return (index==0x123u&&serial==0xabcdeu)||
           (index==0x001u&&serial==0x54321u);
  }
  bool resolve_primary_target_field_0x3b9(std::uint32_t handle,
                                           std::uint8_t& out) override {
    assert(handle==0xabcde123u);
    out=primary_byte;return true;
  }
  void signal_condition_0x18() override {calls.push_back("condition18");}

  // Opaque engine-managed entity tokens, no real entity memory access.
  bool resolve_entity(std::uint32_t handle,std::uint64_t& token) override {
    if(handle==0x54321001u){token=12u;return true;}
    if(handle==0x22222009u){token=9u;return true;}
    if(handle==0x33333008u){token=8u;return true;}
    return false;
  }
  std::uint32_t read_link_handle_plus_0x3bc(std::uint64_t target) override {
    assert(target==12u);return link_encoded;
  }
  void write_link_handle_plus_0x3bc(std::uint64_t target,
                                    std::uint32_t value) override {
    assert(target==12u&&value==cleanup::kInvalidHandle);
    ++link_writes;link_encoded=value;
  }
};

static void test_assault_selected_and_reselected_without_callbacks() {
  Host h;
  assert(selection::choose_registered_behavior(h)==
         selection::SelectionResult::Selected);
  assert(h.current==5&&h.notification_count==1&&h.cleanup_count==0);
  h.calls.clear();
  assert(selection::choose_registered_behavior(h)==
         selection::SelectionResult::Selected);
  assert(h.current==5&&h.notification_count==1&&h.cleanup_count==0);
  for(const auto& event:h.calls) {
    assert(event!="new1c"&&event!="new24"&&
           event!="old20"&&event!="notify");
  }
}
static void test_higher_priority_rappel_replaces_and_unlinks_assault() {
  Host h;
  assert(selection::choose_registered_behavior(h)==
         selection::SelectionResult::Selected);
  h.rappel=true;
  h.calls.clear();
  assert(selection::choose_registered_behavior(h)==
         selection::SelectionResult::Selected);
  assert(h.current==0&&h.cleanup_count==1&&h.notification_count==2);
  assert(h.assault.raw.raw_plus_0x24==0u&&
         h.assault.raw.raw_plus_0x25==0u);
  assert(h.link_writes==1&&h.link_encoded==cleanup::kInvalidHandle);
  std::size_t n=h.calls.size();
  assert(n>=5);
  assert(h.calls[n-4]=="new1c"&&h.calls[n-3]=="new24"&&
         h.calls[n-2]=="old20"&&h.calls[n-1]=="notify");
}
static void test_other_owner_link_survives_behavior_transition() {
  Host h;h.link_encoded=0x33333008u;
  selection::choose_registered_behavior(h);
  h.rappel=true;
  selection::choose_registered_behavior(h);
  assert(h.cleanup_count==1&&h.link_writes==0&&
         h.link_encoded==0x33333008u);
}
static void test_no_eligible_behavior_deactivates_old_assault() {
  Host h;
  selection::choose_registered_behavior(h);
  h.owner_gate=false;
  h.calls.clear();
  assert(selection::choose_registered_behavior(h)==
         selection::SelectionResult::NoEligibleBehavior);
  assert(h.current==-1&&h.cleanup_count==1&&h.notification_count==2);
  assert(h.link_writes==1);
  assert(h.calls[h.calls.size()-2]=="old20"&&h.calls.back()=="notify");
}
static void test_exact_four_second_threshold_can_prevent_selection() {
  Host h;
  h.now=4.f;
  h.assault.time_plus_0x28=0.f;
  assert(selection::choose_registered_behavior(h)==
         selection::SelectionResult::NoEligibleBehavior);
  assert(h.assault.byte_plus_0x26==1u);
  assert(h.current==-1&&h.cleanup_count==0);
}
int main() {
  test_assault_selected_and_reselected_without_callbacks();
  test_higher_priority_rappel_replaces_and_unlinks_assault();
  test_other_owner_link_survives_behavior_transition();
  test_no_eligible_behavior_deactivates_old_assault();
  test_exact_four_second_threshold_can_prevent_selection();
}
