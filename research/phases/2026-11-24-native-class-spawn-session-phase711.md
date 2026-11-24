# Phase 711: native client class spawn session

Phase705/706 owned 15 reconstructed class records/35 tables per role.
Phase708 provided five creator wrappers and leaf constructor defaults.
Phase709 implemented network-index registry. Phase710 implemented
the shared original slot37 initializer. This phase connects these
parts to the actual cdecl class-record creator callbacks.

Single active, explicitly bound session per client DLL (original class
callback ABI has only index and serial, no context). Replaces exactly
CHL2_Player, CPlayerHand, CWeaponMagnum, CWeaponAssaultRifle,
CWeaponScatterGun creation pointers in the owned aggregate. Other ten
creators remain host-owned. Original callbacks restored on unbind.
The aggregate and registry refuse destruction while bound; unbind
requires no owned live entities and no engine decoder/class markers.
Caller must detach all external references and stop calling thunks
before unbind. Single-threaded, non-reentrant contract.

Thunk allocates original-size storage, zeroes, invokes host compatible
constructor, verifies networkable interface, then executes native
Phase710 initialization directly. This is not a synthetic slot37 call.
Tracks primary/networkable pointer, class and packed handle in the
session. Stale handles cannot release another generation. Release
removes registry entry before invoking host cleanup. Host cleans up
remaining attached services and object lifetime after successful init;
failed init with completed rollback is cleaned up as constructed only.
A retained rollback failure remains discoverable by index and is not
returned as a usable object. Caller must not directly remove session
entities or invalidate their vtables while live.

Native regression drives all five actual class creator fields through
allocation, recovered registry/init sequence, handle lookup and release.
Checks late attach failure cleanup, stale handle and duplicate refusal,
registry/aggregate lifetime pins, marker refusal and restored creators.
Host fixture constructors and service callbacks are synthetic, not
retail-compatible gameplay object implementations. Retail class
publication and original engine startup remain disabled.
