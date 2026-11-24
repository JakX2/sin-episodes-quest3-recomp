# Phase 685: native ScatterGun secondary-fire host path

Uses existing original-authenticated Phase167 evidence, not a new
binary audit. Original CWeaponScatterGun slot227 0x2231C810 (612 bytes,
SHA256 323253a226cad936b4bc6c1e56c349b39eae8391b5414e203d86ba725fe16d5e),
and vector helper 0x222CBC20. See Phase167 original report for the full
owner fingerprints and call witnesses.

The independent native server now executes that branch ordering on a
live map-owned scattergun identity. The host supplies the resolved
owner and original timing/ammo fields. Typed query/effect tags preserve
the two independent clip-mode checks and two independent clock reads,
owner-mode cooldown and signed empty-clip gates, animation eligibility,
owner callback, authoritative ammo decrement/consumption and secondary
snapshot write, and the final eight original pellet callbacks.
The callback for the original vector helper owns its actual vectors and
branch resolution. The original pattern address is not used as a native
pointer. The host must implement the actual pellet geometry, trace and
damage path; the module does not claim that dispatching a tag fires
a retail weapon. No copies of live ammo or timer storage are retained.

This intentionally differs from Phase167 portable state-only model:
timer and ammo writes are delegated at their original call boundaries
so the real host can supply dirty notification and networking behavior.
The same map identity and Phase684 resync flag block further pump/fire
after uncertain callback completion. Explicit observed reconciliation is
required. Callback failures are not blindly replayed. Nonfinite clocks
and invalid boolean observations are independent port guards.

Actual loaded-module tests verify early paths, exact full ordered
query/effect trace, clip-mode recheck, animation-disabled path, eight
pellet callbacks, failure after partial dispatch, shared pump resync,
and stale entity rejection. No original executable or asset bytes
committed; real Source engine entity factory/callback binding remains
unfinished.
