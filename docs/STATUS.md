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
