# Gameplay Phase586: original named-action registration, record lookup and revision refresh

## Scope and original 2006 DLL identity

This source-bounded phase follows original Grunt action registration from weighted selector `0x220EB1C0` (Phase585) into its actual **record-access, name resolution and revision-refresh dependencies**. This work uses the unchanged original SiN Episodes: Emergence 2006 owner-private PE32 `server.dll` and `client.dll`, not HL2 SDK source and not unrelated physics, materials, surfaces, props/models, renderer, native Windows boot or Android/Quest runtime code.

- Original `server.dll` full SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` full SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original server PE32 preferred image base `0x22000000`.

| Complete original native method | SHA256-authenticated bytes | Original whole-body SHA256 |
|---|---:|---|
| `0x2211B7D0` | **25** | `f24d61f7f37b4272c44c76b97b7ad4fd467d6ecdb46ff70d659c92e6e97edff3` |
| `0x2211BD00` | **153** | `5535f05bdbc9f26f0c70793c170c2e32c75108aed1f560cc2b572b99d70a52eb` |
| `0x2211C110` | **97** | `bf68f439d4aa80d59b785cf34f2f999de12b1273118b0e369d994baf33430c2a` |
| `0x220EAF70` | **117** | `1b87d88fa667991d994cdd5ef5d077371bc095489c5a7bdc00097445830edf94` |
| `0x220EAFF0` | **80** | `57ae6d9372fd9b88c412fa6fe318bcfd23d0f8239282536f9674ca7466e96db9` |
| **Total** | **472 distinct new original bytes / five complete method extents** | All SHA pins independently checked |

Four methods terminate at an original `INT3` padding byte. **`0x220EAFF0` is different:** its `ret` at `0x220EB03F` is immediately followed by valid code at `0x220EB040` (`56 8B 74 24 08`), so the auditor checks the real next method's five-byte entry instead of fabricating an `INT3` boundary.

## Source-proven record layout and storage split

`0x2211B7D0` is the native record-count getter:
- When the owning collection dword **`+0x150 == 0`**, it reads count directly from **`+0xBC`**.
- Otherwise calls original `0x2211D460` for paged backing data and reads its **`+0x0C`** count.

`0x2211BD00` returns a pointer for a numbered original record. In direct mode (`collection+0x150 == 0`), it clamps negative or out-of-range indexes to **0** and calculates a pointer from a relative offset at collection `+0xC0`. Its original x86 strength-reduced arithmetic computes exactly **`0xD4` / 212 bytes per record**.

In paged mode, this method calls `0x2211D460` and uses an original 16-byte per-index mapping, a partition base at `+0x50`, a 144-byte partition stride and original helper `0x2211D470` before reaching a child array with the same 212-byte record stride. This path is authenticated at the byte level but **not** claimed to be reconstructed as a loadable native paged container. The safe host C++ reference only models the direct mode and rejects missing storage rather than dereferencing invalid original pointers.

`0x2211C110` obtains the registration revision/stamp. In direct mode it returns owner **`+0xC4`**. In paged mode it gets the local revision, then iterates partition revisions from the second partition forward, taking the **minimum**. Its pager/partition ABI remains a dependency.

## Original action names and numeric IDs

`0x220EAF70(registry,index)` is the **name-to-ID preparation** called from Phase585's weighted selection when record byte `+0x0D & 0x10` is clear:

1. Requests the indexed native record through `0x2211BD00` and **sets record byte `+0x0D |= 0x10`** immediately, before doing the name lookup.
2. In the valid-registry/index branch, reads a **record-relative text pointer** from original record **`+0x08`** and adds the record pointer. If its first byte is nonzero, it invokes native name resolver **`0x220E6CB0(string)`**.
3. If the first resolver returns exactly `-1`, it invokes alternate resolver **`0x220E6FE0(string)`**.
4. Writes the resolved numeric ID to record **`+0x10`**. Empty strings preserve the existing record numeric key.
5. In the original diagnostic/invalid branch, the fallback string literal at original `.rdata` **`0x223D9DDC` is exactly `Unknown`**. Native error-message and logging side effects are not assumed to be part of a safe host API.

This provides direct original proof that numeric keys used by the weighted action selector can be **populated from named text registrations** rather than being immutable developer-chosen IDs. It does not yet enumerate the actual action names or their full game-specific enum mapping.

`0x220EAFF0(registry)` is the full registration refresh:
- Null registry -> return without further action.
- For every current entry index (count re-queried via `0x2211B7D0`), call the name resolver/preparation `0x220EAF70`, fetch record using `0x2211BD00`, then perform original secondary record validation `0x220EA730`.
- Finally load the global revision dword `0x224E8854` and stamp the registry through `0x2211C180`. The refresh even updates the revision when the collection has zero entries.

The original weighted selector at `0x220EB1C0` checks `0x2211C110(registry)` against the same global revision dword and calls this full refresh routine when stale. That completes the source-proven **revision check -> name-to-ID refresh -> weighted index selection** dependency chain.

## Auditing, source model and limits

The original-free SHA auditor `research/tools/audit_sin_original_registry_refresh_phase586.py` requires the two exact retail DLL hashes, checks all **five whole-method SHA256s, 472 new distinct bytes, 16 direct E8 call targets, six prior Phase585 cross-calls, 52 byte witnesses** and correct real PE method boundaries. The self-test corrupts each byte of all checked `E8 rel32` calls and rejects nonmatching DLL identities. Original binary audit and synthetic tests: **PASS**.

The independent C++17 model `include/sin_compat/original_grunt_registry_refresh_phase586.hpp` and tests check the direct 212-byte stride, out-of-bounds index clamping when records exist, revision minimum across paged partitions, record flag writes before name lookup, primary/fallback name resolution, unchanged numeric key for empty names, null-registry no-op, per-record follow-up and revision stamp order. Strict GCC and Clang with warnings-as-errors, GCC AddressSanitizer/UBSan, standalone CMake/CTest **2/2 PASS**.

**What is still unresolved:** the names of individual registrations in the private game assets, original text-table loading, paged allocator implementation, full source/ABI of registry `0x2211D460` / `0x2211D470` and validation `0x220EA730`, low-level numeric name converters, and the eventual behavior of a complete replacement `client.dll`/`server.dll`. Original copyrighted DLLs, raw native disassembly dumps and commercial assets are deliberately excluded from all Git commits.

## Source-verified shared activity names (supplemental evidence)

A targeted native provenance check follows the two original name-resolution APIs called above. Their original whole-function SHA256 digests are pinned in the Phase586 auditor as **supplemental shared-engine evidence** (not added to the 472-byte gameplay-accessor count): `0x220E6CB0` (39 bytes), `0x220E6FE0` (84 bytes), and shared named-registration helper `0x220E6F60` (128 bytes). The large registration initializer at `0x220E7040` is 13,780 bytes, with SHA256 `be2d64d38bfffb9d6285fbb36268bc308a5d3d03100caf75dd6081dcb4b82206`; it contains **921** consecutively registered shared `ACT_*` names. This initializer is authenticated as a whole, but its 13,780 bytes are deliberately **not claimed as new reconstructed Grunt gameplay code**.

The auditor cross-checks each selected native `push id`, `push literal-address`, `call 0x220E6F60` triplet, along with the original NUL-terminated `.rdata` string. Verified examples:

| Original shared ID | Original 2006 name | Registration E8 callsite |
|---:|---|---|
| 0 | `ACT_RESET` | `0x220E7047` |
| 1 | `ACT_IDLE` | `0x220E7053` |
| 6 | **`ACT_WALK`** | `0x220E708F` |
| 10 | **`ACT_RUN`** | `0x220E70C2` |
| 15 | **`ACT_SCRIPT_CUSTOM_MOVE`** | `0x220E70FE` |
| 25 | `ACT_FLY` | `0x220E717C` |
| 30 | `ACT_HOP` | `0x220E71B8` |
| 154 | `ACT_DO_NOT_DISTURB` | `0x220E7809` |

This identifies the **shared animation-activity names** for several raw values seen upstream in Phases 583–584, particularly the literal 10-to-6 fallback. It is stronger evidence for an activity translation than a hypothetical generic schedule mapping. It is **not** proof that a Grunt's private `+0x338/+0x33C` transformations or sequence weights cannot remap these values, nor does it establish the emitted animation/sequence or its gameplay meaning. Grunt-specific schedule symbols remain separate research targets.