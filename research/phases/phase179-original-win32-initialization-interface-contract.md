# Phase 179: Real 2006 Win32 game-module Init interface negotiation

11 November 2026. Stacked on Phase178 draft PR #231. Actual owner-supplied original Windows PE32 `server.dll` and `client.dll` binaries were read locally. Neither file, original executable body, commercial assets or full disassembly is committed.

## Original PE32 provenance and complete function body hashes

- Original server `server.dll` full SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, image base `0x22000000`.
- Original client `client.dll` full SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, image base `0x24000000`.
- `CServerGameDLL` vtable slot 0 initialization entrypoint `0x221E7640`: **875 complete original x86 bytes**; SHA256 `abb6eb2fc523fc0cb6301630e7ba9f0b61e70e48f15d5809145171250898b680`; function ends in `RET 0x10` at `0x221E79A8`. The original requires **four stack arguments** (first is engine factory callback and third is filesystem factory callback).
- `CHLClient` vtable slot 0 initialization entrypoint `0x240FE0E0`: **1,220 complete original x86 bytes**; SHA256 `14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db`; function ends in `RET 0x0C` at `0x240FE5A1`. It receives **three stack arguments**, first the engine factory callback.
- Complete original function hashes cover **2,095 native x86 bytes**. The recovered portable C++17 code implements **only the mandatory interface negotiation prefix and isolated late queries**, not the full functions, native engine ABI or playable modules.

## Native mandatory early interface lookup, in exact call order

Both native routines take the returned `void*`, publish it to a hard-coded global DWORD and then test it for null. For every required lookup a null pointer branches to the same method-local `return false` epilogue. No rollback/clearing of previously published pointers is visible on this early failure path. Original string bytes, factory call origin, global store and exact relative conditional jump are checked for every row. The original differs in its handling of **where the factory argument comes from**.

### server.dll: 16 mandatory prefix lookups

| # | Original requested version | Supplier | Query VA | Published global VA |
|---:|---|---|---|---|
| 1 | `VEngineServer021` | engine | `0x221e7649` | `0x22572198` |
| 2 | `VoiceServer001` | engine | `0x221e7662` | `0x2257219c` |
| 3 | `VEngineCvar002` | engine | `0x221e767b` | `0x225721a0` |
| 4 | `VEngineServerStringTable001` | engine | `0x221e7694` | `0x225721a8` |
| 5 | `StaticPropMgrServer002` | engine | `0x221e76ad` | `0x225721ac` |
| 6 | `VEngineRandom001` | engine | `0x221e76c6` | `0x225721b0` |
| 7 | `IEngineSoundServer003` | engine | `0x221e76df` | `0x225721b4` |
| 8 | `SpatialPartition001` | engine | `0x221e76f8` | `0x225721b8` |
| 9 | `VModelInfoServer001` | engine | `0x221e7711` | `0x225721bc` |
| 10 | `EngineTraceServer003` | engine | `0x221e772a` | `0x225721c0` |
| 11 | `VFileSystem012` | filesystem | `0x221e7747` | `0x225721a4` |
| 12 | `GAMEEVENTSMANAGER002` | engine | `0x221e7760` | `0x225721c4` |
| 13 | `VEngineCache001` | engine | `0x221e7779` | `0x225721c8` |
| 14 | `VSoundEmitter001` | engine | `0x221e7792` | `0x225721d0` |
| 15 | `ServerUploadGameStats001` | engine | `0x221e77ab` | `0x22572194` |
| 16 | `ISERVERPLUGINHELPERS001` | engine | `0x221e77c4` | `0x225721d4` |

### client.dll: 21 mandatory prefix lookups

| # | Original requested version | Supplier | Query VA | Published global VA |
|---:|---|---|---|---|
| 1 | `VEngineClient012` | engine | `0x240fe109` | `0x2433f684` |
| 2 | `VEngineModel012` | engine | `0x240fe127` | `0x2433f688` |
| 3 | `VEngineEffects001` | engine | `0x240fe140` | `0x2433f68c` |
| 4 | `VEngineCvar002` | engine | `0x240fe159` | `0x2433f690` |
| 5 | `EngineTraceClient003` | engine | `0x240fe172` | `0x2433f6d8` |
| 6 | `VEngineRenderView011` | engine | `0x240fe18b` | `0x2433f694` |
| 7 | `VDebugOverlay003` | engine | `0x240fe1a4` | `0x2433f698` |
| 8 | `VMaterialSystem076` | engine | `0x240fe1bd` | `0x2433f69c` |
| 9 | `VEngineCache001` | engine | `0x240fe1d6` | `0x2433f6a8` |
| 10 | `VModelInfoClient003` | engine | `0x240fe1ef` | `0x2433f6ac` |
| 11 | `VEngineVGui001` | engine | `0x240fe208` | `0x2433f6b0` |
| 12 | `VEngineClientStringTable001` | engine | `0x240fe221` | `0x2433f6b4` |
| 13 | `SpatialPartition001` | engine | `0x240fe23a` | `0x2433f6b8` |
| 14 | `VEngineShadowMgr002` | engine | `0x240fe253` | `0x2433f6c0` |
| 15 | `StaticPropMgrClient004` | engine | `0x240fe26c` | `0x2433f6c4` |
| 16 | `IEngineSoundClient003` | engine | `0x240fe285` | `0x2433f6c8` |
| 17 | `VFileSystem012` | engine | `0x240fe29e` | `0x2433f6bc` |
| 18 | `VEngineRandom001` | engine | `0x240fe2b7` | `0x2433f6cc` |
| 19 | `VENGINE_GAMEUIFUNCS_VERSION004` | engine | `0x240fe2d0` | `0x2433f6dc` |
| 20 | `GAMEEVENTSMANAGER002` | engine | `0x240fe2e9` | `0x2433f6e0` |
| 21 | `VSoundEmitter001` | engine | `0x240fe302` | `0x2433f6e4` |

