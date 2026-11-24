# Phase 722: original health pickups and maximum-health setter

SHA256 verified both owner DLLs against existing pins. Original server
CHealthKit primary vtable 2246b984 slot184=222f9670; CHealthVial
2246bc94 slot184=222fa330; CMegaHealth 2246bfe4 slot184=222fa520.
Vial/mega factory allocations 0x4cc (222fa2a0 / 222fa490); base item
spawn/touch eligibility not reconstructed here. No distinct network
classes in authenticated registrations.

Kit reads skill convar pointer 225965f8 +28 float; vial 22596640 +28;
mega 225966d0 +28. Original strings sk_healthkit, sk_healthvial,
sk_healthmega. Host supplies live skill float, not guessed defaults.
Kit/vial call player vslot57 with raw f32 amount and flags=0.
Mega converts skill float to integer using x87 truncation, adds it to
current max +98, calls vslot263, converts the same integer to float
and calls vslot57. Max mutation remains even if healing returns zero.
On nonzero heal: player-specific reliable recipient filter, ItemPickup
message writes item's classname +c (empty if null), sound filter and
HealthKit.Touch (kit) or HealthVial.Touch (vial AND mega). Game rules
slot71 (+11c) selects item slot103 (+19c) respawn or helper 22286bf0
remove. Host owns recipient filter/message/audio and actual item
lifetime; no duplicate base touch logic. Original resource names:
models/items/healthkit.mdl, Models/items/antigenvial/antigenvial.mdl,
Models/items/antigen_mega/antigen_mega.mdl (evidence only).

CHL2_Player vtable 224aec3c and CBasePlayer 2245222c slot263 both
2224bfd0: if max differs, call vslot108 (+1b0) dirty before write
at +98. If stored maximum >200, call dirty AGAIN and store 200.
Finally set byte +12a1 iff maximum >100. No lower clamp.
Native reproduces both intermediate writes, original DWORD addition,
and overhealth flag; uses actual compatible player storage. Float
range guards avoid undefined casts, out-of-range x87 indefinite
conversion not reproduced. Direct pickup helper assumes a typed live
player/item with host-supplied eligibility. Deferred removal and
serialization are host requirements; partial failures block both
controls until reconciliation. Tests cover float amount, refusal,
message and sound, removal/respawn, two max writes, cap, overhealth
flag, MegaHealth truncation, null classname and failed callback.
No physics, model rendering or proprietary binary bytes committed.
