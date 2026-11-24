# Phase 687: native rifle-to-grenade registry transaction

Uses prior original-authenticated evidence in
sin-original-rifle-grenade-secondary-fire-integration.md and the
original ar-grenade lifecycle reports. Original CWeaponAssaultRifle
secondary method 0x22313C60, creation call 0x22313E2A, one-unit
player ammo debit 0x22313F39. Original owner branch distinguishes
player and CNPC_GruntBase and multiplies the host-resolved in-game
direction by 2000 or 1000 respectively. The original direction
derivation, full cooldown, animation, collision and NPC authorization
remain external.

The native server now connects the map-owned assault rifle to
Phase686's actual map-owned grenade runtime. Uses the existing
project's explicitly port-only atomic publication contract:
authoritative owner/readiness/ammo observation; validate a candidate
runtime; host stages an unpublished grenade_ar shell; native registry
reserves a fresh generation-safe entry; host commits publication
and optional one-unit player ammo debit; native entry becomes
live only on normal success. Grunt approval bypasses the identified
player debit, not all possible NPC ammo rules. False commit promises
no publication/debit; abort removes staged shell. Unknown ownership
or exception is separately reported, blocks further launches, and
marks any identifiable attached projectile uncertain. No abort on
a returned stale or live collision. No callbacks retained.

Native tests check player/NPC paths, insufficient/unverified ammo,
selection refusal, invalid spawn, preparation refusal, commit
refusal and rollback, retired generation, actual registry publication,
and uncertain commit without replay. Host supplies a real entity
shell and atomic ammo/publication behavior. The original game did
not necessarily implement this transaction; this is a port guard.
No retail engine factory, actual weapon attack, projectile physics,
or complete DLL startup claim.