### Later native interface requests: **not contiguous with the prefix**

After the prefix, the real game executes numerous virtual and direct startup callbacks. The following lookup sites are separately verified **inside the same two complete native bodies**, but their intervening side effects are not reconstructed. A test host invokes each late request individually, not as a made-up complete `Init()` sequence.

| DLL | Version | Native query VA | Source | Failure handling | Global |
|---|---|---|---|---|---|
| server | `VDebugOverlay003` | `0x221e7942` | engine | optional, no immediate null gate | `0x225721cc` |
| client | `VMaterialSystemStub001` | `0x240fe347` | engine | optional, no immediate null gate | `0x2433f6a0` |
| client | `MaterialSystemHardwareConfig012` | `0x240fe362` | derived_virtual | required, branches false | `0x2433f6a4` |
| client | `MatSystemSurface003` | `0x240fe3e1` | derived_virtual | required, branches false | `0x2433f6d0` |

Important distinction: original server `VFileSystem012` is called through its **separate filesystem factory** in EDI, whereas original client `VFileSystem012` uses the regular engine factory in ESI. Client `VEngineClient012` is the first request but stores an unrelated startup global between the pushed name and its call; the original auditor verifies that exceptional instruction layout explicitly.

## Proven early failure and success exit paths

- Server all 16 mandatory prefix branches jump to `0x221E79A1`, pop registers and return false with `RET 0x10`. The optional debug overlay at `0x221E7942` does not receive a subsequent zero-check before the allocator call. Several later helper checks (`0x221F5700`, `0x221AD8A0`, `0x22149B60`) can also fail. The server late 0x510-byte allocation branch **returns true even if allocation returns null**, writing null to global `0x2254B888` and invoking an engine stats callback. That is observed native control flow; no inferred fatal OOM condition is added.
- Client all 21 mandatory prefix and two proven later derived-virtual requests branch to false epilogue `0x240FE59E` on null. The optional `VMaterialSystemStub001` request is not itself null-gated. Further startup callbacks can fail, including the sound-emitter interface initializer, render/material/system setup and final `0x24182C60` check. Success returns true at `0x240FE595` with `RET 0x0C`. These intervening operations remain engine-owned.
- The names `MaterialSystemHardwareConfig012` and `MatSystemSurface003` come from native virtual calls, **not** from the original `CreateInterface` engine callback directly. The latter source is accessed via a separate material system access path. Their native gate and globals are pinned.

## Implementation, verified evidence and regression tests

- `include/sin_compat/original_dll_initialization.hpp`: host-neutral C++17 strict-order early prefix model (requires callback query source, version and global publication), plus separate late query helper. It deliberately never claims to implement complete DLL Init, process static constructors or produce a real Win32 interface object.
- `tests/test_original_dll_initialization.cpp`: both original prefix lists, each failed interface slot, success of prefixes only, exact source distinction, late optional vs mandatory calls and global-write-before-fail ordering; compiled and executed locally under optimized GCC and Clang with `-Werror -UNDEBUG`.
- `research/catalog/original_dll_initialization_requests.json`: exact per-site request VA, native `.rdata` string address, source, global DWORD, required/optional behavior; SHA256 `fcd4df72892e18013d8167e62f65ede456f454a05ee4ee9b0cc85c9077a80bef` including terminal newline.
- `research/tools/audit_sin_original_dll_initialization.py`: chains Phase178 full-original client/server SHA, native PE exports, 65 static registrations, nine factory singleton bodies, five RTTI object families and 105 vtable pointers. Additionally verifies both full original native Init body SHA256s, all **41** interface query names/call instruction patterns/published global addresses, **39** required interface failure branches, the differing filesystem callback origins and original success/failure epilogues.
- `research/tests/test_sin_original_dll_initialization.py`: **10 original-free Python tests** including 41 interface-name byte mutations, 82 instruction mutations, all 37 mandatory-prefix failure edge mutations, two derived failure edges, full native hash rejection for synthetic fixtures and 39 independent early-failure/success branch cases.
- Original reviewed-method atlas **131 -> 133** (`server 75`, `client 58`). The native bodies are hash verified but the executable-original ABI and full function side effects remain unported.
- Root CMake and actual Windows x86 proxy CMake register the independent C++17 regression; Windows PE32 CI does **not** load the real game engine.

### Original owner-local verification

```sh
python3 research/tools/audit_sin_original_dll_initialization.py --server /private/original/server.dll --client /private/original/client.dll
python3 -m unittest discover -s research/tests -p test_sin_original_dll_initialization.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_dll_initialization_tests
ctest --test-dir build -R sin_original_dll_initialization_tests --output-on-failure
```

## Next real Windows milestone

Reconstruct and test the original engine-facing startup **callbacks after each mandatory prefix**; especially server global interface post-registration and client material/renderer subsystem initialization. Derive exact MSVC32 signatures from real native call sites and vtables before implementing them. Then implement the minimum real game-interface objects and test them inside the legally owned original Windows executable rather than only a synthetic loader.

**Not completed:** actual original Windows DLL game initialization, engine map load, networking, AI, original gameplay executable module, Android gameplay or Quest 3 gameplay. The current work is address-verified research plus portable test references.