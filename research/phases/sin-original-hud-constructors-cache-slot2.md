# Phase 93: original SiN 2006 client HUD constructors and server cache third methods

## Original binaries and verification

Analysed the user's genuine Windows x86 `client.dll` and `server.dll`, SHA-256 **`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`** and **`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`**, respectively. The research tool chains prior original-byte audits (Phases 84–92). No DLL or proprietary binary bytes are committed.

## Eight exact HUD constructors in client.dll

Each original function is **16 bytes** and writes only (a) its x86 vtable at `this+0` and (b) zero to member `this+4`. All eight patterns and their sole original `.text` vtable-immediate occurrence are audited; the class names/vtables were independently proved by original RTTI in Phase 91.

| Original RTTI class | Original constructor VA | Original vtable VA |
|---|---|---|
| `CDebugOverlayPanel` | `0x241ECA50` | `0x242ABF0C` |
| `CNetGraphPanelInterface` | `0x241EF9E0` | `0x242ACC34` |
| `CFPS` | `0x241ED190` | `0x242AC1F0` |
| `CMessageChars` | `0x241EEA20` | `0x242ACB38` |
| `CLoadingDisc` | `0x241EE460` | `0x242AC8B0` |
| `CCenterPrint` | `0x241EC5E0` | `0x242ABC4C` |
| `CTextMessage` | `0x241F2310` | `0x242AD484` |
| `CStatsPanelInterface` | `0x24207660` | `0x242AF988` |

**Critical caveat:** Phase 92 confirmed that `CLoadingDisc` also releases an object from `this+8`. The 16-byte constructor **does not touch `this+8`**. Thus initializing that member in a port may be necessary, but is not something proven by this constructor; the actual producer/lifetime must be investigated.

## Two cache virtual slot-2 methods, server.dll

Original RTTI `CSceneCache` vtable slot 2 -> **`0x221AD750`**, 199 bytes, SHA-256 `bcc1b6624167e51ec29137ea7cff43041397674ca1ca76809b7861c7a1b0759e`. It calls a separate candidate predicate at **`0x221AD820`**, 97 bytes, SHA-256 `80451f83b0850ce692f181fdc8dda0ac0a7d7395c92050138f9b194a87626f91`.

The per-candidate helper verifies a non-null candidate, obtains a type with `0x2210B290` and requires type **5**, requires `0x2210BB30` to return zero, extracts a value through `0x2210C880` with capacity **`0x40` bytes**, then asks the original global name-token interface at `0x225721D0` virtual +`0x0C` to resolve it. Token **`0xFFFFFFFF`** is rejected; otherwise it passes token to `0x22097E10`. These are observed x86 gates, **not** a full cache parser or a verified engine-level type enum.

Original `CModelSoundCache` vtable slot 2 -> **`0x221454B0`**, 101 bytes, SHA-256 `c3a10cccfee5943b097bf7af64d27f045436e9ff559670f1ca62f35f2faa5158`. It invokes original interface at global `0x22572198` virtual +`0x10` with argument 1, skips unresolved `0xFFFFFFFF` results, resolves the object via global `0x225721BC` virtual +`0x04`, requires non-null, asks the original object's virtual +`0x24` and requires type **3**, then requires the original virtual +`0x28` associated object to be non-null before forwarding to `0x22145520`. Class names and semantics of subordinate engine interfaces are still unknown.

## Implementation and verification

- `include/sin_compat/original_hud_ctor_cache_slot2.hpp`: host C++17 mock, writes ONLY verified constructor fields and preserves the untouched +8 field; models distinct cache predicates.
- `research/tests/sin_original_hud_ctor_cache_slot2_reference.cpp`: constructor side-effect and scene/model candidate success/failure tests.
- `research/tools/audit_sin_cache_slot2_hud_ctors.py`: chained exact DLL SHA, original RTTI/Init audits, eight constructor byte patterns, **three full code SHA**, **22 opcode anchors**, **15 direct calls** and **five indirect virtual calls**.
- `research/tests/test_sin_cache_slot2_hud_ctors.py`: 12 original-free mutation test groups covering constructor writes, RTTI crosschecks, code hashes, method boundaries and callsites.
- `CMakeLists.txt`: new CTest target.

**Verified in this session on the real DLLs:** complete original binary audit passed; local `unittest` 12/12; CMake/CTest 1/1. GitHub CI is a separate check after publishing.

**Scope:** verified x86 Windows research and host-only C++ references, *not* completed source conversion, real Android `libclient.so`/`libserver.so`, an engine-compatible `CreateInterface` adapter, full scene/model-sound cache format or Quest 3 gameplay.

## Next investigation

Identify the actual code path that first supplies `CLoadingDisc` member +8 and trace `CModelSoundCache` helper `0x22145520` into its real data consumers. Do not infer ARM64 compatibility solely from Source interface version strings.
