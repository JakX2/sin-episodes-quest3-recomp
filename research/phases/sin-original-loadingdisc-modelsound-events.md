# Phase 94: CLoadingDisc lazy resources and CModelSoundCache animation-event names

## Original binary authority

Audited owner-supplied original SiN Episodes: Emergence (2006) 32-bit PE DLLs (both original SHA-256 fingerprints checked and all Phase84–93 checks chained). No proprietary executable bytes committed.

| Module | Original method | VA | Bytes | SHA-256 |
|---|---|---|---:|---|
| client | LoadingDisc context setter | `0x241EE470` | 10 | `0a5b1a6d212e0caedbf2d98dea5a75fa8dba9f19919dae6658f37365af35e72f` |
| client | Primary lazy resource toggle | `0x241EE4D0` | 80 | `d2a35e0881a33f3447d073f20958f374636904b977db66133265bcc6ab3ec184` |
| client | Secondary lazy resource toggle | `0x241EE530` | 96 | `ccb0ba5f5b379d76389e23746ec9973fd70c045f7bf29a6a83da5a053c6fe9ca` |
| server | Model sound event scanner | `0x22145520` | 447 | `49ea37e723784624d6c81a3f45746e06f5e93aea1020b5c196e2699d886fcbb3` |
| server | Resolved sound-token dedup | `0x221456E0` | 97 | `51434bb637d61f0bffcef624958a06b1e95a83c67616cebcad24f97b42542f7a` |

## client.dll: missing +8 field producer found

The original `CLoadingDisc` singleton image at `0x2431DDD4` has `vtable=0x242AC8B0` and **three zeroed pointer fields** at `+4`, `+8`, and `+0x0C`. The 16-byte constructor found in Phase93, however, writes **only** vptr and `+4`. Static image storage independently starts `+8` at zero.

The four vtable methods are `0x241EE470`, `0x241EE480`, `0x241EE4D0`, `0x241EE530`. Slot zero stores a caller-supplied pointer at `+0x0C`. Slot one closes both resources (Phase92). Slot two performs **lazy allocation for pointer `+4`**; slot three separately does it for **pointer `+8`**.

On first enable with a missing pointer, each calls original allocator `0x2416FCF0` for **`0xDC` (220 bytes)**, constructs through `0x241EE350` with `this+0x0C` as argument, calls `0x240049D0`, and stores the returned pointer. Both then invoke nested resource virtual **`+0x78` with the enable/disable flag**. The secondary branch also calls its newly created resource virtual **`+0x28C`** with static argument at `0x242AC8C0`. Disabling a resource does not free it or reset the pointer; later enables reuse it.

**Important fidelity exception:** original secondary code dereferences the new pointer without a clear intervening null check. The host-only C++ adapter returns a safe failure on null instead of reproducing a possible original crash. Neither nested resource class name nor exact native SourceVR ABI has been recovered.

## server.dll: animated model sound events

The original `CModelSoundCache` slot-2 routine calls `0x22145520`, which iterates parsed event descriptors with a **`0x50`-byte stride**, loading event ID at `event+4` and flags at `event+8`. It recognizes **event codes 14, 1004, 1008, 5004, 6004, 6005**. For code **14**, original requires `flags & 0x10` and nonempty `event+0x0C` text.

For IDs **6004 or 6005**, original code generates these exact formats around a base name:

- `%s.FootstepRight`
- `%s.FootstepLeft`
- `%s.RunFootstepRight`
- `%s.RunFootstepLeft`

It also uses a literal fallback base **`NPC_grunt`**, and formatted output capacity **`0x100`**; this is *not* a serialized cache-record size.

The shared `0x221456E0` resolver obtains tokens through global `0x225721D0` vtable byte offset **`+0x0C`**, ignores **`0xFFFFFFFF`**, checks an existing 32-bit token array with count at owner `+0x0C`, and inserts only novel identifiers using original helper `0x22095600`.

This supports a **set of unique sound identifiers** discovered from parsed model event descriptors. It does **not** establish an entire on-disk `modelsounds.cache` record representation, a full model/animation parser, or the identities of all the engine interfaces.

## Implemented, test evidence, limitations

- `include/sin_compat/original_loadingdisc_modelsound_events.hpp`: C++17 host-only, safe lazy resource and parsed event/name-to-token behavioral model.
- `research/tools/audit_sin_loadingdisc_modelsound_events.py`: chained original dual-DLL SHA + 5 complete function SHA-256, original opcode/callsite and static object checks, five exact sound-name strings; emits metadata.
- `research/tests/sin_original_loadingdisc_modelsound_reference.cpp`: mock lifecycle and sound-code/token assertions.
- `research/tests/test_sin_loadingdisc_modelsound_events.py`: 12 original-free, mutation-based regression groups.
- `CMakeLists.txt`: CTest integration.

Local original DLL audit PASS, **Python 12/12 PASS**, **CTest 1/1 PASS** under strict C++17 warnings. GitHub CI checked separately.

**Not claimed:** fully reconstructed client/server DLLs, original data stream parsing or modelsounds cache writer, SourceVR ARM64 method ABI, Quest 3 binary game modules, APK or playable campaign.

Next technical milestone: identify the nested loading-disc resource's actual interface and original sound-token engine map, then validate methods against the selected SourceVR 0.5.5 binaries before binding native game modules.
