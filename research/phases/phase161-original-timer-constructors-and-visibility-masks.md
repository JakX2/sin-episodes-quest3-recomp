# Phase 161: original breath/poison HUD constructors, visibility masks and panel member offsets

**10 November 2026.** Continued SiN Episodes: Emergence original 2006 Windows client.dll PE32 reverse engineering. This phase connects the previously reconstructed CHudElement global visibility gate (Phase160), timer rendering methods (Phases153/155), and the true native class constructors. Both original DLLs are held privately; no commercial executable bytes or game assets are committed.

## Verified original DLL fingerprints

- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

### Two genuinely original constructor code regions

| Constructor | Full native length | Original region SHA256 | Reconstructed status |
|---|---:|---|---|
| CHudBreathTimer 0x24230F30 | 230 bytes | 38c93563155918495201f0bbb99a75c339e8a16533033cb91917ef58424cb3b1 | **Partial direct-field stores**, engine setup unresolved |
| CHudPoisonTimer 0x24235E80 | 201 bytes | 7dc7ea0b731d13de89fcd70f5ff34c5b64aa07ed62a38a1a1d097542817bcafb | **Partial direct-field stores**, engine setup unresolved |

Native breath 0x24230F3B and poison 0x24235E89 both invoke original CHudElement constructor 0x24142220, proved with original CALL rel32 bytes. Breath then constructs its VGUI panel subobject at this+0x14 through 0x24003910; poison uses 0x24153320 for that panel subobject. These are different original engine constructors and **must not be unified just because the two panels look similar**.

Each installed primary and secondary MSVC RTTI-linked vtable is proven directly by original x86 stores:

| Original class | Primary vtable | Secondary panel vtable |
|---|---|---|
| Breath | 0x242C78CC | 0x242C769C |
| Poison | 0x242C8DEC | 0x242C8B24 |

These two identities were already recovered in Phases153/155, but Phase161 pins the **constructor callsites** responsible for their installation.

### Real different element visibility masks

The exact native bytes are unambiguous:

- CHudBreathTimer 0x24230FFC pushes **0x10**, and 0x24231000 calls CHudElement mask setter 0x24141850.
- CHudPoisonTimer 0x24235F27 pushes **0x18**, and 0x24235F3D calls the same mask setter.

Both masks contain bit 0x10; the Phase160 original suppression gate therefore calls the player's signed virtual +0x190 when other short-circuit checks permit. But **only poison includes bit 0x08**. For a local player with effective flags exactly 0x08, engine eligibility true and signed player virtual return >0, the breath HUD passes the shared element gate while poison is suppressed. An effective flag containing hard bit 0x04 still suppresses both. Phase161 includes an actual cross-phase C++ regression of this distinction.

This is a concrete behavioral difference established from two distinct original constructor sites, not an extrapolation from Source SDK defaults.

## Owner layout: explicit field writes, distinct from subobject coordinates

Both original timers have a panel subobject at **complete-this+0x14**. When original panel methods inspect panel-relative field +0xA6, the corresponding complete-object byte is **+0xBA**.

The breath constructor at 0x24230FC7, 0x24230FCD, 0x24230FD3 and 0x24230FD9 explicitly stores **zero** into four consecutive complete-object bytes +0xB7, +0xB8, +0xB9, +0xBA. This matches panel-relative +0xA3..+0xA6, the earlier Phase157/158 renderer input/color-like region. Byte +0xA6 is fourth in that panel-relative group; this supports but does not yet prove the specific source type or that the fourth byte is officially named *alpha*. The actual engine color setter and all later writers still require investigation.

The poison constructor makes **no explicit direct stores** into +0xB7..+0xBA in its own 201-byte function body. It does explicitly zero three DWORDs at **complete+0x2FC, +0x300 and +0x304** (x86 0x24235F2B, 0x24235F31 and 0x24235F37), before calling the common mask setter. The underlying native panel constructor might initialize the four-byte field; this remains unverified. The independent C++ reference **deliberately does not zero poison's field**, avoiding false equivalence.

### Eight fully reconstructed native member-address helpers

The owner original contains separate but byte-identical code in breath/poison modules:

