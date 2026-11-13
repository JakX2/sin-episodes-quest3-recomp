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


## 2026-11-12: original client tier0 memory allocator boundary (Phase185)

- Directly verified **seven distinct complete native client.dll x86 functions covering 262 bytes**, original VAs `0x2416FCF0`, `0x2416FD40`, `0x2416FD60`, `0x2416FD80`, `0x2416FDD0`, `0x2416FDF0`, `0x2416FE10`. All complete original code-region SHA256s match the owner-supplied 2006 DLLs. Two pairs are byte-identical forwarding wrappers at distinct entrypoints.
- Original PE32 **import directory** confirms `tier0.dll!g_pMemAlloc` at IAT VA `0x242761CC` and `vstdlib.dll!KeyValuesSystem` at `0x24276230`. These are imported engine-library services, not implementation inside the SiN DLL.
- Verified original tier0 allocator vtable byte slots **+0** three-argument allocation, **+4** ordinary allocation, **+0x14** unconditional pointer free. Lazy shared debug-context bit0 flag at `0x2437290C` and cached resolver result at `0x24372908`; original resolver `0x2416F7F0` remains unported. Original resolver CALLs at `0x2416FD03` and `0x2416FD93` both verified.
- Added host-neutral C++17 reference tests, including integration through the prior Phase184 KeyValues destructor; synthetic callback test verifies child-before-parent buffer frees, null forwarding, one-time cached owner setup and exact argument values. Local GCC+Clang `-Werror` passes. New owner-only original PE import/function auditor verifies seven full hashes, **37 sparse opcode witnesses and two rel32 calls**; **11 original-free Python grouped tests pass**. Actual Win32 MSVC x86 CTest target added.
- Reviewed unique function atlas **155 → 162 (74 server, 88 client)**. The original `tier0.dll` allocator implementation, `vstdlib.dll` KeyValues internals, full engine ABI and playable original Windows modules are still *not* recovered. See `research/phases/phase185-original-client-tier0-memalloc-bridge.md`. Windows PC gameplay first, then Android and Quest 3.


## 2026-11-12: original client Win32 debug filename resolver and tier0 cache variants (Phase186)

- Full original 2006 `client.dll` SHA256 and `server.dll` SHA256 matched; **four original complete native x86 function regions, 314 total bytes**, verified: `0x2416F7F0` owner resolver (90), `0x2416F850` primary cached size-in-ESI bridge (71), `0x2416F8A0` independent secondary two-register cache (74), `0x2416F8F0` primary cached stack-size bridge (79).
- The original resolver calls imported `tier0.dll!g_pMemAlloc` vtable **+0x5C** as a gate; if true, performs `GetProcessHeap`, `HeapAlloc(heap,0,0x104)`, `VirtualQuery(&global_0x24372904, &MBI,0x1C)`, and `GetModuleFileNameA(MBI.AllocationBase,heap_buffer,0x104)`. Returns the heap buffer, irrespective of GetModuleFileNameA result; does **not** guard HeapAlloc or VirtualQuery errors. This identifies Phase185's previously opaque cached owner as Win32 module-filename storage, not a `tier0`-allocated buffer.
- Primary cached context at `0x24372908` with flags `0x2437290C`; secondary at `0x24372910` with flags `0x24372914`. Original two-register wrapper dispatches through tier0 vtable **+0x0C** for plain two-operand call or **+0x08** with filename/line appended; no unverified C++ method name asserted.
- Native auditor verifies **four complete body SHA256s, 48 sparse opcode witnesses, five exact rel32 CALL edges (including Phase185 callers), five real PE import directory names/IAT VAs**. Local 11 Python mutation groups and optimized GCC/Clang C++17 `-Werror` passed. Added native MSVC Win32 model test and separate **real 32-bit kernel32 API** smoke using a synthetic EXE module anchor (not original SiN runtime).
- Reviewed unique function atlas **162 → 166 (74 server, 92 client)**. Synthetic Windows API smoke is **not** proof of the original engine accepting replacement game DLLs. See `research/phases/phase186-original-client-debug-owner-win32.md`. Next: original-engine-hosted Windows loading and engine callback vtable integration before Android/Quest3 gameplay.


