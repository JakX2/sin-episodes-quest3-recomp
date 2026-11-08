# SiN Episodes 2006 original animated bone-cache remap (Phase 111)

## Original source and scope

Owner-provided original x86 PE32 `server.dll` SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
All virtual addresses below use the original `0x22000000` image base.

This is **independently authored C++17**, not recovered proprietary source,
and it does not contain the original DLL, assets, raw disassembly or engine
pointer structures.

## Original bone-getter path

The Phase 110 animation attachment resolver invokes `CBaseAnimating`
virtual byte offset **`+0x2B0`**, originally `0x220ED1B0`.
The original getter:

1. Calls `0x22139A10` to obtain model-cache data and checks its type
   equals **3**, retrieving the studio header only if valid.
2. Requires the signed requested bone index to be **nonnegative**
   and smaller than the studio header bone count at **`+0x9C`**.
3. Calls `0x220EB8D0` to get or prepare a bone-cache object.
4. Calls `0x221004C0` with that cache and bone index. When it
   returns a matrix pointer, copies 12 dwords (48 bytes) through
   `0x22268EF0` into the output.
5. When the remapper returns null, calls `0x220EBE70` and copies
   the **entity's own world matrix**, not a guessed bone transform.
   That fallback checks the world-transform dirty bit at entity
   `+0xB8`, invokes `0x2213FA60` when needed, and obtains the
   matrix at entity `+0x268`.
6. If model data is unavailable, type is wrong or bone index is out
   of range, the original function returns without modifying the output.

The original `0x221004C0` has an unusually compact layout:

```text
bone_remap = signed_int16(cache + 0x14 + 2 * bone_index)
if bone_remap < 0:
    return null

matrix_offset = unsigned_int16(cache + 0x0E)
world_bone_matrix = cache + 0x14 + matrix_offset + 48 * bone_remap
```

This original helper **does not check the input bone index itself**.
The caller's studio header check must therefore precede the lookup.

## Binary audit

The read-only private verification script at
`research/tools/audit_sin_original_bone_cache.py` matches:

| Original function | VA range | SHA-256 |
| --- | --- | --- |
| Bone getter | `0x220ED1B0..0x220ED22F` | `4fea7931f33e8d0825a422c77b46fe46653b049ceea1e09344f7a7c1bd4e1165` |
| Compact bone remap | `0x221004C0..0x221004E2` | `7fc9d4fdffc06c20875ad7c526541110430b8d1359af39481ff872d7f40be908` |
| Entity world-matrix fallback | `0x220EBE70..0x220EBE8C` | `1139b829a7af912eb57a128d8cbfd4e0957959d4772223504d46ee522e026762` |
| 48-byte matrix copier | `0x22268EF0..0x22268F03` | `eda2988761a27e6b7a0f9a1fe41d482da5c805a3a6bc23ee934769a34c83be6e` |

Plus **7 exact original relative CALL targets and 17 instruction
witnesses** in the owner's unchanged DLL. A synthetic PE32 test uses
fabricated headers and rejects mutations.

## Independent implementation

- `include/sin_compat/original_bone_cache.hpp` introduces safe
  cache-decoding, `IOriginalBoneCacheHost`,
  `OriginalBoneTransformResult` and
  `OriginalBoneCacheStudioAdapter`.
- `src/original_bone_cache.cpp` parses the narrow verified cache
  layout, copies one cached 3×4 matrix on success, and uses a host
  callback for the entity's world-matrix fallback on an **uncached**
  remap entry. It does not access original host process pointers.
- `tests/test_original_bone_cache.cpp` validates compact matrix
  decoding, 1-to-many remapping, signed negative entries, fallback,
  model resource rejection, index bounds, malformed layouts, NaN,
  invalid world fallback and host exceptions.
- `tests/test_original_bone_cache_studio_bridge.cpp` checks that
  Phase 110's attachment resolution consumes both the cached bone
  matrix and the entity fallback correctly.
- New targets also build for the Android ARM64 CI toolchain.

## Explicit differences and incomplete areas

Original x86 uses borrowed raw pointers and copies 48 bytes. The
portable parser uses bounded owned byte arrays, finite-value validation
and return codes. It does not reproduce undefined behavior on damaged
cache structures. An absent prepared cache yields `InvalidCache`;
the original does not visibly guard the returned pointer after
`0x220EB8D0` in this entrypoint.

Most importantly, `0x220EB8D0` performs real native model-cache and
animation setup: its complete behavior, scheduling, bone masks,
`SetupBones`, and `0x22104BC0` cache allocation remain to be
independently reconstructed. This phase only reconstructs the
**getter, remap and fallback**, not complete per-frame skeletal animation.

Full desktop `client.dll`/`server.dll` replacements, map logic,
native SourceVR engine integration and standalone Meta Quest 3
rendering/gameplay are not yet implemented.