| Original VA | Class | Method behavior | Length | Exact full-code SHA256 |
|---|---|---|---:|---|
| 0x242309F0 | Breath | Return nullable input address +0xA3 | 16 | 7b425799d9562e26157a8051624ce73a5a995f1e1c2a0fa98f43f3eb7cf7762b |
| 0x24230A00 | Breath | Return complete-object offset 0xB7 | 6 | bd36350ae3bfe08df6fb572ebee9f406ed4ea953003deb777e245d2f5a156e11 |
| 0x24236030 | Poison | Return nullable input address +0xA3 | 16 | 7b425799d9562e26157a8051624ce73a5a995f1e1c2a0fa98f43f3eb7cf7762b |
| 0x24236040 | Poison | Return complete-object offset 0xB7 | 6 | bd36350ae3bfe08df6fb572ebee9f406ed4ea953003deb777e245d2f5a156e11 |
| 0x24230A10 | Breath | Return nullable input address +0xA8 | 16 | 5eb92598979c5339d1371426334129d2144e713d68e18894b79a2474b90cdae9 |
| 0x24230A20 | Breath | Return complete-object offset 0xBC | 6 | bcc59c532702d05a8d70becbe0c2599d8af785c9b1cba14bc44b166f2c7b5501 |
| 0x24236050 | Poison | Return nullable input address +0xA8 | 16 | 5eb92598979c5339d1371426334129d2144e713d68e18894b79a2474b90cdae9 |
| 0x24236060 | Poison | Return complete-object offset 0xBC | 6 | bcc59c532702d05a8d70becbe0c2599d8af785c9b1cba14bc44b166f2c7b5501 |

For nullable native inputs, the original checks the input before adding the panel-relative offset and returns zero on null; otherwise the addition uses native 32-bit modulo arithmetic. The exact shift +0x14 explains why paired offset-return helpers produce +0xB7 and +0xBC respectively. All eight entire code regions are pinned individually, **even when their instruction bytes coincide**: each separate original virtual/static address counts as one distinct function entry, but they do not represent eight sophisticated game mechanics.

Additional pointer helpers for later resource fields exist nearby, and their linkage to actual resource names should be studied in a later phase.

## New independent code, auditors, and regression tests

- include/sin_compat/original_client_timer_constructor_contract.hpp: native vtable identities, pointer/offset helpers, partially reconstructed direct constructor stores and integration with Phase160's original CHudElement model.
- tests/test_original_client_timer_constructor_contract.cpp: nullable addresses (including 32-bit overflow), vtable/offset invariants, field-store isolation, breath vs poison masks and end-to-end Phase160 suppression behavior.
- research/tools/audit_sin_original_client_timer_constructors.py: owner-only exact SHA of BOTH original DLLs, ten whole native code regions, two RTTI class strings, 30 sparse original instruction witnesses and eight direct constructor CALL targets. No DLL bytes are exported.
- research/tests/test_sin_original_client_timer_constructors.py: mutation tests for every original instruction, call, and RTTI witness; private-full-hash unsatisfiability on a synthetic sparse fixture; independent visibility mask cases.
- Root CMake/CTest target.
- Phase152 manually reviewed original-function ledger grows from **51 to 61 entries** (25 server and 36 client). Eight tiny native helpers are **behavior_reconstructed**; two constructors remain **machine_code_verified** with only direct stores modeled. This ledger is not the total game-function count and does not measure a full game decompilation percentage.

To verify original local owner files:

    python3 research/tools/audit_sin_original_client_timer_constructors.py --client /private/client.dll --server /private/server.dll --output /private/phase161.json

Original-free regression:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_timer_constructors.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_original_client_timer_constructor_contract_tests
    ctest --test-dir build -R sin_original_client_timer_constructor_contract_tests --output-on-failure

## Next targets

Recover the original type/semantics and writes of the packed four-byte panel field, the native panel VGUI constructor and color setters, and class-specific registration callbacks. Continue client player/weapons/prediction beyond the HUD subsystem, while keeping original 2006 server.dll state decompilation progressing. The reconstructed original Windows x86 client/server DLL pair still does not implement full Source VGUI/gameplay ABI; neither Windows gameplay, Android game runtime nor Quest 3 standalone playability is claimed.
