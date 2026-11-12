# Phase 187: original server post-emitter startup gates and game-system registration

**12 November 2026.** This is stacked clean-room original-binary research on
Phase186 draft PR #239. The owner's original *SiN Episodes: Emergence* 2006
Windows PE32 i386 `server.dll` and `client.dll` stay private. Do not commit
commercial DLL images, full native disassemblies, maps or assets.

## Binary provenance and what the five functions actually show

- Server original: 6,049,792 bytes, complete SHA256
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
  original image base `0x22000000`.
- Client original: 3,592,192 bytes, SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`,
  original image base `0x24000000`.
- All five **complete original server x86 function regions** were SHA256
  verified directly from these files; **355 machine-code bytes**, distinct
  original entrypoints, not the amount of entire game code ported.

| Original native VA | Complete length | Exact original SHA256 | Proven role |
|---|---:|---|---|
| `0x221F5700` | 277 | `1e2a6e068b24b061fbd5cba16741f8ca8f470421b7ccbca3fa0239f579a9c530` | Consume pending AutoGameSystem linked nodes, detect duplicates, append to installed pointer array, initialize each via vtable[0], return false on first failed initialization |
| `0x221AD8A0` | 10 | `83762183370ab5370d0c099c4350b20768c57043ed99066edb8e1b1804da49e0` | Hardwired singleton thiscall trampoline: ECX=`0x22503EE0`; unconditional JMP `0x221AD3E0` |
| `0x22149B60` | 10 | `47793d5818d074eb9ad8526102a131bb60fd50ab5fcaaa52aa5afe6e940852d9` | Hardwired singleton thiscall trampoline: ECX=`0x224F0388`; unconditional JMP `0x221498C0` |
| `0x221E6450` | 35 | `93b09228e176da1f87b61fb302a3e761389c8f67a788ec1f3e382ddb22df8470` | Earlier server Init helper: descriptor registration and `VEngineCvar002` virtual+0x0C query for `developer`, save result even if null |
| `0x221F56E0` | 23 | `76cce08b11d37afa67eeda003f01b78d06106290ed4205c165cb6704622534d5` | Earlier forwarding helper: reads native game-system count and calls `0x221F54F0` with original vector and incoming argument pointer |

The two `0x221AD8A0` / `0x22149B60` **trampolines do not
implement their actual repository parsers**. The large original code bodies
at `0x221AD3E0` and `0x221498C0` have only been located and partially
inspected. Their file parsing, version gates, memory layouts and owned engine
callbacks have not been ported by Phase187.

## Original mandatory post-emitter gates inside CServerGameDLL Init

Original `CServerGameDLL` vtable slot-zero `0x221E7640` is an 875-byte
function previously full-body SHA-verified in Phase85 and reconfirmed in
Phase179. After obtaining its 16 mandatory engine-interface pointers and
calling the sound emitter's virtual method 0 successfully, the function
performs additional registration and initialization, including event resource
registration and `developer`/`sv_cheats` handling. Then it executes these
**three additional required boolean gates**, in the exact original order:

| Original Init CALL VA | Original callee | Post-call test / failure branch | Condition |
|---|---|---|---|
| `0x221E7927` | `0x221F5700` (game system list) | `0x221E792C` `test al,al`, `0x221E792E` JZ `0x221E79A1` | Must succeed |
| `0x221E7930` | `0x221AD8A0` (singleton A trampoline) | `0x221E7935` `test al,al`, `0x221E7937` JZ `0x221E79A1` | Must succeed |
| `0x221E7939` | `0x22149B60` (singleton B trampoline) | `0x221E793E` `test al,al`, `0x221E7940` JZ `0x221E79A1` | Must succeed |

A failed gate skips all later gates and lands at the same original Init
failure return, `0x221E79A1`, which restores registers and returns `AL=0`.
**Passing these three gates does not yet indicate complete DLL startup:**
there is a later optional `VDebugOverlay003` query, allocation and setup,
and two success-return paths. Those paths remain an explicit next integration
milestone. Do not model the entire engine as ready merely because these gates
succeed.

The original post-emitter code also supplies case-sensitive strings
`resource/gameevents.res` at `0x224428F0`,
`resource/ModEvents.res` at `0x224428CC`, `sv_cheats` at `0x224428E4`
and `developer` at `0x22441F34`. The original game-system duplicate warning
is `AutoGameSystem already added to game system list!!!\n` at `0x22443E38`.
These exact original `.rdata` literals are byte-verified.

## Recovered registration mechanics, native globals, failure behavior

The original server global linked-list head at `0x22572CC0` contains pointers
whose `+4` DWORD is the next node. The game-system vector is rooted in
`0x22510A58`, with count DWORD at `0x22510A64` and capacity DWORD at
`0x22510A5C`. The original:

1. Walks the pending list and compares pointer identity against each pointer
   already installed in the array.
2. For a duplicate, invokes an original diagnostic with the string above,
   does not append that pointer again, and continues to the next pending node.
3. For a new pointer, asks original capacity growth helper `0x221F51A0`
   for more room if necessary; preserves array order (native insertion uses
   original copy/move helper `0x223A5D10`) and appends the pointer.
4. Clears the pending-head global `0x22572CC0` to null and writes byte 1 at
   init flag global `0x22572CB8` **before** invoking any stored game-system
   callbacks.
5. Calls each stored object's vtable slot 0 in registration order; if any
   returns `AL=0`, exits false immediately, without resetting that global
   flag, restoring the pending head or rolling back previous callbacks.
   Otherwise exits true, including for an empty list.

The host model uses a C++ `std::vector` and a finite-list guard instead of
pretending to own original MSVC game-system allocation/capacity behavior.
The host guard is **not observed** in original native code. The full `0x221F51A0`
resize implementation, system constructors/destructors and actual vtable ABI
remain separately unported.

Earlier original Init CALL `0x221E7842` invokes `0x221E6450`: it first passes
descriptor VA `0x22510164` into function `0x2225B350`, then queries the
original `VEngineCvar002` vtable byte offset +0x0C for literal `developer`.
The result is published at `0x22571C58` with no explicit helper-level fatal
null guard. The original `0x221F56E0` is an independent vector forwarding
bridge called earlier from Init `0x221E78D8`; its callee `0x221F54F0` is opaque.

## Source, automated byte-level tests, and limitations

- `include/sin_compat/original_server_post_emitter.hpp`: C++17 original
  address/global metadata, duplicate-pointer registry model, three-gate
  ordered failure propagation, `developer` cvar publisher and registry forward
  boundary. Synthetic hosts own all game-system callbacks and memory.
- `tests/test_original_server_post_emitter.cpp`: empty list, duplicate nodes,
  system failure positions, both repository failure positions, true success,
  global clear/ready ordering, optional null cvar and vector forwarder,
  explicit host-only cyclic list guard. GCC and Clang `-O2 -Wall -Wextra
  -Wpedantic -Werror -UNDEBUG` pass.
- `research/tools/audit_sin_original_server_post_emitter.py`: standalone
  bounded PE32 x86 parser, full paired original DLL SHA gates and **all five
  entire native function SHA256s**, **37 sparse opcode witnesses**, **9 exact
  rel32 CALL relationships** (five original Init incoming and four internal),
  **2 singleton rel32 JMP relationships**, **3 original shared fatal JZ
  branches**, five exact strings and six original native globals.
- `research/tests/test_sin_original_server_post_emitter.py`: **12 passing
  original-free mutation groups**. Each opcode, call, jump, branch and string
  corrupted separately; synthetic fixture cannot satisfy full function SHA.
- Native Win32 MSVC x86 CTest and root C++17 CTest registered; hosted CI must
  independently pass. No original game DLLs are shipped to CI.

Unique original reviewed-function count is **166 -> 171 (79 server, 92
client)** if all five are previously absent in the head ledger. This remains
a **reviewed-entrypoint count**, never whole game percentage.

**Still not done:** original repository parser A/B bodies and their files,
real Source/game-event vtables, complete engine ABI, full DLLInit success path,
original executable hosted DLL loading, Windows gameplay, Android game runtime,
or Quest 3 rendering/input. Working original Windows game first, then Android,
then Quest 3.

### Exact owner-only verification

```sh
python3 research/tools/audit_sin_original_server_post_emitter.py \
  --server /private/original/server.dll --client /private/original/client.dll
python3 -m unittest discover -s research/tests \
  -p 'test_sin_original_server_post_emitter.py' -v
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target sin_original_server_post_emitter_tests
ctest --test-dir build -R sin_original_server_post_emitter_tests --output-on-failure
```