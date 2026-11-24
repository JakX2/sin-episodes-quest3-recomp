# Phase 708: native client entity creation boundary

SHA-authenticated client.dll inspection:
CHL2_Player creator 0x242168a0, 0x1250-byte allocation,
constructor 0x242167c0; CPlayerHand 0x24216540, 0x480 bytes,
base constructor 0x240cce30; Magnum 0x2422b240, AssaultRifle
0x2422b620, ScatterGun 0x2422a9a0, 0x830 bytes each, base weapon
constructor 0x24092bd0. The latter four replace four vptrs at +0/+4/+8/+c.
Original allocator 0x240c4ff0 zeroes the whole allocation. All five
call primary vslot37 (+0x94) with the two cdecl creator arguments,
then return networkable subobject +8. All use 0x240c8fe0 which performs
entity registration through +4 interface and other host operations,
stores index at +0x50 and returns true.

The port-owned creator adapter accepts host allocator, constructor
and cleanup operations, checks aligned writable storage, zeroes the
exact original size, verifies primary vslot37 and networkable vslot0
ownership/readability/executability, calls the original thiscall ABI
on the host-created object, and returns +8. Host false init is refused
and cleaned up (original five wrappers ignore AL because all five
observed methods return true). No fixed original code pointer invoked.
Caller owns successful objects; host supplies actual compatible vtables,
base class initialization, entity registry and other side effects.
This API does not automatically install cdecl class-record thunks.

Three independently implemented leaf scalar initialization helpers:
- Original HL2Local constructor 0x2421adf0: zeros power, zoom and
  squad/weapon-lowered fields, initializes accuracy, poison and breath
  to 1.0f and invalidates handles +0x2c/+0x54/+0x58. Local vptr and
  untouched active-device bits/padding remain host-owned.
- Original CHL2_Player constructor after interpolation registration
  0x24061220: clears eight DWORDs at 0x1228/2c/30/34/3c/40/44/48.
- Original base weapon constructor after BaseAnimating construction:
  invalidates DWORDs at 0x788/80c/7e0/7e4/7d8/7dc, sets
  0x7ec/7f0 to 65.0f and 0x7f4/7f8 to 1024.0f, clears
  0x78c/81c/7d0/79c and byte 0x7fc, writes 16-bit invalid ID
  0xffff at 0x810 (helper 0x241e6ed0 returns AX=0xffff).

Host invokes these helpers at the appropriate constructor stage;
no automatic reordering around interpolation or vtable installation.
Native tests exercise exact sizes, zeroing, module-owned synthetic
vtable dispatch, argument preservation, returned subobject, failure
cleanup stages, byte canaries, and direct default writes. No complete
base/derived gameplay constructor or retail engine initialization claim.
