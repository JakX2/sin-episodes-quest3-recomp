# Gameplay Phase630: Authenticated Elite Spawn -> native event43 -> original gate and timer

## Original provenance and previous contracts

This phase **composes existing original-source reconstructions** rather than counting new retail bytes:

- Phase622: distinct original `CNPC_EliteGrunt` factory and constructor. The original entity classname is exactly `npc_druglab_grunt_elite`.
- Phase623: complete 505-byte Elite Spawn (`0x22355820`) initializes `this+0xFDC` with a weapon-linked encoded native serial handle, or `0xFFFFFFFF` when unavailable.
- Phase593: complete 360-byte Elite animation-event override (`0x22355480`) handles original event `0x43`, reads that `+0xFDC` handle, calls source gate `0x22315320` at `0x22355503`, publishes Elite byte `+0xFD9`, then conditionally dispatches inherited Grunt animation behavior.
- Phase629: two distinct SHA256-authenticated original 2006 methods. Gate `0x22315320` (82 bytes) and paired timed updater `0x22315390` (173 bytes). The resolved **weapon-linked target** has DWORD `+0x594` phase and float timestamps `+0x590/+0x58C`; the exact runtime C++ class of that target remains unproven. Gate returns true only in phase 2. The updater progresses 1→2 at >=1 second, 2→3 at >=1 second, and resets 3→0 after original dynamic virtual command calls.

All four original sources belong to the owner's unmodified SHA256-authenticated 2006 `server.dll`. No game instruction bodies, complete original dumps or proprietary DLL bytes are published.

## The actual missing integration problem

Phase626's `sin_elite_native_host_c_abi` is an actual 8-export C DLL with per-existing-entity `(index,generation)` identity, source-backed Spawn and event43. However, its original gate callback was an **opaque host answer** and could have fabricated an arbitrary boolean. Phase629's original gate logic operated on a detached `State` struct; it didn't commit each write into the engine-resolved target at the source instruction's exact point.

A second problem: Phase629's original gate **unconditionally reads `[ECX+0x594]`**. The original Elite event path still calls the gate on a null serial-handle result. A synthetic test could return false there, but a hypothetical native call would dereference a null pointer. An independent host must refuse the missing target before making the source-backend call. It cannot claim the retail gate is itself null-safe.

## Phase630 architecture and API

Preserves **every existing Phase626 v1 export unchanged**. New standalone shared library `sin_elite_source_gate_phase630` has exactly **three** new versioned C exports:

1. `sin_elite_source_gate_abi_version_v1`
2. `sin_elite_source_gate_event43_v1`
3. `sin_elite_source_gate_update_v1`

The new C header is `include/sin_compat/elite_native_source_gate_phase630.h`. The standalone DLL dynamically links Phase626's existing `sin_elite_native_host_c_abi`. The owner's hypothetical actual engine host supplies the real original NPC/weapon ownership, native field reads/writes and original virtual-call implementation. The adapter neither locates nor constructs `CBaseNPC`, does not call original game code or load original DLLs, and does not modify `CreateInterface` forwarding.

### Event43 path

```text
Phase626 registry: reattest real engine-owned Elite index + generation
  -> require successfully completed original-source Spawn
  -> re-read live Elite this+0xFDC (ignore stale Spawn cache)
  -> resolve serial to live native target (host validates ownership)
  -> read actual target phase+0x594 and float timestamps+0x590/+0x58C
  -> Phase629 source gate:
       phase0: native virtual+0x3C4(13,0), write native phase=1,
               independent engine clock read, write native +0x590,
               independent clock read, write native +0x58C, BLOCK
       phase2: ALLOW
       phase1/3/other: BLOCK
  -> Phase626 publish Elite byte this+0xFD9
  -> inherited event43 dispatch only when source gate ALLOWS
```

