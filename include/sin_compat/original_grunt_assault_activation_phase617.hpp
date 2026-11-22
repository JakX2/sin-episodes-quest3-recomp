#pragma once
// Phase617: native-authenticated CAI_AssaultBehavior activation slots.
// Real 2006 RTTI vtable 0x223F5544, virtual +0x1C, +0x24, +0x28.
// Original 2006 retail instructions and class pointers are NOT copied here.
namespace sin_compat::grunt_assault_activation_phase617 {

// Real virtual+0x1C -> 0x220CDB40: exactly one RET machine instruction.
// Calling an invented engine callback here would introduce a new side effect.
inline void enter_1c_noop() noexcept {}

// Real virtual+0x24 -> 0x220CD290: loads pointer at behavior+0x0C,
// reads its vtable, tail-jumps to virtual 0. This identical 7-byte
// function is also CAI_BeserkBehavior virtual+0x24. The original symbolic
// name and return semantics of the pointed-to object remain unknown.
// Host must check that object is valid; it must NOT interpret 2006 pointer
// addresses as native pointers in another engine.
class Host {
 public:
  virtual ~Host() = default;
  virtual void invoke_behavior_bridge_plus_0c_virtual_0() = 0;
};
inline void enter_24_bridge(Host& host) {
  host.invoke_behavior_bridge_plus_0c_virtual_0();
}

// Real virtual+0x28 -> 0x220CD900, shared across six original
// registered behavior vtables: exactly one RET machine instruction.
// Phase604 transition does NOT invoke +0x28; this is provided for future
// lifecycle host callers that may dispatch this slot.
inline void virtual_28_noop() noexcept {}

} // namespace sin_compat::grunt_assault_activation_phase617
