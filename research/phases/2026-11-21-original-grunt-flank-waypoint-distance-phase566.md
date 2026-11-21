# Phase 566: original Grunt flank sentence is gated by cumulative waypoint distance

**Source:** owner's original 2006 SiN Episodes: Emergence `server.dll`, with the matching original `client.dll` used as an identity prerequisite. **Scope:** native C++-source recovery for the game-specific Grunt `GRUNT_FLANK` speech decision. It is not an HL2 physics/model implementation, nor playable Windows/Android/Quest runtime code.

## Authentication and complete native function extents

Owner-original SHA-256, verified before reading instructions:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Original preferred server image base `0x22000000`. Every complete extent below is in original `.text`, ends at a real `ret`, and has `0xCC` alignment padding afterward.

| Original function VA | Full length | Whole-method SHA-256 | Direct behavior |
|---|---:|---|---|
| `0x2209D370` | 53 | `8f12ce1dc95011ea1289656287f5a4522322713532878c693aa43e0221acee29` | Recompute cumulative waypoint metric by stepping backward through node `+0x2C` |
| `0x2209D410` | 49 | `9d1f0effa105e5bf3c2a65cfb7a32c1a075cea1dca4cb2cf3ff916d9d56bd071` | Cached distance getter, with original exact `-1.0f` invalidation sentinel and zero fallback |
| `0x220B2A20` | 26 | `921a3f9d537e074580ceb7905ad98a4711f0742875c1c8f2280ffc1cee25517c` | Traverse `+0x28` forward node links to terminal node |
| `0x2206D9F0` | 124 | `89d5af4b7e6ff60288f9d3a636f4ac6f658ce898df290c3c1f67b377300a0526` | Calculate segment distance from two vector positions, with flag selecting XY versus XYZ |
| `0x222680E0` | 7 | `6a9288ec74ac316568f99c1082f7fe146f57cf2af3a34ba4c989052209a3d807` | Read one float argument, execute x87 `fsqrt`, return in ST(0) |

**Totals:** 5 complete previously uncounted methods, **259 distinct authenticated original server code bytes**. Audited direct native E8 edges: `0x2209D38C -> 0x2206D9F0`, `0x2209D413 -> 0x220B2A20`, and `0x2209D426 -> 0x2209D370`; the earlier Phase565 Grunt crosscall `0x2235AE4A -> 0x2209D410` is checked but does **not** count as new source bytes. There are 24 additional short opcode/field witnesses.

The original server data pointer **`0x2251B1C4` initially contains `0x222680E0`**, and original `0x2206DA5F` uses an indirect call through that pointer. This cross-links the vector length to the 7-byte authenticated x87 `fsqrt` function rather than assuming a platform math-library implementation. This is the owner's on-disk pointer value; do not assume another binary or runtime patch has the same pointer.

## The Phase565 event-1 metric identified

The authentic original Grunt event handler `0x2235AE30`, already verified in Phase565, loads owner pointer `+0x8F8`, then pointer/member `+0x24`, invokes **`0x2209D410`**, and compares the x87 result with original float **`240.0`** at `0x223F8EFC`. Only **ordered strictly greater-than** selects original token `GRUNT_FLANK` via Grunt virtual `+0x7FC`, and dispatches the sentence at owner `+0xCC0` with argument `0` and mode `2`. Other event paths, prior sentence policy and queue semantics remain as documented in Phases558 and 565.

The recovered code **identifies the measured quantity as cumulative waypoint/path-node distance**, not an arbitrary timer. It establishes a geometric unit in coordinate-space for the points, but not the tactical meaning of the owning route object or whether each world map unit corresponds to a conventional distance unit outside Source coordinates. That owning runtime type is still unproven.

## Source-bounded recovered behavior

`0x220B2A20` takes a node-chain wrapper in original ECX, obtains its pointer at `+0x00`, then follows each node's **`+0x28` pointer** until the last node or null, returning that final pointer. No original cycle guard was observed.

`0x2209D410` first asks for the final node. If that node is non-null, it compares its float/cache **`+0x14` as a raw DWORD against `0xBF800000`** (exact `-1.0f`). On exact equality it calls `0x2209D370` on that terminal node. It then obtains the **first** node from the original wrapper `+0x00`; if non-null, it returns first node float `+0x14` in x87 ST(0). If none, it returns original constant `0.0f` at `0x223D2590`. If the terminal node's cache **does not** contain the exact negative-one sentinel, it trusts the first node's cached number without recomputing, even if the cache is stale. This distinction is independently tested.

