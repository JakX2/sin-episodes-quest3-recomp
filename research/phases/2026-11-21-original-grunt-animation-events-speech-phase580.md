# Gameplay Phase 580: original Grunt animation-event routing and voice policy

**Scope:** the original retail 2006 SiN Episodes: Emergence `server.dll` and its `CNPC_MobGrunt` voice-specific code. This is a **gameplay-source** continuation of PR #579 (Phase576 gameplay), not a continuation of the separate Windows export/factory research PRs #576–#578. The branch label **Phase580** avoids numerical collision. No generic HL2 physics/surface/model/prop/rendering work, native runtime boot or Android/Quest work.

## Authenticated original binaries

Owner-private `SE1/bin.zip`:

- Original `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, preferred original PE32 base `0x22000000`.
- Original `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

The read-only auditor **refuses all other whole-image identities before inspecting any internals**. No commercial binary, raw x86 disassembly dump, game assets or executable patch is checked into GitHub. Original native VAs below are **preferred base addresses**, not pointers to valid memory in the host process.

## Nine *new* complete original Grunt methods, 307 distinct server bytes

| Native x86 VA | Complete bytes | SHA256 of original method body | Proven role |
|---|---:|---|---|
| `0x223588B0` | 149 | `3569c4d343b38984df800a8688a594749fa8239707b1a4fbeee739e2b0f4b785` | Lost-enemy short/long sentence choice and randomized cooldown |
| `0x22358950` | 29 | `301bdcf5d10c75091894883514cacd3dc2b47948a3669dcbf81ed60f2996879a` | Refind-enemy sentence request |
| `0x22358970` | 70 | `87da0a39820aa483ac0ef3dc81187b4cf7eb325aec47b2793097c6d4f0df9c2a` | Go-alert sentence and strict 10-second cooldown |
| `0x223589C0` | 29 | `5356fbf7af4e628566bf219932f46dd065da293e8f4506d0bfab6a5e55db1f47` | Announce sentence request |
| `0x22353F90` | 6 | `885aa912ed01cc73213d84315e8265bed2dfc71f0f02b9786b4aa9f216601c0a` | `GRUNT_REFIND_ENEMY` literal getter |
| `0x22353FA0` | 6 | `38079625b3de1259cd5d03edeacb3fc01698138beb46a84b7dd83497d9f6e71d` | `GRUNT_GO_ALERT` literal getter |
| `0x22353FF0` | 6 | `a6e8f9905c54751b5be528e9ed71a144060a2dbc8a4b177678c41fe893b2d111` | `GRUNT_ANNOUNCE` literal getter |
| `0x223541B0` | 6 | `2e69f1f5a67aca0826124ef50a6f25986f91e27d9e966861935d110045e48943` | `GRUNT_LOST_SHORT` literal getter |
| `0x223541C0` | 6 | `e7820746ecc3651c2eb523618d38b3431743a5137be091470bd5eb07f7ae6192` | `GRUNT_LOST_LONG` literal getter |

All nine end in native `ret` and are followed by authentic `INT3` padding. These nine **new** methods do not overlap the complete Grunt reaction and speech bodies already counted in Phases 564–565. The shared NPC animation-event dispatcher at `0x22050150`, the Grunt's already-counted `0x2235AE30`/`0x2235AE90`/`0x2235AFF0`, and incoming xrefs are authenticated as **supporting callsite/table witnesses only**; none are counted as new method bytes. Total genuinely newly covered original machine code: **307 bytes**.

## Original animation event ID table from NPC dispatcher `0x22050150`

Original method prologue loads the current event record from its stack argument and original numeric event ID from its first 32-bit field. It computes **`index = event_id - 1`**, checks **unsigned** `index <= 0x89` (138-entry first switch), then dispatches through the original `.text` table at **`0x22052D14 + 4*index`**.

Five adjacent complete, independently verified table pointers:

| Original event code | Exact jump table slot | Table target inside original dispatcher | Original indirect virtual dispatch | Exact Grunt override |
|---|---|---|---|---|
| `0x50` / 80 | `0x22052E50` | `0x2205256A` | `+0x594` | `0x223589C0` `GRUNT_ANNOUNCE` |
| `0x51` / 81 | `0x22052E54` | `0x2205254C` | `+0x590` | `0x22358970` `GRUNT_GO_ALERT` |
| `0x52` / 82 | `0x22052E58` | `0x22052579` | `+0x598` | `0x2235AE90` `GRUNT_PAIN` / `GRUNT_TAUNT` (Phase565) |
| `0x53` / 83 | `0x22052E5C` | `0x2205255B` | `+0x58C` | `0x2235AFF0` `GRUNT_DIE` / `GRUNT_MUTANTDEATH` (Phase565) |
| `0x54` / 84 | `0x22052E60` | `0x2205259D` | `+0x5A8` | `0x2235AE30` custom parameter `0`/`1` (Phase565) |

