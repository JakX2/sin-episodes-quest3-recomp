# Phase 561: Original 2006 SiN Grunt linked-group cached bitmap lifecycle and 30-second deadline

## Provenance / explicit limits

This continues Phase560's original server `0x22054D40` and `0x220AAEB0` shared-condition reservation flow. This phase follows all four native `E8` calls from range reservation into the original bitmap/cache accessor at `0x220AA920`, and authenticates three directly associated bit operations.

The owner provided the original retail 2006 `SE1/bin.zip` through the linked Drive folder. Both exact full-file identities are checked **before any internal method**:

- `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` (PE32 preferred image base `0x22000000`).
- `client.dll` SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

This document contains no original proprietary code bytes or disassembly. It does not assert that an original game DLL can be rebuilt, loaded on Windows, or ported to Android/Quest.

## Four new native functions

| 2006 original server VA | Exact extent | Complete original method SHA-256 | Direct native behavior |
| --- | ---: | --- | --- |
| `0x220AA920` | **733** | `dee90dc641af8bb1a7e92c8a42118dd2eb8238d79e8f17bdd2c8a8c25a28d7ed` | Time-aware per-entity group bitmap lookup, record pool refresh/compaction, cached entry selection or insertion |
| `0x220AAC00` | 38 | `5e08de620d899200b2a2495bceebe81b68716af275e81f308adc638d7375fb1c` | Retrieve original bitmap and OR a selected bit |
| `0x220AAC30` | 40 | `26a74fbb563d364c4467942e6ebfb3b932cfe3af225cd0edfa2e313eabebd670` | Retrieve bitmap and AND with inverse bitmask |
| `0x220AAE80` | 44 | `f101866ac1dd4c33ab4adbb637f7eb21fd611a15e759ddf42b729434428dbdc0` | Retrieve bitmap and normalize selected bit to a Boolean |

**4 distinct complete original server methods, 855 new authenticated code bytes**, **9 original internal direct E8 calls**, **4 prior Phase560 call edges**, **32 independent opcode/offset witnesses** and exact **30.0f** constant. All four methods have verified original `0xCC` padding immediately after their true method extents. All data and method bytes remain private.

## 1. The authentic cache refresh threshold

Original `0x220AA920` reads the engine timing global `0x22572190`, then the single-precision float at global `+0x0C`. It compares that time with linked-group float deadline at **group `+0x74`** using native x87 `fcomp`, `fnstsw ax`, and `test ah, 0x41`. On an **ordered strictly greater-than** result, execution enters the original refresh path. Equality, less-than or unordered comparison take the branch bypassing the refresh path. Ordinary finite C++ values can approximate the gate as:

```cpp
bool should_refresh(float time, float deadline) {
    return std::isgreater(time, deadline);
}
```

That is a human-readable gate, not a bit-identical substitute for all 2006 x87 intermediate precision or exception states.

After the refresh branch, the original loads the same engine-time pointer again, fetches the current time, adds the **exact original 32-bit float `30.0f` stored at `0x223D8FE0`**, then stores the sum back to group **`+0x74`** (`0x220AAAB0` through `0x220AAAB9`).

This is **30 seconds for the bitmap/cache maintenance deadline**. It does not mean every Grunt fires, speaks or changes tactics every 30 seconds. Those would require separate real-game evidence.

The refresh work is conditional on the group having cached records (`group+0x6C != 0` in the examined path); the new deadline can still be advanced when no records are present.

## 2. What is cached

The authentic method accesses several distinct original group members:

| Original offset | Observed role | Evidence |
| --- | --- | --- |
| `+0x08` | Starting address for 32-bit serial entity handles | `0x220AA983` plus generation validation against `0x224F5620` |
| `+0x48` | Number of linked entity handles | `0x220AA94A`, later compared with loop index |
| **`+0x5C`** | Cache pointer to selected record, invalidated/reassigned in native code | `0x220AA952`, `0x220AAABC`, `0x220AAB3F`, `0x220AABC8` |
| **`+0x60`** | Record vector storage | `0x220AAA00`, `0x220AAB3C`; original record stride is 8 bytes |
| **`+0x6C`** | Number of cached records | `0x220AA941`, `0x220AAA6C`, removal path `0x220AAA85` |
| **`+0x74`** | Next refresh time, as 32-bit float | compare `0x220AA931`, write `0x220AAAB9` |

At a due refresh, native code iterates original group entity handles, uses their generation-checked resolution and virtual **`+0x154`** to assemble a temporary context, calls the original helper `0x220A9A00` and conditional follow-up `0x220AA8C0`. It then processes cached 8-byte records, removing/replacing some by swapping the last record into the removed slot and decrementing `+0x6C`. Original memory cleanup may invoke `0x2226E3E0` and array growth invokes `0x220A8460`; native copy helper `0x223A5D10` is also reached.