`0x2209D370` resets the terminal node's **`+0x14=0.0f`** and advances toward earlier nodes using **`+0x2C` backward pointers**. For each current node having a previous node, it calls original `0x2206D9F0` with the **current node's raw value at `+0x24`** and the pair of current/previous node vector positions (the vector is the first three float members, `+0,+4,+8`). It sums the computed distance with the current node's cached `+0x14` and writes it to the previous node `+0x14`. This accumulates distances backward to the chain's first node, whose cache is read by the getter.

The original segment metric at `0x2206D9F0` uses a numeric input flag, not a confirmed developer-named enum:

- **flag == 0:** `sqrt((Ax-Bx)^2 + (Ay-By)^2)`, ignoring z difference.
- **flag != 0:** `sqrt((Ax-Bx)^2 + (Ay-By)^2 + (Az-Bz)^2)`.

The original uses multiple x87 `fld/fsub/fmul/faddp/fstp` instructions and calls the on-disk sqrt pointer described above. Hence the **algorithmic** relationship is supported, while bitwise parity for all floating-point inputs, rounding and x87 exceptions is not claimed for the host C++17 expression.

### Pseudocode, not an original compiler-compatible object layout

```cpp
float original_path_length(Path* path) {
    Waypoint* terminal = last_forward_node(path->raw_ptr_0x00);
    if (terminal && raw_float_bits(terminal->raw_float_0x14) == 0xBF800000) {
        terminal->raw_float_0x14 = 0.0f;
        for (Waypoint* current = terminal; current->raw_ptr_0x2C; ) {
            Waypoint* previous = current->raw_ptr_0x2C;
            const float segment = original_xy_or_xyz_distance(
                current->raw_word_0x24,
                previous->position_0x00, current->position_0x00);
            previous->raw_float_0x14 = current->raw_float_0x14 + segment;
            current = previous;
        }
    }
    return path->raw_ptr_0x00 ? path->raw_ptr_0x00->raw_float_0x14 : 0.0f;
}
```

This highlights original behavior but makes no claim about original class names, ownership, allocation, templates or constructor. The mathematical float expression is not a full x87 emulator. Crucially, **the flag belongs to the current waypoint**, rather than being a single global route-wide flag.

## Independent C++17 host reference and regression coverage

`include/sin_compat/original_grunt_flank_path_phase566.hpp` implements source-level host-only `Vec3`, `Waypoint`, `Route`, `get_length()` and `flank_bark_if_original_metric_eligible()`. These are independent typed records, **not** the original MSVC 2006 ABI or a memory map of the commercial DLL.

The independent test covers:

- An empty original route returns zero and does not qualify for the flank speech.
- A linked route of exact XY length **240.0** does **not** qualify because the native comparison is strictly greater-than.
- Its 3D variant on the final segment is approximately 268.06 and does qualify.
- Forward traversal `+0x28`, backward accumulation `+0x2C` and cache per node `+0x14`.
- A valid non-sentinel terminal cache does not trigger recomputation, so it may return a stale first-node value.
- One-node recache, safe rejection of malformed cycles/broken links and non-finite host inputs. The last two rejections are **host safety behavior not attributed to the original binary**.

Reproduce from repository root:

```sh
python3 research/tools/audit_sin_original_grunt_flank_path_phase566.py --self-test
python3 research/tools/audit_sin_original_grunt_flank_path_phase566.py --bin-zip /private/bin.zip

g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude \
  tests/test_original_grunt_flank_path_phase566.cpp -o /tmp/grunt_flank_length
/tmp/grunt_flank_length
```

Private original SHA audit **PASS**, 5 whole-body hashes / 259 original bytes, 3 new internal E8 calls and 1 Phase565 crossedge, 24 short instruction witnesses, original 240.0f/0.0f constants and original fsqrt pointer **PASS**. Synthetic five-byte call mutation/wrong-binary rejection **PASS**. Strict GCC + Clang C++17 compile/execution **PASS**. GCC AddressSanitizer and UBSan **PASS**, standalone CTest **2/2 PASS**. Original engine runtime playback, boot, route object ownership and x87 exception fidelity are not verified.

## Next original-code targets

1. Identify original owner class/type at Grunt `+0x8F8`, its member `+0x24`, and constructor/call sites that populate the linked waypoints. This would give the recovered path metric a concrete game AI meaning.
2. Recover remaining original call sites for Grunt vtable `+0x5A8` numeric event 1 and when it dispatches relative to combat schedule `0x2235A560`.
3. Continue original route/AI waypoint population and Grunt schedule registrations, not generic HL2 physics, prop/model loading/rendering or game boot until the proprietary server gameplay logic is materially reconstructed.