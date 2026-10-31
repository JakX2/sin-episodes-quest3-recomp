# Phase 61: `InputHideWeapon` owner/active-weapon gate and virtual dispatch

**Status:** corrected in Phase 62 after independent constructor-vptr/RTTI validation; original-binary static verification, 31 October 2026. Reproduced against SHA-256-pinned original SiN Episodes (2006) Windows x86 `server.dll` and `client.dll`. These are *preferred image-base VAs in the Windows DLL*, not native Android addresses. No original binaries or disassembly dumps are committed.

## New original-binary result

Phase 59 identified the server `CBaseCombatWeapon` datamap input `InputHideWeapon` / external name `HideWeapon` at **`0x222AD800`**. Phase 61 traced the input handler's outgoing path and the helper it directly calls:

1. The handler retrieves a handle from **weapon object `+0x494`**, rejects sentinel `-1`, and resolves it through the entity-handle table referenced at **`0x224F5620`**. The original code masks the low 12 bits for the entity index and shifts right by 12 for the serial/generation check; a stale serial or null entity exits the handler.
2. It calls a virtual method at **`owner->vtable +0xFC`**, checks the returned pointer, then performs a *second* guarded owner-handle resolution and calls the same virtual slot again. The verifier asserts both call sites; the purpose of the accessor is inferred from data flow, not recovered symbolic naming.
3. The handler calls **`0x222AA640`** at **`0x222AD87D`**. That helper retrieves an entity handle from its argument at **`+0x78C`**, validates it against the same entity-handle table and returns a resolved pointer or null. The `+0x78C` pointer is therefore an **inferred current/selected weapon** reference, not an unverified native ARM64 layout offset.
4. The handler compares the resolved weapon pointer to the original `this` (`esi`) at **`0x222AD882`**. A mismatch branches out, with no dispatch. Only on equality does it push immediate **zero** at **`0x222AD888`** and make a virtual call through **`weapon->vtable +0x350`** at **`0x222AD88C`**.
5. **Phase 62 correction:** the primary constructor-installed **`CBaseCombatWeapon` vtable starts at `0x2245A554`**, not `0x2245A578` (the latter is exactly nine entries / `0x24` bytes into the table). This is independently confirmed by the constructor store at `0x222AE399` and the MSVC RTTI complete-object-locator immediately before the primary vtable. The actual `+0x350` (zero-based slot 212) target is **`0x222AE960`**, not `0x222AF450`. The former tests the handler's zero argument and manipulates entity effect bit `0x20`, including an `AddEffects`-like path at `0x2214C840`. The earlier connection to the reload-flag reader was **incorrect**; `0x222AF450` belongs at slot **221** in this actual primary table. Phase 62 separately checks derived firearm vtables and the effect bit semantics.

## Why this matters for a SourceVR adapter

The name `HideWeapon` **must not** be interpreted as a free-standing unconditional `weapon.visible = false` operation based on this evidence. The original handler has explicit owner-handle validity, generation checks, an owner/player accessor, an active-weapon identity check, and a conditional dispatch. In the corrected base weapon vtable, the virtual slot points to the entity-effect visibility path, not the shared reload-control implementation. Calling it a hide or holster method solely by name would be unsupported.

The future native SourceVR integration can represent these gates using **native safe handles and explicit host interfaces**, without copying original 32-bit entity/object memory offsets. Do not implement the virtual dispatch's ultimate gameplay action until class-specific overrides and callers have been identified. In particular, no headset-FOV modification is implied by this input.

## Independent reproducibility

With a legally owned copy of the exact original build and a checked-out branch containing Phases 56–60:

```sh
python3 research/tools/verify_weapon_hide_input_gate.py /path/to/SiN/server.dll /path/to/SiN/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The corrected verifier **first runs the Phase 60 verification chain**, including pinned server/client hashes and all prior field/registration checks, then validates **31 exact opcode anchors** across the handler, selected-weapon helper and corrected base virtual target. It also decodes the relative call to `0x222AA640` and checks the vtable's pointer value. The 11 synthetic Python regression tests mutate guards, owner calls, selected-weapon checks, argument bytes, the hide-effect call, dispatch slot, and vtable pointer; synthetic tests contain no original DLL.

**Original Phase 61 local results (31 October 2026):** 51 Python regression tests passed across Phases 56–61, but the vtable address was identified incorrectly. **Phase 62 reruns** the corrected verifier and suite; see Phase 62 report for the new verified test counts. The independent C++ build and CTest are verified separately in this phase's publication log. GitHub Actions status must be checked on the corresponding PR.

## Limits and next steps

- **Not** a complete runtime call graph or alias-aware proof about all writes to `m_bReloadsSingly`; the flag's normal-play setter remains unknown.
- **Not** proof of live engine execution. The corrected slot targets a visibility-effects implementation, while Phase 62 verifies which concrete Magnum, Assault Rifle and Scattergun vtables share it.
- **Not** proof that `InputHideWeapon` reaches all weapons without validating its entity I/O wiring during gameplay.
- **Not** a compiled SourceVR host adapter or Quest 3 APK. The practical next milestone is a native host-agnostic weapon entity I/O and ownership gate with simulated generation-safe handles and tests.

## Erratum provenance

The original Phase 61 report used `0x2245A578` as the vtable start, offsetting slot lookup by **nine entries**. The original 31-anchor verifier mistakenly validated `0x222AF450`, a genuine but unrelated reload-reading function at actual primary-table slot 221. Comparing RTTI, constructor vptr stores and derived weapon vtables exposed this mistake. Phase 62 verifies the correct target and effect-bit path; this correction must not be interpreted as new original game source or proof of runtime visibility in VR.
