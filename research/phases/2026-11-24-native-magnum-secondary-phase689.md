# Phase 689: original Magnum secondary attack and synchronous impact integration

Fresh local SHA256 match of owner server.dll
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Full direct disassembly of CWeaponMagnum slot227 0x22316cf0..0x22317168,
1145 bytes, SHA256 6b89fc7e2b14bfcadfb1d9d7209cc1900358fde284a434c4efbc2f54744f0e4b.
Original 0x222a8250 decrement helper was rechecked; it subtracts the
pointed integer with native dirty notification.

At entry +0x43c uses-clip and exactly zero +0x4f0 dispatch +0x380
before owner lookup; negative clip is NOT this early gate. Owner
+0x120 selects direct owner branch (otherwise fallback). Event3 and
selected activity precede duration and two separate clock reads.
Direct owner gets +0x3d4/+0x484(5). Resolve origin via +0x358; direct
direction via +0x514 with 0x3db27eb6, fallback via 0x22139f60 and
+0x734. Direct clip branch repeats the clip test and signed min
of count=1 and live clip, then 0x222a8250; reserve branch re-reads
count when count>=available and re-reads ammo index each time.
Fallback does not execute those player-side debits.

Owner SDK2004 shareddefs.h corroborates the familiar firing-record
prefix. Original extended record is larger: auxiliary vector at
+0x28, distance +0x34 (float bits 0x475db3d7), ammo +0x38,
tracer count +0x3c, damage/playerdamage/flags zero, force scale 1,
weapon pointer +0x54, extra byte +0x5c. Unknown extension fields
are not assigned names or copied as native stack bytes. Original
owner slot243 +0x3cc supplies spread. If direct and zoom byte zero,
owner +0xb8 bit0x800 selects 0x2213fa60, then helper 0x22269bc0
produces a vector; original constant at 0x223ddcf4 is 4.0f.
Auxiliary point = origin + four times host-observed vector, flag 1.
Owner +0x194 receives the record. Direct path then feedback(6),
owner angle snapshot +0x36c, two original RandomInt(-1,1) slot2
calls, helper 0x222445b0 with adjusted angles and z=0, RandomFloat
(-2,2) slot1 and owner +0x434 with (-8, random, 0).

New game-only host ABI executes this ordered path on a native
map-owned Magnum; engine owner resolution, raw live fields,
tracked setter, exact vector helpers, firing, RNG and feedback
remain host supplied. Missing owner is explicitly refused before
effects rather than recreating an original null dereference.
Finite-value checks are port guards. The extended record is a
semantic descriptor, not a claim of a complete native struct ABI.

Important integration fix: a firing callback now may synchronously
invoke the SAME Magnum impact API. A scoped target and independent
impact guard block other entities and recursion. OutputScope
restores the prior guard instead of incorrectly clearing it on a
nested call. Applies also to the Phase688 deferred primary callback.
An uncertain nested impact blocks the outer attack; all unrelated
map/campaign mutations remain refused. Tests assert ordered path,
exact-zero versus negative gate, two reserve reads, fallback,
zoom, nested impact and no mutation after it, failure/no replay.
No actual retail engine vtable installation or firing claimed.
No proprietary bytes committed.
