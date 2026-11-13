# Phase 212: original client prediction record-type router and inline jump tables

13 November 2026. **Stacked draft on Phase211 PR #265.** This phase advances from the already-recovered original client SaveData setup, argument constructor and linked-record walk into the actual original game record serializer dispatcher at `0x241996F0` in `client.dll`. The owner-provided complete 2006 client and server DLLs remain **private**; no original commercial DLL, game asset, raw complete native body or long disassembly is committed.

## One newly SHA-verified full original native function

| Original module/entry | Complete executable function size | Whole original function SHA256 |
| --- | ---: | --- |
| 2006 `client.dll` at `0x241996F0` | **1,036 bytes** (`0x40C`) | `71406e900eef6ba6d8abd37ab73f8730c71c35d95a8700a8cb4ab7c9a2b6a7a6` |

The complete owner-original `client.dll` is 3,592,192 bytes and has SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. The companion original `server.dll` is 6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. The original client's image base is `0x24000000`.

**Important executable-data boundary:** the original 1,036-byte method starts at `0x241996F0` and ends with `ret 0x10` at `0x24199AF9` (three-byte instruction). There is NO `int3` padding immediately afterward. Instead the compiler embedded **52 bytes of 13 absolute jump destinations** directly in `.text` at `0x24199AFC`, followed by **27 selector bytes** at `0x24199B30`, then **five `int3` bytes** at `0x24199B4B..4F`. The next separately reviewed original function, Phase211 linked-record walker, begins at `0x24199B50`. Treating the jump-table bytes as linear x86 instructions would invent false code/function boundaries.

## Exact original 27-type selector

The original code compares the per-record DWORD type against **26**, reads a case byte from absolute `0x24199B30 + type`, and jumps through the 13-case address table at `0x24199AFC + 4*case`. Values greater than 26 take the error/warning branch.

| Original field-type DWORD values | Case index | Exact original branch target | Behavior shown by native x86 |
| --- | ---: | --- | --- |
| `0,11,12,14,15,16,17,18,19,21,26` | 0 | `0x24199AD5` | Advance to next record without specialist call |
| `1` | 1 | `0x24199847` | Four native specialist calls |
| `2` | 2 | `0x2419987F` | Four native specialist calls |
| `3` | 3 | `0x241998AD` | Four native specialist calls |
| `4` | 4 | `0x241998E7` | Four native specialist calls |
| `5` | 5 | `0x241999B7` | Four native specialist calls |
| `6` | 6 | `0x2419997D` | Four native specialist calls |
| `7` | 7 | `0x241999EF` | Four native specialist calls |
| `8` | 8 | `0x24199A29` | Three specialist calls and conditional byte copying |
| `9` | 9 | `0x24199921` | Three specialist calls and conditional byte copying |
| `10` | 10 | `0x241997CE` | Native recursive call to **the same `0x241996F0` function** at callsite `0x24199825` |
| `13` | 11 | `0x24199A8E` | Four specialist calls |
| `20,22,23,24,25` | 12 | `0x24199AC7` | Calls `tier0.dll!Warning` with `Bad field type\n` |
| `>26` | 12 (direct error) | `0x24199AC7` | Same original warning path |

The raw selector array is exactly
`[0,1,2,3,4,5,6,7,8,9,10,0,0,11,0,0,0,0,0,0,12,0,12,12,12,12,0]`.
All 13 real jump-table target DWORDs are pinned and verified to lie within the original 1,036-byte method.

The original `Warning` import is the **real named `tier0.dll!Warning`** at client IAT `0x242761E0`, with original NUL-terminated `Bad field type\n` literal at `0x242A41E0`. This is **not** `tier0.dll!Error`, not a guessed warning procedure.

## Original per-record filtering and memory layout

Before the type switch, the function gets a record (the native walk advances descriptors by **0x34 bytes**), extracts a signed WORD of flags at **record+0x12**, reads type at **record+0x00** and compares record+0x2C against its current marker. An already-seen/current-marker match bypasses the switch. For types other than **10** it applies all three original x86 filters:

- Original `test dh,2`: skip if flags bit **0x0200** is set.
- When state mode at `this+0x00` equals **1**, skip if flags bit **0x0100** is set.
- When state mode equals **2**, skip if flags bit **0x0100** is **not** set.

Type 10 bypasses those flag filters, **but still obeys the earlier marker equality skip**. All other low-order record flags are not interpreted by this Phase212 reference. The original uses per-record data offsets, persistent record state and function-specific side effects; the portable callback model only decides case/branch and cannot replace original writes.

The original native function invokes **39 exact direct `CALL rel32` instructions** within its 1,036-byte body, reaching **36 distinct entrypoints** (one self-recursive call and many field-specific helper families). All 39 concrete instruction addresses/destinations are pinned in the audit. Type 8 and type 9 include explicit `REP MOVSD` and `REP MOVSB` paths, which are not asserted to be generic `memcpy` replacements without reconstructing their surrounding conditions. Several record cases contain a sequence of four helper calls. All these **external helper method implementations remain opaque** in Phase212.

## Code and verification

- `research/tools/audit_sin_original_client_snapshot_record_router.py` requires **both entire original DLL SHA256s** before authenticating this newly reviewed 1,036-byte function. Validates full original method SHA256, opcode witnesses, **all 39 exact native `CALL rel32` sites**, the complete **13-DWORD jump table**, complete **27-byte selector**, embedded table/next-function boundaries, the original warning string and original import provider/name/IAT address. No original complete function body is stored.
- `research/tests/test_sin_original_client_snapshot_record_router.py` independently synthesizes PE-like bytes, deliberately without full original code, then mutates each of 39 native calls, each of 13 jump pointers, all 27 selectors, each of the sparse instruction witnesses, the warning import/string, image section/base and the inline five-`int3` boundary. Synthetic fixtures **cannot** claim original original-function authentication.
- `include/sin_compat/original_client_snapshot_record_router.hpp`: fully authored C++17 **partial routing model**, exact 27 selector values and 13 native case destinations, original flag/mode truth table and callbacks for filtered, no-action, specialist, nested-record and warning outcomes. It deliberately **does not implement specialist methods or original serialization side effects**.
- `tests/test_original_client_snapshot_record_router.cpp`: exhaustive 27-type route check, thirteen target bounds, all documented original type families, warning and out-of-range types, combinations of mode 0/1/2/3 and three relevant flag bits, marker skip priority, type-10 bypass and opaque host callback dispatch. Native Linux CTest and MSVC Win32 i386 CTest registered.
- Original function inventory **238 → 239** (**128 server / 111 client**). The increase is exactly **one full-function SHA-verified entrypoint**, not 36 implied helper recoveries. These figures are native research counts, not gameplay completion.

## Highest-value next step

Inspect the 36 direct callee entries and prioritize **one complete field-processing case at a time**, particularly the embedded type 10 recursion and the type 8/9 byte-copying paths. Reconcile with Phase211's record walker, original record/field descriptors and the 2006 engine's actual prediction snapshots. Separately continue the original Win32 game-engine factory/initialization gate: passing synthetic CTest still does **not** prove that an authentic retail Windows process can load and execute rebuilt client/server gameplay. Windows PC first, Android ARM64 second, Quest 3 third.
