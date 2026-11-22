#pragma once
// Phase611: original 2006 Assault vtable+0x20 deactivation and relation unlink.
// Original methods: 0x220CEBF0 (105B), helper 0x220CE4A0 (115B).
// Engine entity pointers are opaque host tokens; no game DLL ABI is linked.
// The native second handle resolution can call a helper on nullptr; this
// independent port fails closed rather than intentionally crashing.
#include <cstdint>

namespace sin_compat::grunt_assault_deactivate_phase611 {
inline constexpr std::uint32_t kInvalidHandle=0xffffffffu;

struct State {
  std::uint32_t handle_plus_0x18=kInvalidHandle;
  std::uint8_t byte_plus_0x24=0u;
  std::uint8_t byte_plus_0x25=0u;
};

class Host {
 public:
  virtual ~Host()=default;
  virtual bool resolve_entity(std::uint32_t encoded_handle,
                              std::uint64_t& entity_token)=0;
  virtual std::uint32_t read_link_handle_plus_0x3bc(
      std::uint64_t target_token)=0;
  virtual void write_link_handle_plus_0x3bc(std::uint64_t target_token,
                                            std::uint32_t value)=0;
};

// Real 0x220CE4A0: clear entity+0x3BC only if its encoded relation is
// missing/stale, or if the twice-resolved relation points to this owner.
// A valid linked OTHER owner must not be detached.
inline bool clear_link_when_missing_or_owned(Host& host,
                                             std::uint64_t target,
                                             std::uint64_t owner) {
  const auto first=host.read_link_handle_plus_0x3bc(target);
  std::uint64_t resolved=0;
  if(first==kInvalidHandle || !host.resolve_entity(first,resolved)) {
    host.write_link_handle_plus_0x3bc(target,kInvalidHandle);
    return true;
  }
  const auto second=host.read_link_handle_plus_0x3bc(target);
  resolved=0;
  if(second!=kInvalidHandle) {
    (void)host.resolve_entity(second,resolved);
  }
  if(owner!=0 && resolved==owner) {
    host.write_link_handle_plus_0x3bc(target,kInvalidHandle);
    return true;
  }
  return false;
}

// Real Assault+0x20 method 0x220CEBF0, called during old-behavior
// shutdown. The source reads behavior+0x18 twice, then calls 0x220CE4A0.
inline void deactivate(Host& host, State& state, std::uint64_t owner) {
  state.byte_plus_0x24=0u;
  state.byte_plus_0x25=0u;
  const auto first=state.handle_plus_0x18;
  if(first==kInvalidHandle)return;
  std::uint64_t target=0;
  if(!host.resolve_entity(first,target))return;
  const auto second=state.handle_plus_0x18;
  if(second==kInvalidHandle)return;
  target=0;
  if(!host.resolve_entity(second,target))return;
  if(target==0)return;
  (void)clear_link_when_missing_or_owned(host,target,owner);
}
} // namespace sin_compat::grunt_assault_deactivate_phase611
