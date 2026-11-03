# Phase 93: Original SiN client HUD constructors and server cache third-method predicates

## Original binaries and repeatable evidence

**User-held original Windows x86 SiN Episodes: Emergence (2006) DLLs, verified by the chained previous Phase 84–92 SHA-256 auditors:**

- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Phase 93 adds **eight individual original x86 HUD constructor bodies of 16 bytes each**, **three complete server function regions** (199 + 97 + 101 bytes), **22 exact server x86 opcode anchors**, **15 direct CALL edges**, and **5 virtual-dispatch instruction anchors**. Original DLLs and copyrighted engine bytes are NOT committed.

The audit first runs the full Phase 92 source-binary verifier. It then checks each client constructor's full instruction pattern and SHA digest, cross-checks the original Phase 91 RTTI/vtable identities, checks the literal vtable immediate occurrence in original `.text`, and pins full server code-region SHA256 and specific ISA control-flow evidence.

### client.dll: eight actual 16-byte original constructor bodies

| Original class RTTI | Constructor VA | Actual vtable VA |
|---|---|---|
| `CDebugOverlayPanel` | `0x241ECA50` | `0x242ABF0C` |
| `CNetGraphPanelInterface` | `0x241EF9E0` | `0x242ACC34` |
| `CFPS` | `0x241ED190` | `0x242AC1F0` |
| `CMessageChars` | `0x241EEA20` | `0x242ACB38` |
| `CLoadingDisc` | `0x241EE460` | `0x242AC8B0` |
| `CCenterPrint` | `0x241EC5E0` | `0x242ABC4C` |
| `CTextMessage` | `0x241F2310` | `0x242AD484` |
| `CStatsPanelInterface` | `0x24207660` | `0x242AF988` |

All eight original constructor functions perform only these visible stores: `this+0x00 = original vtable`, `this+0x04 = nullptr`, then return `this`. No original immediate store to `this+0x08` occurs in any of these 16-byte bodies.

**Key unresolved client lifecycle hazard:** Phase 92 proved that `CLoadingDisc` tears down two child objects, at `this+0x04` and `this+0x08`. The original 16-byte constructor initializes **only the first one**. Do NOT invent a second initializer; find where +0x08 is prepared by another code path. Our C++17 reference deliberately leaves the second field unchanged. The absence of a direct relative CALL to a constructor is not proof of never calling it, because static registrations and indirect/inlined calls can exist.

### server.dll: actual third virtual method bodies

Phase 91 verified `CSceneCache` and `CModelSoundCache` RTTI and vtables; Phase 92 followed the first two virtual methods. Phase 93 follows their third original methods and a separately called helper:

| Original class | Method | Bytes | Original code SHA256 |
|---|---|---:|---|
| `CSceneCache` | slot 2 `0x221AD750` | 199 | `bcc1b6624167e51ec29137ea7cff43041397674ca1ca76809b7861c7a1b0759e` |
| `CSceneCache` | candidate helper `0x221AD820` | 97 | `80451f83b0850ce692f181fdc8dda0ac0a7d7395c92050138f9b194a87626f91` |
| `CModelSoundCache` | slot 2 `0x221454B0` | 101 | `c3a10cccfee5943b097bf7af64d27f045436e9ff559670f1ca62f35f2faa5158` |

**Scene path:**
- Slot 2 invokes a file/parse helper at `0x221AD1B0`, processes a candidate list, and passes each item to `0x221AD820`.
- The candidate helper rejects null, requires the helper `0x2210B290` to return integer **5**, and requires `0x2210BB30` to return **zero**.
- It requests a bounded **0x40-byte** extraction via `0x2210C880`, then calls a global lookup object's virtual **+0x0C** through `0x225721D0`.
- Missing token **0xFFFFFFFF** is skipped. Successful tokens are passed to helper `0x22097E10`.

**Model-sounds path:**
- The method starts with an ID lookup through global `0x22572198` virtual **+0x10**, passing integer **1**. Token **0xFFFFFFFF** skips processing.
- It then resolves an object via global `0x225721BC` virtual **+0x04**, requires a non-null result, calls virtual **+0x24** and requires integer result **3**, then calls virtual **+0x28** and requires another non-null result.
- It invokes the separate processing helper `0x22145520` only after all these checks.

**Bounded interpretations:** Names and C++ argument signatures of these callbacks are NOT recovered. The integer comparisons `5` and `3` should NOT be labeled as Source SDK entity enums without independent evidence. These predicates do not constitute a working on-disk cache decoder.

### Committed files

- `include/sin_compat/original_hud_ctor_cache_slot2.hpp`: inert original class/vtable constructor metadata and host-only reference predicates preserving the +8 uncertainty.
- `research/tests/sin_original_hud_ctor_cache_slot2_reference.cpp`: tests all eight constructor writes and scene/model-sound accept/reject gates.
- `research/tools/audit_sin_cache_slot2_hud_ctors.py`: original dual-DLL chained verifier; exact 8 constructor patterns, 3 original SHA256 code regions, 22 opcode anchors, 15 CALL edges, 5 indirect vtable sites.
- `research/tests/test_sin_cache_slot2_hud_ctors.py`: 12 original-DLL-free synthetic mutation groups.
- `CMakeLists.txt`: strict-warning C++17 CTest reference.
- This evidence document.

**Local standalone verification:** dual original DLL audit PASS; Python 12/12 PASS; C++17 `-Wall -Wextra -Wpedantic -Werror`, CTest 1/1 PASS. GitHub Actions checked independently after publication.

## Next: focused original-binary R&D

Trace where `CLoadingDisc` receives its **+0x08** child pointer and examine the real consumers inside `0x22145520` without attributing unsupported methods. Later, the real **SourceVR 0.5.5 native ARM64** engine/game-module interface ABI must be established before native SiN `libclient.so`/`libserver.so` are claimed bootable.

**Status: reference reconstruction, not original C++ decompilation, a native Quest 3 game module, or a playable port.**
