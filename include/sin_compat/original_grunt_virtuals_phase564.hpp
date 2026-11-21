#pragma once
// Portable and source-bounded reconstruction of selected authentic 2006 SiN
// Grunt virtuals. Does NOT define the original 32-bit engine object, vtable,
// allocator or audio runtime. Raw offset labels are from the original DLL.
#include <cstdint>
#include <string_view>

namespace sin_compat::original_grunt_virtuals_phase564 {

constexpr std::uint32_t invalid_handle = 0xffffffffu;
constexpr std::string_view last_of_squad_token = "GRUNT_LAST_OF_SQUAD";
constexpr std::string_view man_down_token = "GRUNT_MAN_DOWN";

struct Grunt {
  std::uintptr_t group_0x9c4 = 0;
  std::int32_t selection_0x9cc = -1;
  std::uint32_t target_handle_0x8a0 = invalid_handle;
  std::uint8_t flag_0xb6 = 0;
  std::uint8_t flag_0xbf = 0;
};

struct Host {
  virtual ~Host() = default;
  // Engine generation-checked handle lookup (original +0x154). Host must
  // bounds-check its own handle table; 2006 x86 code has no such guard.
  virtual std::uintptr_t resolve_entity(std::uint32_t encoded_handle) = 0;
  // Native group +0xAAEB0's two signed bounds are both the selector passed
  // into Grunt +0x54D40. Crucially, `key` is an entity POINTER, not an int.
  virtual bool reserve_group(std::uintptr_t group, std::uintptr_t key,
                             std::int32_t first, std::int32_t last,
                             std::int32_t& selected) = 0;
  // Two original +0xAA920 bitmap lookups occur when release index != -1.
  virtual void bitmap_lookup(std::uintptr_t group, std::uintptr_t key) = 0;
  virtual bool bitmap_has(std::int32_t index) = 0;
  virtual void report_missing_bit() = 0;
  virtual void bitmap_clear(std::int32_t index) = 0;
  // Native group +0x9480 with filtering flag 1.
  virtual std::int32_t eligible_group_members(std::uintptr_t group) = 0;
  // Native Grunt CAI_Sentence +0xCC0 dispatch (Phase558); return ignored.
  virtual int dispatch_sentence(std::string_view token, int argument,
                                unsigned mode) = 0;
  // Original surviving NPC virtual +0x588; not contingent on queue result.
  virtual void owner_mark(std::int32_t argument, std::uint32_t mask) = 0;
  // Unrecovered engine generic method +0x22030520, tail-called by +0x678.
  virtual bool generic_member_callback(std::uintptr_t candidate,
                                       std::uintptr_t position,
                                       std::uintptr_t context) = 0;
};

// Original Grunt primary vtable +0x690 -> 0x22044670. The original writes
// field +0x9C4 and returns with ret 4. Unlike a generic notify callback,
// this is the actual pointer assignment that links the NPC to its group.
inline void set_group_22044670(Grunt& grunt, std::uintptr_t group) {
  grunt.group_0x9c4 = group;
}

// Original Grunt +0x694 -> 0x22044680, two-byte `xor al,al` and `ret`.
inline constexpr bool grunt_virtual_694() { return false; }

// Original +0x154 -> 0x22044840 resolves +0x8A0 using 12-bit index/serial.
inline std::uintptr_t selected_entity_22044840(const Grunt& grunt,
                                                 Host& host) {
  if (grunt.target_handle_0x8a0 == invalid_handle) return 0;
  return host.resolve_entity(grunt.target_handle_0x8a0);
}

// Original +0x22054C80 is a signed inclusive test, independent of group.
inline constexpr bool selected_in_range_22054c80(const Grunt& grunt,
                                                  std::int32_t first,
                                                  std::int32_t last) {
  return grunt.selection_0x9cc >= first && grunt.selection_0x9cc <= last;
}

// Original +0x22054D40 now has an authenticated virtual +0x154 target:
// the argument passed to the linked group's bitmap reservation is the
// serial-checked ENTITY POINTER held in the NPC's handle at +0x8A0.
inline bool admission_22054d40(Grunt& grunt, std::int32_t selector,
                                Host& host) {
  if (grunt.group_0x9c4 == 0) return true;
  const auto key = selected_entity_22044840(grunt, host);
  return host.reserve_group(grunt.group_0x9c4, key,
                            selector, selector, grunt.selection_0x9cc);
}

// Original group 0x220AAFF0: -1 means no work. Otherwise check one bit;
// report a diagnostic if absent; clear the bit anyway. The group bitmap
// accessor is invoked a second time even after first lookup/check.
inline void release_bit_220aaff0(std::uintptr_t group, std::uintptr_t key,
                                  std::int32_t selected, Host& host) {
  if (selected == -1) return;
  host.bitmap_lookup(group,key);
  if (!host.bitmap_has(selected)) host.report_missing_bit();
  host.bitmap_lookup(group,key);
  host.bitmap_clear(selected);
}

// Original 0x22054CA0: when group absent, it does NOT clear +0x9CC;
// when present, releases the slot for the current +0x154 entity and then
// resets the selection to -1. Host callbacks model native bitmap access.
inline void release_selection_22054ca0(Grunt& grunt, Host& host) {
  if (grunt.group_0x9c4 == 0) return;
  const auto key = selected_entity_22044840(grunt,host);
  release_bit_220aaff0(grunt.group_0x9c4,key,grunt.selection_0x9cc,host);
  grunt.selection_0x9cc = -1;
}

// Original Grunt +0x678: suppress if byte +0xB6 bit0 is set; otherwise
// tail-jump to original +0x22030520, here a delegate to an opaque host.
inline bool member_callback_22358590(const Grunt& grunt, Host& host,
                                      std::uintptr_t candidate,
                                      std::uintptr_t position,
                                      std::uintptr_t context) {
  if (grunt.flag_0xb6 & 1u) return false;
  return host.generic_member_callback(candidate,position,context);
}

// Original Grunt +0x750 -> 0x2235AF70, called by Phase562 leave with
// notification enabled on each surviving member. Exact identifiers come
// from native virtual getter +0x7B8/+0x7BC and original .rdata strings.
// Removed member's dword +0x9C is intentionally NOT named health.
inline void on_member_removed_2235af70(const Grunt& grunt,
                                        std::int32_t removed_raw_0x9c,
                                        Host& host) {
  if ((grunt.flag_0xbf & 8u) != 0u) return;
  if (removed_raw_0x9c > 0) return;
  // The original assumes a valid linked group and removed entity pointer.
  const auto eligible = host.eligible_group_members(grunt.group_0x9c4);
  if (eligible < 2) {
    (void)host.dispatch_sentence(last_of_squad_token, -1, 1u);
    host.owner_mark(0,0); // Called even if sentence queue fails.
  } else {
    (void)host.dispatch_sentence(man_down_token, 0, 2u);
  }
}

} // namespace sin_compat::original_grunt_virtuals_phase564