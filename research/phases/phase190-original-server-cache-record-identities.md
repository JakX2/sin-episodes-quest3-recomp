# Phase 190: original server scene/model-sounds cache record identities

12 November 2026. Draft stacked on Phase189 PR #242. Windows PC first, Android second, Quest 3 last. **Owner-original 2006 PE32 binary inspection**, with exact full-file SHA256 verification, not Source SDK guesswork. No DLLs, assets or complete native disassemblies are committed.

## Verified original machine-code entrypoints

| Original function | VA | Length | Complete original code SHA256 |
| --- | --- | ---: | --- |
| Scene record identity/index helper | `0x221ACFD0` | 190 | `48004fea0993479cec22accfbf75523c49b3e7f0cebb5a05ef228e178d4c0804` |
| Model-sounds record identity/index helper | `0x22149770` | 187 | `c79d38ec382e6477cf1872553addccc307629672bcafaf5a5f7e2c22e97b8e49` |

These are **two previously uncatalogued complete original x86 functions, 377 bytes**. Original `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; original `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. Entrypoints and boundaries are observed up to the native `ret 4` and following alignment `int3`.

Phase188 loader calls these per-repository helpers from `0x221AD55E` and `0x22149A3E`; exact incoming x86 relative CALLs are verified. Two different callbacks are required even though their control flow is similar.

## Native dataflow evidence

1. Both take `ECX=this` and one stack parameter (file handle); both return with native `ret 4` and return an unsigned 16-bit index in `AX`.
2. Both dereference original global filesystem interface pointer `0x225721A4` and dispatch through original object vtable byte offset **`+0xD4`** with the incoming file handle argument. The native call is observed; the virtual method's public name, exact semantics and entire interface layout are **not** established here.
3. Both initialize local key descriptor sentinels `0xFFFFFFFE` and `0xFFFFFFFF`, then call repository-specific lookup helpers. A returned `AX == 0xFFFF` means the insertion path is taken; otherwise the original returns the existing index without allocating.
4. Scene insertion requests **0x1C (28) bytes** from the original allocator `0x2226E790`; model-sounds insertion requests **0x18 (24) bytes**. A successfully allocated native object receives different initial vtables (`0x2242DCA0` versus `0x2240ABC8`) and zero-initialized fields. The null allocation branch forwards null to subsequent opaque helpers rather than dereferencing it directly.
5. Both call separate payload-append and key-insertion helpers. The final index is scaled by **24 bytes** (`index*3*8`) and the insertion result pointer is stored into an index slot at **`+0x14`**. No concrete record-key format, ownership contract or record serializer is claimed.

### Exact additional native CALL edges

| Entry | Lookup | Allocator | Append payload | Insert key |
| --- | --- | --- | --- | --- |
| Scene | `0x221A7FB0` | `0x2226E790` | `0x221A6620` | `0x221ACD00` |
| Model sounds | `0x22142670` | `0x2226E790` | `0x22140810` | `0x22146EF0` |

Auditor validates the **8 outgoing** original rel32 CALLs, the **2 incoming** Phase188 call sites, the two full-body hashes, **34 sparse instruction witnesses**, native x86 `ret 4`, original vtable pointers in `.rdata`, native allocation immediates and record-slot publication instructions.

## Code and validation

- `research/tools/audit_sin_original_server_cache_record_identity.py`: original-only strict file hashes, full function hashes and sparse native witnesses. Run with `--server path/to/server.dll --client path/to/client.dll` on privately owned game files.
- `research/tests/test_sin_original_server_cache_record_identity.py`: seven synthetic mutation groups, including independent opcode and CALL corruption and refusing to treat synthetic bytes as original functions.
- `include/sin_compat/original_server_cache_record_identity.hpp`: portable C++17 callback-driven branch reference. Allocation, filesystem, lookup, placement and data ownership remain abstracted to the host.
- `tests/test_original_server_cache_record_identity.cpp`: matched index fast path, both distinct allocation sizes/vtable metadata, new-record insertion order, null-allocation forwarding and 24-byte index slot publication. Root CTest and real Windows 32-bit MSVC CTest registrations added.

Locally tested Python native audit with exact Drive originals (verified both SHA256s), seven synthetic Python test groups, and optimized GCC and Clang C++17 branch tests with `-Wall -Wextra -Wpedantic -Werror -UNDEBUG`. Hosted Windows CI and actual engine-hosted gameplay are **not verified**.

## Status / next step

Unique reviewed function atlas **175 → 177** (**server 83 → 85; client remains 92**). This count measures function research coverage, **not** decompilation completeness. Next, recover or isolate filesystem interface `+0xD4` semantics and the per-repository lookup/insert helper ABI, then run a carefully controlled original Windows game executable load test. The game is not yet playable through reconstructed modules.