Missing serial handle, stale resolution, zero native token or failed native state read cannot be misinterpreted as a permitted event. They return a negative error and do not publish `+0xFD9` or dispatch the inherited event. This deliberate fail-closed behavior is **not** claimed as an authentic null-tolerant retail function.

### Paired timed update path

`sin_elite_source_gate_update_v1` first uses Phase626's public snapshot entrypoint, ensuring native actor classname/owner/generation revalidation and completed Spawn. Then it re-reads original current Elite `+0xFDC`, resolves the serial to a live target, reads the target's authoritative state and runs the Phase629 updater. The real host must enforce the same single-thread/synchronous owner lifetime contract already required by Phase626. It must not detach/change the underlying target mid-callback. The adapter also applies an **independent process-wide nonblocking atomic reentrancy guard** across both its event and timed-update exports, so host callbacks cannot recursively enter either adapter path; a nested call fails with Phase626's existing `SIN_ELITE_NATIVE_REENTRANT` status. This does not replace the owner engine's lifecycle serialization, because a separate call to Phase626 detach could otherwise occur between update validation and later host callbacks.

- Phase1 at >=1 second: write native phase 2, **then** read engine time and write native float `+0x590`, **then** independently reread and write float `+0x58C`.
- Phase2 at >=1 second since native `+0x58C`: write native phase 3.
- Phase3: call actual host-owned original-target virtual `+0x3C8(13)`, then `+0x3C4(14,0)`, then write native phase 0.
- No mutation for phase0, invalid phase, or insufficient time.

The source model's `IHost` gained three **backward-compatible virtual field-write observers** with defaults that preserve the Phase629 pure model. The native Phase630 adapter overrides all three and requires that each callback returns success only when the actual target field has been committed. This preserves original instruction-order effects even if a later callback fails. No invented rollback is attempted.

### Important limits

- This is **now a real callable, separately loadable C ABI DLL composition**, not an unlinked synthetic-only class: Windows x86 and Linux builds link it to the genuine independently built Phase626 v1 library and execute the original-source Phase629 reference.
- The tests supply **synthetic** Source engine entity ownership, resolved target, serial handles, clock and virtual operations. They do not prove the *real SiN Source executable* called our adapter or validate game compatibility.
- The original game has not yet been tested with this DLL and no complete executable-game DLL replacement is claimed. Phase627/628's owner-only Windows original game hardware tracer remains the next indispensable runtime probe.
- The source model uses ordinary bounded double arithmetic for original float time difference; x87 edge/control-word behavior is not fully emulated.
- No HL2 physics, surfaces, models, props or already functioning rendering systems are modified.

## Native tests and validation

`tests/test_elite_native_source_gate_phase630.cpp` drives an actual `sin_elite_native_registry_v1` through full C API create/attach/Spawn and through the Phase630 exports, checking:

- Entire 0→1→2→3→0 gate/timed-update cycle with source-order callback trace and two independent initial clock loads, plus three distinct clock reads in phase1 update;
- Event43 phase2 allowed, native Elite `+0xFD9=1` published before inherited event, and phases0/1/3 blocked;
- Exact original 1-second comparisons, mutated native handle, stale generation, NPC owner invalidation, missing Spawn, serial handle absent/stale, and native target unavailable;
- Individual native callback failures, partial field writes, previous Source phase retained after later callback refusal, and Phase626 reentrant registry calls rejected;
- Invalid ABI versions/sizes and missing callbacks.

`tests/test_elite_native_source_gate_dynload_phase630.cpp` loads the **actual separate shared library** on Windows x86 via `LoadLibrary/GetProcAddress` and on Linux/Android via `dlopen/dlsym`, verifies the exactly named three functions and ABI version. The library's Windows `.def` and Linux version script limit exports independently of the original eight Phase626 exports.

The new research does not add original-decompiled byte counts or claim Android/Quest 3 in-game boot. This phase makes the original NPC→serial-handle→gate→timed-transition composition callable by a future real native host without guessing the 2006 engine's vtables.
