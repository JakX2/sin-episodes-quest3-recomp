#pragma once
// Independent source-bounded C++17 reference for 2006 SiN Grunt events.
// This does not reproduce engine object layout, vtables or animation assets.
#include <cstdint>

namespace sin_compat::grunt_event_parent_phase592 {
inline constexpr std::uint32_t first_native_id = 3u;
inline constexpr std::uint32_t last_native_id = 0x9eu;
inline constexpr std::uint32_t native_table_entries = 156u;
inline constexpr std::uint32_t native_specific_ids = 26u;
inline constexpr std::uint32_t native_default_in_range = 130u;
inline constexpr float original_prefix_float = 0.75f;

struct Event {
  std::uint32_t numeric_id = 0;
  float native_float_parameter = 0.f;
};

// Original 25-case pointer table has case 24 as the inherited fallback.
// These IDs were verified against all 156 original selector bytes.
constexpr std::uint8_t original_case_index(std::uint32_t id) noexcept {
  switch(id) {
    case 0x03:return 0;
    case 0x2b:case 0x2c:return 1;
    case 0x38:case 0x3c:return 2;
    case 0x43:return 3;
    case 0x62:return 4;
    case 0x8b:return 5;
    case 0x8c:return 6;
    case 0x8d:return 7;
    case 0x8e:return 8;
    case 0x8f:return 9;
    case 0x90:return 10;
    case 0x92:return 11;
    case 0x93:return 12;
    case 0x94:return 13;
    case 0x95:return 14;
    case 0x96:return 15;
    case 0x97:return 16;
    case 0x98:return 17;
    case 0x99:return 18;
    case 0x9a:return 19;
    case 0x9b:return 20;
    case 0x9c:return 21;
    case 0x9d:return 22;
    case 0x9e:return 23;
    default:return 24;
  }
}

// In original 0x2202D930 the component's marker at +0x08 is initialized
// to 1, then the component's virtual+0x60 is invoked. The component's
// RETURN VALUE does not determine forwarding: only marker contents do.
enum class Result {
  RequiresOriginalDedicatedHandler,
  HandledSpecial43,
  ConsumedByOptionalComponent,
  DelegatedToSharedNPCEvent,
};

struct Host {
  virtual ~Host() = default;
  // Original Grunt entry always invokes 0x22072100(owner+0x9E4,0.75f).
  virtual void original_prefix_update(float value) = 0;
  // Other 26 original Grunt-specialized event paths require more source work.
  virtual void unresolved_dedicated(std::uint32_t event_id,std::uint8_t native_case) = 0;
  // Observed original +0xC18 component virtual+0x60 with marker preset 1.
  virtual void optional_component_event(const Event& event,std::uint8_t& marker_plus_8) = 0;
  // Original 0x2202D930 event 0x43 bypasses component and base to +0x780.
  virtual void original_special_43(const Event& event) = 0;
  // Shared NPC base 0x22050150, which separately dispatches 0x54 to +0x5A8.
  virtual void original_shared_npc_event(const Event& event) = 0;
};

// Fully source-bounded decision structure of the 72-byte 0x2202D930.
inline Result intermediary_2202d930(
    const Event& event,bool component_at_owner_c18,Host& host) {
  if(event.numeric_id==0x43u) {
    host.original_special_43(event);
    return Result::HandledSpecial43;
  }
  if(component_at_owner_c18) {
    std::uint8_t event_marker = 1;
    host.optional_component_event(event,event_marker);
    if(event_marker!=0) return Result::ConsumedByOptionalComponent;
  }
  host.original_shared_npc_event(event);
  return Result::DelegatedToSharedNPCEvent;
}

// Partial but source-exact dispatch partition for Grunt 0x2235BFD0.
// The original has 26 specialized IDs with complex game-specific effects;
// these are NOT fabricated as an implementation of those branches.
inline Result grunt_parent_2235bfd0(
    const Event& event,bool component_at_owner_c18,Host& host) {
  host.original_prefix_update(original_prefix_float);
  const auto native_case=original_case_index(event.numeric_id);
  if(native_case!=24) {
    host.unresolved_dedicated(event.numeric_id,native_case);
    return Result::RequiresOriginalDedicatedHandler;
  }
  return intermediary_2202d930(event,component_at_owner_c18,host);
}

inline constexpr bool jetpack_event_0x54_uses_generic_case() noexcept {
  return original_case_index(0x54u)==24u;
}
}  // namespace sin_compat::grunt_event_parent_phase592