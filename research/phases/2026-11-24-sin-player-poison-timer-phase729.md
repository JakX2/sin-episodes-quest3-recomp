# Phase 729: original CHL2_Player poison timer producer

Fresh original DLL fingerprints match existing pins. Original
2239f700 (446 bytes SHA256 3e9e9b90ad4b1e8d396e16e30fcbd590ff69b560e95713ab6429811e2a536fe4)
validates serial handle +14d8, then resolves again for enabled byte
+394, again for source origin +2bc, updates player absolute origin,
computes rounded squared distance and sqrt, resolves again for +3a0.
Strict distance < radius+10; out-of-range clears handle, stale or
disabled does not. Host supplies typed serial resolution and ordinary
absolute positions. Repeated null checks are port guards.

223a06a0 (656 bytes SHA256 b307b211cf2a3cdc59d971964b749eb5a5873b49ae38f4fe0ab4f2004db1ca59)
called at 223a22b8. Shared last clock at 2259e7c4 is reduced to now
if greater; shared damage deadline 2259e7c0 reset to now if beyond
now+0.6f. Delta=(now-last)*0.15 (double at 223d9f88), negate if
inside; add to +14d4, store f32, clamp >1 or <=0. At zero inside
and deadline strictly < now, builds damage-info (10.0f, flags
0x40000, source handle, player), calls 2213f460 then samples now
and stores now+0.6f. At timer<1, onset +14dc initialized if zero,
effect virtual +28 gets 26/27/28/29 for strict >.75/>.5/>.25/else,
then global virtual +54 computes intensity from timer and stores
+14e0. At timer>=1 or unordered, if onset>0 sends effect 0 and
resets onset and intensity=1. Final compare +13a0 versus +14d4,
if unequal or unordered notify local object +137c vslot0 BEFORE
copy, including re-read after callback. Actual damage, engine
effects, intensity service and dirty delivery are host boundaries.
No invented intensity curve (test uses a fixture only).
Spawn tail 223a209f initializes +14d4=1,+14d8=-1,+14dc=0.
No shared-global reset or unrelated fields invented.

Explicit shared timing object models original module globals;
one serialized instance per server. Host attachment adapter owns
the earlier damage-info cast at 2239fb16 (flags bits 16/17) and
provides compatible handle; full damage method not implemented.
Tests actual paired DT_HL2Local callbacks at index 10, +13a0 ->
+11ec; range boundary, stale/disabled, accumulation, damage strict
deadline, recovery, visual threshold equality and callback failure.
No renderer, physics, or proprietary bytes committed.
