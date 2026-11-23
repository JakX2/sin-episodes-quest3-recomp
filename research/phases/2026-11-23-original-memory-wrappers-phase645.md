# Phase 645: Original SiN 2006 symbol allocation wrappers and module tags

## Scope and owner retail provenance

This phase continues actual reverse engineering of original *SiN Episodes: Emergence* `server.dll` behavior needed to reconstruct the symbol registration path. Owner priority excludes generic Half-Life 2 physics, models, props, materials, surface properties and rendering, which already function in ported levels. No such work was performed.

Exact owner-private original 2006 retail PE32 binaries verified:
- server.dll SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

**Four complete original server.dll methods totaling 277 bytes**:

| Original method | Executable bytes | SHA256 of *entire original method* |
|---|---:|---|
| `0x2226E290` tag discovery | 90 | `84049bbc203aa4d0f0fa107d0294be44dd7075dcd827a13f7c2fda933f9f0659` |
| `0x2226E390` allocate | 79 | `8c8192a3d478efa799525609e38affe36e9548e5ee449e57aadc914237e13d1a` |
| `0x2226E400` reallocate | 90 | `fccf3ba0ea064576d8806a024df924f2281e8829d202ae99c023010f2283d110` |
| `0x2226E3E0` free | 18 | `4d45603c1544ce399c6783bb8fd0019698929a223663d256d112ce57d66868fc` |

All original complete-method ends have confirmed `INT3` next-byte padding, and original code maps into executable `.text`. Owner-only audit checks **5 original direct rel32 edges**, **7 original conditional/unconditional branch destinations**, **89 address-specific x86 opcode witnesses** and both unchanged original whole-DLL SHA256 fingerprints. No original DLL binaries or executable code blobs are distributed in the repo.

## Native memory-manager interface

The memory-manager singleton is obtained by reading DWORD `[0x223D2124]` to get a pointer-to-pointer, then reading that pointer to get the instance and reading `[instance]` to get its vtable. The original functions select distinct vtable slots using the original call convention (parameters pushed right-to-left):

| Path | Original vtable offset | Actual guest arguments pushed |
|---|---:|---|
| Untagged allocate | `+0x04` | `bytes` |
| Tagged allocate | `+0x00` | `bytes, tag_string_pointer, 0` |
| Untagged reallocate | `+0x0C` | `old_pointer, bytes` |
| Tagged reallocate | `+0x08` | `old_pointer, bytes, tag_string_pointer, 0` |
| Free | `+0x14` | `old_pointer` |
| Probe tag mode | `+0x5C` | no explicit arguments, tests AL |

The original Phase644 owner methods call these exact native wrappers:
- `0x22280E30 → 0x2226E390` to allocate;
- `0x22280E1B → 0x2226E400` to reallocate;
- `0x22280DB0 → 0x2226E3E0` to free.

### Independent one-time tag caches

**Allocation**:
- flag at absolute DWORD `0x225885E0`, cached tag-pointer DWORD `0x225885DC`.
- inspect the flag's low **BYTE** bit 0; on first access perform a full 32-bit OR with 1 **before** calling original `0x2226E290`.
- persist the returned tag pointer even if zero; future allocation calls use the cached pointer without repeating discovery.

**Reallocation**:
- separate flag at `0x225885E8`, cached tag at `0x225885E4`.
- exact same separate first-call mechanism, with a distinct static global cache.
- original direct calls into the same discovery function at `0x2226E3A3` and `0x2226E413`.

The partial initialization order is significant: a failure after the flag write (for example, during tag discovery) leaves flag bit 0 set and potentially the cache pointer unchanged. The portable model preserves completed writes and refuses the failed host action without fabricating rollback.

### Tag discovery original 0x2226E290

Loads the memory-manager interface and invokes its virtual method `+0x5C`, testing **AL** for a nonzero result. When zero, returns pointer zero without calling any Windows APIs.

On a nonzero result, the original dynamically resolves the module filename via imported Windows APIs, as confirmed against the original PE32 import table:

1. `GetProcessHeap()` returns a heap handle.
2. `HeapAlloc(heap,0,260)` obtains an optional 260-byte filename buffer.
3. `VirtualQuery(0x225885D8,...,28)` writes a Win32 28-byte `MEMORY_BASIC_INFORMATION` record, whose first DWORD `AllocationBase` is then loaded.
4. `GetModuleFileNameA(AllocationBase, buffer,260)` fills the tag string.
5. The original returns the `HeapAlloc` buffer address, ignoring `GetModuleFileNameA`'s return value. It also does **not** check the allocated buffer for zero before the other Windows calls.

The portable source requires typed host callbacks for Win32 APIs and vtable calls rather than substituting real Windows imports on Android. On a callback error, source returns `HostRejected`; this is a defensive portable modeling convention, not an error return of the original x86 method, which typically does not check failed imports or out-of-range pointers. In particular, the original's uninitialized stack record on a failed `VirtualQuery` call is deliberately not emulated.

## C++17 reconstruction and tests

Introduces `AllocatorWrappers` with `allocate`, `reallocate`, `free` and standalone `discover_tag`. The reference explicitly re-reads the original manager pointer and vtable for each call rather than caching the native virtual target. The typed `IHost` permits the owner to supply in-engine memory-manager calls at runtime and records dispatch offsets, method pointers, and committed writes.

Compiled local regression tests exercise:
- both first-call **untagged** allocation and reallocation, with independent flag/tag cache, full DWORD high-bit preservation, second-call cached paths and non-probing free;
- both **tagged** allocation/reallocation paths and all four actual named Win32 import-boundary callback sequences;
- original vtable offsets and precise argument counts, including trailing zero parameter in tagged paths;
- first-cache flag-before-discovery and partial-write/host-failure effects; unresolved memory-manager pointer and invalid vtable addresses; host exceptions and reentrant calls;
- complete compiled **Phase638→639→643→640→642→641→644→645** symbol registration through a synthetic guest memory manager that dispatches via the *recovered* original vtable offsets rather than Phase644 direct fake allocation callbacks. First allocate and **five slot-array reallocations** preserve 39 symbol records, the tree shape and mapped IDs; release uses Phase645's recovered free wrapper.

Local compiled C++17 and AddressSanitizer/UndefinedBehaviorSanitizer tests pass. Public CI additionally runs an invented-byte mutation auditor and native Win32 PE32 x86 plus Android Bionic compatibility checks.

Owner-only retail auditor:

`python research/tools/audit_original_memory_wrappers_phase645.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

Original-free public self-test:

`python research/tools/audit_original_memory_wrappers_phase645.py --self-test`

## Known remaining gaps

**The *underlying original memory-manager vtable implementations* are not decompiled.** This phase recovers their ABI dispatch, not the implementation of the engine allocator itself. The virtual tag probe `+0x5C` is host supplied; the Windows imports require host support or equivalent Android adaptation. Original comparator `[table+0]`, original symbol lookup `0x2228F7A0` and actual original game-process invocation remain unresolved. Passing synthetic host-memory tests and Android `dlopen` smoke is not an original in-level game or a Quest 3 playable build.

Continue prioritizing missing original SiN-specific gameplay routines rather than duplicating already-functioning HL2 subsystems.
