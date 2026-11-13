# Phase 205: original SiN 2006 server/client tier0 allocator mirrors

13 November 2026. **Stacked draft on Phase204 PR #258.** Owner-only analysis of the original Windows i386 `server.dll` and `client.dll` from `bin.zip`, each validated against its entire original file SHA256. The original binaries, raw native code bodies, maps and commercial game assets are **not** committed.

## Four NEW complete original functions, not duplicates of Phase186/202

| Original game module | Native entry | Complete bytes | Full original code SHA256 |
| --- | --- | ---: | --- |
| server register-size allocation helper (ESI) | `0x2226E2F0` | 71 | `e2c2ae1408ce91691982e97c2d3836de3345664f7d177fee7c5313bab26d6389` |
| server register-pair resize-like helper (EDI, ESI) | `0x2226E340` | 74 | `2ab07ccb733a46a88571bb1cb32596b85937dc904b5454c2c1e714b4275fd4c2` |
| client tier0 deallocation wrapper | `0x2416F940` | 18 | `c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73` |
| client stack-argument resize-like wrapper | `0x2416F960` | 90 | `c763dd4ddaf1c66858586c95c266ee2d19585a9f655e91edb9bfcb3d6d9e3cc2` |

**253 additional entire original x86 bytes across four distinct new functions.** The other **eight** methods in the same 12-method paired family were already verified/counted in **Phase186** (four client methods) and **Phase202** (four server methods). This phase revalidates those originals but **does not count them twice**.

Whole-file original SHA256: server `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` (6,049,792 bytes); client `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` (3,592,192 bytes).

## Six complete byte-equivalent server/client pairs

The entire native bodies of **six distinct x86 functions in each DLL** become byte-for-byte identical after zeroing **only** these precise 4-byte absolute operands (addresses of imported functions, imported allocator pointer slot, or module-local cached data):

| Pair | Original server VA | Original client VA | Per-body length | Normalized entire-body SHA256 |
| --- | --- | --- | ---: | --- |
| Module filename/debug owner resolver | `0x2226E290` | `0x2416F7F0` | 90 | `ce5fa05dbc531d8e1e56c533e4daf49494b6a299dcab1c0f345c9b562d3078b9` |
| ESI register allocation | `0x2226E2F0` | `0x2416F850` | 71 | `5fce91c2edbdc0a168b7bc0863a8435ae527df3779c2a11d128261f9cd39671e` |
| EDI/ESI register pair dispatch | `0x2226E340` | `0x2416F8A0` | 74 | `6b0df45d7f10cb4aafb611dc1431234c111a7c5bc27fb56f3f52c891fa1f893d` |
| Single stack-argument allocation | `0x2226E390` | `0x2416F8F0` | 79 | `90bc6feef7698711126a7ee4709f4df014192074dd2fe7b4f7fc9b23e0c7fea6` |
| Direct deallocation | `0x2226E3E0` | `0x2416F940` | 18 | `4dce485e5dfc86aeb657bbf2afc126697e94af00560fba6d1f38612be43ee2a7` |
| Two stack-argument resize-like dispatch | `0x2226E400` | `0x2416F960` | 90 | `103d20dddb5d5e77d3c8f47b494798b3c1e75e8336d095a53999344a67d667b1` |

The original call displacements in the eight direct `CALL rel32` edges from four wrappers per DLL to the DLL-local 90-byte context resolver **already match without normalization**. Those are separate, directly verified original native call edges.

The full pair comparison covers exactly **62 absolute DWORD operands** across the 12 methods (31 per module), 48 sparse original opcode witnesses, all original `ret`/INT3 boundaries, both complete DLL SHA256 fingerprints, **12 complete native method hashes**, and six normalized complete-body SHA256s.

## What the native import tables resolve

Both original DLLs import a variable named `tier0.dll!g_pMemAlloc`, **but each has its own import address table slot**: server `0x223D2124`, client `0x242761CC`. Their allocator methods load the slot and indirect through the imported allocator object and its virtual function table. This is not an ordinary statically linked CRT allocator and the two module-local caches are separate.

