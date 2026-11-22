#pragma once
// Phase609, limited portable original Assault target prerequisite.
// Source: original retail 2006 Assault+0x18 method, calls to identical
// missing-target helpers on behavior+0x14 and behavior+0x18 handles.
// This does NOT recreate later Assault time/goal/schedule selection logic.
#include <cstdint>

namespace sin_compat::grunt_assault_handle_guards_phase609 {

inline constexpr std::uint32_t kInvalidHandle = 0xffffffffu;
inline constexpr std::uint32_t kOriginalIndexMask = 0x0fffu;

struct State {
  // Actual native raw behavior member offsets, symbolic names unproven.
  std::uint32_t raw_plus_0x1c = 0u;
  std::uint8_t raw_plus_0x24 = 0u;
  std::uint8_t raw_plus_0x25 = 0u;
};

class Host {
 public:
  virtual ~Host() = default;
  // Return true only when original 16-byte entity-table slot has matching
  // encoded serial and contains a non-null engine-owned entity pointer.
  virtual bool serial_checked_entity_present(std::uint32_t index,
                                             std::uint32_t serial) = 0;
};

inline bool missing_target(Host& host, std::uint32_t raw_handle) {
  if (raw_handle == kInvalidHandle) return true;
  return !host.serial_checked_entity_present(raw_handle & kOriginalIndexMask,
                                             raw_handle >> 12);
}

inline bool both_targets_present_or_reset(Host& host,
                                          std::uint32_t behavior_plus_0x14,
                                          std::uint32_t behavior_plus_0x18,
                                          State& state) {
  if (!missing_target(host, behavior_plus_0x14) &&
      !missing_target(host, behavior_plus_0x18)) {
    return true;
  }
  // Authentic original early exit at 0x220CEBE0:
  // native stores zero to +0x1C, +0x24 and +0x25, then returns AL=0.
  state.raw_plus_0x1c = 0u;
  state.raw_plus_0x24 = 0u;
  state.raw_plus_0x25 = 0u;
  return false;
}

}  // namespace sin_compat::grunt_assault_handle_guards_phase609
