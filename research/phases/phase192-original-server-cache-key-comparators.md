# Phase 192: original server cache key comparator implementations

12 November 2026. Draft stacked on Phase191 PR #244. Independently examined exact owner-provided *SiN Episodes: Emergence* (2006) Windows PE32 `server.dll` and `client.dll`, both complete SHA256 verified. No original binaries, source game assets or full instruction bodies are committed. Prioritize Windows PC gameplay before Android and Quest 3.

## Two new original native functions

| Purpose | Native code address | Length | Exact original complete function SHA256 |
| --- | --- | ---: | --- |
| Scene cache tree comparator | `0x221A2F40` | 17 x86 bytes | `77b87ba9017ac5403ea41c488944263dd6475f82ddf3b8f28b36155b9e8f15f4` |
| Model-sounds cache tree comparator | `0x2213A3F0` | 17 x86 bytes | `77b87ba9017ac5403ea41c488944263dd6475f82ddf3b8f28b36155b9e8f15f4` |

These are distinct pointer-addressed entrypoints with **exactly byte-identical complete native code** (not merely normalized). Each 17-byte body ends with `ret` and is followed by 15 `int3` alignment bytes before the next distinct entry. Total new native functions: **2**, original complete native bytes verified: **34**.

The PE-initialized original singleton objects directly pin these callbacks: scene tree comparator pointer at `server.dll` VA **`0x22503EE4`** contains **`0x221A2F40`**, and model-sounds comparator pointer at **`0x224F038C`** contains **`0x2213A3F0`**. These are each the cached repository singleton's `+4` tree subobject field. They are **real original data pointers**, not reverse-engineer-guessed associations.

## Exact semantics

Original code reads two pointers from the incoming 32-bit stack arguments, loads the first DWORD pointed to by each, performs an x86 `CMP lhs,rhs`, then `SBB EAX,EAX; NEG EAX; RET`. Therefore:

```cpp
return static_cast<uint32_t>(lhs.first_word_00 < rhs.first_word_00);
```

The comparison is **unsigned DWORD ordering**, not signed arithmetic, C-string ordering, structure `memcmp`, or comparison of later key fields. Return value in `EAX` is exactly **1** when the left first DWORD is smaller than the right first DWORD and **0** otherwise. The caller's pairwise comparator calls and `add esp,8` cleanup were separately verified in Phase191, so this comparator itself ends in ordinary `ret` (no `ret 8`).

This closes the previous Phase191 unknown comparator callback for these two specific trees and means the 24-byte indexed tree uses first-DWORD unsigned ordering even when bit31 is set. It does **not** yet establish how the filesystem method at vtable offset `+0xD4` constructs that first key DWORD, or how the remaining original key descriptor fields are serialized, inserted or owned.

## New source, tests and evidence

- `include/sin_compat/original_server_cache_key_comparators.hpp`: C++17 fixed-width 16-byte key *prefix model* with two native original code addresses and exact singleton pointer slots; unsigned first-DWORD comparator, without claiming the remaining three words' meaning.
- `tests/test_original_server_cache_key_comparators.cpp`: tests unsigned/signed divergence at `0x80000000`, ignored extra fields, equal keys, `0` and `UINT32_MAX`, and integration with the existing Phase191 tree for all three sorted keys across each separate sentinel state. Local optimized GCC and Clang `-Wall -Wextra -Wpedantic -Werror -UNDEBUG` passed against the corresponding Phase191 header.
- `research/tools/audit_sin_original_server_cache_key_comparators.py`: strict original SHA256 checks for **both complete DLLs** and **both 17-byte native function regions**, 10 disjoint sparse opcode witnesses, byte-for-byte comparator identity, 15-byte INT3 post-body boundaries and two exact PE-initialized comparator pointers with section checks. Original Drive file audit passes locally.
- `research/tests/test_sin_original_server_cache_key_comparators.py`: **8 original-free mutation groups** checking every sparse witness, pointer slot, boundary, failure of exact identity and synthetic-body/whole-DLL hash rejection. Python unittest passes locally.
- Added CMake CTest registration for Linux and native Win32 MSVC x86; hosted CI pending at commit time. No proprietary game binaries are committed.

## Status and limits

Reviewed atlas **179 → 181 (server 87 → 89; client remains 92)**. This is **research coverage**, not a whole-game decompilation fraction. These two tiny native methods are exact semantic reconstructions, but the full Source engine filesystem interface, record decoder, Windows executable-hosted replacement game modules, complete campaign and Android/Quest runtime remain unimplemented. Next should resolve the record key first-DWORD origin at original filesystem virtual `+0xD4` and the precise use of that key in insertion.