Phase202 described four Win32 IAT helpers in the server context initializer conservatively as opaque. Direct native PE import-name decoding and the **full function body equality** now prove their identities and order: **`KERNEL32.dll!GetProcessHeap`, `HeapAlloc`, `VirtualQuery`, `GetModuleFileNameA`**. This agrees exactly with the Phase186 client analysis of the same original 90-byte routine.

| Actual original import | Server original IAT VA | Client original IAT VA |
| --- | --- | --- |
| `tier0.dll!g_pMemAlloc` | `0x223D2124` | `0x242761CC` |
| `KERNEL32.dll!GetProcessHeap` | `0x223D2024` | `0x24276098` |
| `KERNEL32.dll!HeapAlloc` | `0x223D2020` | `0x2427609C` |
| `KERNEL32.dll!VirtualQuery` | `0x223D201C` | `0x242760A0` |
| `KERNEL32.dll!GetModuleFileNameA` | `0x223D2018` | `0x242760A4` |

Both modules independently store a primary cached owner pointer and initialized flag, and a secondary owner pointer/initialized flag:

- Server `0x225885DC / 0x225885E0` and `0x225885E4 / 0x225885E8`.
- Client `0x24372908 / 0x2437290C` and `0x24372910 / 0x24372914`.

Within each module, **both register and stack wrappers reference those exact same two cache records**. Their native color/context bit tests set bit0 before calling their local resolver. The result may be zero, and the zero is cached as a failure/disabled context. The original 90-byte resolver obtains process heap via `GetProcessHeap`, calls `HeapAlloc` for a 260-byte filename buffer, queries memory with `VirtualQuery` using a 28-byte MBI, then calls `GetModuleFileNameA` with the returned MBI `AllocationBase`. As proven in Phase186, the `HeapAlloc` result is not checked before using the buffer, and the context pointer's ownership and free lifetime are not established here.

Native dispatch behavior verified from both original x86 bodies:

- Size in **ESI** or first stack argument: vtable `+0x04` without an owner or `+0x00` with `(size, owner, 0)`.
- EDI/ESI pair or two stack arguments: vtable `+0x0C` without owner or `+0x08` with `(first, second, owner, 0)`. These are *resize-like* allocator calls; the specific host allocation implementation is not recovered.
- Free pointer argument: **vtable `+0x14`** without a cached owner.

**Caution:** byte-identical dispatch code in client/server does **not** imply pointers may be freely transferred between modules or across a Windows x86 versus Android ARM64 boundary. Actual process-global allocator ownership and lifetime still require engine-level verification.

## Source, tests and limitations

- `research/tools/audit_sin_original_cross_dll_tier0_allocator.py`: owner-original full SHA256 gate, independently decoded original PE INT/IAT bindings using Phase204 import parser, twelve complete method hashes, 62 exact original embedded pointers, 48 sparse opcode witnesses, eight direct native calls, return/alignment boundaries, six normalized complete pairs. Does not embed original code.
- `research/tests/test_sin_original_cross_dll_tier0_allocator.py`: ten original-free synthetic mutation groups, including original/normalized hash refusal, opcode/address/CALL corruption, wrong sections/image base, and wrong import DLL/name ownership.
- `include/sin_compat/original_cross_dll_tier0_allocator.hpp`: C++17 host-neutral compositional contract based on the existing Phase186 client and Phase202 server reference models. Uses **a single 16-byte native cache layout per module**. Register and stack methods adapt their interfaces to the same four native cache DWORDs; server and client use separate instances.
- `tests/test_original_cross_dll_tier0_allocator.cpp`: both module configurations, register-then-stack and stack-then-register caching, owner-disabled zero-cache behavior with unrelated flag bits preserved, basic and owner-tagged virtual dispatch, direct deallocation, and full Phase194 cache slot claim → Phase201 vector growth → Phase202 allocator integration. Host pointer values are synthetic; no retail engine is run.
- Root CTest and native Windows x86 MSVC CTest added. The branch's actual hosted CI must be observed before reporting success.

**Unique reviewed original-function atlas: 203 → 207** (server 111 → 113, client 92 → 94). This is research inventory, **not game readiness**.

The delivery gate remains the owner-only original Windows executable loading independently reconstructed SiN client/server modules and entering actual game startup/map execution. Neither this allocator parity nor a synthetic CI test demonstrates that milestone. Windows PC first, Android next, Quest 3 last.