## 2026-11-12: original server post-emitter game-system Init and repository gates (Phase187)

- Direct owner-original SHA256 verified **five complete 2006 server.dll native x86 routines, 355 bytes**: `0x221F5700` game-system registration/Init (277 bytes), `0x221AD8A0` (10-byte repository A singleton tail thunk), `0x22149B60` (10-byte repository B thunk), `0x221E6450` (35-byte early developer ConVar helper), and `0x221F56E0` (23-byte array forwarder).
- Original `CServerGameDLL::DLLInit` after the sound-emitter gate calls game-system Init at `0x221E7927`, then repository A thunk at `0x221E7930`, then repository B thunk at `0x221E7939`. Three exact native short JZ failure branches lead to common false return `0x221E79A1`. The singleton thunks set ECX to `0x22503EE0` / `0x224F0388` and JMP to original opaque parsers `0x221AD3E0` / `0x221498C0`.
- The game-system list at `0x22572CC0` deduplicates by native pointer against array `0x22510A58`, logs exact duplicate warning, grows original capacity when necessary, **clears pending head and sets init flag `0x22572CB8` before callbacks**, then calls each system's virtual slot zero; a failed system stops without rolling back the flag or completed callbacks.
- Original Init resource strings `resource/gameevents.res` and `resource/ModEvents.res`, `developer` and `sv_cheats` byte-verified. Private original dual-DLL hash gate and all **five complete function hashes, 37 opcode witnesses, nine exact CALL edges, two exact tail JMPs, three shared failure edges, five literal strings and original global locations** pass. Twelve original-free Python mutation tests and GCC/Clang optimized C++17 `-Werror` reference model pass locally. Native MSVC Win32 CTest wired for hosted CI.
- Reviewed unique-function atlas **166 -> 171 (79 server, 92 client)**. The two repository parser bodies, complete server Init, actual Source event interface vtables and original-engine-hosted Windows game DLLs remain unresolved. Passing synthetic tests does not demonstrate playable SiN. See `research/phases/phase187-original-server-post-emitter-auto-systems.md`. Windows gameplay first, Android then Quest 3.


## 2026-11-12: original server scene/model-sounds cache loaders (Phase188)

- Fully SHA-pinned against owner-original 2006 `server.dll`, **four native x86 regions totaling 1,312 bytes**: `scene.cache` Init `0x221AD3E0` (576), `modelsounds.cache` Init `0x221498C0` (576), and reload wrappers `0x221AD620`, `0x22149B00` (80 each). Both entire original DLL SHA256s verified.
- The two 576-byte routines have **identical complete opcode layout after normalizing seven CALL rel32 displacements each**; both 80-byte reload wrappers normalize to an exact common body after five calls each. Exact normalized region SHA256s pinned. The original scene object `0x22503EE0` points to `scene.cache` and expects repo version **3**. Model-sounds object `0x224F0388` points to `modelsounds.cache` and expects version **2**.
- Verified original cache file header: 4-byte system version **2**, 4-byte per-repository version, 4-byte metadata checksum and signed record count, opened from filesystem path ID `MOD` with mode `rb`. A missing cache file is **nonfatal** and marks needs-regeneration; a bad version or nonzero metadata mismatch closes then invalidates file and returns true; a missing configured pathname returns false after setting initialized flag. Reload wrappers have conditional cleanup/rebuild and tail-jump back into Init.
- Private native audit checks **four complete function hashes, 98 independent sparse opcode witnesses, 24 exact direct calls, four reload-to-Init JMP edges, eight original strings, both object pathname pointers/version DWORDs**, and full 576/80-byte normalized sibling SHA. Host-neutral C++17 model/test passes GCC/Clang `-Werror`; 13 original-free Python mutation tests pass. Windows x86 MSVC CTest target added.
- Unique reviewed function atlas **171 → 175 (83 server, 92 client)**. Per-record decoding helpers, concrete Source filesystem vtable and actual original Windows game startup are **not** implemented. See `research/phases/phase188-original-server-scene-modelsounds-cache.md`; Windows first, Android second, Quest 3 last.


