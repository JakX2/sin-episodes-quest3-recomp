# Phase 102: sound-provider epoch safety and pinned native sound ABI evidence

## Reproduced Phase101 bug

Original Phase101 `sin101_register_provider` changes the mock callback table and advances an epoch, **but leaves legacy→native sound token mappings made by the previous provider in Phase100**. The new `sin102_provider_lifecycle` regression fails against the unmodified Phase101 binary when it first tries to look up an obsolete mapping, then succeeds with this patch. This is a real cross-provider namespace contamination hazard.

## Correction without ABI changes

Phase102 keeps the **same 12 public `sin101_*` and 11 `sin100_*` functions**, their versions and signatures. Each `sin101_context` now owns **two independent underlying Phase100 contexts**:
- `legacy` for provider-specific sound tokens;
- `keyboard` for the separately SHA-verified original 2006 VGUI_System010 keymap.

Successful mock-provider registration, including re-registration of the same callbacks, first marks the provider inactive and advances the epoch, **resets only the sound context**, then activates the new provider. If resetting fails, registration fails closed. Rejected unverified real SourceVR providers never clear the old state. Full `sin101_reset` clears both state compartments; destruction releases both contexts before unloading the linked library.

The previous callback epoch guard is retained. Callbacks run outside the mutex and a callback-triggered provider replacement invalidates its in-flight result without deadlock. The new test also exercises 200 concurrent-mutation iterations with a parallel reader. The callback's userdata and whole context lifetime must still be managed externally: **concurrent context destruction and asynchronous userdata disposal are not supported**.

With the owner's private SHA-verified original keymap, **key 48 returns 1, key 8 returns 66 and key 49 returns 2 even across sound provider changes**. No original binary or derived keymap bytes are distributed.

## Actual 2006 vs AArch64 SourceVR sound implementations

The new read-only `audit_sound_candidate_abi_phase102.py` verifies exact whole-file fingerprints:

- Original 2006 `SoundEmitterSystem.dll`: `8c92cdc58f366cde07a1e14b8d95bfc36abdb552242508993189479faa79dba5`.
- Native SourceVR 0.5.5 `libsoundemittersystem.so`: `484e5bb2c1457ee6e62f39d47c83bc68b8cee491c2f1479886a8e0d93740c454`.

It pins **five complete code regions** (original VSoundEmitter001 slots 3 and 6; native VSoundEmitter002 candidate slots 7 and 10 plus validator slot 8), **17 AArch64** and **4 x86** instruction anchors.

AArch64 candidate slot 7 at `0x133A0` receives a name pointer in `X1`, hashes and looks it up and returns a 16-bit native index or -1 in `W0`. Candidate slot 10 at `0x13570` takes an index in `W1`, calls the native validator via vtable byte offset `+0x40`, masks the index to 16 bits and obtains a char-like pointer from a native table at object offset `+0x48`, or returns a fallback pointer.

These observations **do not prove complete method signatures, object ownership, string pointer validity after mutation, concurrency safety or matching original/native token numbering**. The native provider still returns `SIN101_NATIVE_ABI_NOT_VERIFIED`; **no live engine-vtable call is enabled**.

## Files

- `src/source_provider_c_api.cpp`: two underlying Phase100 contexts, epoch invalidation and preserved keyboard map.
- `tests/sin102_provider_lifecycle.cpp`: red-before/green-after regression, provider A→B→A, rejected native providers, callback-triggered replacement, concurrent reads, optional private keymap, full reset.
- `research/tools/audit_sound_candidate_abi_phase102.py`: standalone stdlib-only read-only PE32/ELF64 auditor, SHA/ISA verification, metadata-only output.
- `research/tests/test_phase102_native_abi.py`: eight synthetic test groups, every instruction/site tampering and SHA guards.
- `research/phases/sin-phase102-sound-provider-epoch-abi-evidence.md`: this report.
- `CMakeLists.txt`: desktop CTest and Android executable targets; Python synthetic unit checks.
- `scripts/build_android_arm64.sh` and `scripts/build_android_emulator_x86_64.sh`: build and verify the new lifecycle executable, linked to the existing provider library.
- `.github/workflows/ci.yml`: repair a **pre-existing Phase101 misindented command** and run the provider A→B regression on the real Bionic x86_64 emulator.

## Verification and limitations

- The **unmodified Phase101** standalone source fails the new lifecycle test on its first stale sound mapping assertion.
- The corrected source passes **9/9 local CTest targets**, including the earlier Phase101 C/CPP/Python tests. The owner's 1024-byte keymap was privately reconstructed, SHA-verified and tested.
- Original and native sound DLL **read-only audit PASS**.
- Original Phase100 C ABI remains 11 exports; Phase101 12 exports. No new public functions.
- GitHub CI is tracked separately after opening the draft PR. A Bionic x86_64 emulator pass is *not* a Meta Quest 3 ARM64 gameplay test.

This phase **does not** implement native SiN `libclient.so`/`libserver.so`, a real VSoundEmitter001→002 adapter, SourceVR game initialization, APK or playable Quest 3 content.

**Next:** Phase103 should add an explicit provider lifetime/teardown protocol that cannot leave dangling callback userdata, then safely bind verified native sound methods only after their complete signatures and ownership rules are established. Independently investigate original VGUI InputInternal slot56 for the client-side adapter.