**Crucial separation:** outer animation event **`0x54`** is **not** equivalent to Grunt-custom event **`1`**. Its dispatcher block loads *the float event-parameter at event record +4*, saves the x87 control word, ORs **0x0C into its high byte** to select temporary truncation rounding, performs x87 `fistp` into a 32-bit integer, restores the original x87 control word, and pushes the resulting integer into Grunt virtual `+0x5A8`. That previously authenticated Grunt function then dispatches:

- Integer custom parameter **0** -> original Grunt assault-sentence helper `0x2235A3A0`.
- Integer custom parameter **1** -> original navigation path length `0x2209D410` and **strict `>240.0f`** condition for `GRUNT_FLANK` (Phases565–567).
- Other integers -> no action in this **Grunt** override.

Finite values such as 0.99 truncate to 0, 1.99 to 1, and -1.99 to -1. On invalid x87 conversion, signed overflow or exceptional float payloads, the independent C++ model returns `NeedsNativeX87Conversion` rather than presenting ordinary host `std::trunc` as guaranteed native parity. **No claim that every raw animation data file contains these events**, or what original SDK names correspond to event IDs, follows from a dispatch table alone.

## Original `GRUNT_GO_ALERT`: vtable `+0x590`

`0x22358970` (70 bytes) loads original engine float time at `*(0x22572190)+0x0C`, compares with owner Grunt float field **`+0xC7C`**, and calls the `GRUNT_GO_ALERT` getter only when **ordered strictly later**. Equality, earlier and x87 unordered results skip the entire request. The native sentence invocation uses original Grunt's `+0xCC0` `CAI_Sentence` (`0x220A35C0`) with **argument `2`, mode `2`**.

After the invocation, **without checking its return status**, the routine stores `owner+0xC7C = current_engine_time + 10.0f`. The `10.0f` literal is independently verified at original `.rdata` `0x223D8F14`. The timer is updated even if queuing a sentence fails. This differs from lost-enemy logic below.

`0x223589C0` (29 bytes), Grunt vtable `+0x594`, requests **`GRUNT_ANNOUNCE`** with **argument `0`, mode `2`** and returns without a local cooldown gate.

## Original `GRUNT_REFIND_ENEMY`: vtable `+0x5A4`

`0x22358950` (29 bytes) requests the exact native **`GRUNT_REFIND_ENEMY`** literal from Grunt vtable `+0x7B0`, invoking original sentence policy (`0x220A35C0`) with **argument `2`, mode `2`**, no local cooldown. The authentic external virtual caller **`0x2203056C`** follows a positive original `0x2206B600` predicate and a nonnull original vtable `+0x670` result. These xrefs establish *an actual Grunt-capable call pathway*, not a supposed universal enemy reacquisition event.

Additional unrelated-event uses of Grunt `+0x594` exist (e.g. original call `0x22045798`); the original animation dispatcher is **not** the only legal caller.

## Original `GRUNT_LOST_SHORT` / `GRUNT_LOST_LONG`: vtable `+0x5A0`

`0x223588B0` (149 bytes), called through original shared method virtual invocation **`0x2203ED50`**:

1. Compare original engine time with Grunt float **`+0xC88`**. Its native x87 sequence is `test ah,0x41; jnp` (rather than `jne`). This allows **both ordered greater** (`AH&0x41 == 0`) **and unordered** (`AH&0x41 == 0x41`) to proceed, while exact equality and ordered less-than block. This nonintuitive unordered behavior is demonstrated by raw status-bit tests; no blanket finite-only claim for this branch.
2. Resolve target-like entity via original Grunt virtual `+0x154` (Phase564 identifies its generation-checked owner handle).
3. **If there is a target**, call native `0x220317A0(this)` and compute `current_engine_time - returned_time`. When this is **strictly greater than 10.0f** (native x87 comparison, AH&0x41==0), select `GRUNT_LOST_LONG` via Grunt virtual **`+0x83C`**; otherwise select `GRUNT_LOST_SHORT` via **`+0x838`**. **Without a resolved target**, select `GRUNT_LOST_LONG` without this time-delta query. The original last-observation timestamp's precise semantic name is not independently authenticated.
4. Invoke original `CAI_Sentence +0xCC0` with **argument `0`, mode `2`**. **If the returned integer is negative, exit without a new cooldown.**
5. On nonnegative queue status, call native engine-global random interface `*(0x225721B0)` virtual **`+0x4`** with the immediate float arguments **5.0f and 15.0f**. Store **`owner+0xC88 = (newly read original engine time) + random_float`**.

