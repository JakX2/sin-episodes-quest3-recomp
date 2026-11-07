# Phase: generation-safe native assault NPC sessions and ai_goal_assault fanout

**7 November 2026 | Draft implementation stacked on PR #140**

## Why this is necessary

The original 2006 SiN Episodes server.dll exposes the Hammer input
ai_goal_assault::InputBeginAssault at **0x220CFF30**. Its actor list
(+0x3AC handle storage, +0x3B8 count) is iterated per actor;
for each resolved CAI_AssaultBehavior (lookup 0x220CDC80)
the owning NPC's two transient fields are reset, received cue
at behavior +0x20 is set to **1**, and original
COND_PROVOKED (native 2006 numeric ID 0x18) is raised.

Original code facts, independently hash-pinned and verified, are
recorded in docs/ORIGINAL_GOAL_ASSAULT_BEGIN_INPUT_NOV06.md and
research/tools/audit_sin_goal_assault_begin_input.py.
Original condition names/IDs are separately evidenced by
research/tools/audit_sin_assault_condition_registry.py.

The PR #140 shared C ABI implements this sequence for **one** NPC
session, but does not include a goal actor list, a live entity
registry, or any SourceVR game-module implementation. This phase
adds a host-side fanout of per-NPC C ABI sessions, and closes the
stale-owner-handle boundary.

## Critical liveness hardening

sin_assault_create_v1 now validates declared callback struct size
before looking at the callback slots. The session stores an
opaque {index,generation} actor reference, never the 2006
Windows x86 EHANDLE or any SourceVR internal pointer.

The separate shared C ABI now calls the registered host
is_actor_live callback for its **own NPC**, not merely
for owners of competing rallypoints:

- At the beginning of sin_assault_bind_v1, before acquiring a lock.
- **Again** after successful initialization, before committing
  configured mode and received cue state. If the host killed or
  replaced the actor during initialization, release the acquired
  lease and refuse the commit.
- Before sin_assault_begin_v1 resets transient NPC state.
- Again after host reset, because a host callback might invalidate
  the actor while resetting its state.
- Before sin_assault_check_cue_v1 queries engine conditions or
  touches the cue gate.

If the exact registered actor generation is no longer live, the
C ABI returns **SIN_ASSAULT_ACTOR_EXPIRED (-6)**. Host query errors
remain SIN_ASSAULT_HOST_FAILED. On a failed unlock, the C ABI
retains the locked rally token; the host can call finish_v1 later
for cleanup. Crucially, **finish_v1 must not check actor liveness**:
otherwise a dead or unloaded NPC could permanently strand its
rallypoint reservation. Unlock must match original owner identity
including generation; the host is responsible for a safe level
shutdown/release path.

These extra live-actor checks, exception cleanup, and statuses are
**new Quest-port safety design**, not claims about the original
2006 game's x86 implementation or tick/event scheduler.

## Host-side ai_goal_assault fanout

include/sin_compat/assault_goal_fanout.hpp implements
begin_original_goal_assault_fanout using a host-owned stable
snapshot of actor references and a trusted session resolver.
Each successfully resolved session invokes the already compiled
sin_assault_begin_v1 C entry point.

This fanout respects the original per-actor iteration and
skip-unresolved behavior, but explicitly reports:

- successful signal counts,
- invalid and duplicate host refs,
- missing session bindings,
- sessions not yet bound to rallypoints,
- expired generations,
- reentrant session calls,
- host callback or resolver failures.

Duplicate suppression, per-actor error isolation, and refusing to
signal an unbound session are **new safety policies**. The
original handler's behavior for a valid assault behavior with no
selected rallypoint must not be inferred from this host policy.

The resolver must map each exact current {index,generation} to
the matching session and maintain its lifetime for the entire
fanout call. This is currently a host-side C++ helper, **not a
new C export or a native Source engine ai_goal_assault factory**.

## Genuine compiled shared-module integration regression

tests/test_assault_goal_fanout.cpp links against the existing
libsin_assault_npc_c_abi.so, not a duplicated implementation
of the C ABI. Its synthetic level hosts **four initial NPCs and
four rallypoints**, and exercises:

1. Three NPCs independently binding priority-ranked points
   under one globally shared fake-level lock registry.
2. A goal input fanout with valid, invalid, duplicate,
   unresolved and unbound actors.
3. Ordered per-actor transient reset and registered Provoked
   condition signal, preserving mode-1/2/3 distinctions.
4. Lazy HearCombat lookup only on gunfire-mode NPCs.
5. Old generation invalidation after an entity slot is recycled,
   without signaling the new occupant accidentally.
6. Rally unlock after the previous occupant dies, followed by
   reuse from the new generation.
7. Host initialization killing a fourth NPC mid-binding,
   triggering lease rollback with unchanged cue state.
8. A host session resolver exception for one snapshot entry,
   without dropping subsequent valid actors.

The existing C11 dlopen smoke adds checks for expired NPC
callbacks before and during binding and BeginAssault reset.
Both desktop build and Bionic x86_64 CI run the tests with
assertions **enabled in Release**. ARM64 CI compiles and
checks the fanout executable's ELF machine and dependency
on the separately built NPC C ABI module; that executable
is *not* run on actual Quest hardware.

## Reproduce on desktop

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_assault_npc_c_abi sin_assault_npc_c_abi_smoke sin_assault_goal_fanout_tests
    ctest --test-dir build -R 'sin_assault_(npc_c_abi_smoke|goal_fanout_tests)' --output-on-failure

## Outstanding acceptance gates

- Confirm real SourceVR 0.5.5 ARM64 CreateInterface, game-module
  factory, entity handle lifetime and NPC condition registry.
- Implement **actual** callback bodies on verified engine
  interfaces. Current test callbacks are deterministic mocks.
- Integrate host snapshot generation with actual ai_goal_assault
  map I/O, Spawn/Activate, SetParameters, BeginAssault, rally
  lease lifecycle, scheduler and OnArrival outputs.
- Instantiate a real SinTEK grunt in a legal locally owned level
  and verify movement, weapon combat, native Android loader,
  then Meta Quest 3 rendering and controls.

The original client.dll, server.dll, maps, models, scripts and
other proprietary game assets remain outside this repository.
No playable Quest 3 game or real SourceVR NPC integration is
claimed by this phase.
