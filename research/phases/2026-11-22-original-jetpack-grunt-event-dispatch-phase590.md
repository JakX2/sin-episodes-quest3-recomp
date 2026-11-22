# Gameplay Phase 590: original Jetpack Grunt animation-event override

## Scope and SHA provenance

This phase follows the owner-private 2006 **SiN Episodes: Emergence** `server.dll` Jetpack Grunt startup/attachment research (Phases 588–589) into the **Jetpack-specific virtual animation-event handler `+0x444`**. It does not implement generic Half-Life 2 physics, surface parameters, models/props, rendering, Windows factory/boot, Android or Quest 3 integration.

SHA-256 of authentic original retail pair:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## One newly whole-body authenticated native method, 1,146 bytes

| Native address | Entire native size | Original complete SHA256 | Role |
|---|---:|---|---|
| **`0x22364280–0x223646F9`** | **1,146 bytes** | **`2e155aff9b5440527fa4eeed36fff5ae527957b5106a88c711e18724e951b9a6`** | Jetpack-specific NPC event override, executable bodies plus original two-level switch tables |

The method contains **178 bytes of original embedded dispatch data** that should not be disassembled as x86 instructions:

- Nine native DWORD case-target pointers at **`0x22364648–0x2236466B`** (36 bytes).
- **142 one-byte case indices** at **`0x2236466C–0x223646F9`** (142 bytes).
- The following original byte **`0x223646FA`** is `INT3` padding, not another table entry.

This is one native function, not nine different functions, and the total 1,146-byte extent **includes** the 178 dispatch-table bytes. The Phase589 original 248-byte Jetpack attachment method is SHA-authenticated as a cross-phase dependency only; it is **not** included as newly recovered bytes here.

## Native four-class virtual ownership from original MSVC RTTI

These four distinct primary vtables all contain **`0x22364280` at byte slot `+0x444`** and Phase588's **`0x22364700` at slot `+0x338`**. RTTI class names are authenticated from the MSVC Complete Object Locator at `vtable[-4]`:

| Native RTTI class name | Primary vtable |
|---|---|
| `CNPC_DruglabJetpack` | `0x2248C514` |
| `CNPC_EliteJetpack` | `0x2248E83C` |
| `CNPC_JetpackGruntBase` | `0x2249964C` |
| `CNPC_SintekJetpack` | `0x224A4A4C` |

The normal non-jetpack `CNPC_GruntBase` and `CNPC_MobGrunt` primary vtables instead map `+0x444` to original `0x2235BFD0`; `CNPC_EliteGrunt` maps to another Grunt-specific method `0x22355480`. This independently confirms that the nine special IDs below belong to the original Jetpack subclass override, not a generic Source NPC event switch.

## Original event table and exact case branches

The original computes unsigned **`event_id - 0x19`** and bounds it against `0x8D`, allowing original IDs **`0x19–0xA6`** inclusive. Any ID outside this range jumps directly to its Jetpack fallback. **Exactly nine IDs** inside the table have dedicated Jetpack cases, whereas **133 other in-range IDs** use default case 8.

