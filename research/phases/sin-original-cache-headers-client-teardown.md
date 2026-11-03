# Phase 89: Original SiN cache header checks and remaining client shutdown helpers

## Primary provenance

Read-only inspection of the user's original Windows x86 SiN Episodes: Emergence (2006):
- `client.dll` SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The Phase89 verifier **chains all prior Phase88 original dual-DLL verification**, including prior client/server `CreateInterface`, Init, AutoGameSystem and cache close checks. This phase adds **four whole-code-region SHA256 fingerprints**, **51 exact opcode anchors**, **six direct CALL edges**, and verifies both cache singleton filename pointers/version fields. Nothing from the original proprietary binary is distributed.

## Server: exact four-word cache header read

The original two 576-byte cache initialization bodies have the same optimized template logic, with distinct data and handlers:

| Cache | Original server reader | Singleton | Expected format |
|---|---|---|---:|
| `scene.cache` | `0x221AD3E0` | `0x22503EE0` | **3** |
| `modelsounds.cache` | `0x221498C0` | `0x224F0388` | **2** |

Both use filesystem global `0x225721A4` to open a file and then invoke its virtual slot `+0` **four times with size 4** before entering the per-entry loop. The first four on-disk values are therefore **four 32-bit words, in little-endian order**. The first three have independently observed comparisons:

| Byte offset | Observed meaning | Original proof |
|---:|---|---|
| `+0x00` | Fixed tag **2** | CMP `0x221AD45E` / `0x2214993E` |
| `+0x04` | Per-cache format value **3 / 2** | CMP against instance `+0x38` at `0x221AD488` / `0x22149968` |
| `+0x08` | **Optional source key** | Reads instance `+0x44`, computed by callback pointer at instance `+0x40` when nonnull; comparison `0x221AD4AA` / `0x2214998A` |
| `+0x0C` | **Signed record count** | read and tested with signed branch `JLE` at `0x221AD4F1` / `0x221499D1` |

If the callback-derived expected source key is **zero**, the code bypasses comparison of the third header word (`0x221AD4B0` / `0x22149990`). The word's actual algorithm, e.g., whether it is a timestamp, CRC, file-content key or game-state revision, is **not yet verified**. Therefore the reference deliberately calls it `source_key`, **not** `crc32`.

A missing or incompatible header takes a **recovery path**: the file is closed through filesystem virtual `+0x0C`, cache singleton byte `+0x3C` is set, and the cache loader frequently **returns success** (`AL=1`). Only the missing or empty filename path has the separately observed `AL=0` return. Thus **bad cache != instant game startup failure**.

After a header succeeds, the fourth signed count gates a loop of additional reads and per-record handlers. The target in-memory indexing uses **24-byte entries** (`index * 3 * 8`), but this **does not establish a 24-byte on-disk record format**. The original disk record payload includes additional variable-length/indirect data paths that need further study. **There is no complete cache decoder or writer in this phase**, and no original cache files were present among the available local game inputs.

## Client: two more original shutdown helpers

Previous Phase88 verified the `VClient011` slot-one original shutdown `0x240FF3D0`. This phase follows two further original direct calls from that method:

### `0x241E5050`: optional global object release

At caller `0x240FF41D`, the original helper:
1. Loads optional global object pointer **`0x24378658`**.
2. If non-null, calls that object's **virtual slot +0 with argument 1**.
3. Always writes **zero** back to `0x24378658`.

Entire 27-byte function SHA256: `b46aa3159ec436d2a49e839506758499c47fd13919dc8de46790e75fe01c58b6`.

### `0x24143E80`: reverse array object release and ownership handling

At caller `0x240FF484`, the original passes ECX **`0x243623D0`** into the 97-byte cleanup helper. Its original code:

1. Invokes opaque helper `0x241F3530` (unverified side effects).
2. Reads active count `this+0x24`, iterates pointer array `this+0x18` from last entry to first, skips null entries, calls each object's **virtual +0 with argument 1**.
3. Resets active count **`this+0x24` to zero**.
4. If `this+0x20 != -1` and dynamic buffer `this+0x18` is non-null, frees it using `0x2416F940`, then clears buffer and `this+0x1C`; if marker is `-1`, it does **not free** that buffer.
5. Copies current buffer into `this+0x28`, clears byte `this+0x2C`, returns.

The meaning of the sentinel `-1` is not yet proven. The C++17 host adapter treats it as a **special ownership mode** for this release routine, not as a fully known Source class type. It does not emulate the opaque preliminary helper or real heap ABI.

Entire 97-byte function SHA256: `121ec6b42c0d35770e68281a5eb0d24b891266a747b69a4ccf497d005b94eeff`.

## Deliverables and verification

- `include/sin_compat/original_cache_header_client_cleanup.hpp`: safe, no-I/O, little-endian 16-byte header parser and original decision gates, plus two host-owned client cleanup adapters. Rejects truncated memory buffers defensively; original x86 did not explicitly validate every short read.
- `research/tests/sin_original_cache_header_client_cleanup_reference.cpp`: C++17 tests for versions, fixed tag, conditional source key, signed counts, truncated input, client reverse-release, optional pointer, ownership sentinel.
- `research/tools/audit_sin_cache_headers_client_teardown.py`: original SHA-256 chained server/client verifier, four function SHA fingerprints and exact evidence anchors, JSON metadata only.
- `research/tests/test_sin_cache_headers_client_teardown.py`: **10 grouped synthetic mutation tests** covering all 51 opcode anchors, 6 CALL edges, both cache strings/field markers, original hash enforcement.
- `CMakeLists.txt`: adds independent CTest target.

Locally the owner-held DLL auditor passed: 2+2 function SHA regions, 51 opcode anchors and 6 CALLs. C++17 built with `-Wall -Wextra -Wpedantic -Werror` and the reference executable passed; all **10 Python groups** passed. GitHub CI is checked independently after publishing.

## Limits and Phase 90

These findings cover **real original game server cache header validation and real client shutdown behavior**, not the full `scene.cache` or `modelsounds.cache` payload format. No original file payload was decoded; no native ARM64 game-specific `libclient.so`/`libserver.so`, game entity runtime, SourceVR 0.5.5 ABI adapter, live Quest 3 map, or gameplay execution is claimed.

**Next:** identify individual record structure and source-key callback in both cache readers, and trace the remaining original client shutdown helpers. Build game-specific native interfaces only after the original/Android Source interface contracts are genuinely matched.
