# Phase 686: native map-owned grenade_ar lifecycle

Uses prior original-authenticated CGrenadeAR recovery in the original
ar-grenade lifecycle, detonation, AI-sound and touch reports. No new
binary audit is claimed. Original factory 0x223C4FC0; Spawn 0x22314B10;
Touch 0x223142F0; Think 0x22314360; Detonate slot250 0x22314570.
Original PE32 offsets remain evidence, not runtime pointers.

Links the existing portable Runtime and original touch evaluation
into the actual independent server module. The map registry accepts
grenade_ar, but generic map Spawn refuses because live kinematics
and game time are mandatory. This is a dynamic entity path, not
ordinary BSP host support. Explicit spawn attaches the prior verified
runtime to the map identity. Native API supports observed kinematics,
think, contact flag observation, touch and detonation; original
+0x795 is distinct from the one-shot portable armed bit.

Touch uses the original gate model. The native host must perform
the requested original contact setter and separate detonation byte
clear before dispatching effects. Flight and explosion AI stimuli,
trace-selected decal, screen-shake descriptor, radius-damage
descriptor and deferred removal route through an explicit typed host
callback. Host supplies actual physics/trace predicates and decal;
no projectile collision or damage implementation is fabricated.
Scheduling is returned as state for a real host to apply.
Callbacks are synchronous and reentrant mutation is refused.
Uncertain effects permanently block further grenade operations
(except state/removal), avoiding duplicate damage or deletion.
No claim of full original think detonation predicate, original
networking, complete effects or playable native game.

Win32 and sanitized Linux builds link the same existing runtime
sources. Actual loaded-module tests verify spawn/invalid input,
original initial schedule, flight event, touch gates, distinct
contact/armed bits, original effect order, one-shot and failure
without replay. No original binary bytes or assets committed.