## 2026-11-12: original server cache-record identity lookup/insertion (Phase190)

- Original owner-provided SiN 2006 PE32 server/client whole-file SHA256 verified. Two **new complete server.dll native x86 functions** hashed: scene record helper `0x221ACFD0` (190 bytes), model-sounds record helper `0x22149770` (187 bytes).
- Both helpers use the filesystem global `0x225721A4` and vtable byte offset **+0xD4** with the incoming file handle, lookup existing index and test sentinel **0xFFFF**. The scene insertion allocates **28 bytes**, model sounds **24 bytes**; each sets its distinct native vtable on non-null memory. Both publish an inserted record pointer into a 24-byte index entry at **+0x14**.
- Original auditor validates both full code hashes, **34 sparse opcode witnesses**, **8 outgoing + 2 incoming exact rel32 CALLs**, original `ret 4` and static vtable locations. Native original-file audit and **7 original-free Python mutation groups** pass locally. C++17 synthetic branch model passes local optimized GCC/Clang `-Werror`. Native Win32 MSVC CTest registration added; hosted Windows CI pending.
- Unique-reviewed atlas **175 → 177 (85 server, 92 client)**. Filesystem vtable +0xD4 semantics, internal lookup/insert helpers, the serialized record format and original executable-hosted replacement DLLs remain unimplemented. Windows PC first, then Android, then Quest 3.
- See `research/phases/phase190-original-server-cache-record-identities.md` (draft stacked on Phase189).


## 2026-11-12: original scene/model-sounds cache tree lookup pair (Phase191)

- Two newly reviewed complete original **server.dll** functions, each **260 x86 bytes**: scene tree lookup `0x221A7FB0` and model-sounds lookup `0x22142670`. Both original full function SHA256s and original full `server.dll` / `client.dll` SHA256s verified against private owner Drive files.
- Both methods are fully byte-identical after normalizing **only 14 native absolute DWORD addresses** pointing at their respective static sentinel records, normalized whole-body SHA256 `9dc7a270eed6024f374820355e3097d25f96d64547b3c38d29d25c502732ec41`.
- Observed root field `+0x10` unsigned 16-bit, `0xFFFF` not-found sentinel, 24-byte indexed node stride, child indices at `+0/+2`, node key address at `+8`, indirect two-argument comparator at object `+0`, and lazy distinct `.data` sentinel setup.
- New strict original-only auditor validates **72 sparse opcode witnesses**, four indirect comparator dispatches, four `ret 8` exits, 28 absolute address references, and boundaries. Eight original-free Python mutation groups and optimized GCC/Clang C++17 synthetic branch model pass locally. Windows x86 MSVC CTest target registered, hosted result pending.
- Reviewed native atlas **177 → 179 (87 server, 92 client)**. Key format, actual comparator callback, original tree allocator and original-engine DLL execution remain opaque. Windows PC first, Android second, Quest 3 third.
- See `research/phases/phase191-original-server-cache-tree-lookups.md` in the stacked draft branch.


## 2026-11-12: original scene/model-sounds unsigned key comparators (Phase192)

