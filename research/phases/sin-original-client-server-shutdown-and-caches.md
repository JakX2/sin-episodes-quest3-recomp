# Phase 88: original SiN client/server shutdown and cache lifecycle

## Original binary evidence

Read-only disassembly of user's **exact SHA-256-verified original Windows PE32 SiN Episodes (2006)** files:
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The new verifier chains Phase 87's entire original Init/subsystem audit and additionally pins **seven whole code-region SHA-256 hashes, 64 x86 opcode anchors, 31 original direct CALL targets, six relative JMP targets and four vtable entries**. It also verifies two original cache singleton filename pointers/format-marker fields, their NUL-terminated names, and a literal console command.

No original DLLs or proprietary binary bytes are committed.

## Original client.dll shutdown: VClient011 vtable slot 1

The previously verified original `VClient011` vtable at `0x24296C6C` has **slot 1 = `0x240FF3D0`** (204-byte function SHA-256 `4cab24e5b8920392fdcf69020e709da740e06f2963f01ca2b374a6c3755f92d1`).

The exact original x86 sequence:

1. Calls helper `0x240FC900`, then three opaque helpers `0x241DAB30`, `0x2418D1C0`, `0x241ACF50`, forwarding each result through virtual method **+0x28** of the engine-held object referenced by `0x242FF470`.
2. Calls `0x241E5050`, then **`0x241640F0`**, the same global client setup/rollback cleanup recovered in Phase 87. Thus ordinary client shutdown reuses the registered teardown callbacks.
3. Calls other proven indirect interfaces: virtual +0x18 and +0x10 of object at `0x2437F8A0`, virtual +0x04 via `0x24320BF8`, direct `0x24053AB0` and `0x24085220`, and virtual +0x08 via `0x24377060`.
4. Reads **active count `this+0x10`**, indexes pointer array **`this+0x04`** from last element down to first, invoking each registered system's **virtual +0x34**. After finishing, sets active count to **zero**. This is the exact reverse shutdown loop, not a guess.
5. Invokes `0x241566C0`, `0x24143E80`, `0x241EE0C0`, `0x240FC100` and resets global `0x2433F6D0` to zero.

The reference C++17 only models verified reverse order with host-owned objects; original virtual interface signatures, class owners, full heaps and engine side effects are not emulated.

## Original server.dll: three distinct observed vtable methods

Original `ServerGameDLL003` vtable base `0x22442EFC` includes:

| Slot | Original target | Observed behavior |
|---:|---|---|
| 1 | `0x221E7B00` | Calls global reset `0x221F4980`, then passes literal **`exec game.cfg\n`** via virtual +0x90 of global interface `0x22572198`, calls its virtual +0x94, returns success byte 1 |
| 7 | `0x221E7B30` | Tail-jumps to `0x221F4980`, a 69-byte global reset helper |
| 8 | `0x221E79B0` | Closes cache objects and performs other resource teardown, see below |

Original hashes for slot 1 (41 bytes) `eef931750a7ee8975e75405e8b2fc4607904eeae15aec74a356035f129363c35`, slot 7 (5 bytes) `bf4cd45b2447cdeca904e392cfceb2525169471f83eba87ad88280d24ab4d71c`, reset helper (69 bytes) `7c13db622884650619706d474b2f5165bb01bbf31bbf70574dad5519d0710c16`, and slot 8 (210 bytes) `f6c9415aaacd6601867d81610fd6ea162a588012dc71e105a6de05660fe01e8c`.

We intentionally use **verified vtable slot numbers**, not unverified semantic names from newer Source SDK revisions.

**Server slot 8's original order**: call `0x22149710` (modelsounds cache cleanup), then `0x221AC910` (scene cache cleanup); prepare five opaque results via distinct helpers and forward each through virtual **+0x28** of an interface at global `0x2251BA80`; execute more helper work including `0x221F5570`; if pointer stored in global **`0x2254B888`** is nonnull, invoke its virtual **+0x00** with argument 1 and set the global to null. If already null, skip that release.

## Confirmed cache names, format marker values and close behavior

These are real initialized server singleton fields, **not just discovered strings**:

| Original singleton | Original filename +0x34 | Original marker +0x38 | Close routine |
|---|---|---:|---|
| `0x22503EE0` | **`scene.cache`** (pointer `0x2242ED68`) | **3** | `0x221AC910` |
| `0x224F0388` | **`modelsounds.cache`** (pointer `0x2240BC9C`) | **2** | `0x22149710` |

Phase 87's proven read routines `0x221AD3E0` and `0x221498C0` compare a parsed value to their singleton `+0x38` value at `0x221AD488` and `0x22149968`. This supports **on-disk format compatibility checks**, but *does not* establish the complete file schema.

Both close functions (89 bytes; SHA-256 `2815f0d5793c8cc5a8e82318a19d8e6083a832f545217d81aa31235818bd0e59` and `16e8e4cbcc11f001a469b61a8c0c6e86cbc8bedebd9836fa60d9a10baceeb2fe`) exhibit equivalent branch structure:

1. If singleton byte **+0x3D** is zero, return without further cleanup.
2. Otherwise read **+0x3C** and **set +0x3D to zero before cleanup callbacks**.
3. If +0x3C is set, invoke one special helper and the shared finish helper.
4. Else if **+0x4E** is set, invoke a check helper; if that returns false, invoke a fallback helper; then invoke the same finish helper.
5. Otherwise invoke shared finish helper only.

The reference's names `dirty`, `check_on_close` and `fallback` are **explanatory names**, not original recovered class-member names. No claim that cache file data was parsed or rewritten at runtime.

## Artifacts and verification

- `include/sin_compat/original_shutdown_contract.hpp`: C++17 host reference for cache close outcomes and verified client/server teardown ordering.
- `research/tests/sin_original_shutdown_reference.cpp`: reverse shutdown, all cache close branches, check success/failure, five server virtual dispatches, optional object release.
- `research/tools/audit_sin_shutdown_caches.py`: chained full-original SHA validator and seven function hashes, opcode/call/jump/vtable/field checks, JSON metadata only.
- `research/tests/test_sin_shutdown_caches.py`: **11** synthetic DLL-free adversarial test suites covering all instruction/call/jump/vtable anchors and both cache singletons.
- `CMakeLists.txt`: CTest registration.

Local on owner's original DLLs: full audit PASS; **11/11 Python suites PASS**; C++17 `-Wall -Wextra -Wpedantic -Werror` and isolated **CTest 1/1 PASS**. GitHub CI is evaluated separately.

**Remaining:** complete `client.dll` / `server.dll` implementations, actual scene/modelsounds cache file formats, full ABI vtables, original engine integration, native `libclient.so`/`libserver.so` modules, running maps and real Quest 3 headset gameplay are **not yet achieved**. Next step is tracing cache field serialization and the owners of remaining shutdown callbacks, then testing a verified ARM64 SourceVR game-module interface.
