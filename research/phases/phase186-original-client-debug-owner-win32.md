# Phase 186: Original Win32 client.dll allocator filename context and cached dispatches

12 November 2026. Stacked draft continuation of Phase185 (#238). The owner's
original Windows x86 PE32 SiN Episodes: Emergence (2006) `client.dll` and
`server.dll` are private. Only independent code, tests, SHA256 evidence and
sparse opcode signatures are published, not original binaries or complete
native disassemblies.

## Native provenance and original entrypoints

- Original `client.dll`: 3,592,192 bytes; SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`;
  preferred PE32 image base `0x24000000`.
- Original `server.dll`: 6,049,792 bytes; SHA256
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

**Four new, completely hash-verified native client.dll routines, totaling
314 bytes**. These are newly reviewed distinct original entrypoints, not
proof that the game runs with our replacement DLLs.

| Native VA | Bytes | SHA256 of entire original function | Role |
|---|---:|---|---|
| `0x2416F7F0` | 90 | `c2a27c407aa1fa127dc525282abb96d283e1026826f428c0855df9480123cce0` | Imported allocator's virtual `+0x5C` gate, Win32 module-filename context creation |
| `0x2416F850` | 71 | `e42a8f93b8166c2510985587937279800f654aa5ec7483aa6efec9cfaa6f200c` | Cache-based allocation with requested size in original ESI register |
| `0x2416F8A0` | 74 | `fd969723fbe917bc9107e2920679fed8dcfd5c91809fe57db7330651d4dbe7ac` | Separate cached context for two raw values from original EDI and ESI |
| `0x2416F8F0` | 79 | `e4682f7b52bf956d9769e11f7390b3f6b17076d25e36f35951211b5a6e1fe82a` | Same primary cache, requested size from original first stack slot |

### What the original resolver actually does

At `0x2416F7F0` the original code first loads the imported
`tier0.dll!g_pMemAlloc` pointer at IAT `0x242761CC`, dereferences it,
and calls the allocator vtable **byte offset `+0x5C`**. A zero AL skips
all Windows API calls and returns null. We do not assert a formal C++ name
for that virtual method.

On a nonzero AL, it executes the following native order:

1. Call `KERNEL32.dll!GetProcessHeap` (IAT `0x24276098`).
2. Call `HeapAlloc(heap, 0, 0x104)` (`0x2427609C`) and save the returned
   pointer, even if zero.
3. Call `VirtualQuery(&global_0x24372904, &stack_MBI, 0x1C)`
   (`0x242760A0`). The original 28-byte `MEMORY_BASIC_INFORMATION` has
   `BaseAddress` at +0 and **`AllocationBase` at +4**.
4. Use **AllocationBase**, not BaseAddress or `0x24372904`, as the module
   handle for `GetModuleFileNameA(AllocationBase, heap_buffer, 0x104)`
   (`0x242760A4`).
5. Ignore the return values of `VirtualQuery` and `GetModuleFileNameA` and
   return the allocated pointer. Heap allocation failure is not checked
   either. Thus native invalid API outcomes may lead to undefined behavior;
   a portable host fixture always defines its MBI instead of reproducing
   uninitialized stack data.

**Crucial dependency:** this resolves the formerly opaque filename/debug
owner pointer cached by Phase185. It is allocated through the Win32 **process
heap** using `HeapAlloc`, not through `tier0.dll!g_pMemAlloc`. This phase
proves that the cache refers to module-name storage but does not prove a
later corresponding `HeapFree` site or its lifetime policy.

### Three adjacent cached dispatches

- `0x2416F850`: original size passed in ESI, primary cache flags
  `0x2437290C` and pointer `0x24372908`; vtable +4 plain allocation or
  +0 debug-owner allocation `(size, owner, 0)`.
- `0x2416F8F0`: first x86 stack argument, same primary cache and dispatch.
- `0x2416F8A0`: raw EDI and ESI operands and **independent** secondary cache
  flags `0x24372914` and pointer `0x24372910`; calls original allocator
  vtable +0x0C with two operands, or +0x08 with `(EDI, ESI, owner, 0)`.
  No unsupported high-level name such as `Realloc` is assigned.
- All three OR bit0 into original flags *before* calling `0x2416F7F0`,
  then cache the returned pointer once even if zero. Unrelated flags remain
  unchanged. The two primary wrappers share one cache; the dual-register
  wrapper uses another.

## Recovered code, verification and CI

- `include/sin_compat/original_client_debug_owner.hpp`: self-contained
  C++17 host-neutral ABI constants, exact 28-byte Win32 MBI structure,
  original resolver call order, and independently stored cache dispatch
  behavior. Every native engine allocator/Win32 action is injected via a
  host adapter. The resolver intentionally does not pretend that missing
  `VirtualQuery` output is a valid address.
- `tests/test_original_client_debug_owner.cpp`: tests original allocator
  probe short-circuit, full five-call Windows API order, allocation-base
  usage, failed-heap return without fabricated checks, two separate cache
  domains, once-only resolution including zero, caller-register/stack
  source agreement and integration with **Phase185** allocation bridge.
- `research/tools/audit_sin_original_client_debug_owner.py`: parses the
  original PE32 sections and actual import directory, verifies exact
  five original KERNEL32/tier0 imported symbols/IAT VAs, all four native
  whole-function SHA256 digests, **48 disjoint sparse x86 witnesses** and
  **five exact CALL rel32 edges** (including two incoming from Phase185).
- `research/tests/test_sin_original_client_debug_owner.py`: 11 original-free
  Python regression groups mutating every sparse witness, all five call
  edges, all five imports and invalid binary hashes. Native original-only
  audit and optimized GCC/Clang `-Werror` tests passed locally.
- `tests/test_original_client_debug_owner_win32.cpp`: **actual Windows x86
  API integration smoke**, still synthetic with respect to the tier0 vtable.
  On a 32-bit Windows CI runner it invokes actual GetProcessHeap, HeapAlloc,
  VirtualQuery against a variable in the smoke executable, and
  GetModuleFileNameA using the MBI AllocationBase; verifies the executable
  filename then calls HeapFree for the test allocation. The original anchor
  `0x24372904` is remapped to the synthetic executable variable and no
  claim of loading the original SiN engine is made.
- CMake root CTest and two MSVC Win32 x86 tests (synthetic model plus real
  kernel32 integration smoke) are registered. Existing CI status must be
  verified after PR creation.

## Remaining limitations and next milestone

The byte-verified native snippets do not make the game executable. The
external tier0 IMemAlloc vtable `+0x5C` implementation, original engine
callback lifecycles and full Source virtual method signatures remain unknown.
There is no proof that replacement `client.dll`/`server.dll` initialize under
the original Windows engine. Next: inspect the original DLL load/Init
callers, implement engine-owned `VClient011` and `ServerGameDLL003` objects,
and perform a controlled Windows-original-executable loader test using
legally owned game assets. Only after Windows gameplay should Android and
Quest 3 ports be attempted.