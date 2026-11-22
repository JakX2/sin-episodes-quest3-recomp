#pragma once
// Phase610: source-guided original 2006 Assault eligibility state machine.
// Source: full 255-byte server.dll method 0x220CEAF0, SHA256
// 0c5b688341149ae7ed28a49f9f930c87116810c67f1107a444e48742ba570466.
// This model assumes finite game-tick timestamps and stable engine entity
// handles. It refuses exceptional x87 values or stale SECOND resolutions.
#include "sin_compat/original_grunt_assault_handle_guards_phase609.hpp"
#include <cmath>
#include <cstdint>

namespace sin_compat::grunt_assault_eligibility_phase610 {
namespace guard = sin_compat::grunt_assault_handle_guards_phase609;

struct State {
  std::uint32_t handle_plus_0x14 = guard::kInvalidHandle;
  std::uint32_t handle_plus_0x18 = guard::kInvalidHandle;
  guard::State raw{}; // original +0x1C dword, +0x24 and +0x25 bytes
  float time_plus_0x28 = 0.f;
  float deadline_plus_0x2c = 0.f;
  std::uint8_t byte_plus_0x26 = 0u;
};

class Host : public guard::Host {
 public:
  virtual bool owner_gate_0x450() = 0;
  virtual bool owner_has_condition_0x3b() = 0;
  virtual bool owner_virtual_0x154_nonnull() = 0;

  // Native calls two different resolver instructions on the SAME first
  // behavior+0x14 handle after validating both +0x14 and +0x18.
  // Second failure in retail can dereference null; safe port refuses it.
  virtual bool resolve_primary_target_field_0x3b9(std::uint32_t encoded_handle,
                                                   std::uint8_t& value) = 0;
  virtual void signal_condition_0x18() = 0;
};

// Return value is full native boolean for normal finite clock and stable
// entity state, subject to supplied engine-owned callback fidelity.
// An exceptional x87/NaN path is intentionally not modeled as valid gameplay.
inline bool eligible(Host& host, State& state, float now) {
  if (!std::isfinite(now) || !std::isfinite(state.deadline_plus_0x2c) ||
      !std::isfinite(state.time_plus_0x28)) return false;

  if (!host.owner_gate_0x450()) return false;
  if (host.owner_has_condition_0x3b()) return false;
  if (state.deadline_plus_0x2c > now) return false;
  if (state.raw.raw_plus_0x1c == 0u) return false;

  // Original Phase609 early-failure path resets only +0x1C, +0x24 and +0x25.
  if (!guard::both_targets_present_or_reset(host, state.handle_plus_0x14,
                                            state.handle_plus_0x18,
                                            state.raw)) return false;

  // The original checks owner virtual +0x154 and conditionally stores
  // the current global time in behavior float +0x28.
  if (host.owner_virtual_0x154_nonnull()) state.time_plus_0x28 = now;

  std::uint8_t ignored_first_field = 0;
  if (!host.resolve_primary_target_field_0x3b9(state.handle_plus_0x14,
                                                ignored_first_field)) {
    return state.byte_plus_0x26 == 0u;
  }

  std::uint8_t primary_field = 0;
  if (!host.resolve_primary_target_field_0x3b9(state.handle_plus_0x14,
                                                primary_field)) {
    return false; // safe departure from original possible null dereference
  }
  if (primary_field == 0u) return state.byte_plus_0x26 == 0u;

  // The original x87 compare first tests whether +0x28 equals 0.0f.
  // Only when zero does it compare now-(+0x28) to 4.0f. Exactly 4.0
  // fails, it is NOT part of the permitted less-than window.
  if (state.time_plus_0x28 == 0.f &&
      (now - state.time_plus_0x28) >= 4.f) {
    state.byte_plus_0x26 = 1u;
    return false;
  }

  if (state.byte_plus_0x26 != 0u) host.signal_condition_0x18();
  state.byte_plus_0x26 = 0u;
  return true;
}

} // namespace sin_compat::grunt_assault_eligibility_phase610
