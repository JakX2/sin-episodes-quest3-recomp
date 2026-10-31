# Phase 60: `m_bReloadsSingly` constructor stores and weapon-map pointer boundaries

**Status:** Static original-binary check completed 31 October 2026. **Input:** legally obtained, SHA-256-pinned 2006 Windows x86 SiN Episodes `server.dll` and `client.dll`; hashes are checked by the Phase 56–59 verifier chain. No original DLL bytes or proprietary Source implementation are committed. Numeric VAs are original preferred image-base addresses, not Android pointers.

## The useful result

Phase 59 identified 42 server-side weapon datamap entries, including named entity callbacks and outputs. Phase 60 investigated two related questions: what the verified x86 assembly actually does to `m_bReloadsSingly` (`CBaseCombatWeapon` server offset `+0x508`), and whether nearby direct data-map pointers establish a *runtime engine consumer*.

### Five inspected direct constructor writes all clear `+0x508`

| Site (VA) | Encoded write | Zero-value evidence |
|---|---|---|
| `0x222AE408` | byte store `[esi+0x508] <- bl` | `xor ebx, ebx` at `0x222AE3AE` in shared weapon constructor at `0x222AE390` |
| `0x223168C0` | byte store `[esi+0x508] <- al` | `xor eax, eax` at `0x223168B8` in derived constructor |
| `0x22316AEF` | byte store `[esi+0x508] <- immediate 0` | Immediate zero in the instruction itself |
| `0x22317BA0` | byte store `[esi+0x508] <- al` | `xor eax, eax` at `0x22317B98` in derived constructor |
| `0x22317D7E` | byte store `[esi+0x508] <- bl` | `xor ebx, ebx` at `0x22317D5E` in derived constructor |

The derived constructors shown here call the shared base constructor and then explicitly clear the same byte again. **None of these five checked sites enables single-round reload mode**. These sites include the inspected firearm constructor family discussed in Phases 53–55. This is **not** proof no other code ever enables it: indirect writes, copy/restore paths, script-configured values, alias-based writes, or unexamined classes could change memory at `+0x508` without having one of these five literal instruction sites.

### The shared reload code reads this flag

Two directly verifiable instruction sites:

- `0x222AF457`: compares the byte at `[esi+0x508]` with `bl` before the shared reload-selection branches.
- `0x222AF7F5`: compares `[esi+0x508]` with zero after an ammunition transfer branch; the following path can clear `m_bInReload` (`+0x4AC`).

These reads are evidence that the dormant flag has a control-flow purpose, but **do not prove a normal-play runtime setter exists**. The current host-agnostic C++ prototype is therefore correct to keep the single-round mode disabled by default unless the host has proven configuration/evidence for a specific gun.

### Direct metadata references do not identify the save/restore consumer

The compiled server includes at least two static initializer instructions that store a pointer to the **extended** `CBaseCombatWeapon` map at `0x2251F810` into other global records:

| Instruction VA | Target global | Stored map address |
|---|---|---|
| `0x22173C8F` | `0x224F9E98` | `0x2251F810` |
| `0x222CD01F` | `0x22522E7C` | `0x2251F810` |

These are verifiable pointer links in compiled code, **not** a trace of actual save, restore, replication, or prediction calls. They may be class-description/static-registration wiring. The two map getter slots from Phase 58 remain valid and separate.

### A near-address false positive: `game_weapon_manager`

At `0x223C2B10`, another initializer invokes the object returned by `0x2228B110`, passing static storage address `0x2251F828` and the string `game_weapon_manager` (`0x2245B128`). The passed storage is only `0x18` bytes after the weapon map start and contains pointer `0x2245ABC0` in the on-disk image. **The string identifies this as a registration associated with `game_weapon_manager`, not proof that this call consumes all `CBaseCombatWeapon` datamap fields for saves.** Nearby addresses are not interchangeable object identities, and interpreting every reference in the map's vicinity as savegame logic would overclaim.

## Reproduce on your own original DLLs

```sh
python3 research/tools/verify_weapon_reload_constructor_refs.py /path/to/SiN/server.dll /path/to/SiN/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

The verifier is standard-library-only and invokes the Phase 59 chain first, so the binary hashes, names, flags, 42-entry descriptor count and tail registrations must still match the original verified build. Phase 60 then checks five exact constructor byte-store instruction anchors, their zero-value producer instructions (or immediate), two reload reads, two absolute map-pointer stores, and the neighboring `game_weapon_manager` registration and text string. The verifier outputs concise JSON, **not disassembly or proprietary contents**. The synthetic tests construct instruction-pattern fixtures without shipping original binary bytes or original code sections.

**Recorded results:** both original DLL SHA-256 digests matched; Phase 60 original-binary verifier passed; **40 Python synthetic tests** passed across Phases 56–60 (12 new in Phase 60), and the independent C++17 library compiled and passed CTest locally. GitHub CI should be read from the PR check status, not assumed from these local results.

## Still unresolved and next steps

1. Find an **actual** engine call path that consumes the extended weapon `datamap_t` and distinguish save/restore from the mirrored prediction-oriented map. Static registrations alone are insufficient.
2. Perform **alias-aware, whole-function** analysis of writes through pointers/references to `weapon+0x508`, and search game script parsing for enabling paths. A bounded scan of five known stores is not exhaustive.
3. For gameplay progress, implement a testable weapon interaction/event adapter in the host boundary from Phase 59 callback/output names, without copying 32-bit vtable pointers into native ARM64 code.

The Quest 3 port remains unimplemented at this level; the research advances a reusable, evidence-backed weapon state model.
