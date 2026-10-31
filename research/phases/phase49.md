> **Archival research record (phase49).** Transcribed unchanged below from `SiN_Phase49_Weapon_Script_Viewmodel_Offset_Reverse_Engineering.zip` (`SiN_Phase49/README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence, Reverse Engineering Phase 49

## Weapon-script viewmodel offset and live networked viewmodel field

**Research sources:** original `binsin.zip` (`server.dll`, `client.dll`), and optionally `binhl2.zip`, as supplied by the user. Both are Windows x86 builds. This package contains **bounded machine-code excerpts, an independent verifier, and a clean-room behavior contract**, but **does not bundle game binaries or model assets**.

## Verified new findings

**1. The weapon script parser contains `viewmodelOriginOffset`, on both client and server.**

In SiN `server.dll`, the parser at **`0x222B4827`** retrieves the script value by key name; its default argument is the literal **`"0 0 0"`** (`0x2245C6C8`). Three pointers derived from the weapon-info object at offsets **`+0x6CC`**, **`+0x6D0`**, **`+0x6D4`** are passed to the float-triplet conversion using **`"%f %f %f"`** (`0x223D4A44`). The analogous client code begins at **`0x241E69B7`**, with the same destination offsets, the same format, and the same zero-vector default. The information-structure initialization paths at `0x222B4E82` and `0x241E6FE2` also zero these three floats.

The original DLLs therefore **support a configurable three-dimensional weapon-script viewmodel origin offset**. The actual numeric values from the shipped scripts are **not present in the provided archives**, so actual Magnum, rifle and Scattergun offsets are still unknown. The parser's default is not evidence of every real weapon's configured offset.

**2. The script offset is separate from the live `CBaseViewModel` field.**

`CBaseViewModel` has the registered server-side 12-byte field **`m_viewmodelOriginOffset`** at **entity `+0x4B0`** (`0x22240CAA` and following); the `C_BaseViewModel` client has the corresponding 12-byte receive field at **entity `+0x7A8`** (`0x2409CD3C` and following). The Phase 48-recovered client viewmodel positioning function at **`0x2409C970`** consumes these three floats as a view-orientation-relative translation.

The script-info `+0x6CC..+0x6D4` and the server entity `+0x4B0..+0x4B8` are not interchangeable offsets. The matching property names alone **do not establish a transfer between these objects**.

**3. The server viewmodel has a change-aware vector setter.**

At **`0x22241420`** the server function takes three float parameters and compares each against current viewmodel fields. When values differ, it updates the component and follows the usual entity-change bookkeeping path (e.g. `or DWORD PTR [eax], 1` or a byte flag). At **`0x22241570`**, the `CBaseViewModel` constructor pushes three zero values and calls the vector setter (`0x222415D3`). The `CBaseViewModel` RTTI/vtable pointer has been cross-checked against the constructor. The client contains a distinct complete-vector comparison-and-copy function at **`0x2409D1B0`**.

An exhaustive search over *raw E8 near-call encodings* in the server `.text` region finds only the constructor's direct call to `0x22241420`. This is a narrow, useful fact: it **does not** rule out indirect calls, inlined updates, external engine code, or runtime network reception. It certainly does not prove the script field is unused.

**4. Direct HL2 comparison.**

Neither DLL in the *specific supplied* `binhl2.zip` contains a standalone literal `viewmodelOriginOffset`, `m_viewmodelOriginOffset` or `ZoomFOV`; all six string-absence assertions passed. This does **not** establish that other HL2 builds lack these names or that they could not be implemented in a compatible engine.

## What has NOT been recovered

- No proof that pressing ADS updates the live viewmodel origin-offset field.
- No verified call chain transferring `WeaponInfo +0x6CC..+0x6D4` into `CBaseViewModel +0x4B0..+0x4B8`.
- No shipped weapon `.txt` scripts or `.mdl`/`.vvd`/`.vtx` model sequences in these archives. Actual viewmodel alignment, sight axis, per-weapon ZoomFOV numbers and zoomed sequence-to-model mapping remain unknown.
- No in-game PC behavior test, Android/ARM64 build, VR-headset test or proof of network replication end-to-end.

## Quest 3 implementation consequence

Treat the weapon-script translation as *configuration metadata*, the viewmodel's networked vector as *game entity state*, and the headset/controller-derived pose as *VR presentation state*. Keep these distinct until a verified original data flow justifies linking them. Never narrow the HMD's physical projection because the original flatscreen `ZoomFOV` changes. A future adapter could apply verified script metadata to a VR viewmodel, but that would be a **port design choice** rather than a confirmed reproduction of SiN's ADS implementation.

## Reproduce

```bash
./REPRODUCE.sh /path/to/binsin.zip /path/to/binhl2.zip
```

The second archive is optional. Requirements: Python 3 standard library and a C++17 compiler. `phase49_verify.py` checks SHA-256 hashes, **19 fixed instruction signatures**, literal strings and defaults, viewmodel RTTI/vtable evidence, direct call census and the optional HL2 comparison. `tests/test_phase49.cpp` checks the **clean-room reference implementation** (not the original DLL) with 12,031 assertions.

### Contents

- `phase49_verify.py`: independent file/hash/PE VA/string/fixed-byte verifier.
- `verification.json`: recorded results on the supplied originals.
- `evidence/*.asm`: ten bounded excerpts from the original DLLs.
- `src/sin_viewmodel_offset_contract.hpp`: clean-room C++17 vector/data contract.
- `tests/test_phase49.cpp`: tests of the reference, not a game engine test.
- `REPRODUCE.sh`: performs binary verification and standalone tests.
- `SHA256_MANIFEST.json`: self-consistent package-file integrity checks.

### Proposed Phase 50

If the original SiN weapon scripts and model assets become available, directly extract each weapon's `viewmodelOriginOffset`, the zoomed idle and attack sequence mappings, and the model sight axes. Without assets, focus on more exhaustive pointer/dataflow analysis around the server viewmodel creation path and the weapon info getter; validate any claimed ADS-to-offset transfer before implementation.
