# Phase 481: original VClient011 Init tail, exact two-gate control flow

## Why another startup study?

Previously, owned Stats handling and a synthetic PE32 VClient011
slot33 fixture were successfully tested, but the production client
still intentionally returned 0 from Init. Phase 480 narrowed
CClientShadowMgr Init top-level behavior without implementing its
unreconstructed rendering/engine dependencies.

The next real boot blocker is **not** merely turning that 0 into 1.
The original Init includes ordered callbacks, the global game-system
Init run, physics factory dependencies and several post-physics
registrations. Returning success before those dependencies have
real behavior risks the original engine invoking unsafe virtual
functions on partly constructed systems.

## Exact private original 2006 evidence

The connected owner original SE1/bin.zip supplied a read-only
client.dll copy with exact SHA256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original VClient011 Init tail at VA **0x240FE486..0x240FE5A3**
contains 286 bytes with SHA256
e302338aece1719a43720fa454e80cf1866d925e723d50d403af3f0d5457bfe9.

A private audit verifies the full tail-body SHA, exact fourteen
relative CALL edges, twelve indirect virtual calls and both
conditional branches. No original code, disassembly or extracted
retail bytes have been added to GitHub.

The 26 calls divide into three original control-flow windows:

| Sequence window | Original VAs / behavior | Guaranteed by original Init? |
|---|---|---|
| Before game-system gate | 0x240FE491..0x240FE4B0, four calls | Return values not checked here |
| AutoGameSystem Init | 0x240FE4B3 -> 0x24156860 | Return false branches to 0x240FE59E |
| Between gates | 0x240FE4C8..0x240FE538, fourteen calls | Return values not checked here |
| Mandatory physics triple | 0x240FE540 -> 0x24182C60 | Return false branches to 0x240FE59E |
| Final callbacks | 0x240FE554..0x240FE590, seven calls | Return values not checked here |
| Normal-success exit | 0x240FE595 | Returns 1 only after all preceding calls |
| Shared-failure exit | 0x240FE59E | Returns 0 |

The 2006 original physics helper is already independently
SHA-audited in research/tools/audit_sin_original_client_postinit.py.
It requests VPhysics030, VPhysicsSurfaceProps001 and
VPhysicsCollision007 from a supplied factory, publishing owned
original pointers and failing on the first missing dependency.
This phase does **not** recreate those engine interfaces.

## New independent code and fixtures

client_original_init_tail_gates.h contains the exact 26-call
source-authenticated metadata. It provides a *test-only order
walker*, where each stage is simulated by an injected owned callback.
The walker interprets the return value **only** at the two observed
original failure checks. An unavailable callback is refused
before any stage. This is NOT VClient011::Init or an original
Source-engine callback implementation.

test_client_original_init_tail_gates.cpp tests all 26 original
callsite identities and order, each of the two early exits, the
shared failure destination, the observed success return
destination, and false results from all 24 unchecked callbacks.
It proves the model cannot inadvertently invent extra fatal gates.
No engine vtable pointer or original callback is invoked.

audit_sin_original_client_init_tail_gates.py verifies the original
region's full SHA256 and read-only callsite/opcode/branch witnesses
when given the authentic owner's original executable. The public
--self-test builds a completely synthetic method region and
tests mutations; the original game file is NEVER required in CI.

## What remains before actual successful client Init

This map establishes **control flow**, not side effects. All
virtual-call object ownership, post-game-system initialization,
physics-interface lifetime, startup dependency resource creation,
HUD and shutdown handling must be reconstructed independently.

Specifically, previously four of twelve registered client
AutoGameSystems remain not fully implemented:
CClientShadowMgr, C_StatsSystem, C_SoundscapeSystem and
CPerfVisualBenchmark. Modeling known ShadowMgr/Stats portions
does not make those complete.

The production client.dll still intentionally returns 0 and
retains a guarded actual slot33. The latest owner-proven **real
retail-engine** checkpoint remains 0x53494E74, not a successful
game engine boot. No further owner-side game replacement test is
requested until a new executable path tests genuinely new behavior.
