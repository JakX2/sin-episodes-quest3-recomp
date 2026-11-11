# Phase 177: Original 2006 Windows DLL export and `CreateInterface` ABI map

**11 November 2026.** Stacked, draft, unmerged continuation of Phase 176 (#229).
This phase deliberately pivots from isolated registry internals toward a testable
**Windows x86 game-module loading boundary**, based on the exact owner's original
*SiN Episodes: Emergence* 2006 `server.dll` and `client.dll`.

## Executable provenance, exact PE exports

- Native original `server.dll`: **6,049,792 bytes**, SHA256
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
  PE32 x86 image base `0x22000000`.
- Native original `client.dll`: **3,592,192 bytes**, SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`,
  PE32 x86 image base `0x24000000`.

**Server original PE exports:**

| Name | Original VA |
|---|---|
| `CreateInterface` | `0x221FB5F0` |
| `?Detonate@CGrenadeHomer@@UAEXXZ` | `0x2230F4E0` |
| `?Detonate@CGrenadePathfollower@@UAEXXZ` | `0x22310EA0` |
| `?Detonate@CGrenadeSpitMutant@@UAEXXZ` | `0x223120B0` |
| `?Detonate@CGrenadeSpitPoison@@UAEXXZ` | `0x22312B70` |

**Client original PE exports:** a *single* undecorated `CreateInterface` at `0x24164180`.
The four unusual server decorated exports must not be silently omitted when
planning a drop-in-compatible replacement DLL, though their necessity under
the running game loader has not been experimentally established.

## Four complete original function regions recovered

| DLL | Native VA | Length | Full original function SHA256 | Behavior |
|---|---|---:|---|---|
| server | `0x221FB5D0` | 32 | `5e4a38bbe846cdfddece56859b823676d516af3f171181d7415ebbdfc1376654` | Linked-list registration constructor |
| server | `0x221FB5F0` | 114 | `39ed7fe9be839faa563964d7f5c2ad810dcd1992511131659da7fe823194b98a` | Original `CreateInterface` dispatcher |
| client | `0x24164160` | 32 | `48699a1f29f3e63113789813739fcb588bfdc154be3047b2a5e712ea873c1a80` | Linked-list registration constructor |
| client | `0x24164180` | 114 | `f74837ce59ab917becd4c214e627ed91762085cb52883617278a206118267eec` | Original `CreateInterface` dispatcher |

Total **292 original native bytes**. These are complete SHA-pinned function
regions, not their proprietary raw code. The owner's actual native assembly
shows identical logic with distinct server/client image-relative addresses.

### Recovered algorithm and ABI contract

1. Registration constructor is original MSVC x86 **thiscall**, receiving
   `ECX = registration node`, stack arguments `(factory function pointer, name
   pointer)`. The node is **exactly 12 bytes**: three DWORDs at `+0`, `+4`,
   `+8` (factory, name, next).
2. It writes factory and name, links `next` to the current module-local head,
   then updates the head to this node; native `RET 8` cleans both args.
   Server registry head is `0x22573C5C`; client head is `0x24369378`.
3. Exported `CreateInterface` uses standard C-style **cdecl** stack args
   `(const char* requestedName, int* returnCode)` with plain `RET`.
   It reads the module-specific registry head and iterates newest-first.
4. Matching is case-sensitive and tests full C-string equality. It is not
   substring matching, prefix matching, or a case-insensitive registry.
5. On miss, writes `1` to non-null return-code pointer and returns `nullptr`.
   On a matching registration, writes `0` to non-null return-code pointer
   **before calling the factory** and returns the factory's pointer. An
   internally null factory return still means a *matched name* and leaves
   return code `0` (this is how the native control flow behaves).
6. Original code assumes valid interface-name and factory pointers on the
   nonempty-registry path. Host models should not interpret the absence of a
   null-name check as an intentional, safe public contract.

The registration constructor changes a **registration list**, not the
Source-engine entity classname factory registry recovered in Phases 170–176.
These are two separate registries and should not be conflated.

## Original native static interface registration inventory

Read-only byte search across both original `.text` sections found exact MSVC
static registration wrappers containing *PUSH interface name, PUSH factory,
MOV ECX static node, CALL native registration constructor, RET*, with one
compiler-generated `POP EBP; RET` epilogue. All wrapper operands are validated:
interface strings point into `.rdata`, factory thunks into `.text`, and static
12-byte registration nodes into `.data`. The owner-only audit pins SHA256 of
the **complete ordered manifest**, preventing missing or changed registration
thunks, function pointers, strings or object addresses.

- **Server: 10 original registrations**, manifest SHA256
  `586dc57ff45e59d4b10e313245a120c02643418eea34403544bdd1edb480d461`.
  Includes `ServerGameDLL003`, `ServerGameEnts001`, `ServerGameClients003`,
  `GameMovement001`, `IEffects001`, `PlayerInfoManager001` and `002`,
  `BotManager001`, `HLTVDirector001`, `PluginHelpersCheck001`.
- **Client: 55 original registrations**, manifest SHA256
  `88c9b99ce45befa36461be556e49d0f4abdb47c9cd759c80db74ad82dd3034a9`.
  Includes `VClient011`, `VClientEntityList003`, `VClientPrediction001`,
  `GameClientExports001`, `ClientLeafSystem001`, `VCENTERPRINT002`,
  `GameMovement001`, `IEffects001`, and 47 material proxy registration names.
  (The total is 55, **not** 48.)

See `research/catalog/original_dll_interface_registrations.json` for
all 65 original registration stubs, names, original factory-thunk VAs,
name VAs and 12-byte registration-node VAs. **The list order is sorted by
original static registration wrapper VA, not a claim about global runtime
initializer execution order.** The *runtime* linked list is newest-first.

## New code, independent tests, Windows bridge smoke

- `include/sin_compat/original_dll_interface_abi.hpp` gives the
  three-DWORD original x86 record layout and host-neutral `Registry`
  implementing actual constructor insertion/lookup/return-code semantics.
- `tests/test_original_dll_interface_abi.cpp` exercises duplicates, LIFO
  precedence, exact/case-sensitive match, return values including successful
  name match but null factory return, and separate client/server registries.
- `research/tools/audit_sin_original_dll_interface_abi.py` independently
  parses original PE32 data directories, exports, sections, four full
  original function hashes and all 65 raw wrapper call sites. No image
  contents or proprietary assembly text is published.
- `research/tests/test_sin_original_dll_interface_abi.py` contains **11
  synthetic original-free unit tests**, including PE export ordinal and
  forwarder mutation, `.text`/`.rdata`/`.data` operand guards, frame-pointer
  epilogue variant, corrupt registration calls and complete-SHA rejection.
- `tests/original_win32_loader_probe.cpp`,
  `tests/original_win32_loader_probe.def`, and
  `tests/test_original_win32_loader.cpp` form a **synthetic 32-bit Windows
  LoadLibrary/GetProcAddress smoke test**, compiling a minimal DLL whose
  undecorated export is exactly `CreateInterface`, then calling it with
  the original parameter/return-code convention. This does **not** emulate
  an original `IServerGameDLL` or `IBaseClientDLL`.

### Owner-local original audit invocation

```sh
python3 research/tools/audit_sin_original_dll_interface_abi.py \
  --server /private/original/server.dll \
  --client /private/original/client.dll \
  --output /private/phase177-report.json
python3 -m unittest discover -s research/tests -p test_sin_original_dll_interface_abi.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_dll_interface_abi_tests
ctest --test-dir build -R sin_original_dll_interface_abi_tests --output-on-failure
```

Windows 32-bit CMake/CTest also adds `sin_original_win32_loader_tests` and
verifies loading **the synthetic probe**. An original Steam/engine-hosted
runtime test is the subsequent milestone.

## Next requirements for a genuine drop-in replacement

1. Map the exact original 2006 `IServerGameDLL`, `IServerGameClients`,
   `IServerGameEnts`, `IBaseClientDLL`, `IClientEntityList` and prediction
   interface **vtable layouts and calling conventions**, including
   engine/interface version negotiation and callbacks.
2. Build a replacement module with the required exports and actual
   engine-owned interface implementations. A synthetic export success is **not**
   game initialization or level loading.
3. Run under the legally owned **original Windows game executable**:
   instrument `LoadLibrary`, `GetProcAddress`, `CreateInterface`, first DLL
   init callbacks, and shutdown, comparing against the unmodified original.
4. Reach an original test map, player spawn, first weapon, network update,
   and renderable HUD. Port the verified behavior incrementally, then
   target Android and Quest 3 **only after Windows gameplay works**.

Nothing in this phase proves the original engine loads our reference library,
returns a valid game interface object, or plays a level. Both original DLLs
and all game assets remain private, never in this repository.