# Phase 85: Original 2006 SiN client/server Init dependency handshake

## Primary evidence

Inspected the user's original 2006 Windows x86 `client.dll` and `server.dll`, independently verified by exact SHA-256:

- `client.dll` `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll` `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Phase 84 previously verified actual exported `CreateInterface` registrations and the original vtable slot 0 pointers. This phase follows precisely those slot 0 original methods:

| Interface | Slot 0 function | Function's original-byte SHA-256 | Length verified |
|---|---|---|---:|
| `VClient011` | `0x240FE0E0` | `14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db` | 1,220 bytes |
| `ServerGameDLL003` | `0x221E7640` | `abb6eb2fc523fc0cb6301630e7ba9f0b61e70e48f15d5809145171250898b680` | 875 bytes |

These are original x86 virtual slot zero implementation bodies. In standard engine terminology they perform initialization; their complete function-level ABI and every subordinate method are not claimed to be ported.

## Original required factory sequence

The client method makes **21** ordered calls to its *first* factory argument (`ESI`), with the exact case-sensitive interface string and a null status pointer. Following each call, original code stores the result into a module global and jumps to the shared failure label if the returned pointer is null.

**Client early required order:** `VEngineClient012`, `VEngineModel012`, `VEngineEffects001`, `VEngineCvar002`, `EngineTraceClient003`, `VEngineRenderView011`, `VDebugOverlay003`, `VMaterialSystem076`, `VEngineCache001`, `VModelInfoClient003`, `VEngineVGui001`, `VEngineClientStringTable001`, `SpatialPartition001`, `VEngineShadowMgr002`, `StaticPropMgrClient004`, `IEngineSoundClient003`, `VFileSystem012`, `VEngineRandom001`, `VENGINE_GAMEUIFUNCS_VERSION004`, `GAMEEVENTSMANAGER002`, `VSoundEmitter001`.

Original request call-site range: `0x240FE115` through `0x240FE309`. Each missing pointer jumps to `0x240FE59E` to return 0. The function returns 1 at `0x240FE595` only after a larger set of post-query setup tasks also succeeds.

The server method makes **16** ordered strict queries. **15** use its first-argument factory saved in `ESI`. The **11th** request, `VFileSystem012`, instead uses the **third-argument factory** saved in `EDI` by `MOV EDI,[ESP+0x20]` at `0x221E7743`. This observation is grounded in the original stack arithmetic and does not assign an unverified Source SDK formal parameter name.

**Server early required order:** `VEngineServer021`, `VoiceServer001`, `VEngineCvar002`, `VEngineServerStringTable001`, `StaticPropMgrServer002`, `VEngineRandom001`, `IEngineSoundServer003`, `SpatialPartition001`, `VModelInfoServer001`, `EngineTraceServer003`, `VFileSystem012` (**third argument**), `GAMEEVENTSMANAGER002`, `VEngineCache001`, `VSoundEmitter001`, `ServerUploadGameStats001`, `ISERVERPLUGINHELPERS001`.

Original request call-site range: `0x221E7650` through `0x221E77CB`. Each missing pointer jumps to `0x221E79A1` to return 0. The successful return paths are at `0x221E7978` and `0x221E7997`.

## Original checks beyond the first lookup block

Even after all 21 or 16 pointers have been obtained, both original methods perform an additional required check: they invoke **virtual slot +0** on the obtained `VSoundEmitter001` pointer and test its boolean result.

- Client: vtable dispatch at `0x240FE320`, boolean test at `0x240FE322`, near conditional fail at `0x240FE324`.
- Server: vtable dispatch at `0x221E77E6`, boolean test at `0x221E77E8`, near conditional fail at `0x221E77EA`.

Both functions also perform more mandatory setup after these checks. This is **not** an exhaustive model of the complete initialization sequence. In particular, later factory queries (e.g. the server's `VDebugOverlay003` at `0x221E7949`, and the client's `VMaterialSystemStub001` at `0x240FE34E`) do not use the same immediate fatal null-guard pattern. Therefore treating every possible factory request as mandatory would be wrong.

The client method returns with `RET 0x0C` (three 32-bit stack arguments; `this` is passed separately in thiscall), whereas the server method ends with `RET 0x10` (four 32-bit stack arguments). This does **not** establish complete high-level parameter names, function signatures or interoperability with AArch64 ABI.

## Rebuilt artifacts

- `research/tools/audit_sin_init_contracts.py`: chains Phase 84's exact dual-DLL SHA256 and original factory/vtable evidence, pins entire original slot-zero code regions and verifies every name, 37 factory-call instructions, 37 original pointer stores, 37 error branches, register sources, sound-emitter gate and return conventions. Reports every original call and global storage VA as JSON.
- `research/tests/test_sin_init_contracts.py`: **10 grouped synthetic original-free regression cases**, with mutations covering all 37 calls, stores, names, guards, factory source, and gate/ret instructions.
- `include/sin_compat/original_init_contract.hpp`: pure C++17 original-order request arrays and a mocked first/third factory resolver. Preserves partial stored pointers when a later query fails; does not silently substitute a modern API version.
- `research/tests/sin_original_init_contract_reference.cpp`: tests all-request success, first/middle/late failures, third-argument routing, emitter gate, partial pointer preservation, exact-version comparisons.
- `CMakeLists.txt`: CTest reference target.

Original DLL read-only audit passed, with original full-function SHA-256 checks; local 10/10 Python regression cases and C++17 `-Wall -Wextra -Wpedantic -Werror` tests passed. GitHub CI is tracked separately.

## Quest 3 integration implications

The earlier user-supplied HL2VR 0.989 ARM64 `.so` libraries visibly contain strings for newer engine versions such as `VEngineClient014`, `VEngineCvar004` and `VFileSystem022`. Original SiN requests `VEngineClient012`, `VEngineCvar002`, and `VFileSystem012`. The Android strings *alone* are not proof of an actual interface registration or method ABI; newer versions must not be aliased blindly.

The newer SourceVR 0.5.5 package is a distinct manifest-selected revision and its actual ARM64 `CreateInterface` dispatch still needs direct verification. The correct next step is to compare verified method contracts and bootstrap at least a minimal real SiN server/game-client interface under Android, not to load Windows x86 DLLs on Quest.

**Not claimed:** complete original C++ recovery, all vtable signatures, full client/server lifecycle, real SiN engine plugin, playable Quest APK or the original campaign running on a headset.
