#pragma once
// Clean-room BEHAVIORAL reference ONLY. Not decompiled Ritual C++.
// Models the verified order of valid animation request -> model bind -> script offset -> sequence.
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace sin_phase51 {
struct WeaponConfiguration {
  std::string viewModelName;              // stand-in for weapon-info string at +0xA6
  std::array<float, 3> viewmodelOriginOffset; // parsed data at weapon-info +0x6CC
};
struct Weapon { const WeaponConfiguration* info = nullptr; int viewmodelIndex = 0; };
struct Viewmodel {
  const Weapon* boundWeapon = nullptr;
  std::string modelName;
  std::array<float,3> originOffset{{0,0,0}};
  unsigned modelChanges = 0;
  unsigned offsetChanges = 0;
  unsigned sentSequences = 0;
  int currentSequence = -1;
};
struct Player {
  bool valid = true;
  std::vector<Viewmodel*> viewmodels;
};
struct Event {
  enum Type { BindModel, CopyOrigin, SendSequence } type;
};
struct DispatchLog {
  std::vector<Event> events;
  unsigned resolvedActivities = 0;
  unsigned rejectedActivities = 0;
};
inline bool componentDifferent(float a, float b) {
  // x87 unordered floating-point comparisons are considered different. The reference tests
  // use finite values; NaN behavior is a conservative explicit modeling choice.
  return !(a == b);
}
inline bool originDifferent(const std::array<float,3>& a, const std::array<float,3>& b) {
  for (size_t i=0;i<3;i++) if(componentDifferent(a[i],b[i])) return true;
  return false;
}
inline Viewmodel* GetViewmodel(const Weapon& weapon, Player* owner) {
  if(!owner || !owner->valid || weapon.viewmodelIndex<0) return nullptr;
  const size_t i=static_cast<size_t>(weapon.viewmodelIndex);
  return i<owner->viewmodels.size() ? owner->viewmodels[i] : nullptr;
}
inline bool RequestActivitySequence(const Weapon& weapon, Player* owner,
                                    int activity, int sequence, DispatchLog& log) {
  (void)activity;
  // Original 0x222afc10 tests for sequence == -1 before reaching weapon slot 198.
  if(sequence == -1) { ++log.rejectedActivities; return false; }
  ++log.resolvedActivities;
  // Original slot 198 skips invalid (negative) sequence ids and missing owner/viewmodel.
  if(sequence < 0) return true; // not proof that the original lookup returns other negatives
  Viewmodel* vm=GetViewmodel(weapon,owner);
  if(!vm || !weapon.info) return true;
  // slot198 -> slot199: bind weapon and model (viewmodel slot185).
  vm->boundWeapon=&weapon;
  if(vm->modelName != weapon.info->viewModelName) {
    vm->modelName=weapon.info->viewModelName;
    ++vm->modelChanges;
  }
  log.events.push_back({Event::BindModel});
  // Then change-aware script offset update (viewmodel slot186).
  if(originDifferent(vm->originOffset,weapon.info->viewmodelOriginOffset)) {
    vm->originOffset=weapon.info->viewmodelOriginOffset;
    ++vm->offsetChanges;
  }
  log.events.push_back({Event::CopyOrigin});
  // Finally slot198 sends the selected sequence through viewmodel slot184.
  vm->currentSequence=sequence;
  ++vm->sentSequences;
  log.events.push_back({Event::SendSequence});
  return true;
}
} // namespace sin_phase51