This differs from the go-alert behavior, which updates its timer irrespective of a failed queue. It also differs from a claim that a sentence actually finished playing. Exact full game `CAI_Sentence` arbitration was studied separately in Phase558.

## Verified cross-call edges, virtuals, original strings

- Five native direct calls in newly recovered methods: `0x223588E0 -> 0x220317A0`; `0x22358916`, `0x22358966`, `0x2235899B`, `0x223589D6` all -> original sentence `0x220A35C0`.
- **Twelve** exact primary original Grunt vtable pointer slots, including the five original voice getter slots; seven of them voice-handler slots `+0x58C,+0x590,+0x594,+0x598,+0x5A0,+0x5A4,+0x5A8`.
- **Fourteen** native `call [vtable+N]` witnesses, including five dispatch-body calls, existing original `0x2203056C`, `0x2203ED50`, `0x22045798`, `0x22351C5F`, and five intra-handler phrase getters.
- **Forty** independent original instruction/field witnesses in newly recovered and adjoining original dispatch blocks; all **five null-terminated tokens** verified against original `.rdata`. The real full DLL digests and whole-function byte hashes are the authoritative provenance.

## Host reference, tests and limitations

`include/sin_compat/original_grunt_events_phase580.hpp` uses an original-free typed host boundary with injected sentence queue/random callbacks. It has:

- A clear `DispatchOriginalEvent(0x50..0x54)` model with **delegation, not reimplementation**, for previously recovered Phase565 custom voice functions and Grunt event parameters 0/1.
- `GoAlert` with original **strictly later** deadline and update-even-if-sentence-fails semantics, finite host inputs.
- `LostAttemptAllowedByX87` and `ElapsedStrictlyGreaterThanTenByX87` accepting original raw AH flags to model x87 **parity** rather than ordinary C++ comparisons; sentence success and bounded injected random delay.
- Explicit host-only nonfinite/invalid random safety checks. Those checks do **not** describe the original native crash or exceptional-float behavior.
- Five native phrase tokens and mode/argument preservation; no native ABI object construction, source-level animation system replacement, runtime sound playback, or claim of engine-equivalent x87 status production.

`tests/test_original_grunt_events_phase580.cpp` checks exact event IDs and delegation, x87 float-to-integer truncate vs rounding, negative and out-of-range payload handling, strict go-alert equality boundary, update on failed alert request, lost target/no-target paths, 10-second status decisions, native unordered parity, failed lost request not starting cooldown, and random **5.0–15.0** argument range.

Reproduction:

```bash
python3 research/tools/audit_sin_original_grunt_events_phase580.py --self-test
python3 research/tools/audit_sin_original_grunt_events_phase580.py --bin-zip /private/SE1/bin.zip
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Owner-private exact DLL audit: **PASS** (9 complete authentic original methods, 307 unique code bytes, 5 native direct calls, 14 indirect calls, 5 jump-table events, 12 Grunt vtable entries, 40 native opcode witnesses and 5 original strings). Synthetic mutation self-test: **PASS**, rejects all five-byte original E8 changes and fake DLL identity. Strict GCC+Clang host reference, GCC AddressSanitizer+UBSan, and standalone CTest **2/2: PASS**. GitHub CI is a separate verification and will be reported only after GitHub provides its own outcomes.

### Open authentic gameplay questions

- The original developer-assigned animation event **symbolic names** for `0x50–0x54` are not proven, even though the numeric mapping and Grunt overrides are.
- Actual game animation files that emit event `0x54` with float parameter `1.0` and their associated activity/schedule remain unidentified. A valid dispatch branch is not evidence that an animation uses it in every release asset.
- Original runtime x87 exceptional conversion, sentence queue arbitration and engine-random distribution beyond the observed `5.0f,15.0f` arguments need runtime-specific verification.
- Numeric combat schedule IDs from Phase559 and their original registration remain unresolved; this phase explains event routing, not a full Grunt combat AI port.