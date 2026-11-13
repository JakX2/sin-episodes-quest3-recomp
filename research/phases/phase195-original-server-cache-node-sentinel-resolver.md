# Phase 195: original server scene/model-sounds cache node/sentinel resolvers

13 November 2026. Stacked DRAFT on Phase194 PR #247. Original *SiN Episodes: Emergence* (2006) i386 DLLs inspected privately. No commercial DLLs, game assets, complete native instruction bodies or raw disassembly are committed. PC gameplay remains first, then Android, then Quest 3.

## Exact native function provenance

Original full-file SHA256 digests match the project's pinned references:

- server.dll: 6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- client.dll: 3,592,192 bytes, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

| Original native helper | Entry VA | Length | Complete original native function SHA256 |
| --- | --- | ---: | --- |
| Scene cache node/sentinel resolver | `0x221A4D90` | 84 bytes | `84be32439e28d397699f9afaadf46ac70d857f88bd26ab6d19bd8eb2d7b7c855` |
| Model-sounds cache node/sentinel resolver | `0x2213E990` | 84 bytes | `fd045383426de31ea6e97afd788d3959bcf057a2290be6cff4b870e5a2a82a64` |

**Two additional distinct SHA-verified complete original server.dll functions, 168 bytes total.** Both full 84-byte bodies become byte-identical after normalizing only seven original absolute DWORD address operands per body, at local offsets `7,20,27,34,41,47,77`. Complete normalized SHA256: `2eab8413dc78ddc215c2c60be186262755adfa638f84e5de6c73be2725a69a1c`. Both functions end in `ret 4` followed by 12 `int3` alignment bytes.

### Native exact call edges and repository-specific globals

- Phase194 scene allocator calls this resolver from `0x221A68C2 -> 0x221A4D90`.
- Phase194 model-sounds allocator calls this resolver from `0x22140932 -> 0x2213E990`.
- The scene helper's 8-byte static sentinel starts at `0x2256E1F0`, with its DWORD bitfield at `0x2256E1F8`.
- The model-sounds helper's independent sentinel starts at `0x22558D50`, with its DWORD bitfield at `0x22558D58`.
- Each set of seven absolute address operands belongs to the original virtual `.data` section. These globals are not guaranteed to be present as file-backed raw bytes; the audit checks mapped sections, not file-backed sentinel payload.

## Branch-level recovered behavior

The original 32-bit thiscall helper receives the repository tree via `ECX` and a single 16-bit slot index on the stack. Before examining that index, it tests bit 0 of its own static flag. When the bit is clear, it ORs the bit into the flag and writes the sentinel's four consecutive WORDs as **`0xFFFF, 0xFFFF, 0xFFFF, 1`**. Subsequent calls with bit 0 set skip this initialization.

If the incoming index equals `0xFFFF`, the helper returns the native address of the **8-byte static sentinel**, not a pointer into the ordinary node array. Otherwise it reads the original tree object's pointer at `+4` and returns `slots_base + (uint16(index) * 24)`. It returns using `ret 4`, meaning one stack argument is cleaned by the callee.

Phase194 then reads WORD `+2` from the returned pointer to advance the free-list head. The sentinel case would yield `0xFFFF`. The original allocator's normal free-list branch passes a non-`0xFFFF` index, but this helper supports both values. Critically **the lazy static sentinel setup is executed for ordinary node lookups as well as missing-index lookups**, so deferring it until the sentinel branch would be a reconstruction error.

The new C++17 host model returns a typed selection with the resolved WORD rather than inventing x86 pointers. It retains separate scene and model-sounds sentinel state, preserves unrelated flag bits, and uses a deliberate host-only index bounds guard. The original code has no such guard. The 8-byte sentinel must not be treated as a complete 24-byte node.

## Source, tests, and audit

- `include/sin_compat/original_server_cache_node_resolver.hpp`: fixed-width C++17 original metadata and selected-value model, lazy static sentinel setup and checked node lookup.
- `tests/test_original_server_cache_node_resolver.cpp`: both repository configurations; normal and `0xFFFF` resolution; initialized-once flag behavior; preservation of unrelated bits; invalid-index guard; and actual composition with Phase194's synthetic free-list `claim`.
- `research/tools/audit_sin_original_server_cache_node_resolver.py`: exact original full-file and full-function SHA256s, **32 sparse instruction witnesses**, **14 exact native absolute address operands**, two exact original incoming `CALL rel32` edges, both `ret 4`/12-byte INT3 boundaries, and whole normalized twin equality and SHA256.
- `research/tests/test_sin_original_server_cache_node_resolver.py`: eight synthetic mutation-test groups. Unlike the original binary auditor, fabricated test fixtures deliberately omit part of each native instruction stream and cannot satisfy full original-function hashes.
- Added root CTest and Windows x86 MSVC CTest targets.

**Executed locally:** owner-original DLL audit passes against both original complete file SHA256s and all 168 bytes of native function hashes; eight Python mutation-test groups pass; optimized C++17 GCC and Clang integration tests pass with `-Wall -Wextra -Wpedantic -Werror -DNDEBUG` using matching previous-phase header structure. Hosted reference CI and Windows x86 test outcomes are to be checked against this exact branch SHA.

## Limits and next work

Unique researched function atlas **185 -> 187 (95 server, 92 client)**, not whole-game decompilation completeness. Native growth and tree-link functions, `IFileSystem` method at virtual offset `+0xD4`, complete serialized cache format and actual original Windows game-executable-hosted replacement module remain unresolved.

Next higher-value action: build an owner-only, reversible Windows retail engine startup trace using the existing factory-forwarding proxies, or source-ground more of the original tree-link/vector-growth ABI. A synthetic Win32 loader test does not establish a running campaign.
