# Phase 112: SiN Episodes (2006) CBaseAnimating bone-cache preparation

**Source of truth:** owner-provided original x86 PE32 `server.dll`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
preferred image base `0x22000000`. The independent C++17 modules do not
include original game DLL bytes or decompiled original source.

## Original verified control flow

`0x220EB8D0..0x220EB9EB` is a bone-cache lifecycle function called by the
Phase 111 getter `0x220ED1B0`. Its code uses:

1. Model resource through `0x22139A10`, type `3` through the engine model
   cache vtable, and studio header bone count at `studio+0x9C`.
2. Entity `+0x478` stores a bone-cache handle. Resolve through
   `0x22101AE0` (backed by an engine cache registry).
3. For a resolved object, call `0x220FC5A0` on the global frame time
   (`0x22572190+0xC`) and **float threshold `0.1f`**. Its x87 comparison
   is `(now - cache[0]) <= 0.1f` for ordered operands. A cache timestamp
   in the future is accepted, while an unordered/NaN comparison is rejected.
   **Important:** the freshness check precedes any cache-type check, so a
   fresh object is returned regardless of its tag.
4. On an **expired** cache, read cache DWORD `+0x04` and compare with **`0x300`**.
   If different, release the old handle through `0x220FC5F0` and zero the
   entity handle at `+0x478`.
5. If no usable cache remains, or a stale correctly-tagged cache needs an
   update, call animation virtual **byte offset `+0x2B4`** with an output
   pose buffer and mask **`0x300`**. MSVC RTTI-inherited vtables in eight
   classes, including `CNPC_Jessica` and `CHL2_Player`, map this slot to
   **`0x220EF510`**. This is *not* a complete reconstruction of that solver.
6. If a stale cache with tag `0x300` exists, call `0x22100450` to copy the
   computed pose matrices into its compact matrix block; the original returns
   that same cache object. If none exists, call `0x22104BC0` to allocate
   and register a new cache, install its handle at entity `+0x478`, and
   resolve it through `0x22101AE0`.

## New detail: reverse remapping in `0x22100450`

Phase 111 recovered *bone ID → compact matrix* mapping. The update function
uses a **different reverse table**:

```
count       = uint16(cache + 0x0C)
matrixBase  = cache + 0x14 + uint16(cache + 0x0E)
reverseBase = cache + 0x14 + uint16(cache + 0x10)
for compactIndex in 0..count-1:
    sourceBone = int16(reverseBase + 2*compactIndex)
    matrixBase[compactIndex] = sourcePose[sourceBone] // 48 bytes
cache.timestamp = current_time
```

The local C++17 implementation validates buffer bounds, source bone IDs,
finite matrices, host callbacks and cache registration rather than copying
memory from untrusted addresses like the original x86 helper. Source pose
matrices are passed through a host callback. It uses a staged byte copy
so malformed reverse mappings cannot partially update the cache.

## Owner-side binary verification

Read-only `research/tools/audit_sin_original_bone_preparation.py` checks
**six complete SHA-256 function ranges, 15 exact x86 CALL targets,
22 instruction anchors, eight class vtable pointers to `0x220EF510`**.
The original functions SHA-256 witnesses are:

| Original address range | Function role | SHA-256 |
| --- | --- | --- |
| `0x220EB8D0..0x220EB9EB` | Cache preparation | `1967c6985ea3943cb498a36f0da3b7d35132fbf40011ebef078c5e832b772dbf` |
| `0x22100450..0x221004B0` | Reverse-map matrix update | `757c7ed176aed5cfdc0af6b2c312400aa75f72be1fb09feb651e9a083f51ffe4` |
| `0x220FC5A0..0x220FC5B8` | Freshness comparison | `4e04df9d5ff71668fc822766eee904f89d6a63f272c8559d30f74bc07e03d7d8` |
| `0x22101AE0..0x22101AEF` | Cache handle resolver | `e42940c4cf5597fbef25aaefc9b05db496aa96c923133552878ff3282ab4542a` |
| `0x220FC5F0..0x220FC5FF` | Cache release | `d4c0d7b1912cc03169515416e2759732bfc8cbeb42926fa85eb7a40b8eab1ed3` |
| `0x22104BC0..0x22104C11` | Cache allocator/registry | `cbaee30b2cb2794fa1f21eceea91d6e88ae19c102050a142bea9f154b9c9a430` |

These byte hashes and addresses are evidence, not gameplay/runtime tests.
The virtual bone solver `0x220EF510` is separately identified by RTTI/vtable
pointers but its substantial internals remain a **future decompilation task**.

## Independent portable source and tests

- `original_bone_preparation.hpp/.cpp` implements cache state routing, 
  freshness, and staged reverse-map compact matrix update. Real cache
  handles and pose computation stay behind `IOriginalBonePreparationHost`.
- `OriginalPreparedBoneCacheAdapter` connects to Phase 111's getter. The
  Phase 110 attachment resolver therefore receives newly prepared cached
  matrices on a stale cache and reuses them without calculating on a fresh
  cache.
- `test_original_bone_preparation.cpp` verifies reverse-map orientation,
  cached timestamps, stale/fresh routes (including a fresh wrong-tag case),
  release/reallocate and malformed source/host failures.
- `test_original_bone_preparation_studio_bridge.cpp` exercises the full
  adapter path into studio attachment pose reconstruction.
- Synthetic PE32 mutation tests and owner-side SHA audit pass; the standalone
  test project passes **12/12** CTests, including prior phase tests. Debug
  UndefinedBehaviorSanitizer suite also passes **12/12**.

## Explicit remaining gaps

The `+0x2B4` virtual function `0x220EF510` contains the substantive pose
computation, bone-mask logic and MDL data interfaces. The allocator
`0x22104BC0` uses a native engine cache registry. Neither can be called
by the portable C++17 host without implementing a verified engine ABI.
The original stack allocation, x87 rounding, raw-pointer lifetime and
low-level invalid-memory behavior are intentionally not reproduced. Some
native callbacks can have side effects, so production host writes must be
transactional and address failure recovery.

No full game `server.dll`, `client.dll`, direct HL2-compatible DLL swap,
Quest 3 native VR gameplay, engine renderer or campaign map execution is
claimed. Original assets/binaries remain private.
