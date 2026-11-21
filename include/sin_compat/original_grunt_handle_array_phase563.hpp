#pragma once
// Independent original-free host model of selected original 2006 x86 group
// array methods. This is NOT an original engine class or C++ ABI structure.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace sin_compat::original_grunt_handle_array_phase563 {

constexpr std::uint32_t invalid_handle = 0xFFFFFFFFu;
constexpr std::size_t capacity = 16; // From the original caller, NOT append itself.

struct IHandleHost {
  virtual ~IHandleHost() = default;
  // Simulates original serial-checked handle -> active entity pointer.
  // Zero represents the original null result. Valid handles can alias.
  virtual std::uintptr_t resolve(std::uint32_t handle) const = 0;
  virtual void or_member_flag(std::uintptr_t entity, std::uint32_t mask) = 0;
};

struct FixedGroupHandles {
  std::array<std::uint32_t, capacity> slots{};
  std::size_t count = 0;
};

// Original 0x220A8610 does NOT compare native encoded handles directly:
// it compares resolved entity pointer identity (possibly null) to candidate.
inline bool points_to_other(const IHandleHost& host, std::uint32_t handle,
                            std::uintptr_t candidate) {
  return host.resolve(handle) != candidate;
}

// Original 0x220A9790: inserts at the *end*, returns the former count.
// No duplicate suppression or overflow/capacity test is present in the x86
// function. This host adds a capacity exception to avoid arbitrary writes.
inline std::size_t append(FixedGroupHandles& list, std::uint32_t handle) {
  if (list.count >= list.slots.size()) {
    throw std::length_error("independent host fixed group is full");
  }
  const std::size_t index = list.count;
  list.slots[index] = handle;
  ++list.count;
  return index;
}

// Original 0x220A97E0: resolves the argument and each member with
// generation-checked engine handle resolution. Returns first matching
// *pointer*, including the case where both sides resolve to nullptr.
inline int find_by_resolved_identity(const FixedGroupHandles& list,
                                     std::uint32_t candidate,
                                     const IHandleHost& host) {
  if (list.count > list.slots.size()) {
    throw std::logic_error("host group count exceeds storage");
  }
  const std::uintptr_t target = host.resolve(candidate);
  for (std::size_t i = 0; i < list.count; ++i) {
    if (host.resolve(list.slots[i]) == target) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// Original 0x220A8670: shifts later 32-bit handles left, decrements count;
// no NPC callback, owner flag clearing or virtual dispatch in the helper.
// Native code does not itself bounds-check index; host rejects invalid use.
inline void erase_ordered(FixedGroupHandles& list, std::size_t index) {
  if (list.count > list.slots.size() || index >= list.count) {
    throw std::out_of_range("invalid independent group deletion index");
  }
  for (std::size_t i = index + 1; i < list.count; ++i) {
    list.slots[i - 1] = list.slots[i];
  }
  --list.count;
  // Original does not clear the spare trailing slot; do not do so here.
}

// Original 0x220A9720: broadcast an OR mask to owner fields +0x988 for
// members whose serial-checked encoded handles resolve to live entities.
// An entity may receive multiple OR callbacks when duplicate members exist.
inline void broadcast_or_flags(const FixedGroupHandles& list,
                               std::uint32_t mask, IHandleHost& host) {
  if (list.count > list.slots.size()) {
    throw std::logic_error("host group count exceeds storage");
  }
  for (std::size_t i = 0; i < list.count; ++i) {
    const std::uintptr_t entity = host.resolve(list.slots[i]);
    if (entity != 0) {
      host.or_member_flag(entity, mask);
    }
  }
}

} // namespace sin_compat::original_grunt_handle_array_phase563