- **Two new exact original 2006 server.dll native comparator functions**: scene `0x221A2F40` and model sounds `0x2213A3F0`, **17 bytes each** and **byte-identical**. Both complete SHA256 `77b87ba9017ac5403ea41c488944263dd6475f82ddf3b8f28b36155b9e8f15f4`; original full server/client DLL hashes verified privately against Drive binaries.
- Native singleton tree `+4` pointers verify exact callback association: `0x22503EE4 -> 0x221A2F40`, `0x224F038C -> 0x2213A3F0`. The comparator reads **only the first DWORD of each pointed-to key** and returns **unsigned lhs < rhs as 0/1**. Exact native `cmp/sbb/neg` semantics proven, including bit31. The original routine returns with plain `ret`, caller cleans two arguments.
- Host-neutral C++17 reference plus Phase191 tree integration tests pass optimized GCC/Clang `-Werror`. Original-only Python auditor checks complete method hashes, both 15-byte INT3 boundaries, 10 sparse opcode witnesses and both data pointers. Eight original-free mutation test groups pass. Root and Win32 x86 CTest targets registered; hosted CI pending.
- Unique reviewed functions **179 → 181 (89 server, 92 client)**. Native filesystem virtual `+0xD4` and complete record payload encoding still opaque; replacement DLL gameplay not achieved. Windows PC first, Android second, Quest 3 third. See `research/phases/phase192-original-server-cache-key-comparators.md`.


## 2026-11-12: native scene/model-sounds cache tree insertion wrappers (Phase193)

- **Two SHA-verified complete original 2006 server.dll x86 functions**, scene `0x221ACD00` and model sounds `0x22146EF0`, each 112 bytes. Distinct method hashes verified against the original SHA-matched 2006 server/client DLLs. Both complete bodies become byte-identical after normalizing only three direct CALL rel32 operands in each; normalized SHA256 `dd1f7514e89298739816f025baf317f3b87424ee8a637cda3ed3289acecf2725`.
- Original insert path invokes three repository-specific helper calls, increments 16-bit tree count at `+0x12`, indexes a 24-byte node using the **low 16 bits** of the returned index and copies 16 bytes of key from input into node `+0x08` through `+0x17`. Phase190 subsequently writes a payload pointer at node `+0x14`, overwriting the fourth key DWORD. Unported allocator/linker remain host callbacks.
- Owner-only auditor covers **52 sparse instruction witnesses and eight exact CALLs**, both complete native hashes and normalized twin check. Original private DLL audit passes. Eight original-free Python mutation groups, and optimized GCC/Clang C++17 integrated tests pass locally. Root CTest plus native MSVC x86 test registered; hosted CI to be checked independently.
- Unique reviewed atlas **181 → 183 (91 server, 92 client)**. Original filesystem virtual `+0xD4`, record serialization and game-loadable replacement modules remain unresolved. Windows PC first, Android second, Quest3 third. See `research/phases/phase193-original-server-cache-tree-insertion.md`.


## 2026-11-13: original scene/model-sounds cache tree slot allocators (Phase194)

- Two new full original SiN 2006 server.dll native x86 functions verified from SHA-matched owner files: scene slot allocator 0x221A6890 (119 bytes), model-sounds allocator 0x22140900 (119 bytes), **238 additional original native bytes**. Both complete bodies are byte-identical after normalizing only two CALL rel32 displacement DWORDs per function, full normalized SHA256 66d48e1aaa823eeb953c3c70acf9c8095bb601326fe1bad696dd517266d15a51.
- Native tree fields pinned: +0x08 array capacity DWORD, +0x14 free-list head WORD, +0x16 next-unused WORD, +0x18 base pointer snapshot DWORD. If free list is empty, conditionally grow by one and increment next-unused; otherwise reuse head and read next from freed node +2. Both paths initialize 16 bytes at 24-byte slot +8 to 0/0/0xFFFFFFFE/0xFFFFFFFF; Phase193 subsequently writes the 16-byte key.
- Owner-only auditor verifies both complete DLL and original function SHA256s, 40 sparse opcode witnesses, 4 outgoing and 2 incoming CALL edges, 9-byte INT3 boundaries and complete normalized twin bodies. Private native audit PASS; 8 original-free Python mutation groups PASS; optimized local GCC/Clang C++17 contracts PASS. Added root CTest and Win32 x86 MSVC test targets; hosted CI to be checked independently.
- Reviewed native atlas **183 → 185 (93 server, 92 client)**, not whole-DLL completion. Growth, freed-node helper, native engine filesystem +0xD4 and actual Windows game-hosted replacement DLLs still unported. See research/phases/phase194-original-server-cache-slot-allocation.md. PC before Android before Quest 3.


