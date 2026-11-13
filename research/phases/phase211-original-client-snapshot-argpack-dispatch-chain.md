# Phase 211: original client prediction snapshot argument block and record dispatcher

13 November 2026. **Stacked draft on Phase210 PR #264.** This phase follows the original SiN Episodes: Emergence 2006 `client.dll` SaveData helper beyond Phase210: the snapshot argument constructor (`0x241978A0`), dispatcher (`0x24199BA0`) and linked-record walker (`0x24199B50`). All three are fully SHA-verified original x86 functions with exact native sizes and original method alignment. The owner-original DLLs are strictly private; no commercial binaries, source assets, full original x86 code bodies or bulk disassembly are committed.

## Original binary provenance and three newly recovered methods

The entire owner-provided original `client.dll` (3,592,192 bytes; PE32 image base `0x24000000`) matches SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. The companion original `server.dll` (6,049,792 bytes) matches `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

| Original entrypoint | Complete x86 bytes | Complete original native SHA256 | Next INT3 padding |
| --- | ---: | --- | ---: |
| `0x241978A0` argument-block constructor | 104 | `13bac9785ff2c2f21cd983d5d911f542538a449f4990bd21ab825a96d24f2c82` | 8 |
| `0x24199BA0` snapshot dispatcher | 71 | `9dfd73ea32a62b437e7492038a3b7f01706d6ba62966d162e8b776c897cfeab3` | 9 |
| `0x24199B50` linked-record walker | 65 | `45490bbfdb993b0b40c255aaf9612d1f1e116d10de5b3c55d220f10a524e5005` | 15 |

**Three new distinct SHA-verified client functions, 240 complete original native bytes.** These entrypoints were absent from the Phase210 catalog, which was 235 unique methods. New cumulative inventory is **238 functions (128 server, 110 client)**, a *research tally*, not game-port readiness percentage.

## Verified 104-byte 0x241978A0 argument-block layout

The original entrypoint has ECX pointing to a mutable struct-like block and **10 32-bit stack arguments**, with original `ret 0x28`. Original x86 MOV, CMP/SETNE and byte stores establish that it initializes the following 52-byte memory span:

| Offset in destination block | Source of written value |
| --- | --- |
| `+0x00` DWORD | Original stack argument 1 |
| `+0x04` DWORD | Argument 2 |
| `+0x08` DWORD | Argument 4 |
| `+0x0C` DWORD | Boolean `low_byte(argument 3) != 0` |
| `+0x10` DWORD | Boolean `low_byte(argument 5) != 0` |
| `+0x14` BYTE | Argument 6 low byte |
| `+0x15` BYTE | Argument 7 low byte |
| `+0x16` BYTE | Argument 9 low byte |
| `+0x18`, `+0x1C`, `+0x20`, `+0x28` DWORDs | Zero |
| `+0x24` and `+0x25` BYTEs | Zero |
| `+0x2C` BYTE | Argument 8 low byte |
| `+0x30` DWORD | Argument 10 |

**Not all 52 bytes are initialized.** In particular `+0x17`, `+0x26..0x27` and `+0x2D..0x2F` are left untouched by this constructor. The portable reference uses an explicit 52-byte little-endian array and verifies that those holes retain their prior values rather than silently zeroing the entire original object. The actual C++ source class name and full object lifetime are unknown.

## Verified 71-byte 0x24199BA0 dispatcher

The original function increments DWORD global **`0x242FDB50`** by one **before** it invokes any other callback. This storage lies in the original `.data` segment. It then:

1. Reads the third argument as an opaque record descriptor pointer. If its byte `+0x10` is **zero**, calls original **`0x24197F50(descriptor)`**. A nonzero value skips this preprocessor.
2. Passes all three original stack arguments to **`0x24199100`** with the snapshot block as original ECX (`this`). The meaning and implementation of that helper are not asserted.
3. **Re-reads the process-global counter after the helper** (important if other work modifies it) and calls native linked-record routine **`0x24199B50`**, forwarding that counter and the descriptor pointer.
4. Returns the current DWORD at **snapshot block `+0x28`**. This word may have been modified by external callbacks; the dispatcher itself does not compute a success/error value.

There is no native null-pointer guard on the descriptor, no exception handling and no native reentrancy lock. The reference model uses host-token validation and preserves the counter ordering, but does not impersonate the original engine serializer.

## Verified 65-byte 0x24199B50 linked-record walker

The original method receives two 32-bit stack arguments and returns with `ret 8`. Its second argument is the descriptor/head pointer. It sends the head record **unconditionally** to **`0x241996F0`**, forwarding four arguments in source order:

`(process_global_counter, record_pointer, record.DWORD_at_0x00, record.DWORD_at_0x04)`.

It then reads **record+0x0C** after that callback to find the next record and repeats until the pointer becomes zero. Critically, the original reads `next` **after** the callback, so modifications to a record's next pointer during a callback affect traversal. There is no cycle detection, maximum iteration count or null-head check in original 2006 x86 code. The independent host reference limits traversal to 4,096 records and detects cycles as **host-only safety adaptations**, not native behavior.

The callee at **`0x241996F0` remains opaque**. Exact snapshot serialization formats, record ownership, engine physics or prediction-side effects are **not** recovered by these three wrappers.

## Thirteen complete, exact original native CALL edges

All sites are verified original instruction-boundary `CALL rel32` instructions, checked against their exact original destinations:

| Caller site | Target | Significance |
| --- | --- | --- |
| `0x240C5A13` | `0x241978A0` | Phase210 SaveData constructs snapshot block |
| `0x240C5A34` | `0x24199BA0` | Phase210 SaveData dispatches snapshot |
| `0x240C6987` | `0x241978A0` | Alternate client caller A constructs |
| `0x240C699F` | `0x24199BA0` | Alternate client caller A dispatches |
| `0x240CC4E3` | `0x241978A0` | Alternate client caller B constructs |
| `0x240CC504` | `0x24199BA0` | Alternate client caller B dispatches |
| `0x24154A42` | `0x241978A0` | Alternate client caller C constructs |
| `0x24154A5A` | `0x24199BA0` | Alternate client caller C dispatches |
| `0x24199BB6` | `0x24197F50` | Optional descriptor preprocessing |
| `0x24199BCB` | `0x24199100` | Snapshot preparation |
| `0x24199BDA` | `0x24199B50` | Linked-record forwarding |
| `0x24199B68` | `0x241996F0` | First/head record |
| `0x24199B7F` | `0x241996F0` | Every subsequent record |

Four separate caller pairs demonstrate that this argument-pack/dispatch chain is reused beyond Phase210's single SaveData callsite. Their exact enclosing original source classes are **not** inferred from these callsites alone.

## Implementation and validation

- `include/sin_compat/original_client_prediction_snapshot_dispatch.hpp`: original-source-grounded C++17 `ArgumentBlock32`, a strict 52-byte little-endian writer, dispatcher with explicit module-global counter parameter, callback-ordered opaque preparation, and host-token linked-record iteration. No original game pointers, tier0 implementation or actual engine callback are exposed.
- `tests/test_original_client_prediction_snapshot_dispatch.cpp`: verifies all written/untouched struct bytes; low-byte-only boolean arguments; callback ordering; counter increments before optional preprocessing; counter is re-read after prepare; head and subsequent record forwarding; linked-list mutation after callback; host-only loop/null guards and unsigned counter wrap; an integration through Phase210's `save_data_slot`.
- `research/tools/audit_sin_original_client_prediction_snapshot_dispatch.py`: read-only dual-original full SHA256 gate, **three complete native body SHA256s, 62 sparse opcode witnesses, 13 exact native direct CALL rel32 edges**, two precise reads/writes to original global counter and original `.text/.data` sections and RET/INT3 alignment.
- `research/tests/test_sin_original_client_prediction_snapshot_dispatch.py`: 11 synthetic-only mutation tests including all 62 witnesses and 13 original calls, function boundaries, fake-original hash refusal, code/data section and process-global address checks. The fixtures are independent synthetic bytes, not game data.
- Root CMake/Linux and native Win32 i386 MSVC CTest target registered. **Hosted CI must pass on the exact Phase211 commit before a validated-green claim.**

## Next dependency: actual record serializer

The original routine **`0x241996F0`** is the actual record-writing action invoked for the head and the rest of the linked list. It should be a priority for the next deep original-body audit. Other non-recovered dependencies include `0x24197F50` and `0x24199100`. The game has **not** yet been demonstrated launching the Windows retail campaign with reconstructed DLLs. Continue Windows PC engine startup first, then Android ARM64 and Quest 3.
