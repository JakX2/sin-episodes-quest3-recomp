#pragma once
// Phase608: source-guided original Grunt Follow eligibility continuation.
// Native original: Follow vtable+0x18 -> 0x220D15D0, then +0xE4 -> 0x220D2370.
// The original re-reads an encoded handle twice; a changed/stale handle on
// the second lookup can lead to a null dereference. Our PORT model refuses it.
#include <cstdint>

namespace sin_compat::grunt_follow_eligibility_phase608 {

inline constexpr std::uint32_t kInvalidOriginalHandle = 0xffffffffu;
inline constexpr std::uint32_t kIndexMask = 0x0fffu;
inline constexpr std::uint32_t kDisqualifyingEntityFlag = 0x8000u;

class Host {
 public:
  virtual ~Host() = default;

  // Native outer eligibility method's owner virtual at byte offset +0x450.
  virtual bool owner_gate() = 0;

  // Native original behavior+0x14 is loaded separately before EACH resolve.
  virtual std::uint32_t read_encoded_handle() = 0;

  // Original entity-slot address: original table+4+16*(encoded&0xfff).
  // Serial comparison uses the 20 high bits. Return false on missing/stale
  // entity instead of reproducing native's unsafe second invalid path.
  virtual bool resolve_flags(std::uint32_t entity_index,
                             std::uint32_t serial,
                             std::uint32_t& entity_field_plus_bc) = 0;
};

inline bool eligible(Host& host) {
  if (!host.owner_gate()) return false;

  const std::uint32_t first = host.read_encoded_handle();
  if (first == kInvalidOriginalHandle) return false;
  std::uint32_t ignored_first_field = 0;
  if (!host.resolve_flags(first & kIndexMask, first >> 12,
                          ignored_first_field)) return false;

  const std::uint32_t second = host.read_encoded_handle();
  if (second == kInvalidOriginalHandle) return false;
  std::uint32_t flags = 0;
  if (!host.resolve_flags(second & kIndexMask, second >> 12, flags)) {
    return false;  // Explicit portable safety policy on native unsafe path.
  }
  return (flags & kDisqualifyingEntityFlag) == 0u;
}

}  // namespace sin_compat::grunt_follow_eligibility_phase608