## 2026-11-13: native scene/model-sounds cache node and static sentinel resolvers (Phase195)

- Inspected owner-original SHA256-matched SiN 2006 server/client DLLs. Two **new full original server x86 functions**: scene node resolver `0x221A4D90` (84 bytes), model-sounds resolver `0x2213E990` (84 bytes), **168 verified native bytes**. Complete bodies become byte-identical after normalizing only seven original absolute DWORD addresses each, normalized SHA256 `2eab8413dc78ddc215c2c60be186262755adfa638f84e5de6c73be2725a69a1c`.
- The resolver tests bit 0 of a module-static flag, lazily initializes a separate 8-byte sentinel to WORDs `FFFF,FFFF,FFFF,1` before checking the index, then returns either that sentinel address for index `0xFFFF` or the native 24-byte indexed slot pointer. Phase194 consumes WORD `+2` to advance the free-list head. The original lacks a bounds check; the host version adds one and avoids treating the 8-byte sentinel as a full slot.
- Original-only auditor passes both complete DLL and native method SHA256s, checks **32 sparse x86 witnesses**, **14 exact address operands**, two original Phase194 incoming CALL edges and both native boundaries. Eight original-free Python mutation groups and optimized GCC/Clang C++17 Phase194 integration tests pass locally. Root CTest and native Win32 x86 CTest registered; hosted CI checked separately.
- Reviewed original native functions **185 → 187 (95 server, 92 client)**. This is research coverage, not playable game progress. Original vector growth, tree linking and Source filesystem +0xD4 remain incomplete. Windows-first engine startup tracing is the next integration milestone.
- Research: `research/phases/phase195-original-server-cache-node-sentinel-resolver.md` (stacked draft).


## 2026-11-13: paired original server cached-tree link wrappers (Phase196)

- Read-only original SHA-matched 2006 SiN PE32 DLL inspection identifies **two new complete original server functions**, scene `0x221AA890` (109 bytes) and model-sounds `0x221441B0` (109 bytes). Their complete bodies normalize byte-identical after removing only three native balance-helper CALL rel32 operands per function; normalized SHA256 `2b1dd748abfd04da2818f807c2ce84be6be4e28aef95c8d92f8ad6b41573d461`.
- Native link implementation sets new 24-byte slot's child WORDs at +0/+2 to `0xFFFF`, parent WORD +4, flag WORD +6 to zero; sets root +0x10 when parent missing, otherwise links first child +0 for nonzero selector or second child +2 for zero. It preserves slot key/payload. It passes the **full 32-bit index** into a repository-specific opaque balance function after indexing slots with the low 16 bits.
- New C++17 host-neutral model and Phase191/192/193 integration tests; original-only SHA auditor checks **36 sparse native opcode witnesses**, six outgoing balance calls, two incoming insertion-wrapper calls, full method SHA256s and post-return INT3 boundaries. Eight independent original-free mutation groups included. Hosted CI must be observed on PR #249 before claiming success.
- Unique reviewed method atlas **187 -> 189 (97 server, 92 client)**. Real balancing logic and Windows retail-engine startup remain unimplemented. Windows PC first, Android second, Quest 3 third.
- See `research/phases/phase196-original-server-cache-tree-link.md`.


## 2026-11-13: original scene/model-sounds tree insertion balancers (Phase197)

