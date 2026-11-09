# Phase 144: original client viewmodel origin-offset composition

**Evidence provenance:** preserved Phase 48/49 static analysis of the original 2006 SiN `client.dll`, SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. The current run located the owner-held `binsin.zip` and SourceVR archive in Library metadata, but Project storage denied raw-byte materialization. This phase therefore reuses the preserved original-binary report and does not claim fresh disassembly.

## Original client evidence

- RTTI identifies `C_BaseViewModel`; its vtable at original VA `0x24285DF4` has slot 200 pointing to `0x2409C970`.
- The client receive property `m_viewmodelOriginOffset` is a 12-byte, three-float field at original client entity offset `+0x7A8`.
- The verified offset composition is:
  `viewmodelOrigin = inputOrigin + right * offset.x + forward * offset.y + up * offset.z`.
- The server-side `CBaseViewModel` field is a distinct 12-byte value at `+0x4B0`. The original server weapon-info script triplet at `+0x6CC..+0x6D4` is a third object and must not be conflated with either live entity field.
- The client has a separate complete-vector compare/copy routine at `0x2409D1B0`; this phase does not claim to reconstruct the receive proxy or network decoder.

## Independently authored implementation

Adds `sin_original_viewmodel_origin_offset`:

1. Accept the native host's input origin and already-derived right/forward/up basis vectors.
2. Apply the three received offset components in the original coordinate convention.
3. Return the translated origin without normalizing/reinterpreting the host basis, clamping the offset, changing FOV, or touching HMD projection.

The host remains responsible for the actual RecvTable/network update, Source basis generation, the rest of `CalcViewModelView`, model/viewmodel conditions, stereo submission and physical-controller alignment. No x86 field offset or vtable slot is dereferenced by this ARM64-portable code.

## Regression and build coverage

The focused C++17 test covers identity and rotated bases, non-unit host basis preservation, zero offsets, and negative/mixed components. The component is added to the desktop CTest suite and both Android NDK build scripts; the Bionic x86_64 emulator job executes the same portable math test with no engine host.

## Explicit limits

- This is one verified client render-math stage, not a complete viewmodel renderer or network replication implementation.
- Original shipped per-weapon offset values and model/sequence assets are not present in the preserved report.
- The evidence does not establish that ADS changes `m_viewmodelOriginOffset`; do not use this vector as a guessed sight calibration.
- No SourceVR game module, stereo renderer, OpenXR session, campaign map, APK, or Quest 3 execution is produced by this change.
- No proprietary DLL, APK, ELF, BSP, model, map, script, sound, or game asset is committed.

Keep this change stacked and draft; do not merge without explicit user instruction.
