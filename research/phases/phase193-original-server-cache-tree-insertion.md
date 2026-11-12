# Phase 193: original server cache-tree insertion pair and 16-byte key publication

12 November 2026. Owner-original *SiN Episodes: Emergence* (2006) Windows PE32 files, no proprietary binaries or complete original instruction bodies committed. Stacked draft on Phase192 PR #245. Windows PC gameplay first, Android second, Quest 3 third.

## Exact original native evidence

The original `server.dll` (6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`) and `client.dll` (3,592,192 bytes, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`) both match the private owner-supplied Drive archive.

| Original function | VA | Native bytes | SHA256 of complete original native function |
| --- | --- | ---: | --- |
| Scene indexed-tree insert wrapper | `0x221ACD00` | 112 | `b878d9830b6e5cee73e17681d2c89c518f45c8f8f1f5811aab516c42f14e0640` |
| Model-sounds indexed-tree insert wrapper | `0x22146EF0` | 112 | `2bc1a6156b3b845d14935402d317b8ec6f285f1d9858a67fd4054a780db821af` |

**Two newly reviewed original server functions, 224 complete x86 bytes.** Both distinct complete native 112-byte implementations are byte-identical after normalizing only the three local direct CALL-relative displacement DWORDs at offsets `0x1B`, `0x22` and `0x36` in each body. Full normalized SHA256: `dd1f7514e89298739816f025baf317f3b87424ee8a637cda3ed3289acecf2725`. They are separate native entrypoints with different direct helper destinations, not a duplicated method count.

### Original outgoing call targets

| Call role (opaque implementation) | Scene | Model sounds |
| --- | --- | --- |
| Position/find/preparation helper, first CALL at `+0x1B` | `0x221A6510` | `0x22140700` |
| Allocate/claim new slot, second CALL at `+0x22` | `0x221A6890` | `0x22140900` |
| Link inserted slot into tree, third CALL at `+0x36` | `0x221AA890` | `0x221441B0` |

Earlier Phase190 record-identity helpers call the wrappers at native incoming addresses `0x221AD06F -> 0x221ACD00` (scene) and `0x2214980C -> 0x22146EF0` (model sounds). The new auditor verifies all **six outgoing plus two incoming exact relative CALL instructions**.

### Verified native control flow and memory offsets

1. Each receives `ECX` pointing to its repository tree object and **two stack arguments**, one of which points to a 16-byte key descriptor. Both return with `ret 8`.
2. Each invokes the three opaque native helper functions in the exact order above, passing the full 32-bit raw slot index from the second helper to the third helper.
3. Both increment a native **WORD** field at tree object `+0x12` (the native 16-bit record count) after calling the link helper. This count can wrap in 16-bit arithmetic; the native wrapper contains no count-limit guard.
4. Both use the lower 16 bits of the returned slot index (`DI`) when accessing an indexed slot in the array at original object `+4`. Slot stride is **24 bytes**, as established by Phase191.
5. The original x86 code explicitly copies **four 32-bit key words**, totaling 16 bytes, from the incoming descriptor into slot offsets `+0x08`, `+0x0C`, `+0x10`, and `+0x14`. The first DWORD drives Phase192's unsigned comparator; the rest are not compared in those two native comparator functions.
6. Immediately after the wrapper returns to Phase190, that caller stores the appended record payload pointer into slot offset `+0x14`, overwriting the key descriptor's fourth DWORD. The first three DWORDs remain in the slot on that path. This explains an otherwise surprising overlap in the native slot structure.
7. The native code computes a pointer to the target slot, then tests that computed pointer for zero before the 16-byte copy. That is **not** a general bounds or memory-validity check. The C++17 model uses `std::vector` and throws on a missing slot deliberately as host-only safety behavior.

## Source, tests, audit and limits

- `research/tools/audit_sin_original_server_cache_tree_insert.py`: strict original dual-DLL whole-file SHA256 gate; both entire native function SHA256s; **52 sparse original x86 witnesses**, exact eight CALL edges, original `ret 8` epilogues, count increment and four-word node copy, next aligned function entrypoint prefixes; normalized whole-body equality and hash. Original owner-only audit passes locally.
- `research/tests/test_sin_original_server_cache_tree_insert.py`: eight original-free Python unit-test groups, including separate corruption of all 52 instruction witnesses and all eight edges, boundary checks, whole normalized-twin mismatch detection and deliberate synthetic-whole-hash rejection; pass locally.
- `include/sin_compat/original_server_cache_tree_insert.hpp`: portable C++17 three-host-callback insertion model, exact original addresses, 16-byte key copy to the existing Phase191 24-byte node layout, 16-bit counter increment and exact low16 return-index semantics. The actual native tree placement/slot allocation/linker helpers remain injected opaque callbacks.
- `tests/test_original_server_cache_tree_insert.cpp`: tests native-specific callback ordering, key field copy, Phase190 payload write into slot `+0x14`, Phase191/192 lookup/comparator integration including unsigned high-bit key, 16-bit index truncation and count wrap, and host-only out-of-range guard. Optimized GCC and Clang with `-Wall -Wextra -Wpedantic -Werror -UNDEBUG` pass locally.
- Root CTest and Windows 32-bit MSVC CTest targets added. **Hosted CI must be checked independently before claiming success.**

The complete original comparator for this tree is now known (Phase192) and the wrapper's last slot write is understood. However **the three native insertion helpers remain opaque**, and the original `IFileSystem` vtable method at byte `+0xD4` still has not been proven to mean any specific Source SDK method; the binary only proves a one-file-handle virtual dispatch producing the first key DWORD. The original cache serialized record format, full engine integration, playable original Windows replacement modules, Android and Quest 3 game runtime are not yet implemented.

Unique reviewed original methods **181 -> 183 (91 server, 92 client)**. This is a count of specifically reviewed native functions, not a percentage of total game code.