- Owner-original 2006 `server.dll` research recovered **two complete 1,387-byte x86 function bodies**, scene `0x221A8F20` and model sounds `0x221429D0`, totaling **2,774 new complete original function bytes**. Whole original DLL and individual function SHA256s pin both functions. The complete methods become byte-for-byte identical after normalizing only **85 embedded static address operands and 8 direct-call operands per function**, normalized SHA256 `2883dbef8df10e4d3cdf27e81d0b513c2b4c5d71467fc24618c880a0bce2c683`.
- Verifiable native behavior: root-at-`+0x10` fast path; parent-color WORD at slot `+0x06` checked for nonzero; common exit forces root color WORD **1** (black). Remaining red-parent cases use grandparent/uncle recoloring and two distinct rotation-like helper methods per repository, **not yet implemented**.
- A fail-closed C++17 prefix model exercises only root and already-black-parent cases; it **refuses red-parent paths** rather than fabricating balancing. The owner-only original-function auditor validates full hashes, 30 sparse x86 witnesses, 170 exact static references, 16 outgoing and 6 incoming direct CALL edges, post-`ret 4` alignment and full normalized twin identity. Binary-free Python mutation regression and native Win32 CTest targets added. Hosted CI to be checked on this draft branch.
- Reviewed native function atlas **189 → 191 (99 server, 92 client)**. The helper algorithms and engine-hosted PC gameplay are not yet complete. Next audit rotation-like functions `0x221A6BB0 / 0x221A6CA0` (scene) and `0x221409B0 / 0x22140AA0` (model sounds). See `research/phases/phase197-original-server-cache-tree-balancing-evidence.md`.


## 2026-11-13: original four cache-tree left/right rotation helpers (Phase198)

- Original owner-provided SiN 2006 PE32 `server.dll` SHA256 matched. Four new complete server native functions: scene left `0x221A6BB0` (237 bytes), right `0x221A6CA0` (240 bytes); model-sounds left `0x221409B0` (237 bytes), right `0x22140AA0` (240 bytes). Complete SHA256s and every native `ret 4` boundary verified. **954 new original function bytes**. Each direction's scene/model-sounds methods are byte-identical after normalizing only **nine original resolver CALL rel32 operands** per method.
- Real native tree rotation behavior: promote x.right (left rotation) or x.left (right rotation), move the promoted node's opposite child (beta), rewrite beta's parent, adjust x's old parent link or root `+0x10`, and set promoted child/x parent relationships. Original 24-byte node's key, payload and color WORD are preserved. The native code has no bounds guard; portable model validates before writes.
- Original-only auditor checks four complete original x86 hashes, two normalized whole-function pair SHA256s, **72 opcode witnesses, 36 exact resolver CALLs and eight Phase197 incoming rotation CALLs**. Root and Windows x86 CTest plus eight mutation-test groups added. Portable model intentionally does not simulate the separate resolver's lazy static initialization or finish Phase197 red-parent balancing loop.
- Unique reviewed native function catalog **191 -> 195 (103 server, 92 client)**, a research inventory not playable progress. Remaining priorities: recover full red-parent fixup and test actual Windows-engine startup with owner-provided original DLLs. See `research/phases/phase198-original-server-cache-tree-rotations.md`. Windows PC -> Android -> Quest 3.


## 2026-11-13: portable end-to-end cache tree fixup composition (Phase199)

- Added a **C++17 red-black insertion-fixup reference model** built from original SHA-reviewed 2006 SiN server.dll node layout (Phase191), link wrapper (Phase196), color/root fast paths (Phase197) and actual left/right rotation behavior (Phase198).
- Handles root/black-parent, red-uncle recoloring, and LL/RR/LR/RL rotation cases. Transactional host-side validation prevents partial updates of corrupt synthetic trees; original x86 has no equivalent guard.
- Tests cover both original cache configurations and **1,440 six-key insertion-order trees** plus branch-specific shapes, with structural, black-height, BST, key/payload and Phase191 lookup invariants. Root CTest and native Win32 x86 CTest registered, CI checked separately.
- **Important provenance:** This completes a useful *portable red-black algorithm*, NOT the fully decompiled/behaviorally proven 1,387-byte Phase197 native fixup. No new original code region is claimed. Reviewed original-function atlas remains **195 (103 server, 92 client)**.
- Retail Windows startup has not been tested with reconstructed gameplay modules; next prioritize original engine launch trace and differential native red-parent testing. See `research/phases/phase199-original-server-cache-balance-composition.md`.


