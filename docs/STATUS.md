# Phase 56 checkpoint: codebase bootstrap

## Evidence boundary

The phase research archives contain byte excerpts and verification scripts for the original SiN DLLs. This repository vendors **reference code**, not binary disassembly as drop-in C++ source. No new reverse-engineering claims are made by this scaffold. The mock host simulates engine calls; its behavior does not prove the original game or Quest 3 has been executed.

### Source evidence included in references

- Phase 46: zoom button rising-edge model and shared ADS transition model.
- Phase 47: normal versus zoomed activity requests; assault-rifle recoil activities.
- Phase 51: valid animation sequence before viewmodel model/offset binding and playback.
- Phase 52: draw/holster contract and ADS-exit ordering.
- Phases 53–55: guarded clip and reserve transfer; reload stage, mode508 default and conditional single-round branch.

The eight reference headers in `third_party/research` retain their original comments and limitations. Their exact origins are in `PROVENANCE.md`.

## New code written for this scaffold

- An `IWeaponHost` integration boundary.
- A portable `WeaponRuntime` class that composes reference state transitions into host calls.
- A fake host and cross-system integration tests.
- CMake build system and an opt-in GitHub Actions CI workflow.

## Tests completed

- Linux CMake / GCC C++17 compiler build.
- `ctest` mock-host integration test suite (zoom, animations, holster, reload and clip accounting).
- Additional compiler/sanitizer checks are recorded in the accompanying release manifest if performed.

## Known integration gaps and decisions

1. The origin offset is a per-weapon general viewmodel setting, not proof of ADS-specific pose alignment.
2. The `+0x508` one-round mode is `false` for inspected firearm constructors; the runtime enabling path is unresolved. Keep it disabled by default.
3. The prototype injects an explicit transfer-at-validated-event call. The host must derive that event from the original reload schedule; the mock host cannot determine it.
4. Enter/exit zoom has original FOV request timings, but a physical VR implementation must not change headset FOV.
5. Current `holster()` considers a resolvable sequence a successful host animation operation, **not** original Source engine holster success semantics.
6. No actual Source engine ABI, vtable layout, save/load, network tables, or engine hooks are implemented.

## Next milestones

1. Identify a suitable, legally usable SourceVR engine and SDK target, and compile the host adapter against *that actual branch*.
2. Add a headless simulated player/weapon lifecycle test at engine-API boundary including reload timing and weapon switching.
3. Extract original per-gun `ZoomFOV`, weapon script and model/sequence data from legally owned assets, as available.
4. Revisit SiN SinTEK AI and vehicle behavior in a separate compatibility component, then integrate one playable level.
5. Investigate static x86-to-ARM64 translation feasibility only with an isolated representative function and a testable ABI, not promises of automatic complete porting.

## Continuity

This is the first **buildable project foundation**. All future scheduled research phases should update this repository and a versioned checkpoint when a persistent remote is connected. In the meantime, keep the released ZIP available for subsequent runs.

## Public research archive checkpoint

Phase 44–55 original reports and selected verifiers, plus carefully labeled early enemy/vehicle/difficulty historical summaries, are indexed at [`../research/README.md`](../research/README.md). This does not add runnable AI, car or campaign gameplay. The original DLL binaries, assets and excerpt dumps are excluded.

## Phase 56: Named reload-field metadata (verified static evidence)

The original SiN `server.dll` and `client.dll` were available during this investigation and both SHA-256 hashes matched `research/binary-provenance.json`. Typed metadata identifies `m_bReloadsSingly` (`server+0x508` / `client+0x7FC`), `m_bInReload` (`+0x4AC` / `+0x7A0`), and clip1/clip2 fields. See [`../research/phases/phase56-weapon-field-metadata.md`](../research/phases/phase56-weapon-field-metadata.md). Both a binary verifier and independently generated synthetic PE-parser unit tests were run; **no Quest, engine runtime or original game execution was performed**. The meaning of two parallel server descriptor groups and runtime enabling of one-round reload are unresolved.

## 2026-11-08: original Jessica scripted throw C++ reference

