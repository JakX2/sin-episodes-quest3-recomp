# Phase 137: original grunt grenade-kick runtime reconstruction

## Evidence provenance

This implementation is independently authored from the preserved Phase 10–13 static-analysis reports for the user's original 2006 Windows x86 server.dll:

- SHA-256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- Original PE image base: 0x22000000
- The DLL was not re-materialized or freshly disassembled in this pass. No original code was executed. The evidence is preserved static disassembly, not Ritual source.

All original object offsets and virtual addresses are evidence identifiers only. They are never used as ARM64 object offsets or method slots.

## Reconstructed path

### Eligibility routine 0x22359A70

The portable evaluator preserves the observed guard order:

1. The grunt's current enemy resolves (GetEnemy, primary vtable slot 85; saved m_hEnemy at original +0x8A0).
2. The tracked grenade handle resolves (m_hCurrentGrenade, original grunt +0xCB8).
3. Absolute vertical separation is at most 12 Source units.
4. 3D distance is strictly less than half the live sk_fraggrenade_radius ConVar. Its original default is 250, so the default limit is 125.
5. The host-supplied horizontal direction dot product is at least 0.8.
6. The host-supplied trace fraction is exactly 1.0 for MASK_NPCSOLID (0x0202400B).
7. The candidate passes the original CBaseGrenade RTTI class check.
8. m_lifeState is zero (LIFE_ALIVE).
9. m_flDetonateTime minus curtime is strictly greater than 0.5 seconds.

The two dot-product vector operands remain incompletely identified. The host supplies the computed dot rather than this module inventing a vector pair. The trace itself is also host-supplied; the constant records the verified original mask.

### Schedule and task

- TASK_GRUNT_KICK_ATTACK is registered as local task ID 0x9A.
- The eligibility/handle-transfer branch copies the current grenade handle to m_hPhysicsEnt (original +0xCB4), clears m_hCurrentGrenade, and returns SCHED_GRUNT_KICK_GRENADE (0x73). PR #189 now gates the existing portable KickableGrenade selector branch through this operation; an ineligible candidate falls through to later known selector priorities instead of selecting the kick schedule.
- The task-start case stores current time at original +0x99C and passes the registered ACT_MELEE_KICK activity ID to the host.
- The task's update case is shared with TASK_GRUNT_SIGNAL_BEST_SOUND at 0x2235D125. That helper/virtual-call path remains unresolved and is not replaced by guessed behavior.

### Animation event 3

The original grunt's slot-176 animation-event handler dispatches raw event ID 3 to the kick path.

When the physics entity resolves and has a physics object, the portable implementation builds:

- Linear velocity from the normalized target-minus-grenade vector, magnitude 800, plus 200 to Z.
- Angular impulse X sampled from [-360, 360].
- Angular impulse Y sampled from [-180, 180].
- Angular impulse Z fixed at 20.

The historical x86 call is strongly identified as IPhysicsObject::AddVelocity at slot 44 / byte offset 0xB0. That is not an ARM64 ABI contract. The native host must map add_velocity to its actual physics interface.

The related target position is likely the current enemy, but the original operand identity is not treated as proven. A zero-length direction is explicitly left unresolved rather than inventing the original normalization result.

If the physics handle does not resolve, event 3 uses the close-range melee fallback with configured sk_grunt_kick damage (default 10) and sound NPC_Grunt.WeaponBash. The trace parameters and damage-info helper at 0x221F89C0 remain host-owned/unresolved.

If the entity resolves but has no physics object, the original debug error is surfaced and the path is left explicitly unresolved. The preserved report does not establish enough of that branch's cleanup semantics to fabricate them.

## Code

- include/sin_compat/original_grunt_kick.hpp
- src/original_grunt_kick.cpp
- tests/test_original_grunt_kick.cpp

The implementation is a portable gameplay component with explicit engine host seams. It is not a CNPC_GruntBase replacement, a native Source game module, or a complete kick task implementation.

## Verification

The local C++17 regression executable passed with -Wall -Wextra -Wpedantic -Werror.

The PR CI additionally checks:
- Desktop CTest, including selector-to-eligibility handle-transfer integration.
- Android NDK ARM64 compilation of the actual gameplay library and test executable.
- Android Bionic x86_64 execution of the mock-host regression.

These are source/build/emulator checks, not SourceVR execution or Quest 3 gameplay.

## Remaining integration work

- Implement the shared task update case at 0x2235D125.
- Resolve the original dot-product vector operands.
- Implement the real NPC-solid trace and damage-info path.
- Verify missing-physics-object cleanup semantics.
- Map the host callbacks to a supported native engine and actual NPC/physics objects.
- Connect this behavior to campaign map entities and run real encounters.

No proprietary DLL, APK, original ELF, BSP, model, map, script, or game asset is included.
