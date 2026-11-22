// Phase606: host-only regression for source-backed MobGrunt first-eligible
// priority. Not an original DLL runtime or complete AI condition engine.
#include "sin_compat/original_grunt_behavior_priority_phase606.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace original = sin_compat::grunt_behavior_priority_phase606;

struct RecordingHost final : original::Host {
  std::array<bool, 8> can_select{};
  std::array<bool, 8> accept{};
  std::vector<std::string> events;
  int current = -1;
  bool current_was_present = false;

  RecordingHost() { accept.fill(true); }  // original MobGrunt +0x77C
  bool eligible(std::size_t i) override {
    events.push_back("eligible:" + std::to_string(i));
    return can_select.at(i);
  }
  bool owner_accepts(std::size_t i) override {
    events.push_back("accept:" + std::to_string(i));
    return accept.at(i);
  }
  void transition_to(std::size_t i) override {
    events.push_back("transition:" + std::to_string(i));
    current = static_cast<int>(i);
    current_was_present = true;
  }
  void clear_current_and_notify_if_needed() override {
    events.push_back(current_was_present ? "clear-and-notify" : "clear-empty");
    current = -1;
    current_was_present = false;
  }
};
static void test_original_registration_order() {
  assert(original::kOriginalRegistrationOrder.size() == 8);
  assert(original::kOriginalRegistrationOrder[0] == "CAI_RappelBehavior");
  assert(original::kOriginalRegistrationOrder[1] == "CAI_FuncTankBehavior");
  assert(original::kOriginalRegistrationOrder[2] == "CAI_TakecoverBehavior");
  assert(original::kOriginalRegistrationOrder[3] == "CAI_BeserkBehavior");
  assert(original::kOriginalRegistrationOrder[4] == "CAI_ActBusyBehavior");
  assert(original::kOriginalRegistrationOrder[5] == "CAI_AssaultBehavior");
  assert(original::kOriginalRegistrationOrder[6] == "CGruntStandoffBehavior");
  assert(original::kOriginalRegistrationOrder[7] == "CAI_FollowBehavior");
}
static void test_first_eligible_wins() {
  RecordingHost h;
  h.can_select[0] = true;
  h.can_select[3] = true;
  assert(original::choose_registered_behavior(h) ==
         original::SelectionResult::Selected);
  assert(h.current == 0);
  assert((h.events == std::vector<std::string>{
      "eligible:0", "accept:0", "transition:0"}));
}
static void test_registration_order_overrides_numeric_member_offsets() {
  RecordingHost h;
  h.can_select[2] = true;  // Takecover
  h.can_select[5] = true;  // Assault
  assert(original::choose_registered_behavior(h) ==
         original::SelectionResult::Selected);
  assert(h.current == 2);
  assert((h.events == std::vector<std::string>{
      "eligible:0", "eligible:1", "eligible:2", "accept:2", "transition:2"}));
}
static void test_no_eligible_clears_prior_behavior() {
  RecordingHost h;
  h.current = 6;
  h.current_was_present = true;
  assert(original::choose_registered_behavior(h) ==
         original::SelectionResult::NoEligibleBehavior);
  assert(h.current == -1);
  assert(h.events.size() == 9);
  for (std::size_t i = 0; i < 8; ++i) {
    assert(h.events[i] == "eligible:" + std::to_string(i));
  }
  assert(h.events.back() == "clear-and-notify");
}
static void test_no_prior_behavior_is_noop_cleanup() {
  RecordingHost h;
  assert(original::choose_registered_behavior(h) ==
         original::SelectionResult::NoEligibleBehavior);
  assert(h.events.back() == "clear-empty");
}
// This tests generic +0x77C override routing. Retail MobGrunt accepts
// unconditionally, but related behavior-host NPCs can supply another policy.
static void test_generic_host_rejection_continues_search() {
  RecordingHost h;
  h.can_select[0] = true;
  h.can_select[1] = true;
  h.accept[0] = false;
  assert(original::choose_registered_behavior(h) ==
         original::SelectionResult::Selected);
  assert(h.current == 1);
  assert((h.events == std::vector<std::string>{
      "eligible:0", "accept:0", "eligible:1", "accept:1", "transition:1"}));
}
int main() {
  test_original_registration_order();
  test_first_eligible_wins();
  test_registration_order_overrides_numeric_member_offsets();
  test_no_eligible_clears_prior_behavior();
  test_no_prior_behavior_is_noop_cleanup();
  test_generic_host_rejection_continues_search();
}