After refresh, the method tries to reuse the existing `+0x5C` selected-record pointer when its owner resolves to the supplied original argument. Otherwise, it scans the 8-byte record collection for a matching owner identity, and, if absent, extends the record array and creates a new entry. `+0x5C` is updated to identify the selected cached record.

**Important uncertainty:** the original full semantics of `0x220A9A00`, `0x220AA8C0` and the record owner identity conversion are not fully decompiled in this phase. The 733-byte method's complete body/hash and branch side effects are authenticated, but its real object and allocation ABI are not replicated. Do not treat a generic vector erase or cache reset as exact original behavior.

## 3. Exact bit indexing and operations

Three independent complete methods call the same `0x220AA920` accessor with the provided owner/category argument, then index the returned bitmap using signed x86 arithmetic shift and low-five-bit masking:

```cpp
// Semantic explanation, not original 2006 source or memory ABI.
int word = signed_index >> 5;
unsigned bit = unsigned(signed_index) & 31;
uint32_t mask = uint32_t(1) << bit;
// bitmap DWORDs start at returned_record + 0x04.
```

- `0x220AAC00`: native `OR` into bitmap DWORD and `ret 8`.
- `0x220AAC30`: native `NOT mask`, then `AND` into bitmap DWORD and `ret 8`.
- `0x220AAE80`: native `AND bitmap DWORD`, then `neg; sbb; neg` to normalize to 0/1, `ret 8`.

Those operations are **per retrieved owner record**, not necessarily one global bitfield shared by every Grunt. The `0x220AAEB0` reservation algorithm from Phase560 calls `0x220AA920` four times from its paths to read or mutate those same record bitfields.

The native record stride is 8 bytes in the original cached-record vector, so the supported index width and memory allocation boundaries must **not** be generalized from these bit shifts. The independent host model uses explicitly allocated vector words and refuses invalid bit indices rather than reinterpreting out-of-range data as neighboring original C++ objects.

## 4. Separate proof levels

1. **Original x86 authenticated:** 733-byte cache accessor, three complete bitmap helpers, precise 30.0f literal, deadline/time field offsets, record cache metadata, native virtual/call targets and next-method boundaries.
2. **Source-bounded reconstruction:** time gate, 30-second deadline update, handle-based record refresh/compaction and cache lookup/creation, bit operations.
3. **Independent C++17 reference:** `include/sin_compat/original_grunt_bitmap_phase561.hpp`, host `TimedBitmapHost` and `Bitmap` with safe allocated word vectors, a host-supplied refresh callback, explicit owner category IDs, deterministic tests of strict refresh ordering, no-data refresh, record selection/set/clear/test and 31/32 boundary.
4. **Not yet proven:** genuine engine execution, native MSVC field types and ownership, identical refresh/compaction across all original stale-handle cases, original `0x220A9A00` semantics, exact game-wide scheduler integration.

Synthetic reference callback and owner removal functions are host conveniences; they deliberately avoid pretending to be the original 733-byte x86 allocator/collection logic.

### Reproducibility

```bash
python3 research/tools/audit_sin_original_grunt_bitmap_phase561.py --self-test
python3 research/tools/audit_sin_original_grunt_bitmap_phase561.py --bin-zip /path/to/private/bin.zip

c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude \
  tests/test_original_grunt_bitmap_phase561.cpp -o /tmp/sin_group_bitmap
/tmp/sin_group_bitmap
```

Owner-private original SHA audit PASS, 4 whole methods / 855 original bytes / 9 native direct calls / 4 earlier xrefs / 32 opcode witnesses / original `30.0f` deadline delta. Python mutation selftest PASS (all five bytes at each verified `E8` edge). Strict GCC and Clang C++17 PASS, GCC ASan/UBSan PASS, standalone CTest **2/2 PASS**. Hosted tests do not need proprietary original game bytes.

## Next original DLL research

- Decompile **`0x220A9A00`** and **`0x220AA8C0`** to establish exact record/selector construction and detach/refresh behavior.
- Resolve native RTTI/class ownership for the linked group, registration strings, and `CNPC_MobGrunt+0x9C4` lifecycle; only semantic method names are used so far.
- Link the now-proven reservation gate and 30-second refresh to named original Grunt schedules, not guessed HL2 constants.
- Continue full source translation of the target selection remainder in Phase559's `0x2235A560`.

No original commercial DLLs, raw native disassembly dumps or game assets have been committed.