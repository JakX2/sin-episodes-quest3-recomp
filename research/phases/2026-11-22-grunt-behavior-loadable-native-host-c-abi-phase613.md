# Gameplay Phase613: loadable Grunt behavior C ABI for Windows x86 hosts

## Real executable implementation milestone

Phases601–611 directly SHA-authenticated 40 complete original 2006 SiN Episodes gameplay native method addresses and 11,224 machine-code bytes, and reconstructed original Grunt behavior selection, Assault eligibility and Assault old-behavior cleanup. Phase612 combined these into a passing original-free C++ host test, but still had no portable export interface a Windows host could dynamically load.

**Phase613 introduces a real compileable SHARED C ABI library** that hosts this source-backed behavior: no commercial original executable bytes or Source SDK objects are copied. The caller supplies actual engine-owned entity handles, NPC identity/generation, live AI conditions and lifecycle event callbacks. This is **not yet** the game-facing CreateInterface server.dll or an NPC spawned in a real level.

### Five actual exported C entrypoints

Public C header: include/sin_compat/grunt_behavior_c_api_v1.h
Shared library implementation: src/grunt_behavior_c_api_v1.cpp

- sin_grunt_behavior_abi_version_v1: static version 1.
- sin_grunt_behavior_create_v1: validate an exact-version callback table, NPC (index,generation) identity, nonzero host-owned owner token, distinct registered condition IDs, finite initial Assault timing; return a per-NPC session.
- sin_grunt_behavior_step_v1: check host actor liveness; iterate eight original registered behaviors in Phase535/606 priority; delegate other seven eligibility predicates to real host; compute the original Assault behavior's native finite-time+serial-handle eligibility through Phase609/610 code; apply original Phase604 current-behavior same-pointer suppression and lifecycle sequencing, including Phase611 Assault old-behavior+0x20 cleanup; return status and current index.
- sin_grunt_behavior_snapshot_v1: return active behavior index and complete typed raw Assault state without exposing original game memory.
- sin_grunt_behavior_destroy_v1: refuse destruction during callbacks; free owned session memory. Caller manages live engine NPCs and entity-link lease lifetimes.

The eight original Grunt priorities are Rappel, FuncTank, Takecover, Beserk, ActBusy, Assault (zero-based index 5), GruntStandoff and Follow. The original 2006 MobGrunt owner accepts every eligible candidate, so this ABI always accepts a candidate that passes eligibility. The other seven candidate predicates remain engine Host callbacks rather than invented successes. Later phases may replace individual callbacks with additional exact native models.

### Host callback requirements and safety

The callback table is a versioned, size-checked exact C struct. All callbacks return **1 yes/success, 0 no/not found, negative error**. Out-parameters are valid only on return 1. Every callback and user pointer must remain alive for the whole session and may not reenter its API or throw. The synchronous session is single-threaded. The adapter guards against nested snapshot, step and destroy, and checks the actor's full index/generation identity through the host before selection and transitions.

Critical original-vs-port boundary:

- Raw original condition IDs 0x3B and 0x18 must NOT be forwarded directly as newly assumed modern engine IDs. Create receives two **host-registered**, distinct condition IDs. Original condition 0x3B queries use the host's first ID; original condition 0x18 sets the second. These are tested with 7001 and 7002 as synthetic IDs.
- Serial-validated entity handles use low12 index / high20 serial. The host validates these in its own entity table. Opaque uint64_t tokens identify linked entities without casting to 2006 MSVC object pointers.
- The original Assault eligibility x87 code is supported only for stable finite timestamps, and the new ABI rejects nonfinite time. Unknown/unsafe native second lookup behavior fails closed rather than dereferencing a null entity.
- The original Assault shutdown clears raw flags +0x24/+0x25 and conditionally unlinks an invalid/stale or SAME-owner relation at entity+0x3BC. A relationship to a different valid owner must remain intact.
- A host error after partially applying an eligibility state write or lifecycle callback is NOT automatically rolled back. Snapshot reflects the current state and the host is responsible for reconciling the side effects. This is explicit rather than inventing rollback that was never verified in original source.

### Native library build and tests

The top-level CMake now builds **sin_grunt_behavior_c_abi** as a real SHARED library, with ELF version script src/grunt_behavior_exports.map restricting visibility to exactly five exported functions. A separate dynamic smoke executable uses dlsym on ELF and GetProcAddress on Win32.

The independent Windows x86 native factory/proxy CMake project also builds this library as **sin_pc_grunt_behavior_c_abi**, *without replacing the original game's server.dll*. For MSVC x86, src/grunt_behavior_win32.def explicitly aliases decorated internal cdecl entries to predictable undecorated GetProcAddress names. Windows CTest tests actual DLL LoadLibrary/GetProcAddress/create/step/snapshot/destroy and the full C ABI host regression using synthetic callbacks.

The host regression tests:
- exact ABI table size, version, actor lifetime and condition ID validation;
- selecting original Assault index 5, reselecting without duplicate lifecycle callbacks;
- Rappel priority takeover and Assault link cleanup;
- valid OTHER owner link preserved on Assault deactivation;
- no eligible candidate clears current Assault with cleanup and owner notification;
- strict Assault 4.0-second timing refusal and host-registered condition 0x18 signaling;
- generational actor expiration, invalid host callback return, invalid time;
- reentrant API calls from lifecycle callbacks return a deterministic error rather than recursing.

Local tool validation performed:
- GCC C++17 -Wall -Wextra -Wpedantic -Werror combined C ABI + host regression: PASS.
- GCC libsin_grunt_behavior_c_abi.so built with hidden C++ visibility; exported C API symbols confirmed using nm -D: exactly five.
- Independent dlopen/dlsym/load/create/step/snapshot/destroy regression: PASS.
- GCC ASan + UBSan host test run: PASS, no memory or undefined-behavior diagnostics.

GitHub CI and genuine 32-bit Windows DLL tests must independently verify the committed target; no real retail game Windows process has loaded the bridge yet.

## Native source accounting and next gate

**Phase613 adds zero original retail machine-code functions**, since it composes already source-verified Phase606/610/611 methods in a new, original-free shared library. Native recovery remains 40 distinct complete 2006 original method addresses / 11,224 SHA256-authenticated machine-code bytes (Phases601–611).

Next real runtime task: implement host callbacks in the Windows x86 game-host adapter, mapping Source engine NPC generation, actual registered condition identifiers, valid entity resolver, NPC virtual lifecycle and linked relationship access. Use synthetic Win32 DLL smoke first, then in-game engine-owned NPC initialization. Do not conflate this ABI bridge with the original SiN 2006 server.dll CreateInterface export or silently replace legacy original binary factories.

Generic HL2-equivalent physics, model/prop reading/rendering, surface properties and parameters remain excluded by user priority.
