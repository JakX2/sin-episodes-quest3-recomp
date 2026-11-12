# Phase 181: Original 2006 client surface-properties manifest startup

12 November 2026. Stacked on Phase180 draft PR #233; these original Win32 PE32 `client.dll` and `server.dll` files remain owner-only, with no proprietary game DLLs, game assets, original function bodies or complete disassemblies committed. **Windows engine integration first, Android and Quest 3 later.**

## Original executable evidence

- Owner-original client.dll: 3,592,192 bytes, whole-file SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, PE32 base `0x24000000`.
- Owner-original server.dll: 6,049,792 bytes, whole-file SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, PE32 base `0x22000000`.
- Original physics-boot caller, independently recovered in Phase180, uses `CALL 0x241927A0` at **`0x24182CB1`** after successfully obtaining all three original client physics interfaces, `VPhysics030`, `VPhysicsSurfaceProps001`, and `VPhysicsCollision007`.
- New owner-only Phase181 audit SHA256-verifies **seven entire original client x86 function regions totaling 355 native bytes**:

| Native VA | Length | Complete original region SHA256 | Observed role |
|---|---:|---|---|
| `0x241927A0` | 247 | `7357bdfb9934f175e0d67f4a81236d81087a07bb6145918083c44fd255ca6905` | Surface-properties manifest loader, dispatch and cleanup |
| `0x24164B90` | 4 | `0b78473b3a56c63ff2a64431fb600078066cbe407d3dee7ea8fd7575bd7f82d5` | First KeyValues child from node +0x1C |
| `0x24164BA0` | 4 | `38058506e8e859d5be73a33baa621352c08ea01f2224ee170173253757193941` | Next KeyValues peer from node +0x18 |
| `0x24164B20` | 32 | `5264003f4859e6bc4704ba42c9ba251e689725135f8d3c4c6b4a4b3815ee90da` | Find KeyValues child by a DWORD symbol at node +0 |
| `0x24164770` | 21 | `e6fedc91b795bc4c5e8120f9744bfc75b5ca12785e94bc50411881423cf5710f` | Get entry name via external KeyValuesSystem interface virtual +0x10 |
| `0x24164C60` | 19 | `33a5a0dbfb95c16aeb48f7a64b929b8ccf3742e823518c3b01e20a076ac6dd11` | Forward allocation size via external KeyValuesSystem interface virtual +0x04 |
| `0x24166050` | 28 | `7907cfaa8ae3f25951a87879d7050db0d8491e6b835008976883899a004814f2` | Null-guarded KeyValues cleanup and allocator release |

The audit checks 56 **sparse original x86 instruction witnesses** (not complete executable bodies), 11 direct original calls *inside* these routines, one Phase180 incoming caller, two original tail JMPs into the cleanup routine, five exact game/manifest strings, and the original global filename pointer. It chains Phase180 and preceding original binary auditors; **seven native function hashes are new and distinct**, not a speculative tally of every original routine in the DLL.

## What the native 0x241927A0 loader does

The exact original binary identifies the manifest by a module `.data` pointer at `0x242FD18C` to the `.rdata` C-string `scripts/surfaceproperties_manifest.txt` at `0x242A2C5C`. The parser uses `GAME` at `0x24293B58` as its search-path ID.

1. Request 0x28 bytes through original KeyValues allocator bridge `0x24164C60`; on success construct a KeyValues root via `0x24165010`. The source has no general allocator-failure safety contract; on a zero allocator result it still follows the parse call with a zero record pointer. Our C++ model instead explicitly throws.
2. Invoke the KeyValues file-parsing helper `0x24164A50` with the original filesystem pointer, manifest name and `GAME` search path. If it fails, issue the original diagnostic `Unable to load manifest file '%s'`, then call KeyValues destruction `0x24166050`. The loader is a `void`-like callback, not a recovered success/failure-return interface.
3. On successful parse, walk the first child at +0x1C and peers at +0x18. Read each KeyValues entry name through the original dynamic KeyValuesSystem +0x10 bridge `0x24164770`.
4. Compare the entry name against literal `file` at `0x24293D00` using original function `0x2425F81B`. Disassembly of that function shows **case-insensitive** comparison (with locale-dependent character folding), not a case-sensitive `strcmp`. For matching entries, the original dispatches an opaque KeyValues value lookup `0x241667C0`, then a separate surface-file loading helper `0x24191EC0`. The model represents the pair as one explicit host callback rather than inventing the intermediate file-string behavior.
5. Other names issue the original diagnostic `surfaceprops::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'`, through an original function pointer; this does not stop the loop.
6. Advance to next peer and repeat. After exhausting entries, tail-jump to the same native KeyValues cleanup used on parse failure.

**Important original parameter orientation:** Phase180 `0x24182CB1` calls this loader with `(surfaceProps, filesystem)` on the C stack. In this function, after its four register saves, it reads `[ESP+0x18]` as **filesystem** and `[ESP+0x14]` as **surfaceProps**. The host-neutral model preserves this argument order.

The original lookup bridges `0x24164B90` / `0x24164BA0` access `+0x1C` / `+0x18`. Separate `0x24164B20` walks +0x18 until a child symbol DWORD at +0 matches the requested argument, returning null otherwise. All three original methods are pointer-width sensitive and assume valid native C++ object memory. The C++ model provides 32-bit **layout metadata** plus separate safe host objects; it never treats 64-bit host pointers as executable original x86 objects.

## Artifacts and tests

- `include/sin_compat/original_client_surface_manifest.hpp`: host-neutral C++17 branch/reference model, checked linked-node helpers, original Win32 offset assertions and injected engine callbacks. Callback boundaries include KeyValues allocation/constructor/parser, string comparator, surface-file dispatch, diagnostics and destruction.
- `tests/test_original_client_surface_manifest.cpp`: successful empty/file/unknown mixed parsing, case-insensitive file type, parse failure log+cleanup, null allocator/constructor safe diagnostics, linked symbol search and cycle guard, destructor callback order. Passed locally under GCC and Clang, optimized `-Wall -Wextra -Wpedantic -Werror`.
- `research/tools/audit_sin_original_client_surface_manifest.py`: original owner-only SHA-pinned binary and sparse x86 witness verifier, chained through Phase180 and prior original full-DLL provenance.
- `research/tests/test_sin_original_client_surface_manifest.py`: **11 passing** original-free independent fixture/mutation tests, covering every sparse instruction, all direct/incoming calls, both tail JMPs, every string literal, filename pointer, native length/hash fixture rejection, and malformed full-DLL rejection.
- Root CTest and actual Win32 PE32 CTest target `sin_original_win32_client_surface_manifest_contract` run the new model against synthetic host callbacks. **Not original game execution.**
- Existing recovered-function atlas: **134 → 141 (74 server, 67 client)**, 7 new SHA-pinned original functions. Original function count is a reviewer index, not a whole-game decompilation percentage.

### Run with legally held private originals

```sh
python3 research/tools/audit_sin_original_client_surface_manifest.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_surface_manifest.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_client_surface_manifest_tests
ctest --test-dir build -R sin_original_client_surface_manifest_tests --output-on-failure
```

## Unresolved dependencies / next steps

Original KeyValues file parser `0x24164A50`, its key/name allocator interface, string extraction at `0x241667C0`, and actual surface file loading at `0x24191EC0` remain unresolved native engine/library behavior. This phase does not implement full Windows physics, Source engine vtable signatures, client/server game DLL initialization, map startup, or original SiN asset loading. Next: inspect the original surface-file loader and/or the server-side post-init path; validate original gameplay with Windows x86 runtime before any Android/Quest3 game port.