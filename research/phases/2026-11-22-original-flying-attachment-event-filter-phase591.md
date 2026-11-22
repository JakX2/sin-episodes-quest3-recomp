# Gameplay Phase 591: original flying-attachment event filter and Jetpack-to-Grunt fallback

## Exact original artifact and reason for this phase

Phase590 authenticated the original Jetpack Grunt `+0x444` override (`0x22364280`), which forwards default animation events only if the attached flying entity's callback at **`0x2238B3C0`** returns false. Phase591 recovers that callback's **complete original method extent**, two-level event table, return-value partition and direct gameplay side-effect call graph. This is original 2006 **SiN Episodes: Emergence** NPC/gameplay research, not generic HL2 physics/surfaces/model/prop/rendering or Windows/Android/Quest runtime work.

Owner-private retail SHA256 identities:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## One new complete method: **560 distinct original native bytes**

| Original x86 address | Size | Exact whole-method SHA256 | Role |
|---|---:|---|---|
| **`0x2238B3C0–0x2238B5EF`** | **560 bytes** | **`66002e4d22399e1af4767a15152e3f00142a2940009a48c29528c482bf61677f`** | Flying entity event filter and conditional event side effects; signed/native return AL |

The native method includes two embedded tables, which are **original data**, not extra functions:

- Five original DWORD branch targets at **`0x2238B5A8–0x2238B5BB`**, 20 bytes.
- 52 original byte case selectors at **`0x2238B5BC–0x2238B5EF`**, 52 bytes.
- Total 72 embedded table bytes within 560 native method bytes.

Unlike the Phase590 method, the next method **starts immediately at `0x2238B5F0`** with genuine bytes `83 EC 20 8B 44 24`; there is **no `INT3` padding**. The binary auditor authenticates the actual next-method entry rather than imposing a false padding requirement. The whole-body hash includes both original tables.

Phase590's full 1,146-byte Jetpack event override is reauthenticated as an **incoming cross-edge witness** but is not counted as new Phase591 code.

## Source-proven native event range and result

The original callback takes an event pointer, reads its numeric first field, computes **`event_id-9`** and requires unsigned `<=0x33`, so only event IDs **`0x09–0x3C`** can reach its 52-entry switch. Out-of-range events immediately take default exit `0x2238B461` with **`AL=0`**. In-range records comprise exactly **six special IDs** and **46 default IDs**:

| Original animation event ID | Table case | Native branch | Observed terminal return | Scope of side effects |
|---|---:|---|---|---|
| `0x09`, `0x0A` | 0 | `0x2238B48B` | **`true`** | Native owner backreference `+0xC50`, `0x220330C0`, navigation/event movement and fallback signals; host delegates effects |
| `0x28` | 1 | `0x2238B531` | **`true`** | Native `0x22075AB0` and original owner/navigator callbacks; host delegates effects |
| `0x2B` | 2 | `0x2238B46C` | **`true`** | Owner navigation `0x22073E00(7)` followed by common reset; host delegates effects |
| `0x38`, `0x3C` | 3 | `0x2238B3E8` | **`false`** | Still performs possible owner virtual `+0x770` and reset `0x2203D2D0(0)`; false return does **not** mean no side effects |
| All other IDs in range | 4 | `0x2238B461` | **`false`** | No special branch |
| All IDs outside `0x09–0x3C` | bypass table | `0x2238B461` | **`false`** | No table-branch action |

The four true-return IDs reach original `mov al,1` at `0x2238B528` or `0x2238B59C`, after engine-dependent side effects. The two `0x38/0x3C` IDs reach original common zero-return after their branch. Exact native binary methods and E8 edges substantiate these distinctions; the independent C++17 host reference **only** reproduces event partition/known return and delegates engine callbacks explicitly, reporting that their effects are unimplemented.

## Consequence for Grunt `GRUNT_FLANK` and original event `0x54`

A Jetpack Grunt receives outer event `0x54` via its +0x444 virtual `0x22364280` (Phase590). Because `0x54` is **greater than 0x3C**, the attached flying entity's event filter at `0x2238B3C0` returns false without entering its case table. The Phase590 Jetpack fallback then calls original inherited Grunt event handler **`0x2235BFD0`**. This now establishes a **source-proven unconditional forwarding decision for event 0x54 at the flying-attachment filter layer**, not merely the earlier possible forwarding hypothesis.

It does **not** yet establish that every `0x54` produces `GRUNT_FLANK`: Phase580/565 found additional original float-parameter truncation, the Grunt's inner parameter `1` and the cached path-length `>240.0f` gate. The inner inherited Grunt handler and native event source still need full translation to establish that higher-level behavior.

## Authentic verification

Read-only auditor `research/tools/audit_sin_original_flying_event_filter_phase591.py` verifies:

- Both original owner-private DLL SHA256s, the **entire 560-byte original method**, and the earlier entire Phase590 method: **PASS**.
- All five native absolute case pointers, all 52 selector bytes, exactly 6 special / 46 default entries, and correct next-method entry rather than artificial INT3: **PASS**.
- **21 direct native E8 call edges within the new method, two genuine incoming/parent call edges from Phase590 (23 total)**, including `0x2236462C -> 0x2238B3C0` and `0x22364638 -> 0x2235BFD0`: **PASS**.
- **26 independent original instruction/field witnesses**, original `mov al,0` default and `mov al,1` handled exit: **PASS**.
- Original-free synthetic mutation test changes each byte in all 23 authenticated E8 instructions; fake DLL pair refused: **PASS**.

The C++17 host reference `include/sin_compat/original_flying_event_filter_phase591.hpp` models native ID classification, original true/false grouped returns, and nontrivial side effects via explicit callbacks. It returns a host-only safety condition for special cases with unresolved owner backreference. Native crashes on null, full environment/vector behaviors, actual animation sequence dispatch and commercial DLL loading are **not** claimed as reproduced.

Strict GCC and Clang `-Wall -Wextra -Wpedantic -Werror`: **PASS**; GCC ASan/UBSan **PASS**; isolated CMake/CTest **2/2 PASS**.

## Next Grunt-specific boundary

The remaining large inherited Grunt animation-event routine **`0x2235BFD0`** is the next full original method to reconstruct in slices or with a complete SHA-pinned control-flow map. The important priority is the original NPC/Grunt voice and combat behavior, not generic HL2 physics/renderer or the unrelated `prop_dynamic` model/prop lifecycle.