- Re-audited six exact functions from the unchanged 2006 Windows x86 `server.dll` (SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`) for `CNPC_Jessica` scripted item throwing. The original DLL is not in the repository.
- New independent `sin_original_jessica_throw` portable C++17 library and synthetic unit tests: typed map inputs, AI condition `0x47`, throw schedule `0x59`, animation event ID forwarding, ammo transfer and original callback ordering.
- Added a read-only, SHA-pinned Python PE32 verifier and binary-free parser regression tests. Local original function/check audit and independent C++ tests passed.
- **Not implemented:** native Jessica SourceVR factory or engine scheduler, original throw trajectory equation, actual animations and any game-level validation. This does not change Quest 3 runtime readiness.
- Research: [original Jessica throw deep dive](../research/phases/sin-original-jessica-scripted-item-throw-20261108.md). Work is kept in stacked draft PR #155 for review.

## 2026-11-08: original Jessica ballistic trajectory (stacked draft)

- Re-inspected the original 2006 SHA-256-pinned `server.dll` and recovered the previously unknown x87 helper at `0x2204C850`, called by Jessica's throw animation at `0x2236251A`.
- The throw reads the registered `sv_gravity` ConVar (original default 600), combines it with the caller's float 15.0 as **extra apex height**, calculates an initial three-axis velocity and calls the original entity motion method at `0x2214BA20`.
- New host-neutral `sin_original_jessica_toss` C++17 solver, optional `IOriginalTossProvider` integration, analytical trajectory tests, mock-host sequence test and read-only original SHA-pinned binary verifier.
- Local binary verifier passed four complete function hashes and five direct call witnesses; standalone C++17 original-free tests passed. Hosted CI is checked separately.
- **Not completed:** real native engine NPC, physical entity/animation ownership, first actual SiN map load, complete actor/player engine vtables or a runnable Quest 3 port. No proprietary binaries/assets committed.
- Evidence: [Jessica original ballistic throw](../research/phases/sin-original-jessica-ballistic-toss-20261108.md). Kept as a stacked draft above previous Jessica PR #155.

## 2026-11-08: original Jessica velocity impulse correction

- Verified original 2006 `server.dll` x86 function at `0x2214BA20` is **additive**, not an initial-velocity setter. SHA-256 of exact 177-byte function: `5eb87e06301e647f9ab83b4463e940e6bc14b5e78eae08897a5723ad58e5a3f4`.
- Original type-6 physics branch invokes physics virtual byte offset `+0xB0`, with null angular argument. Other movement types conditionally refresh dirty absolute velocity and add the requested vector to fields `+0x250..+0x258` before calling `0x22141A00`.
- New C++17 `sin_original_jessica_impulse` library and synthetic host tests, including invalid data and physics dispatch, correct the previous `set_initial_velocity` design.
- Original DLL full hash, nine exact opcode anchors and three relative calls were privately checked against the unchanged original PE32. Script: [original impulse audit](../research/tools/audit_sin_original_jessica_impulse.py).
- Evidence and open questions: [Phase 105 report](../research/phases/sin-original-jessica-additive-velocity-20261108.md).
- **Not yet proven:** complete real SourceVR physics ABI, real player virtual target getter, runtime value of zero-vector sentinel under all conditions, or actual Quest 3 gameplay. This is a host-neutral approximation, not a native client/server module.


## 2026-11-08: original Jessica player collision-center getter

- Original 2006 SHA-pinned server.dll vtable byte slot +0x214 in CBasePlayer, CHL2_Player and CNPC_Jessica resolves to the same function at `0x2214BD40`. Jessica calls this slot at `0x22362509`.
- This returns a copyable world-space collision OBB midpoint, **not eye position**. Helpers `0x2214B020` and `0x2214B0D0` compute the midpoint and select translation vs entity-world transform based on collision flags, numeric solid type and orientation.
- New independently authored C++17 `sin_original_jessica_target_center` library and target/ballistic integration tests. A read-only private binary verifier checks three complete function digests, three RTTI/vtable chains, three call targets, twelve opcode anchors and the original 0.5f multiplier.
- Local original-DLL audit and C++ tests passed. GitHub CI should be treated separately until its run completes.
- This is **not** an engine-owned player, native game DLL, executable campaign, Android APK or Quest 3 headset test. Source engine vector temporary storage, collision snapshots and correct real entity transforms require host implementation.
- Detailed provenance: [Phase 106 report](../research/phases/sin-original-jessica-player-world-center-20261108.md). Kept on a stacked draft branch above PR #157.

## 2026-11-08: original parent-aware absolute velocity (Phase 107)

- Recovered additional authentic 2006 `server.dll` x86 entity motion functions: `0x22141A00` absolute velocity update, `0x2213D750` lazy absolute velocity refresh, `0x22268E90` inverse parent rotation, `0x22268C80` forward parent rotation, `0x2213E1A0` local vector setter and `0x2214B620` dependency invalidation.
- Original DLL read-only verification passed **six complete function hashes, ten relative call targets, fifteen instruction signatures**. Source-like fields are world velocity `+0x250`, local velocity `+0x2C8`, parent matrix `+0x268`, parent handle `+0xF0`, dirty flags `+0xB8`.
- Portably reconstructed parent-relative set and recursive refresh with matrix transpose, cache invalidation mask 4, host-based original effects, and added malformed-parent/NaN safety checks. New `OriginalAbsoluteImpulseAdapter` connects this to the prior additive velocity/Jessica throw reconstruction without pretending a native engine vtable is complete.
- Standalone original-free C++ behavior, adapter integration and synthetic PE32 tests passed locally; GitHub CI is checked separately. Full research: [Phase 107 original parent-aware absolute velocity](../research/phases/sin-original-parent-aware-absolute-velocity-20261108.md).
- **Not yet playable:** full native SourceVR `CBaseEntity` ownership, EHANDLE resolution, `0x2213FA60` world-transform recalculation, real entity virtual side effects, `client.dll` game HUD connection, running SiN maps or Quest 3 standalone VR package. No original binary/assets committed.

## 2026-11-08: original parented world-transform refresh (Phase 108)

- Verified authentic original 2006 server.dll world-transform routine at `0x2213FA60`, local AngleMatrix helper `0x2226CC20`, parent affine concatenation `0x22269520`, MatrixAngles `0x22268A60`, parent/attachment transform getter `0x2213D960`, matrix copy, translation extraction and angle-comparison helper.
- Private read-only SHA-pinned audit PASSED **8 complete function hashes, 12 relative CALL targets and 14 instruction anchors** against the owner's unchanged original `server.dll`. Original DLLs are never committed.
- New host-neutral `sin_original_world_transform` reconstructs local/world origin, angles and 3x4 matrices with optional parent/attachment transform, lazy dirty cache, finite checks and cycle/depth guards. The host retains real EHANDLE ownership, attachment lookup and engine callback side effects.
- New `OriginalWorldTransformVelocityBridge` connects the preceding Phase 107 absolute-velocity module to parent matrix refresh. An integration test shows the existing additive impulse uses the updated parent orientation.
- Local C++17 standalone tests, adapter integration and synthetic PE32 audit passed; GitHub Actions status must be checked on the final commit.
- Evidence: [Phase 108 original parented world transforms](../research/phases/sin-original-world-transform-parent-matrix-20261108.md). This is a stacked draft above PR #159.
- **Not yet done:** original native CBaseEntity ABI/attachments, full SourceVR engine integration, playable SiN maps, replacement client/server DLLs or a standalone Quest 3 APK.

## 2026-11-08: original parent attachment selector (Phase 109)

- Examined genuine untouched x86 `server.dll` function `0x2213D960` and its `0x220ED470` attachment getter, `0x22269F60` identity writer and `0x2226CC20` matrix constructor. The private binary verifier passed **four complete function hashes, nine relative call targets and sixteen opcode anchors**.
- Parent EHANDLE `+0xF0` includes a 12-bit slot/generation check: a missing or stale parent produces an identity matrix. A valid parent with nonzero child attachment `+0xEC` checks parent animating virtual `+0xC4`; attachment getter virtual `+0x2C4` must return **true** before attachment angle/origin values are converted to a matrix.
- **Failure gate recovered:** unsuccessful attachment lookups fall back to the parent's cached world matrix `+0x268`, optionally refreshing dirty bit `0x800` with `0x2213FA60`. The original getter's fallback output coordinates are not used as an attachment when its return is false.
- Added `sin_original_parent_attachment` C++17 module, standalone original-free behavior tests, Phase 108 world-transform bridge tests and synthetic PE32 audit mutation tests.
- Local C++ tests and original binary SHA audit passed; check hosted CI separately. [Phase 109 technical report](../research/phases/sin-original-parent-attachment-selector-20261108.md).
- Remaining: real SourceVR EHANDLE/animation virtual callbacks, engine-owned matrix lifetimes and model attachment data, an integrated native `client.dll`/`server.dll`, and a playable Meta Quest 3 campaign. This is still testable reconstructed behavior, **not** native game execution.

## 2026-11-08: original studio bone attachments (Phase 110)

- Binary-verified `CBaseAnimating` attachment virtual `+0x2C4` -> original `0x220ED530` and bone getter `+0x2B0` -> `0x220ED1B0` in eight MSVC RTTI-linked classes, including `CNPC_Jessica`.
- Original attachment indices are **1-based**, records stride 92 bytes, and `flags & 0x100` chooses **world-aligned orientation with bone-transformed position** instead of standard bone-world × attachment-local matrix composition.
- Reconstructed a pure C++17 studio attachment host, one-based validation, ordinary/world-aligned matrix branches, missing-resource errors and finite guards. `OriginalStudioNativeAttachmentAdapter` hooks the original-style success/failure gate to Phase 109 parent-selection and Phase 108 world-refresh code.
- Read-only private original `server.dll` audit passes **8 whole function SHA-256 witnesses, 15 call targets, 15 instruction anchors and 8 RTTI/vtable bindings**. Synthetic PE audit and new integration unit tests are registered in CMake.
- Full proof and native limitations: [Phase 110 original studio attachment](../research/phases/sin-original-studio-attachment-bone-matrix-20261108.md).
- Still **not a game runtime**: native `CStudioHdr`, `SetupBones`, x86-to-ARM64 engine callback adapters, complete `client.dll`/`server.dll` reconstruction, campaign map execution and Quest 3 VR APK remain pending. No original game binary or assets committed.

## 2026-11-08: compact original bone-cache lookup (Phase 111)

- Reconstructed from unchanged original 2006 `server.dll` the bone getter `0x220ED1B0`, signed 16-bit compact bone-cache remap `0x221004C0`, entity world-matrix fallback `0x220EBE70` and 48-byte matrix copier `0x22268EF0`.
- Verified **4 complete original function SHA-256 ranges, 7 relative CALL targets and 17 instruction anchors**. Cached matrix layout is `cache+0x14+uint16(cache+0x0E)+48*int16(cache+0x14+2*bone)`, with **negative entries uncached**. Studio bone count is checked at original header `+0x9C` before the helper.
- Added safe bounded C++17 decoder, host callback for actual SetupBones and model cache, owner-owned entity world-matrix fallback, and an adapter into Phase 110's attachment resolver.
- New CTest units check packed matrices, malformed caches and signed remaps; integration test verifies cached and uncached attachment positions; synthetic PE32 audit verifies corruption is rejected.
- Details: [Phase 111 original bone cache](../research/phases/sin-original-compact-bone-cache-20261108.md).
- **Not yet playable**: original `0x220EB8D0` bone preparation, native studiohdr, full SourceVR/SiN entity runtime, actual desktop game modules, and Quest 3 Android application remain pending. No original game binaries/assets are committed.

## 2026-11-08: original CBaseAnimating bone cache preparation and reverse-map refresh (Phase 112)

- Original 2006 `server.dll` entry `0x220EB8D0` uses bone cache handle at entity `+0x478` and compares elapsed `curtime - cache.timestamp` with **0.1f**, returning fresh caches without a bone computation. Cache type **`0x300`** is tested only if expired; a stale wrong-type cache is released before allocation.
- Resolved virtual byte slot **`+0x2B4`** -> `0x220EF510` across eight inherited MSVC RTTI classes, including `CNPC_Jessica`; its output pose buffer and **mask `0x300`** are consumed by cache update or native cache allocation. The actual skeletal solver behind this virtual call is **not yet reconstructed**.
- Original `0x22100450` uses a **reverse signed-int16 table** at `cache+0x14+uint16(cache+0x10)` to map *compact cached matrix index -> source bone index*. It copies 48-byte matrices to `cache+0x14+uint16(cache+0x0E)` and updates the timestamp.
- Added C++17 safe cache-preparation state machine, reverse-matrix updater, and `OriginalPreparedBoneCacheAdapter` to Phase 111 bone getter/Phase 110 studio attachment. Tests exercise cache hits, expiration, dirty updates, wrong-type eviction, allocation, reverse mappings, malformed buffers and attachment results.
- Private original binary verification: **six complete function SHA-256 witnesses, 15 exact CALL targets, 22 instruction anchors, eight inherited vtable slot matches**. Standalone project: **12/12 CTests**, including undefined-behavior sanitizer.
- Research and limitations: [Phase 112 original bone-cache preparation](../research/phases/sin-original-bone-cache-preparation-20261108.md).
- This is **not** a replacement game server/client DLL or a playable Quest 3 app. Native MDL and bone solver `0x220EF510`, renderer, complete client HUD and engine interfaces are still pending.

## 2026-11-08: original skeletal setup dispatch (Phase 113)

- Byte-verified 2006 SiN `server.dll` virtual `+0x2B4` at `0x220EF510`: acquires MDL-cache lock, refreshes world origin, conditionally evaluates pose-driver stages when entity `+0x3D8` is nonnull, always calls virtual base pose `+0x2AC`, then chooses parent-bone merge `0x220ED290` or ordinary construction `0x220FE0C0`.
- Source evidence: **3 complete SHA-256 function witnesses, 22 exact x86 relative CALL edges, 21 instruction byte anchors and 8 inherited virtual table checks** against the owner-provided original DLL.
- Verified `CBaseAnimating` virtual `+0x2AC` maps to `0x220F2AC0`; `CNPC_Jessica` and other derived classes checked map to `0x220F5430`. Virtual `+0x2B8` is `0x220F0B80` in all eight tables.
- New host-neutral `IOriginalSkeletalSetupHost` and portable `run_original_skeletal_setup` coordinate callback ordering while preserving native-owner boundaries for real model-cache locks, animation solvers, parent EHANDLE validation, and bone matrices.
- `OriginalSkeletalPreparationAdapter` connects the dispatcher to Phase 112 cache preparation, Phase 111 compact matrix remap and Phase 110 studio attachment. Added tests for normal/pose-driver/parent-merge routes, dynamic parent changes, failure cleanup, stale versus fresh cache effects and synthetic PE32 mutations.
- Locally verified **15/15 standalone CTests** in release and Debug UndefinedBehaviorSanitizer; **private original DLL audit passed**. GitHub CI results should be checked on the exact branch head.
- Research and unimplemented internals: [Phase 113 original skeletal setup](../research/phases/sin-original-skeletal-setup-dispatch-20261108.md).
- **Not yet playable**: `0x22106840` / `0x22103A60` / `0x22104D10` pose internals, `0x220ED290` / `0x220FE0C0` bone math, native model cache, `client.dll`/full `server.dll`, engine initialization and standalone Quest 3 VR gameplay remain pending.

## 2026-11-08: parent-linked bone-name merge (Phase 114)

- Original 2006 `server.dll` parent-bone constructor `0x220ED290` now reconstructed as independent C++17 behavior, including original case-insensitive studio-bone name resolver `0x220FFB30`, quaternion+position matrix construction `0x2226D250`, cached-parent-world bone copies, and local hierarchical fallback.
- Proprietary PE32 verification passed **6 complete function SHA-256 ranges, 12 relative CALL targets and 20 instruction anchors** against unchanged `server.dll`. Original studiohdr bone count `+0x9C`, bone offset `+0xA0`, sorted-name byte indices `+0x16C`, and 216-byte bone-record stride are preserved in the safe metadata decoder.
- Added `OriginalParentBoneMergeSkeletalAdapter` to plug the actual parent-bone matrix calculation into Phase 113's previous `compose_parented_bones` native callback while leaving model cache locking, pose drivers, live studiohdr and actual `SetupBones` host-controlled.
- Added **4 new CTests**: cached/uncached matching and fallback, copied studiohdr validation, Phase 113 integration, and synthetic binary-audit corruption tests. Local standalone **19/19 CTests**, repeated under **UndefinedBehaviorSanitizer**, pass.
- Research: [Phase 114 original parent-bone name merge](../research/phases/sin-original-parent-bone-name-merge-20261108.md).
- Still **not a playable SiN build**: real animation solve, IK, native engine interfaces, complete `client.dll`/`server.dll`, campaign runtime, graphics and Meta Quest 3 APK remain pending. No proprietary binary or assets committed.

## 2026-11-08: masked ordinary skeletal hierarchy (Phase 115)

- Original 2006 `server.dll` **`0x220FE0C0`** independently reconstructed as a host-neutral C++17 masked ordinary bone builder. This complements Phase 114's name-based parent rig merge and completes both previously stubbed skeletal matrix construction **branches** in the Phase 113 dispatcher, not its pose evaluation algorithms.
- The original `-1` target processes all bones in studio order; requesting an individual index first gathers its parent chain and processes root-to-leaf. Invalid indices map to bone 0. A studio bone's record **flags at +0xA0** are masked before quaternion+position composition. Masked-out output slots remain untouched.
- Checks against the unchanged private original `server.dll` pass **3 whole-function hashes, 4 exact relative CALL targets and 14 instruction anchors**. No original game bytes/asset files have been checked in.
- Added bounded 216-byte studio bone-mask metadata parsing, seedable output matrices for skipped bones, hierarchy and finite guards, and `OriginalOrdinaryBoneSkeletalAdapter`. Two adapters can be composed to choose parent-merged or ordinary bone paths in `run_original_skeletal_setup` while keeping engine-side animated poses and model cache as callbacks.
- **4 new CTests**: masked and ancestor-only calculation, studio flag parser, parent/ordinary dispatcher integration and synthetic PE32 audit; the standalone **23/23** suite passes in Release and with UndefinedBehaviorSanitizer.
- Research: [Phase 115 original ordinary bone constructor](../research/phases/sin-original-ordinary-masked-bone-hierarchy-20261108.md).
- **Still no playable game or standalone Quest 3 build**: real animation pose blending, native engine interfaces, original DLL exports, rendering, audio and input integration remain pending.

## 2026-11-08: original pose-driver initialization and first command pass (Phase 116)

- Independently reconstructed the first loop of SiN's original `server.dll` `0x22103A60`, following Phase 115's masked ordinary-bone construction. The original **3,701-byte function** remains only partially reconstructed; the second loop is intentionally not claimed.
- Verified **four complete code-region SHA-256 witnesses, sixteen original relative CALL targets, twenty-three instruction anchors, four command jump-table entries** and original numeric constants against the owner's private unchanged 2006 PE32 DLL.
- Track reset stride is **0xE4 (228 bytes)**; group stride is **0x14 (20 bytes)** and command stride is **0x7C (124 bytes)**. The jump table implements type **3** (pose with side effect), **4** (cap/attenuate), **5** (pose with optional payload) and **6** (cap only). Unknown types skip this first-pass dispatch.
- New bounded C++17 value-owned record parser and grouped first-pass engine preserve original reset omissions, model direct pose assignments and weighted accumulation, and delegate quaternion interpolation to a host callback matching the original `0x2226D210` boundary.
- Added two C++ test executables and a synthetic PE32 audit CTest. The standalone harness passes **26/26** in Release and UndefinedBehaviorSanitizer locally, plus the owner-only original-DLL binary audit.
- Details: [Phase 116 original pose-driver first pass](../research/phases/sin-original-pose-driver-initial-command-pass-20261108.md).
- Still missing: actual second-pass pose evaluation, full native skeleton animation, full `client.dll`/`server.dll` replacements, campaign execution, and a standalone Meta Quest 3 VR application. No original executable or game assets distributed.

## 2026-11-08: recursive pose-bone world matrices (Phase 117)

- Verified the entire original `server.dll` recursive pose bone-world-matrix function `0x22100F30..0x22101026` called by the second pose pass at `0x22103DC8`. It tests a 32-bit per-bone computed bitset and avoids rebuilding cached results.
- Original parent IDs come from 216-byte studio bone records; root bones multiply the external entity-root matrix by their local quaternion matrix. Child bones recursively ensure the parent world matrix, then compose parent × local, marking each resulting bone as computed.
- Added `sin_original_pose_bone_matrix` portable C++17 library, retained bitset/snapshot support, cycle and invalid input guards, a Phase 116 pose-driver integration test and new CTest targets.
- Original read-only DLL audit passes **5 SHA-pinned code regions, 8 CALL targets, 15 opcode anchors**. Local fresh ZIP extraction, Release tests and UBSan each pass 29/29. [Research](../research/phases/sin-original-pose-recursive-bone-matrices-20261108.md).
- **Still not playable**: the rest of the second pose-driver pass, animation sampling, `CStudioHdr` and MDL-cache native adapters, full `client.dll`/`server.dll` replacements, actual campaign and Quest 3 VR app remain pending.

## 2026-11-08: second pose-pass track admission and history (Phase 118)

- Binary-verified the original `0x22103D70` second pose loop: influence `+0x5C` must be >0 to evaluate a track; zero/negative/NaN skip it without updating its metadata.
- After native geometry operations, cap `+0x44` equals **1.0** marks byte `+0xA0=1`; other values set `0`. Previous sequence `+0xA8` is retained only if current sequence is nonnegative and prior index lies between `current-1` and `current`; otherwise clear `+0xA1` history and opaque DWORD `+0xA4`.
- Independently authored safe C++17 evaluator, native track-state parser, host geometry callback and Phase 116→117→118 pipeline test. Original private DLL audit: **5 code-region SHA-256, 5 relative CALL targets, 21 instruction anchors and 2 floating-point constants**.
- Local Release and UBSan research harness: **33/33 tests each**.
- [Phase 118 original pose second pass](../research/phases/sin-original-second-pose-pass-track-state-20261108.md).
- Still not a complete effect solver or runnable SiN game; the rest of the original loop, native entity animation, full `client.dll`/`server.dll`, and Quest 3 gameplay remain incomplete. No original game binaries or assets committed.


## 2026-11-09: original vehicle view-record permission byte

- Phase 123 extends the bounded 100-byte view-record decoder using the preserved SHA-pinned Phase 24 verifier: `view_allow_weapons` is the raw byte at record `+0x55` (constructor default zero), and `view_damage_mod` is the 32-bit float at `+0x60` (constructor default 1.0).
- The original SetView path loads the permission byte, passes it to the player setter, and the player-side consumer compares the stored byte against zero. The portable record and host contract therefore preserve the byte rather than narrowing it to a C++ `bool`.
- Added a decoded-record-to-SetView regression proving the selected view's permission byte reaches the player host unchanged, plus multi-record zero/nonzero parsing coverage.
- Evidence source: the Library-held `phase24_verify.py` witness table, including the exact constructor, SetView load, player setter, consumer and record-stride anchors. The raw DLL ZIP was not materialized in this run; no fresh original-binary audit is claimed.
- This completes two verified view-record fields only. Other view fields, complete damage arithmetic, native vehicle host, client monitor rendering, campaign map I/O and Quest execution remain unimplemented.


## 2026-11-11: original CreateInterface singleton RTTI/vtable inventory (Phase178)

- Chained the original 2006 server/client full-DLL PE32 hash and Phase177 export/registration audit. Recovered nine complete native 6-byte interface singleton-return factory functions, five PE-initialized C++ interface objects, five original 20-byte MSVC32 RTTI CompleteObjectLocator records and **105 original vtable method-pointer addresses** across five distinct original classes.
- Original server `ServerGameDLL003` = `CServerGameDLL` instance `0x22510650` and 31 executable prefix method slots (slot0 `0x221E7640`); original client `VClient011` = `CHLClient` instance `0x242FA4B8` and 48 executable prefix method slots (slot0 `0x240FE0E0`). Also pinned `CServerGameEnts`, `CServerGameClients`, and `CGameClientExports`.
- Critical evidence boundary: `GameMovement001` server/client, `VClientEntityList003`, and `VClientPrediction001` return singleton addresses in uninitialized `.data`; the original file cannot establish their runtime vtable pointers. The auditor explicitly rejects fabricated static entries.
- Added host-neutral C++17 reference, original-only SHA-pinned byte-level auditor and original-free mutation regressions. Reviewed original method atlas **122 -> 131 (74 server, 57 client)**. These are NOT callable native engine interfaces nor proof that the original game loads reconstructed modules.
- See `research/phases/phase178-original-engine-interface-singletons-vtables.md`. Next: original slot-0 initialization functions and real MSVC x86 engine interface callbacks. Windows PC gameplay before Android/Quest3.


## 2026-11-11: original game module Init negotiation (Phase179)

- Original server `CServerGameDLL` slot0 method `0x221E7640` (875 original x86 bytes) and client `CHLClient` slot0 `0x240FE0E0` (1,220 bytes) each **fully SHA256 verified against the exact owner-original DLL**, chaining prior Phase178 original binary/RTTI audits.
- Recovered **16 server** and **21 client** ordered mandatory engine-interface lookups. Native code publishes returned pointer to a specific global, tests null and branches to the shared `false` exit. Server `VFileSystem012` uses a distinct filesystem factory argument; client `VFileSystem012` uses the engine factory. Two late requests have no immediate null check; two more client derived-virtual lookups remain mandatory.
- Added `research/catalog/original_dll_initialization_requests.json`, a SHA-pinned owner-only audit of both whole function bodies and every interface-version/call/global/branch, ten original-free Python regression tests and C++17 native failure-injection reference tested under GCC/Clang and wired into MSVC Win32 CI.
- **Important:** This extends earlier **Phase85 original Init handshake** evidence, without recounting its two already reviewed functions. The host-neutral model covers only early negotiation and separately invoked late requests. Original helper callbacks, full native initialization, game engine acceptance and level loading are not implemented. The ledger remains **131 reviewed functions: 74 server and 57 client**.
- See `research/phases/phase179-original-win32-initialization-interface-contract.md`. Next: reconstruct post-query engine callback chain, real x86 interface vtables and original-engine-hosted Windows test.


## 2026-11-11: Original client app-system Init, rollback and physics triplet (Phase180)

- Verified **three new complete original client.dll x86 functions**, 199 bytes: `0x241640F0` app-system linked-list cleanup, `0x24164120` required vs optional app-system factory creation with whole-list rollback on required null, and `0x24182C60` physics negotiation.
- Native physics handshake requires, in order, `VPhysics030`, `VPhysicsSurfaceProps001`, `VPhysicsCollision007`, each stored into its original client global before a fatal null test. Only three successes invoke the opaque `0x241927A0` coupling callback with SurfaceProps and the original filesystem global. Original CHLClient Init caller edges and app cleanup edge are pinned.
- Owner-only original full dual-DLL SHA256 gate and Phase179 audit chain, all **three complete client function hashes, 51 opcode witnesses, 4 original exact CALL edges, 3 name/string and result-global addresses** passed. Ten original-free Python tests and local GCC+Clang C++17 `-Werror` reference tests pass; native MSVC Win32 CTest target added for CI.
- Original reviewed-function atlas **131 → 134 (74 server, 60 client)**. An invalid native success with null output would dereference null; portable model deliberately rejects it instead.
- Source: `research/phases/phase180-original-client-app-physics-postinit.md`. The original physics coupling callback, full client engine ABI, and playable original game DLLs remain unresolved. Windows PC first, then Android and Quest3.


## 2026-11-12: original client surface-properties manifest bootstrap (Phase181)

- Directly disassembled owner-original `client.dll` from 2006: **seven additional SHA256-verified complete native functions totaling 355 bytes** (manifest dispatcher `0x241927A0`, KeyValues name/child/next/search/allocator and cleanup). Original full `client.dll` and `server.dll` SHA256 unchanged and matched.
- The original manifest filename is `scripts/surfaceproperties_manifest.txt`, requested from filesystem search path `GAME`. Successful KeyValues parsing walks child nodes via +0x1C and peers via +0x18. The native entry-name comparison via `0x2425F81B` is case-insensitive against `file`; matching entries invoke a still-opaque value/string and surface-properties file loader. Unknown entry names log a warning and parsing continues. Both successful exhaustion and parse failure clean up the KeyValues root.
- Private binary auditor pins **seven complete function hashes, 56 independent instruction witnesses, 11 exact internal direct calls, one original Phase180 incoming call, two cleanup tail jumps, five exact strings and original global pathname pointer**. 11 original-free Python mutation tests plus GCC/Clang C++17 `-Werror` tests pass locally; added Win32 MSVC native CTest.
- New portable `original_client_surface_manifest.hpp` and focused tests include KeyValues native 32-bit layout offsets, host-safe failed allocation/cycle guards and explicit unimplemented engine callback boundaries. Neither the original KeyValues file parser nor surface file loader has been independently reconstructed.
- Unique reviewed-function atlas **134 -> 141 (74 server, 67 client)**. Not a completion percentage or executable game DLL validation.
- See `research/phases/phase181-original-client-surfaceprops-manifest.md`. Next recover `0x241667C0`/ `0x24191EC0` and original Windows engine-side initialization; Android / Quest3 only after Windows gameplay.


## 2026-11-12: original surface-property file read and typed KeyValues data (Phase182)

- Five additional complete 2006 `client.dll` native x86 functions **454 bytes**, independently full-function SHA256-pinned against the original client DLL: KeyValues typed value-to-string `0x241667C0`, surface file open/read/close/parse `0x24191EC0`, and guarded/unguarded surface-properties vtable wrappers `0x24192050`, `0x24192070`, `0x24192090`.
- Separately pinned original **20-byte jump table** at `0x241668A8`: tags 1..5 map to existing string, signed decimal, float, signed decimal and Win32 CP_UTF8 wide conversion. Original `GetString` uses node type DWORD +0x10, string DWORD +4, numeric DWORD +0xC, and wide source DWORD +8.
- Surface-file loading opens referenced entry path using native mode `rb`, reads file byte count, closes handle **before** passing NUL-terminated contents to the physics surfaceprops parser. Missing files print original filename+manifest warning. Actual Source filesystem/physics and KeyValues setter/find-key implementations remain callback boundaries.
- Owner-only original dual-DLL SHA256 gate and Phase181 chaining; five whole native function hashes, **48 sparse opcode + 8 indirect vtable CALL witnesses, 5 internal and 2 incoming direct CALLs**, separate type-table SHA and four original strings verified. Eleven original-free Python tests, local GCC/Clang optimized C++17 `-Werror` reference tests pass; real Win32 MSVC CTest target added.
- Unique reviewed-function atlas **141 -> 146 (74 server, 72 client)**. This is not whole-game completion or original-engine gameplay proof.
- See `research/phases/phase182-original-client-surface-file-typed-values.md`. Native file data parser, real Win32 engine startup and game-level validation still outstanding; Windows PC first, Android then Quest 3.


## 2026-11-12: original nested KeyValues lookup and setter routines (Phase183)

- Directly recovered **six complete original 2006 client.dll x86 functions totaling 609 bytes**: slash-segmented `FindKey` search/creation `0x241656E0` (267 bytes), narrow string setter `0x24165AE0` (126), wide UTF-16 setter `0x24165B60` (117), and three 33-byte tagged integer/float/opaque-DWORD setter wrappers at `0x24165BE0`, `0x24165C10`, `0x24165C40`.
- Nested lookup uses KeyValuesSystem virtual +0x0C for symbol lookup, child +0x1C / peer +0x18 / fallback chain +0x20, creation via `0x24165010` and 0x28-byte allocation, optional recursion on slash suffix, and parent type clear on child creation. String setters release prior narrow/wide values; raw setters write type-tag +0x10 and raw bits +0x0C.
- Owner-only paired original DLL SHA256 and six complete native region SHA256 checks, **54 sparse instruction witnesses, 16 internal and 4 incoming exact x86 CALL rel32 edges, two original empty literals**. Standalone native audit, eleven original-free Python mutation tests and local GCC/Clang C++17 `-Werror` tests passed. Synthetic real-MSVC Win32 CTest target added; CI result must be checked independently.
- Reviewed unique function ledger **146 → 152 (74 server, 78 client)**. The original KeyValuesSystem symbol allocator, constructor, memory ownership, whole physics file parser, live Source engine startup and gameplay are still unresolved. This is a host-neutral model and not a working original client DLL.
- See `research/phases/phase183-original-client-keyvalues-find-create-and-setters.md`; original Windows executable gameplay first, Android second, Quest3 third.


## 2026-11-12: original client KeyValues native constructor and recursive cleanup (Phase184)

- Newly SHA-pinned **three complete owner-original 2006 client.dll x86 functions totaling 231 bytes**: `0x24165010` KeyValues constructor (61 bytes), `0x24165050` scalar deleting destructor (35 bytes), `0x24166620` recursive contents destructor (135 bytes).
- Original 0x28-byte KeyValues fields: name symbol +0, narrow +4, wide +8, payload +0xC, type +0x10, peer +0x18, first child +0x1C, fallback +0x20, escape byte +0x24. Constructor **does not** initialize +0x14 or trailing bytes +0x25..0x27; it interns its name via KeyValuesSystem provider `0x24276230`, vtable +0x0C.
- Recursive destructor releases child list before peers, severs each freed node's next link before recursive destruction, special-cases peer == self, frees string buffers through original `0x2416FD60` even for null, and clears buffer pointers. Deleting wrapper conditionally frees *this* through KeyValuesSystem virtual +8 only when deletion flags bit0 is set.
- Verified original dual-DLL whole-file SHA256s, all three complete original native-body SHA256s, **40 sparse instruction witnesses, five internal and two incoming exact CALL rel32 edges**; optimized GCC/Clang C++17 tests and 11 original-free Python mutation tests pass. New Win32 x86 MSVC CTest registration pending hosted CI.
- Unique native reviewed-function atlas **152 → 155 (74 server, 81 client)**. Host model deliberately adds cycle/depth protection and severs returned host child links; original does not.
- Report: `research/phases/phase184-original-client-keyvalues-construction-destruction.md`. Engine allocator/KeyValuesSystem interface and actual Windows playable game module remain unresolved; PC gameplay first, Android then Quest 3.
