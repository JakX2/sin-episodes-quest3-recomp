# Phase 90: original cache-record insertion and client late shutdown dispatch

## Exact original binary provenance

The owner's original **SiN Episodes: Emergence (2006) 32-bit Windows** binaries were inspected directly. Previous Phase 89's exact SHA-256 and all earlier original analysis checks are chained in the read-only auditor:
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

This phase adds **four fully hashed original function bodies**, exact opcode anchors and direct-call edges:

| Module | Function | Address | Size | SHA-256 |
|---|---|---|---:|---|
| server | Scene record find/insert | `0x221ACFD0` | 190 | `48004fea0993479cec22accfbf75523c49b3e7f0cebb5a05ef228e178d4c0804` |
| server | Model sounds record find/insert | `0x22149770` | 187 | `c79d38ec382e6477cf1872553addccc307629672bcafaf5a5f7e2c22e97b8e49` |
| client | Late subsystem shutdown dispatcher | `0x241EE0C0` | 120 | `f291a3fc3a0d72d08184ce42c4e426e247c6d23b90c47af728d8eb3296e61783` |
| client | Another optional global object release | `0x240FC900` | 27 | `71721cdd2db6d9210f6b335faca52209973d792a0aa0b802c0c7306241303e88` |

The previous Phase 89 phase recovered *only the four 32-bit header fields* of `scene.cache` and `modelsounds.cache`. The two further original functions now demonstrate *how entries are found/inserted in the corresponding in-memory tables*, not a full disk-file record schema.

## server.dll: 16-bit record indices and distinct allocations

Both original functions accept a record descriptor and return an **unsigned 16-bit index**, zero-extended by the original reader. The search helpers are distinct:
- Scenes: `0x221ACFD0` calls `0x221A7FB0`, tests `AX == 0xFFFF` at `0x221AD01C`. If missing, allocates a **28-byte** per-record helper object via `0x2226E790` (push `0x1C` at `0x221AD023`), then uses helper calls `0x221A6620`, `0x221ACD00` and stores an auxiliary pointer into **record +0x14**.
- Model sounds: `0x22149770` calls `0x22142670`, tests the same **0xFFFF** sentinel at `0x221497BC`. On miss, allocates a **24-byte** helper object (push `0x18` at `0x221497C3`), then uses `0x22140810` and `0x22146EF0` before storing the auxiliary pointer at **record +0x14**.

Original helpers maintain other fields and associated objects. The allocations above **are not** the record's own size; do not conflate the 28/24-byte helper objects with the table entry.

The original readers independently repeat a matching per-entry sequence:

| Scene reader | Model sounds reader | Observed behavior |
|---|---|---|
| `0x221AD566` | `0x22149A46` | Zero-extend record index `AX` |
| `0x221AD569/56C` | `0x22149A49/4C` | Compute **`base+index*24+8`** table entry address |
| `0x221AD57C` | `0x22149A5C` | Store parsed/derived pointer-like value to **entry+0x04** |
| `0x221AD579` | `0x22149A59` | Read handler index from **entry+0x0C** |
| `0x221AD57F/582` | `0x22149A5F/562` | Get handler pointer from array at **owner+0x20** |
| `0x221AD58C` | `0x22149A6C` | Invoke selected handler's **virtual byte offset +0x04**, passing temporary record data |

The record address description is relative to **the pointer read from owner+0x08** and `+8` extra per original code. Slot fields are thus at effective offsets relative to that computed entry, not an independently verified C++ object type. The C++ reference retains 24-byte word layout and neutral handler indices but does **not** use x86 addresses as host pointers, own original handlers, or decode original payloads.

The disk-record parsing loop includes other original calls `0x2228D7D0`, `0x2228D6A0`, `0x2228D810` with a `0x200` size parameter before the record is inserted. **Exact on-disk per-record structure, text encoding, variable-length components and callback semantics remain unknown.** Neither a 24-byte disk entry nor a working cache writer is claimed. As in Phase89, corrupted cache content can result in recovery and regeneration rather than a fatal server startup failure.

## client.dll: eight virtual shutdown callbacks

Original `VClient011` shutdown (Phase88 at `0x240FF3D0`) calls a subordinate **`0x241EE0C0`** through site `0x240FF489`.

The subordinate first calls `0x2422ECE0`, then uses these exact global pointers and original x86 virtual slot byte offsets **in this order**:

| Order | Global pointer address | Virtual byte offset |
|---:|---|---:|
| 1 | `0x2431DC90` | `+0x04` |
| 2 | `0x2431DEA4` | `+0x04` |
| 3 | `0x2431DD00` | `+0x04` |
| 4 | `0x2431DE3C` | `+0x04` |
| 5 | `0x2431DDD0` | `+0x04` |
| 6 | `0x2431DBEC` | `+0x1C` |
| 7 | `0x2431DF40` | `+0x04` |
| 8 | `0x2431EEBC` | `+0x04` |

Then it checks optional global `0x2437F8A0`; if present, invokes its virtual **+0x0C**. It finally invokes `0x24001060` to obtain another interface pointer and tail-jumps through its virtual **+0x14**. The eight subobjects' semantic Source SDK names are **not yet proven**. The original addresses are 32-bit PE virtual addresses, not ARM64 pointers.

A separate **`0x240FC900`** helper, directly invoked earlier from main client shutdown at `0x240FF3D4`, checks global `0x2433F3F4`: if nonnull, invokes object's virtual **+0x00** with integer argument **1**, then always clears the global. This is a distinct global from the optional pointer at `0x24378658` released by helper `0x241E5050` in Phase89.

## Implementation and constraints

- `include/sin_compat/original_cache_record_dispatch.hpp`: C++17 host-only record structure constants, 16-bit sentinel branch, callback handoff, exact eight-call late shutdown ordering and optional global release, no original asset or runtime linkage.
- `research/tests/sin_original_cache_record_dispatch_reference.cpp`: validates existing/new index branching, offset-based dispatch to mock handler, all eight client virtual-call slots, optional and absent callback, and idempotent global release.
- `research/tools/audit_sin_cache_records_client_shutdown.py`: exact SHA256-provenance auditor covering four new function bodies plus instruction anchors and direct call edges, chained to all earlier client/server original checks, outputs metadata JSON only.
- `research/tests/test_sin_cache_records_client_shutdown.py`: ten synthetic/original-free mutation groups for each x86 anchor, original CALL edge and full code-region hash.
- `CMakeLists.txt`: C++17 `-Werror` CTest regression target.

**Limitations:** No claim of original x87/ARM64 byte parity, complete `client.dll` or `server.dll`, game entities, original on-disk cache payload serialization, SourceVR game ABI, or playable Quest 3 build. No original DLLs or copyrighted engine files are present in the public repository.

## Phase 91 candidate

Trace the **cache record handler vtables** selected through owner+0x20, and the original client late shutdown's eight object types. This can establish meaningful gameplay/resource semantics before attempting a native Android `libserver.so`/`libclient.so` adapter. Any proposed ARM64 game plugin must be tested against real SourceVR `CreateInterface` contracts, not merely old version strings.
