# Phase 185: original SiN client.dll `tier0` heap forwarding boundary

**12 November 2026.** Stacked on Phase184 draft PR #237, unmerged. Original
Windows x86 *SiN Episodes: Emergence* (2006) owner-provided `client.dll` and
`server.dll` verified by full SHA256. Original binary bytes, assets and full
native disassemblies remain private and are not committed.

## Why this matters, and what it does *not* recover

The KeyValues lifecycle recovered in Phases 183–184 calls the original
`client.dll` function **`0x2416FD60`** to release value buffers. This is
**not an in-module heap implementation**. It is a direct forwarding function
through the imported `tier0.dll` **`g_pMemAlloc`** global.

A direct parse of the original `client.dll` PE32 import directory proves:

| Imported DLL | Actual imported symbol | Original IAT VA |
|---|---|---|
| `tier0.dll` | `g_pMemAlloc` | `0x242761CC` |
| `vstdlib.dll` | `KeyValuesSystem` | `0x24276230` |

The original KeyValues constructor's call to `KeyValuesSystem()` therefore
crosses **another independent external engine-library boundary**, not the
`tier0` allocator interface itself. The concrete implementations of
both imported facilities are outside these inspected SiN game DLLs and
remain unresolved without the corresponding original dependency binaries or
runtime traces.

## Seven complete original Win32 client functions recovered

Native client file: **3,592,192 bytes**, SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
Native server file: **6,049,792 bytes**, SHA256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
Native client PE32 original image base `0x24000000`.

| Original VA | Region length | Exact original full-region SHA256 | Behavior |
|---|---:|---|---|
| `0x2416FCF0` | 79 | `84fffc04db883f146d64edf1f474b61b5fe03cb36e95b5fcd717ece17e793acf` | Lazy cached-context allocation forwarding, variant A |
| `0x2416FD40` | 27 | `06b28fa8c3b5d5701c214e37a09179591199cf5de9364c3f37e0349fbafc7b17` | Explicit three-argument allocation forwarding, A |
| `0x2416FD60` | 18 | `c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73` | Direct free forwarding, A |
| `0x2416FD80` | 79 | `44318a97f116b1b629cb2fe1794e14c28ed58b61a7606b7784a4e7b785c20d08` | Lazy cached-context allocation forwarding, variant B |
| `0x2416FDD0` | 27 | `06b28fa8c3b5d5701c214e37a09179591199cf5de9364c3f37e0349fbafc7b17` | Explicit three-argument allocation forwarding, B; byte-identical to A |
| `0x2416FDF0` | 18 | `c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73` | Direct free forwarding, B; byte-identical to A |
| `0x2416FE10` | 14 | `e0cbbd04cffa5347279a626191f42bafb3f112ce195ffd26d344f03b26a896e4` | Report whether imported `g_pMemAlloc` pointer is nonzero |

**Seven distinct original native entrypoints, 262 original code bytes**.
Identical free and explicit-debug-alloc routines occur at distinct native
addresses; they are counted as distinct *entrypoints*, not different logical
algorithms. The two cached allocator variants differ in their original `CALL
rel32` displacement because they reside at different native VAs; both target
`0x2416F7F0`.

### Native x86 allocator callback contract

The original routines load IAT slot `0x242761CC`, then **dereference the
imported `g_pMemAlloc` global**, read the resulting allocator object's vtable
and call one of three original virtual **byte offsets**:

- **`+0x00`** with three original stack arguments: size, opaque owner/context,
  and line (in the cached variants this final argument is constant **0**).
- **`+0x04`** with a single size argument, the fallback allocation path.
- **`+0x14`** with a single pointer, the native free path. Neither of the
  two free bridges checks for a null pointer before calling this slot.

Both lazy allocator wrappers check bit 0 of the DWORD global at
`0x2437290C`; if not set, they OR bit 0 into it, call `0x2416F7F0`, then
cache its returned pointer at `0x24372908`. This happens **once even if the
resolver returns null**. If cached pointer is zero they call allocator
virtual `+0x04`; if nonzero they call `+0x00` with the cached pointer and zero
as extra arguments. This is a process-global cache shared by both wrappers,
**not** per-call memory. The other flag bits are left unchanged.

`0x2416F7F0` itself calls a different `IMemAlloc` virtual offset `+0x5C` and
additional imported OS APIs; its full behavior remains unported. Treat the
cached DWORD as opaque, not proof of a filename, debug mode enabled or any
particular owner semantics. The original external `tier0.dll` allocator,
heap lifetime, thread safety, and exact engine object ownership are **not
reconstructed** by these seven forwarding stubs.

The original `0x2416FE10` checks the pointer value stored in the imported
allocator global and returns `0` or `1`. It is not a heap-integrity
validation nor proof that allocation is always safe.

### Original evidence and independent tests

- `research/tools/audit_sin_original_client_memalloc_bridge.py` independently
  parses the original PE32 import directory, checks the exact **module and
  symbol names and original IAT VAs**, both complete original DLL hashes,
  **seven complete native function-region SHA256s, 37 sparse original x86
  opcode witnesses, and both direct rel32 calls to `0x2416F7F0`**.
- `research/tests/test_sin_original_client_memalloc_bridge.py` has **11
  grouped tests**, original-free mutation checks of all 37 witnesses and
  both call targets, IAT/loader-import errors, bad PE base and synthetic
  full-body hash rejection. A separately guarded original-only test is
  skipped automatically when original DLL files are absent in CI.
- `include/sin_compat/original_client_memalloc_bridge.hpp` is a portable C++17
  callback-driven **dispatch/reference model** including the precise cached
  debug bit and one-time resolution, direct three-argument allocation, raw
  unconditional free forwarding, and allocator pointer presence check.
- `tests/test_original_client_memalloc_bridge.cpp` verifies context-null
  fallback, cached-context path, first-call cache behavior, preservation of
  unrelated flag bits, direct debug argument order, unconditional free of
  `nullptr`, original 32-bit allocation size forwarding, and native metadata. The
  same test also links the **Phase184 KeyValues destructor** through its
  `release_buffer` callback to the Phase185 tier0 Free bridge and checks
  ordered child-before-parent buffer releases and forwarding of null values.
  Locally tested with GCC and Clang `-O2 -Wall -Wextra -Wpedantic -Werror`.
- Root CMake and the real Windows x86 MSVC proxy CI both add a CTest target.
  That test uses synthetic callbacks and never links the original engine.

## Next steps and limitations

1. Analyze original `0x2416F7F0` and global debug-owner setup, and map the
   `tier0.dll`/`vstdlib.dll` exported ABI when their exact original binaries
   are available. Avoid assuming a modern Source SDK has an identical layout.
2. Reconnect Phase184 KeyValues cleanup and Phase183 string ownership through
   an **actual tested Win32 engine adapter** instead of only reference models.
3. Demonstrate an original executable calling real replacement
   `ServerGameDLL003` and `VClient011` interface `Init`/shutdown methods,
   then a level, player/weapon and HUD, before Android and Quest 3 gameplay.

**Reviewed-function atlas** grows **155 → 162** (**74 server, 88 client**).
This is a count of separately verified native function entrypoints, not a
percentage of the complete game, playable DLLs, or a runnable Quest port.