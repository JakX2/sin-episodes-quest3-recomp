# Phase 202: original SiN 2006 server allocator dispatch and ownership boundary

13 November 2026. **Draft stacked on Phase201 PR #255.** Original SHA256-verified owner-supplied Windows 32-bit `server.dll`/`client.dll` inspected privately. No original commercial DLLs, game assets, or complete disassemblies are committed. Windows PC engine integration first, Android next, Quest 3 afterward.

## Four complete original server functions

| Original function | Native VA | Complete x86 size | SHA256 of complete native body |
| --- | --- | ---: | --- |
| Optional allocation-context initializer | `0x2226E290` | 90 | `84049bbc203aa4d0f0fa107d0294be44dd7075dcd827a13f7c2fda933f9f0659` |
| Allocator wrapper | `0x2226E390` | 79 | `8c8192a3d478efa799525609e38affe36e9548e5ee449e57aadc914237e13d1a` |
| Deallocation wrapper | `0x2226E3E0` | 18 | `4d45603c1544ce399c6783bb8fd0019698929a223663d256d112ce57d66868fc` |
| Reallocation wrapper | `0x2226E400` | 90 | `fccf3ba0ea064576d8806a024df924f2281e8829d202ae99c023010f2283d110` |

**Four distinct newly reviewed original native functions, 277 complete original x86 bytes.** Owner-original complete `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` and complete `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` were independently rechecked. Original method hashes, instruction witnesses and exact address operands are additionally checked by the new auditor.

## Native allocator interface is not a direct CRT malloc/free path

Original allocator interface pointer is loaded via the module's global slot at **`0x223D2124`** (the slot itself is in original `.rdata`). The native wrappers dereference the allocator object pointer and its vtable, then dispatch:

| Original interface byte offset | Observed callsite role |
| --- | --- |
| `+0x00` | Allocate with extra context pointer and trailing zero |
| `+0x04` | Allocate bytes without context |
| `+0x08` | Resize with old pointer, bytes, extra context and trailing zero |
| `+0x0C` | Resize with old pointer and bytes, without context |
| `+0x14` | Release pointer, no context |
| `+0x5C` | Test an allocator-side feature bit/boolean used to decide whether optional context is built |

**Separate module-static lazy caches** (not one shared cache): allocation context `0x225885DC`, initialized-flag `0x225885E0`; reallocation context `0x225885E4`, initialized-flag `0x225885E8`. These globals are virtual `.data` addresses; the owner-only auditor does not assume zero-initialized data is file-backed.

The wrappers test bit zero of their own static flag, OR bit zero **before** invoking the common initializer at `0x2226E290`, and cache its result. Zero means basic virtual dispatch. Nonzero means contextual dispatch with explicit extra trailing zero. **A failed/disabled context initialization is also cached**; a subsequent call does not retry unless external code resets the flag. The original free wrapper bypasses both caches and goes straight to virtual `+0x14`.

The context initializer first checks the allocator vtable `+0x5C`, returning zero when false. The enabled branch invokes **four original indirect IAT helper calls** through address slots `0x223D2024/2020/201C/2018`, passes the original metadata source `0x225885D8`, and returns an opaque pointer. It includes literal buffer length `0x104` and input `0x1C`. Their exact imported function identities and metadata encoding are **not established here**. Consequently the C++ model provides a narrowly scoped opaque callback; it does not fabricate Windows API semantics.

### Native caller edges verified

- Phase201 scene growth's allocate CALL at `0x221A3A79` and model-sounds growth's at `0x2213AB29` both target `0x2226E390`.
- Phase201 scene growth's reallocate CALL at `0x221A3A61` and model-sounds growth's at `0x2213AB11` both target `0x2226E400`.
- The original allocation wrapper at `0x2226E3A3` and original reallocation wrapper at `0x2226E413` call `0x2226E290`.

All six are exact original `CALL rel32` instructions, not inferred from a symbolic address list. The four original functions have plain `ret` epilogues; original caller stack cleanup is governed by each allocator interface function's calling convention. Precise original code boundaries, including method-alignment INT3 bytes, are pinned.

## Independently authored source and tests

- `include/sin_compat/original_server_allocator_dispatch.hpp`: C++17 four-field fixed-width 16-byte cache layout; explicit original native global, entry and metadata addresses; basic versus contextual virtual-call choices, separate lazy context bits and deallocation bypass. A fake 32-bit token is never cast into a live host pointer.
- `tests/test_original_server_allocator_dispatch.cpp`: live allocation-token bookkeeping with both enabled and disabled optional-context paths; cached zero/no-retry; independence of the two flags even after feature availability changes; preservation of unrelated flag bits; 32-bit pointer identity; direct free without context; and **Phase194 slot-claim → Phase201 growth → Phase202 allocator dispatch** end-to-end integration for both tracked and untracked scenarios. Optimized local GCC and Clang strict C++17 `-Wall -Wextra -Wpedantic -Werror -UNDEBUG` tests passed against equivalent previous-phase headers.
- `research/tools/audit_sin_original_server_allocator_dispatch.py`: owner-only full original DLL/entire native-function SHA256 gate, **38 sparse opcode witnesses, 19 exact original absolute address operands and six direct CALL edges**, original section placement and RET/INT3 boundaries.
- `research/tests/test_sin_original_server_allocator_dispatch.py`: eight synthetic-only Python mutation groups reject altered instruction witnesses, address operands, CALL destinations, function boundaries, wrong image base/sections and attempts to treat fake fixture bodies as original.
- Root Linux CTest and native MSVC x86 CTest targets registered. GitHub hosted CI must be examined at the exact new branch SHA, separate from local pass claims.

## Scope, next milestone

Original SHA-reviewed function atlas rises **199 → 203 (111 server / 92 client)**, a research inventory, **not a percentage of playable game code**. The allocator object's actual engine implementation and ownership across game/engine boundaries remain *opaque*. The host model uses fake storage and instrumentation toggles, not actual retail Source engine memory or ABI. A real original Windows executable-hosted game session using the reconstructed modules is still not demonstrated.

Next priority: investigate allocator singleton creation and original `CreateInterface` dependency, then test a **reversible owner-only retail Windows engine load trace** with the existing proxy. The unmerged draft stack must stay intact.
