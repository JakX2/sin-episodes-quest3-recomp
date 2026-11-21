# Phase 567: original Grunt +0x8F8 navigation object and route-chain construction

**Source of truth:** owner's untouched 2006 *SiN Episodes: Emergence* native PE32 `server.dll`, with its exact paired `client.dll` checked. This phase advances original NPC/gameplay decompilation and traces the Phase566 `GRUNT_FLANK` cumulative-waypoint value to its storage **allocation, initialization and virtual interface**. No physics, surfaces, model/prop rendering, Android, VR or new game loader is implemented.

## Exact original image identity and complete methods

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `server.dll` preferred base `0x22000000`, all nine function extents in native `.text`.

| Original VA | Size | Whole original native method SHA256 | Verified behavior |
|---|---:|---|---|
| `0x22031D50` | 276 | `575ffffba81df6bc80a78e347c3c8323f73412852556a9d632f79cf8ef75ebbd` | Original ordered seven-virtual NPC component setup; stores virtual +0x40C return into NPC+0x8F8 |
| `0x22031F50` | 50 | `8d099947aff7f4bb4f70dbc940af5c91543412ecec895fc4617d2f5a3da638aa` | Allocates **0x9C / 156 bytes**, clears memory and calls component constructor |
| `0x2216DDD0` | 10 | `69beb53321d2f642bfcdaa9f2968f086157c785785fdd2b0d5448433f2579527` | Grunt vtable +0x40C: call native component allocation then clear component byte +0x38 |
| `0x220747A0` | 272 | `efa418a1dc175deb2e47b3f6cc1e0a45c8b830a5def42e55186c616ea1a3cb66` | Constructor: stores owner at component +0x04, allocates **0x6C / 108-byte** route, stores pointer at +0x24, initializes navigation state |
| `0x2209D3B0` | 89 | `323005e7e1a163240b53764b5574b093479a375108b49539049488a030b2fa10` | Route constructor: first waypoint null at +0x00, state +0x44 zero, sentinel cache fields |
| `0x22074090` | 15 | `8f9128e602be535749c7fb3db522ed78d9af3ab0c8974e2ca1dde8fb8fb4eb79` | Routes component +0x24 to `0x2209D0F0` then normalizes its nonzero +0x44 result |
| `0x220740A0` | 21 | `38155a2c8150d8ec632f8923e4f18feb2ce220019dc86f3aca2983b5d596cbfd` | Return true only if route pointer **and its first waypoint** are nonnull |
| `0x220746C0` | 46 | `759c3ddf7c122d6e55634db106e4843f6453d50c340f37853b8506071e7b5ff6` | Initialize a 0x30-byte waypoint by copying 12 DWORDs then set cache +0x14 to raw -1.0f and both links +0x28/+0x2C to null |
| `0x2209D0F0` | 4 | `75315a13a7503cb9a126675518a19ff201faa8c510e8a849d0f971d9c61bbb2b` | Raw route +0x44 DWORD getter |

**Total: 9 newly authenticated original complete functions, 783 distinct native server bytes, 11 verified original E8 rel32 direct call edges, 58 short instruction/field witnesses, and Grunt vtable +0x40C pointer authenticity.** This is a tiny but connected original gameplay-code subset, **not** a percentage estimate of the full DLL.

The original method at `0x220747A0` ends in native `ret 4` immediately before the **next actual method** at `0x220748B0`. There is **no `INT3` gap**. The auditor verifies the actual `C2 04 00` and next-entry bytes rather than mistaking an embedded instruction for padding; the remaining eight methods use their authentic `INT3` post-boundary markers.

## Original cross-subsystem dataflow

The Grunt primary vtable begins at original `0x2249AAA4`. The on-disk DWORD at **`0x2249AEB0`** (`+0x40C`) resolves exactly to **`0x2216DDD0`**. This native wrapper invokes original **`0x22031F50`**, the 156-byte allocator, then clears byte `+0x38` in the returned navigation component. The allocation helper calls **`0x220747A0`** after its original memory-clear operation.

The shared NPC component initializer at `0x22031D50` calls these original virtual slots in this **specific order**, checking each result for null and returning false at the first failure:

1. `+0x3FC` -> NPC `+0x898`.
2. `+0x404` -> NPC `+0x908`.
3. `+0x408` -> NPC `+0x8FC`.
4. `+0x400` -> NPC `+0x904`.
5. **`+0x40C` -> NPC `+0x8F8`**, the nav component constructed here.
6. `+0x410` -> NPC `+0x900`.
7. `+0x414` -> NPC `+0x9D8`.

On full success, original code continues to link components through `0x2206BF10`, `0x220674D0`, the nav virtual `+0x10`, `0x22083CD0` and `0x220AB0D0`, then returns true. These related component methods are **not** falsely counted as recovered in this phase. Failure at a late slot does **not** imply the initializer rolls back previously stored pointers; none is observed in this native method.

The navigation constructor at `0x220747A0` separately allocates **0x6C** bytes and calls the already newly SHA-authenticated original route initializer **`0x2209D3B0`**. This route pointer becomes nav component field **`+0x24`** (`0x22074803`). It initializes this field to null on the observed failed allocation path; the original engine allocation/lifetime contract is not recovered. The constructor also allocates another **4-byte** object stored at `+0x28`; it is **not** the route's first waypoint.

