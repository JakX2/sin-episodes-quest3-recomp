#pragma once
// Independent, original-free model for exact 2006 SiN server-side eligibility
// gates. NOT a compatible Source x86 class, vtable or substitute game DLL.
#include <cstdint>
#include <string_view>
#include <vector>

namespace sin_compat::original_grunt_combat_phase559 {

// A portable representation of the original entity handle's index/serial
// fields, with intentionally additional host bounds checking.
struct Record {
  std::int32_t identity = 0;
  std::int32_t raw_dword_0x9c = 0; // Meaning unknown; do not rename health.
  std::string_view classname;
  bool raw_dword_0x204_nonzero = false;
  bool embedded_virtual_0x2c_nonzero = false;
  bool entity_virtual_0x694_nonzero = false;
};
struct Slot { const Record* object = nullptr; std::uint32_t serial = 0; };
struct HandleTable {
  std::vector<Slot> slots;
  const Record* resolve(std::uint32_t handle) const {
    if (handle == 0xffffffffu) return nullptr;
    const std::size_t index = handle & 0xfffu;
    if (index >= slots.size()) return nullptr; // host-only guard
    const auto& slot = slots[index];
    if (slot.serial != (handle >> 12)) return nullptr;
    return slot.object;
  }
};
inline bool eligible_220a_shared(const Record* entity) {
  // Native path requires entity +0x204 !=0 OR an embedded +0xfc
  // component's virtual +0x2c, then rejects virtual +0x694 !=0.
  return entity && (entity->raw_dword_0x204_nonzero ||
                     entity->embedded_virtual_0x2c_nonzero) &&
         !entity->entity_virtual_0x694_nonzero;
}
struct LinkedGroup {
  std::vector<std::uint32_t> handles; // Native array begins at +0x08.
};

// Native 0x220A9480: argument byte ==0 -> raw group count; else the
// number of eligible entities. No removal or mutation is performed.
inline std::int32_t group_count_220a9480(const LinkedGroup& group,
                                           const HandleTable& table,
                                           bool filter) {
  if (!filter) return static_cast<std::int32_t>(group.handles.size());
  std::int32_t count = 0;
  for (auto handle : group.handles)
    if (eligible_220a_shared(table.resolve(handle))) ++count;
  return count;
}

// Native 0x220A9210 is subtle: it returns the *first* eligible entity,
// but ONLY if the group has STRICTLY MORE THAN ONE eligible entity.
// Earlier generic 'first-or-only' interpretations invert this branch.
inline const Record* group_first_when_multiple_220a9210(
    const LinkedGroup& group, const HandleTable& table) {
  const Record* first = nullptr;
  std::int32_t n = 0;
  for (auto handle : group.handles) {
    const Record* entity = table.resolve(handle);
    if (!eligible_220a_shared(entity)) continue;
    if (!first) first = entity;
    ++n;
  }
  return n > 1 ? first : nullptr;
}

// Native 0x220AA2D0: validate candidate, then require candidate ==
// the first eligible entity of a group with >1 eligible entries.
inline bool group_candidate_matches_220aa2d0(
    const LinkedGroup& group, const HandleTable& table,
    const Record* candidate) {
  if (!eligible_220a_shared(candidate)) return false;
  return group_first_when_multiple_220a9210(group, table) == candidate;
}

inline constexpr bool is_special_object_class(std::string_view name) {
  // The four original immediate string pointers and 0x220F5700's byte
  // comparison are case-sensitive. Do not apply modern class aliases.
  return name == "func_physbox" || name == "prop_physics" ||
         name == "func_breakable" || name == "func_door";
}

enum class PrefixKind { Continue, ReturnNumeric };
struct PrefixDecision {
  PrefixKind kind;
  std::int32_t raw_schedule;
};

struct IPrefixHost {
  virtual ~IPrefixHost() = default;
  virtual bool has_condition(int numeric_id) = 0;  // native 0x220397C0
  virtual const Record* resolve_grunt_handle_0x98c() = 0; // 0x22035250
  virtual bool external_gate_22054d40(int numeric_id) = 0;
  virtual std::uint32_t grunt_handle_0xcb4() = 0;
  virtual bool is_live_handle_0xcb4(std::uint32_t handle) = 0;
};

// Strictly bounded native *prefix*, from 0x2235A568 through 0x2235A6AF.
// After a Continue outcome, the remainder of the original 1327-byte method
// has NOT been simulated by this function. Never confuse Continue with a
// retail numeric schedule 0 or absence of later side effects.
inline PrefixDecision evaluate_original_combat_prefix(IPrefixHost& host) {
  // Original C++ short circuit: condition 0x0C is never queried if 0x2B.
  if (host.has_condition(0x2b) || host.has_condition(0x0c)) {
    const Record* selected = host.resolve_grunt_handle_0x98c();
    if (selected && selected->raw_dword_0x9c < 1000) {
      if (selected->raw_dword_0x9c > 0 &&
          host.external_gate_22054d40(0x0e))
        return {PrefixKind::ReturnNumeric, 0x70};
      if (is_special_object_class(selected->classname))
        return {PrefixKind::ReturnNumeric, 0x70};
    }
  }
  if (host.has_condition(0x45))
    return {PrefixKind::ReturnNumeric, 0x66};
  if (host.has_condition(0x16)) {
    const auto handle = host.grunt_handle_0xcb4();
    if (handle != 0xffffffffu && host.is_live_handle_0xcb4(handle))
      return {PrefixKind::ReturnNumeric, 0x72};
    return {PrefixKind::ReturnNumeric, 0x27};
  }
  return {PrefixKind::Continue, 0};
}

} // namespace sin_compat::original_grunt_combat_phase559