| Original event ID | Native case | Native case address | Verified effects or unresolved boundary |
|---|---:|---|---|
| `0x19` | 0 | `0x22364505` | Tests original `0x22363870`, conditionally calls owner virtual `+0x47C(1)` or executes two alternative navigation-route paths. Model deliberately marks native navigation body unresolved |
| `0x38`, `0x3C` | 1 | `0x223642AF` | Checks original target virtual `+0x154`, uses owner `+0x1024` through serial resolver `0x22363D50` and attached virtual `+0x770`, then affects navigator. Exact target/control contract unresolved |
| **`0x9F`** | **2** | **`0x22364438`** | **Write owner raw dword `+0xFEC=0`, then call original `0x2203D2D0(0)`** |
| **`0xA0`** | **3** | **`0x22364470`** | **Write owner raw dword `+0xFEC=2`, then call same original reset `0x2203D2D0(0)`** |
| **`0xA3`** | **4** | **`0x223644A7`** | Optional debug output conditioned by original diagnostic-global flag; normal path performs no gameplay field write |
| **`0xA4`** | **5** | **`0x223644D0`** | **Call original `0x220325C0` with immediate float `0.05f` (`0x3D4CCCCD`) and integer `0`, discard x87 return** |
| **`0xA5`** | **6** | **`0x22364403`** | Call native Jetpack helper `0x22363510`, then call `0x2203D2D0(0)` |
| `0xA6` | 7 | `0x22364311` | Distinct Jetpack target/navigation branch. Authenticated as part of whole function but model remains deliberately opaque |
| all other in-range, plus out-of-range | 8 | `0x223645FD` | Resolve owner handle `+0x1024`, call original entity/event predicate `0x2238B3C0`; **only on predicate false** forward to original Grunt handler `0x2235BFD0` |

The explicit native `0x9F` and `0xA0` writes to `+0xFEC` are source-authenticated, but the exact original named enum is unknown. Event `0xA4`'s literal is exactly original little-endian IEEE float `0.05f`; its callback's physical significance is unproven. Original runtime exceptions, unusual x87 float conditions, original activity names of the event IDs, animation-resource sources and actual engine callback implementation remain unresolved.

The general Grunt `0x54` animation event previously researched in gameplay Phases 580–581 **is not a Jetpack-specific index** in this override. It goes through case 8, including the native attachment-aware filter that might consume it before inherited Grunt dispatch. It is therefore **incorrect to claim every Jetpack outer event `0x54` automatically emits `GRUNT_FLANK`**. The outer event parameter still needs the separate Phase580/565 inner value `1` and native route-distance condition.

## Original method SHA audit and independently authored host model

`research/tools/audit_sin_original_jetpack_events_phase590.py` validates:

- Untouched original `server.dll`/`client.dll` whole SHA256 pair, full **1,146-byte original method digest** and authentic next `INT3`: **PASS**.
- All **26 original direct E8 rel32 calls**, including the attached-handle resolver, route helpers, all reset/timed-action calls, the filter `0x2238B3C0` and parent handler `0x2235BFD0`: **PASS**.
- All **nine original absolute case-target pointers**, **142 exact byte case indices**, nine special IDs and exactly 133 default indices: **PASS**.
- All four Jetpack RTTI class identities and corresponding `+0x444` / `+0x338` original vtable entries: **PASS**.
- **20 independently pinned original opcode/field witnesses**, original 0.05f immediate: **PASS**.
- Original-free mutation selftest corrupting each byte of each native E8 call and unsupported retail hashes: **PASS**.

Authored host C++17 reference `include/sin_compat/original_jetpack_events_phase590.hpp` preserves the entire original **event-id dispatch partition**, models source-confirmed case `0x9F`, `0xA0`, `0xA3`, `0xA4` and `0xA5` effects and callback order, and explicitly returns `RequiresOriginalJetpackBranch` for `0x19`, `0x38`, `0x3C`, `0xA6`. The original fallback uses abstract native `0x2238B3C0` and `0x2235BFD0` callbacks in original order. This is **not** claimed equivalent to complete native engine/AI event processing.

Tests: strict GCC and Clang C++17 `-Wall -Wextra -Wpedantic -Werror` **PASS**, GCC AddressSanitizer/UBSan **PASS**, standalone CTest **2/2 PASS**. No commercial binaries, raw disassembly dump, proprietary assets, native executable or full replacement DLL are distributed.

## Highest-value next source question

Trace original Jetpack fallback predicate **`0x2238B3C0`** and common Grunt event override **`0x2235BFD0`**, to establish when outer event `0x54` is consumed and when it can reach the previously authenticated assault/flank speech route. This is Grunt-specific AI control flow; do not substitute generic HL2 animation/physics code.