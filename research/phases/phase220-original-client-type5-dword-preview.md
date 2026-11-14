# Phase 220: original 2006 client.dll snapshot type-5 DWORD comparison and symbol-aware preview

14 November 2026. **Stacked DRAFT on Phase219 PR #273.** This phase uses the owner's authentic original PE32 Windows x86 game DLLs from private bin.zip. Both complete original DLL SHA256 hashes match the existing research provenance:

- client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d, 3,592,192 bytes.
- server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650, 6,049,792 bytes.

No proprietary DLL, complete original native code/disassembly, engine asset or map is committed.

## Two complete new native function bodies SHA-verified

| Original client.dll VA | Complete x86 bytes | Entire original native method SHA256 | Original Phase212 direct CALL |
| --- | ---: | --- | --- |
| 0x24197DB0 | **125** | 256748201d4c29fb46076705316ea2345bbeb24a885a978e21d7b6bb9824cdbf | 0x241999BC |
| 0x241983B0 | **144** | 88eb84ee0d78cae0b3754d272b8b462e89e662e52f2735a8c547018b48fad874 | 0x241999E5 |

**Two original machine-code functions, 269 complete x86 bytes.** The former ends with native RET 12 followed by three original INT3 padding bytes. The latter ends with RET 16, *immediately adjacent* to Phase219's type-6 Boolean preview at 0x24198440, with **no INT3 padding**. The auditor confirms the next function's entry opcode, so the two are not silently joined.

The two previously SHA-verified original type-5 direct callees are Phase213 DWORD copy 0x24198A10 (caller 0x241999CB) and Phase217 integer diagnostics 0x24197B60 (caller 0x241999D6). Their native original code bodies are NOT counted again.

## Verified native semantics and significant ordering

**0x24197DB0: exact DWORD-array comparison, with immediate first-mismatch logging.** The original object's byte this+0x14 == 0 returns status **0**, without reading the descriptor or the arrays. Otherwise descriptor[this+0x18]+0x13 bit 0x04 forces status **1**. With the normal comparison path, a signed DWORD element count <= 0 gives **1**. For a positive count, the function reads matching-index little-endian 32-bit DWORD pairs in order. Any unequal pair triggers a call to original 0x24197910 with the authentic string "int differs (net %i pred %i) diff(%i)\n" at 0x242A3ED0, reporting the **first mismatching index's** source/net, destination/predicted and 32-bit wrapping (predicted minus net). It then returns **0**; when all elements match it returns **1**.

Crucial detail: this is not a silent memcmp. It has an observable mismatch formatter **inside the comparator**, before the Phase213 conditional copy. The separate Phase217 0x24197B60 integer diagnostic, called later in the native router, reports the **FIRST DWORD** in the buffers AFTER any copy. Consequently one native type-5 case may emit **two different mismatch messages**: first for the mismatching element before copy, then for the first element after copy, even if the destination has become equal to the source.

**0x241983B0: conditional integer display, with optional name resolver.** It first tests DWORD(this+0x34) against DWORD(this+0x18). If not equal, returns without reading data, independent of the this+0x14 logging/compare gate. Otherwise it takes the first predicted DWORD. Descriptor flag +0x13 bit 0x08 enables an original process-global registry at .data 0x2433F6AC; its virtual method slots +0x04 and +0x0C are used to resolve an optional name, followed by original import vstdlib.dll!Q_FileBase at IAT 0x24276254 into a 512-byte local buffer. A successful lookup calls original formatter 0x24197FE0 with "integer (%i->%s)" at 0x242A4020. Missing lookup or absent flag falls back to "integer (%i)" at 0x242A4010. The registry object, virtual function implementation, string lifetime and formatter implementation are NOT recovered; the host reference accepts a supplied resolver and adds deliberate host-only capacity checks.

**Exact original Phase212 type-5 direct CALL order:**

1. 0x24197DB0: compare and report the first *pre-copy* differing DWORD, if any.
2. 0x24198A10: already-verified Phase213 conditional forward REP MOVSD over the unsigned DWORD count. Copy requires this+0x2C enabled and status != 1.
3. 0x24197B60: already-verified Phase217 enabled-only first-int mismatch/status reporting and optional symbol resolution. Reads *post-copy* predicted data.
4. 0x241983B0: new conditional, name-aware preview of the first *post-copy* predicted DWORD.

The original signed count test in comparison differs from the unsigned count used by the copy helper; the portable model explicitly refuses unsafe multiplied copy lengths, which is a **host-only safety boundary** and not claimed to reproduce x86 invalid-pointer faults. As in other phases, the native formatter CALLs are authenticated *targets*, not newly decompiled functions.

## Files and validation

- research/tools/audit_sin_original_client_type5_specialists.py: read-only owner-only dual-entire-DLL SHA256 gate, two exact whole native function SHA256s, **44** sparse independently checked x86 opcode witnesses, 4 native original type-5 router CALLs, 3 nested formatter CALLs, 3 real original format strings, exact RET/INT3 boundaries, the zero-padding transition to Phase219, original symbol registry .data section and named Q_FileBase IAT. No game binaries or full method code distributed.
- research/tests/test_sin_original_client_type5_specialists.py: synthetic-only mutation tests for every short opcode witness, all seven relevant native CALL instructions, all stack-cleanup/padding boundaries, the adjacent method entry, three formats, registry global, IAT and original base. A synthetic fixture must produce **zero** authenticated original functions.
- include/sin_compat/original_client_snapshot_type5.hpp: independently authored C++17 host reference. Reuses Phase213 forward DWORD copy and Phase217 integer diagnostics/symbol-host interfaces; adds early-log compare and independent name-aware preview without recreating game-engine objects.
- tests/test_original_client_snapshot_type5.cpp: exact DWORD comparison and first mismatch, signed count <=0, pointer bypass gates, 32-bit wrapping subtraction, immediate pre-copy versus post-copy diagnostic ordering, copying controls, preview enabled independently of logging, registry success/fallback/capacity, and unsafe-count refusal.
- Root Linux and native MSVC Win32 x86 CTest targets validate the reference; **neither is the original retail Source engine playing the campaign**.

Reproduce against owner's original files:

    python3 research/tools/audit_sin_original_client_type5_specialists.py --archive /private/bin.zip
    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_type5_specialists.py' -v

## Honest checkpoint

The original verified-function inventory advances **260 → 262**, with **128 server / 134 client**, after the original SHA gate. The Phase212 snapshot router's narrow 36-destination graph has **12** still-unreviewed direct native destinations (14 minus 2); this is not the number of functions remaining across either DLL or a full game decompilation percentage.

Type-5's four direct native calls are individually reviewed, but the complete serializer, Source engine ownership, game object layouts, original formatter bodies and Windows retail engine startup with reconstructed gameplay DLLs remain incomplete. Windows PC playability is still the first runtime milestone, followed by Android ARM64 and Quest 3.