## 2026-11-13: independent cache-balancing stress and concurrent Phase199 reconciliation (Phase200)

- Detected concurrent hourly draft **PR #252** implementing the same Phase199 red-black composition; closed overlapping draft **#253** without merging or overwriting either branch. Phase200 is stacked **on #252** and changes only independent tests, CMake registrations and documentation.
- Tested the canonical Phase199 candidate via Phase196 native-layout link and Phase198 rotation models using **40 deterministically shuffled 64-key insertion sequences**, across both scene and model-sounds caches. Checks after every insertion: unsigned BST ordering (including `0`, `0x7FFFFFFF`, `0x80000000`, `0xFFFFFFFF`), all nodes reachable, parent/child reciprocity, equal black heights, black root, no red/red edges, and preservation of payload and opaque record bytes. Alternates raw32 and truncated low16 index behavior.
- Adds deliberately corrupt upward parent chain causing one valid recoloring round *before* an exception, verifying synthetic reference's **transactional rollback** with no caller tree writes. Local optimized GCC/Clang strict `-Werror` tests passed against matching headers; hosted Linux/Windows x86 CI checked independently.
- No new original native method recovered. Reviewed SHA-verified function inventory stays **195 (103 server, 92 client)**. Neither this stress test nor Phase199's conventional red-black algorithm proves exact 2006 x86 red-parent branch equivalence or playable Windows engine hosting. See `research/phases/phase200-original-server-cache-balance-stress.md`. PC first, Android next, Quest 3 last.


## 2026-11-13: original cache array growth and key descriptor initialization (Phase201)

- This draft stacks on concurrent canonical Phase200 PR #254 (64-node balance stress) and does not overwrite concurrent work. Owner's SHA256-matched original SiN 2006 DLL binaries reveal four complete previously unreviewed server functions: scene `0x221A3A10` / model-sounds `0x2213AAC0` **119-byte vector growth** methods and scene `0x221A3A90` / model-sounds `0x2213AB40` **36-byte key initializers**, **310 new original x86 bytes**. Full original code hashes verified privately.
- Verified array subobject starts at tree `+0x04`, with backing pointer +0, capacity DWORD +4, growth policy DWORD +8 (native tree `+0x0C`). Policy `0xFFFFFFFF` skips growth; zero policy doubles existing capacity; positive policy adds fixed steps; empty vector starts at capacity 2. Regardless of whether capacity increases, native calls either heap allocate or reallocate with **24 × capacity** bytes. Native heap targets `0x2226E390 / 0x2226E400` are not yet recovered.
- Original-only verifier pins both whole DLL hashes, **4 exact complete native function hashes, 52 opcode witnesses, 4 outgoing heap CALLs and 2 incoming Phase194 calls**, twin normalized growth bodies and byte-identical key initializers. Eight original-free mutation groups, local optimized GCC/Clang tests passed. Cross-phase C++17 Phase194 slot allocation uses the newly recovered growth policy; hosted root and MSVC Win32 x86 CI to be checked.
- Native reviewed-function atlas **195 → 199 (107 server, 92 client)**; research coverage, not gameplay-completion percentage. Actual Windows engine game module load, cache record inputs and allocator ownership remain unresolved. Windows PC first, Android second, Quest 3 third.
- Evidence: `research/phases/phase201-original-server-cache-array-growth-and-key-init.md`.
