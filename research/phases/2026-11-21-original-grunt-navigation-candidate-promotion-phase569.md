# Phase 569: original Grunt navigation candidate-to-active route promotion

**Scope:** decompile the genuine 2006 `server.dll` route acceptance procedure reached after Phase567 navigation construction and Phase568 waypoint installation, not re-create Half-Life 2 or Source SDK pathfinding. The original code names, exact virtual prototypes, object lifetime rules, and run-time game behavior remain unproven where noted.

## Private original-binary authentication

All authentic original x86 bytes come from the owner's original read-only `SE1/bin.zip`, with both complete DLLs SHA-checked before any per-function analysis. No original commercial DLL, raw disassembly, map, model or executable patch is distributed.

- `server.dll` complete SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` complete SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Original 2006 `server.dll` PE32 preferred image base: **`0x22000000`**.

| Original virtual address | Complete method bytes | Authenticated whole-method SHA256 | Original bounded function role |
| --- | ---: | --- | --- |
| `0x22075480` | 81 | `fe29569ef963232204929ba6b34afe9231defe600c49a45d0f46411646d11551` | candidate holder reset + conditional NPC callback and timestamp |
| **`0x220754E0`** | **139** | `c656b3404e31a18bcc6c7a0565f3a2cc3df9feb973a23e0b152a567517ad79fa` | conditional candidate refresh, **promote candidate to active route**, clear candidate and change route state |
| `0x220B2CF0` | 22 | `f80c12d876d67ed678b264d92cc0e901eef9e35d31ea209343761fcb365aca36` | delegate node cleanup then clear route holder pointer |
| `0x2209D000` | 50 | `a7b13eb20aa9fe59fadbd580a02812bad11ef485c1e1eda746bf3fc68d84f497` | mark final waypoint and copy its absolute position into the route endpoint |
| `0x2209D040` | 37 | `deecc5f623a17f579381567e40e29782cf627c5965e8aac8817475ffadceb0ce` | set route endpoint to **route origin + input vector** |
| `0x2209CEF0` | 17 | `6a09a9fdf9bc245627b133c7ecaea0c46dfcd352d5fca99ff8140de8bbef12a3` | route raw `+0x24=argument`, raw `+0x28=-1` |
| `0x22074770` | 43 | `ce6bd4dd2a17df571f25e9a805616f6aaaff6ace7557dbbbe03bcc3ce5e4cb06` | resolve navigator `+0x10` original 12-bit slot/serial handle into entity pointer |

**Totals:** seven separate complete original methods, **389 distinct native server code bytes**, 9 directly authenticated `E8 rel32` calls within those functions, 3 previously uncovered original caller cross-edges, **40 targeted instruction/field witnesses**, all complete SHA256 method hashes, and every original method's terminal RET and subsequent `INT3` padding. Original method full-hash verification checks far more than the individual witnesses.

The previous Phase568 commits and Phase569 are deliberately separate: Phase568 authenticates the per-route first pointer, advance and state setter; Phase569 authenticates the **owner/navigation object that calls them** to accept a candidate. The research labels are not recovered original source identifiers.

## Original navigation pointer hierarchy and fields

This component is the exact object stored at original NPC/Grunt `+0x8F8` by Phase567's initialization. In this phase its native fields are:

| Native component offset | Original use established here |
| --- | --- |
| `+0x10` | serialized handle, resolved through original `0x22074770` |
| `+0x24` | pointer to the **active route object** (route's `+0x00` = current waypoint) |
| `+0x28` | pointer to a **separate candidate route-holder object** (its `+0x00` = candidate first waypoint) |
| `+0x2C` | raw float cache/deadline, reset to `-1.0f`, sometimes set to engine time |
| `+0x30` | optional dword written into active route `+0x08` after successful promotion |
| `+0x34` | optional dword passed through `0x2209CEF0` to active route raw `+0x24` |

Do not confuse the **component's candidate holder at `+0x28`**, the route object's `+0x28` metadata, and **a waypoint's `+0x28` forward-link**. They belong to three separate native objects with different layouts.

## `0x22075480`: candidate clear and optional refresh

1. Store the sentinel **raw `0xBF800000` (`-1.0f`)** into navigation component `+0x2C`.
2. Call original `0x220B2CF0` on the separate holder at component `+0x28`. That helper first invokes the real original node-cleanup procedure `0x220B2C80`, then clears the holder pointer.
3. If the active route's first waypoint is null, return, leaving the candidate empty and deadline invalid.
4. If the active first waypoint's dword `+0x24` equals **1 or 3**, or a distinct original global selector's dword `+0x2C` is nonzero, invoke navigation virtual `+0x54`, passing the candidate holder. Its exact purpose is unresolved; it can be thought of as a *candidate-generating callback* for analysis, not a recovered original method name.
5. Only if that virtual returns true, copy the original engine time (`global 0x22572190 +0x0C`) into navigation `+0x2C`. The original instruction sequence does not itself prove that the callback populated the candidate route.

The `+0x24` dword in step four is *waypoint type/state*, not automatically the XY/XYZ distance mode. Phase566/568 showed that the same offset selects a numeric XY/XYZ branch in a distance helper; other consumers may interpret 1/3 distinctly. Do not assign a developer symbolic enum without the original definition.

## `0x220754E0`: the actual candidate-to-active transition

This is the central finding. Its exact control-flow structure is:

```cpp
// Bounded C-like pseudocode; original 2006 object ABI and virtuals remain opaque.
bool original_promote_candidate(NavigationObject* nav) {
    if (nav->candidate_holder && !nav->candidate_holder->first)
        original_22075480(nav);       // resets candidate, may invoke vtable+0x54

    if (!nav->candidate_holder || !nav->candidate_holder->first)
        return false;

    OriginalRoute* active = nav->active_route;
    if (active->first) {
        original_220B2CF0(active);  // original native node cleanup + first=null
        active->raw_dword_0x2C = -1;
    }
    original_2209D160(active, nav->candidate_holder->first, true);
    original_220B2A10(nav->candidate_holder, nullptr);
    original_2209D0C0(active, 0);
    original_2209D0C0(active, 4);
    if (nav->raw_dword_0x30 != 0)
        active->raw_dword_0x08 = nav->raw_dword_0x30;
    if (nav->raw_dword_0x34 != 0)
        original_2209CEF0(active, nav->raw_dword_0x34);
    return true;
}
```

The native `0x2209D160` setter marks the candidate's last waypoint with flag `8`, invalidates the distance cache, and derives an endpoint vector; the separate candidate holder is **cleared without freeing its moved node chain**. Native `0x220B2CF0` is called on the **previous active** chain only when that active first pointer was non-null, and runs original node deletion. Replacing the node pointer in a later port without this ownership transfer would produce leaks, double-frees or stale pointers.

The two state calls **0 then 4** must not be folded into a single `4` write. Phase568 proved `0x2209D0C0` bases its engine-time capture and transient marker on the *old* route state, so `old->0->4` is observably different from `old->4` even when the final state dword matches. The second call always sees old state zero and therefore clears route byte `+0x40` again, leaving a timestamp only if the pre-promotion state was nonzero.

The optional component `+0x30/+0x34` fields are only consumed when nonzero. These numbers are not assigned speculative schedule or nav enum names.

## Endpoint and handle helpers

The original has **two distinct endpoint-setting methods**:

- `0x2209D000`: walk the current route to the **final waypoint**, copy its raw XYZ position directly into route `+0x34..+0x3C`, set byte `+0x30=1`, set final waypoint flag bit 3 (`0x08`). No route origin is added in this method.
- `0x2209D040`: take a separate external vector argument, set route byte `+0x30=1`, and set each endpoint coordinate to **route base vector `+0x14..+0x1C` plus that argument vector**. It does not walk waypoint links and does not by itself set waypoint flags.

The original `0x2209CEF0` stores its argument at route raw `+0x24`, writes `-1` to route `+0x28`, returns with native stack pop of 4. The original `0x22074770` reads an encoded handle from **component `+0x10`**, checks `-1` first, extracts a 12-bit slot index and 20-bit serial, compares against the original global entity registry's generation word, and returns the resolved entity pointer or null. Exact registry bounds/lifetime are unverified, so the host model adds an explicit slot bounds guard rather than risking native memory reads.

## Reproducibility and regression gates

- Authenticated owner-original audit: `python3 research/tools/audit_sin_original_grunt_route_promotion_phase569.py --bin-zip /private/SE1/bin.zip` **PASS** with both original whole-file SHA pins, seven full method SHA256s, 389 code bytes, nine original `E8` calls, three incoming callsites, and 40 targeted original opcode witnesses.
- Original-free byte-mutation test: `--self-test` **PASS**, rejects each five-byte call opcode/rel32 modification and unrelated binary.
- Original-free C++17 host model: `include/sin_compat/original_grunt_route_promotion_phase569.hpp`, consumes Phase568 safe route model to preserve linked waypoint behavior and two-step native state setter ordering.
- `tests/test_original_grunt_route_promotion_phase569.cpp` exercises absence/refusal of candidate, original kind 1/3 versus global selector callback gate, optional callback timestamp, existing active-route cleanup order, candidate ownership transfer, original endpoint distinction, optional route dwords, serial handle generation comparison and host-only malformed graph safety.
- Strict GCC and Clang C++17 compilation with warnings treated as errors: **PASS**; GCC ASan/UBSan: **PASS**; standalone CMake/CTest **2/2 PASS**.

The C++ reference is **not** a binary-compatible 2006 navigation class or a proof of live Grunt pathfinding. It intentionally avoids invoking unknown native virtuals, native pointer dereferences, x87 edge cases and the original `0x220B2C80` game allocator. The original DLLs and their full disassembly remain private.

## Following original decompilation targets

1. Reconstruct `0x220B2C80` and waypoint pool ownership/chain frees to establish original path lifecycle without guessing allocator policy.
2. Identify navigation virtual `+0x54`, the source and meaning of the candidate selector values 1 and 3, and the path-building logic it invokes.
3. Follow the original Grunt flank event-1 source back into actual schedule/condition transitions and later candidate route installation.
4. Keep source-first; do not divert into generic Half-Life 2 materials, props, physics, model rendering or Android/VR boot until native gameplay behavior is recovered.