Original component numeric fields include default distance/threshold-like values `+0x64=-1.0f`, `+0x68=0.25f`, `+0x6C=-1.0f`, `+0x70=3.0f`, `+0x74=-1.0f`, `+0x78=5.0f`. The exact original semantic field names/units are unproven, so the independent host model preserves the numeric values without overidentifying them.

The original 108-byte route initializer sets first waypoint **route `+0x00 = null`**, `+0x10=-1`, `+0x24/+0x28/+0x2C=-1`, `+0x44=0`, `+0x4C = FLT_MAX` raw `0x7F7FFFFF`, and `+0x20=0.1f`. Additional fields initialize route state/global-vector settings; those parts are authenticated by its whole-method digest but are not pretended to be fully typed C++.

### Why this proves Phase566's flank route ownership

The original Grunt event handler at Phase565 address **`0x2235AE41`** reads NPC `+0x8F8`, **`0x2235AE47`** reads that object's `+0x24`, and **`0x2235AE4A`** calls Phase566's original waypoint length getter `0x2209D410`. These exact first two original instruction witnesses are validated in Phase567 and the latter was authenticated in Phase566. The resulting x87 route distance is compared **strictly >240.0f** before requesting `GRUNT_FLANK`.

The constructor and caller therefore agree on the **same offsets** for the navigation component and route. The original virtual-slot mapping establishes how the NPC receives that component. **What is still unknown:** the exact developer class names for the navigation and route objects, whether the Grunt's tactical planner actually chooses the path, and all routes that can trigger the event. The fact that the component is allocated and contains a route does **not** mean the route has any waypoint or that a flank line plays.

The distinction is source-verified at `0x220740A0`: return true only when component `+0x24` route pointer exists **and its first-node pointer** is nonnull. The separate `0x22074090` method instead queries the **route state field `+0x44`**, normalizing it to Boolean. The two predicates are **not interchangeable**.

### Native waypoint record creation (0x220746C0)

Original waypoint construction copies **12 DWORDs (48 bytes)** from its input record via `rep movsd`, then forcibly sets `+0x14 = 0xBF800000` (bit-exact -1.0f) and both next/previous links at `+0x28` and `+0x2C` to zero. The newly constructed waypoint is **not automatically linked** by this constructor; insertion and navigation route-producer paths remain targets for subsequent decompilation. Phase566 proves the cumulative-distance getter trusts a non-sentinel terminal cache and recaches via these two links only when the sentinel matches.

## Portable source-like reconstruction and deliberate limits

`include/sin_compat/original_grunt_nav_phase567.hpp` uses typed host route/nav records and injected component-creation callbacks. It models:

- The original component initialization **order**, early exit, and the point when NPC `+0x8F8` becomes nonnull.
- Raw `+0x24` route allocation and empty route state, the different route-present and route-flag predicates, and selected numeric constructor defaults.
- The original waypoint's 12-DWORD copy followed by invalid cache and null-link overwrites.

It does not reinterpret 2006 x86 absolute pointers as actual host pointers or assume original allocation is safe under OOM. The original zeroing routine includes unconditional `rep stosd` before a null branch: a host-safe allocator must not replicate invalid writes. All x87, engine virtual state and full event scheduling not explicitly recovered remain outside equivalence claims.

### Verification

```sh
python3 research/tools/audit_sin_original_grunt_nav_phase567.py --self-test
python3 research/tools/audit_sin_original_grunt_nav_phase567.py --bin-zip /private/SE1/bin.zip

g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude \
  tests/test_original_grunt_nav_phase567.cpp -o /tmp/grunt_nav_test
/tmp/grunt_nav_test
```

- Original SHA pair / **9 complete method hashes**, **783 unique bytes**, **11 real direct call edges**, 58 opcode/field witnesses, original Grunt vtable +0x40C and callsite +0x8F8/+0x24: **PASS**.
- Synthetic audit mutation of all five bytes of each direct call and incorrect original-file identity: **PASS**.
- Strict GCC 14 and Clang C++17, ASan and UBSan: **PASS**.
- Standalone CTest **2/2 PASS**. Hosted CI is separate evidence to check after pushing; original game runtime behavior was not executed.
- No owner retail DLLs, raw full disassembly, proprietary maps/models/assets or executable patches committed.

## High-value next native functions

1. Recover the insertion/linking/removal of waypoint records into route `+0x00` and the route's first/terminal links, starting at native `0x2209D720` and `0x220B2A10`.
2. Trace the original event-dispatch virtual +0x5A8 callers. Most known native callsites pass event numbers in the `0x3E8` range; **only the original direct Grunt handler's event 0/1 branches are evidenced here**, not their producer names. Avoid assuming any `+0x5A8` call will emit flank speech.
3. Find original navigation factory state setters and Grunt schedule paths that populate nav `+0x24` and select event 1.
4. Continue source-first recovery of original game-specific Grunt logic and `client.dll`, rather than reimplementing already-HL2-compatible physics/surfaces/models/props.