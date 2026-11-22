#pragma once
// Phase606: source-guided, original-free Grunt behavior candidate loop.
// Native source evidence: 2006 server.dll function 0x2234E590.
// This is a PORT REFERENCE for the selection skeleton only. The Host owns
// lifecycle transitions, engine integration, and the real predicate methods.
#include <array>
#include <cstddef>
#include <string_view>

namespace sin_compat::grunt_behavior_priority_phase606 {

inline constexpr std::array<std::string_view, 8> kOriginalRegistrationOrder = {
    "CAI_RappelBehavior",
    "CAI_FuncTankBehavior",
    "CAI_TakecoverBehavior",
    "CAI_BeserkBehavior",
    "CAI_ActBusyBehavior",
    "CAI_AssaultBehavior",
    "CGruntStandoffBehavior",
    "CAI_FollowBehavior",
};

enum class SelectionResult { Selected, NoEligibleBehavior };

class Host {
 public:
  virtual ~Host() = default;

  // Original vtable +0x18 on each embedded behavior, interpreted here only
  // as a candidate-eligibility boolean. Real implementation is still needed.
  virtual bool eligible(std::size_t registration_index) = 0;

  // Original owner virtual +0x77C. The actual MobGrunt entry 0x2216DDE0
  // unconditionally returns true, but retain a hook for derived NPCs.
  virtual bool owner_accepts(std::size_t registration_index) = 0;

  // Original helper invokes 0x2216E510(newBehavior), which owns old/new
  // lifecycle callbacks and changes owner+0xC18.
  virtual void transition_to(std::size_t registration_index) = 0;

  // No winner: original 0x2234E590 sets owner+0xC18 to null, performs prior
  // behavior virtual +0x20 cleanup and owner virtual +0x778 notification
  // only if old current was nonnull.
  virtual void clear_current_and_notify_if_needed() = 0;
};

inline SelectionResult choose_registered_behavior(Host& host) {
  for (std::size_t i = 0; i < kOriginalRegistrationOrder.size(); ++i) {
    if (host.eligible(i) && host.owner_accepts(i)) {
      host.transition_to(i);
      return SelectionResult::Selected;
    }
  }
  host.clear_current_and_notify_if_needed();
  return SelectionResult::NoEligibleBehavior;
}

}  // namespace sin_compat::grunt_behavior_priority_phase606
