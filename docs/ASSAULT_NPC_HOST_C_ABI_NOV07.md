# SiN Episodes assault NPC: native host C ABI (7 November 2026)

**Scope:** new portable, loadable C ABI with fake host callbacks. This is NOT
the original SiN 2006 server DLL, a SourceVR CreateInterface game module,
a Quest APK, or a playable campaign.

## Evidence and design boundary

Original x86 server.dll research is recorded in:
- docs/ORIGINAL_ASSAULT_RALLY_LOCK_HANDOFF_NOV07.md
- docs/ORIGINAL_ASSAULT_SETPARAMETERS_OUTCOMES_NOV07.md
- docs/ORIGINAL_GOAL_ASSAULT_BEGIN_INPUT_NOV06.md
- docs/ORIGINAL_ASSAULT_CONDITION_IDS_NOV07.md

The original assault code ranks eligible rallypoints by priority and
squared-distance, calls the rally lock at VA 0x220CFE49, and does not
inspect that lock's returned Boolean before retrieving the entity handle.
Successful parameter setup writes configured cue mode and resets received
cue to zero; a selected but unresolved rally clears configured mode only.
BeginAssault resets two original NPC transient fields, stores received
cue 1, and signals original COND_PROVOKED. Gunfire cue mode queries
original COND_HEAR_COMBAT. The legacy condition numbers (0x18 and 0x33)
are metadata, NOT SourceVR/ARM64 engine condition IDs.

## Eight exported C functions

The separate library libsin_assault_npc_c_abi.so, with version node
SIN_ASSAULT_1, exports:

| Function | Purpose |
| --- | --- |
| sin_assault_abi_version_v1 | Query exact interface version |
| sin_assault_create_v1 | Create per-NPC session with generation-aware ref, callback table and native condition IDs |
| sin_assault_destroy_v1 | Reject destruction if reentrant or still bound |
| sin_assault_bind_v1 | Filter live owner handles, rank rally candidates, acquire exclusive lock, resolve and initialize |
| sin_assault_begin_v1 | Request NPC field reset, record received cue 1 and signal host-registered Provoked |
| sin_assault_check_cue_v1 | Evaluate original cue predicate with lazy host HearCombat query |
| sin_assault_finish_v1 | Release selected rally ownership and clear configured mode |
| sin_assault_get_state_v1 | Query selected rally, configured mode and received cue |

The host must provide callback implementations for liveness, exclusive
rally lock/unlock, valid rally resolution, additional NPC behavior
initialization, BeginAssault transient-field reset, condition setter
and condition query. Callback results are exactly 0, 1 or negative
error; callbacks must not throw, reenter, or outlive userdata.
The host must supply ACTUALLY REGISTERED modern condition IDs, not
silently reuse original x86 numeric values. Equal IDs and UINT32_MAX
are rejected at creation. Candidate IDs are nonzero and unique
within one candidate snapshot.

## Intentional port-side safety differences

1. After ranking, failed exclusive acquisition returns
   SIN_ASSAULT_LOCK_REJECTED and leaves cue state alone.
2. After a successful lock but failed point resolution, the host
   unlocks the point and the configured cue mode becomes zero;
   the previously received cue is preserved.
3. If initialization fails after locking, the port attempts
   an unlock without committing new cue fields.
4. BeginAssault requires a bound session. A failed host condition
   setter restores the prior received-cue value, though the
   host's earlier reset side effects are not undone.
5. Nested calls and destruction from a host callback are refused.
   Bound sessions must release their rallypoint before destroy.

These rollback, guards, and explicit error statuses are NEW design,
not claims about Ritual's original source. The host must ensure
callbacks really access engine-owned NPC state and must handle
any failed unlock requiring external cleanup.

## Reproduction

Desktop shell commands:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_assault_npc_c_abi sin_assault_npc_c_abi_smoke
    ctest --test-dir build -R sin_assault_npc_c_abi_smoke --output-on-failure

The C11 smoke independently dlopens the C++ shared library,
dlsyms all eight exports, and executes mock host callbacks. It checks
generation-aware actor references, mismatched version/condition IDs,
occupied higher-priority points, reentrant API rejection, cue modes
2 and 3, typed conditions, callback failures, lease rollback,
duplicate points, and release lifecycle.

Android NDK CI now builds a genuine ARM64 API26 library and
C11 dlopen executable. Android x86_64 API29 emulator CI pushes and
actually executes those two Android Bionic artifacts using a fake
host, retaining a pass marker as evidence. Android compilation
and emulator success do not establish Meta Quest 3 on-device load
or original map entity execution.

## Required next gates

Verify the native ARM64 SourceVR game-module interface and entity
factories (no guessed x86 vtables). Connect real engine callback
implementations and native condition registration. Implement MobGrunt
AI, navigation, assaultpoint chain, weapon handling, and map I/O.
Only then validate a real level with the user's own original assets.
No proprietary DLLs, scenes, maps or game assets have been committed.
