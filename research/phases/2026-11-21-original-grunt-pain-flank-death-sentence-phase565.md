# Phase 565: original Grunt flank, pain/taunt and death-sentence selectors

This is a decompilation-first continuation of the owner's original SiN Episodes: Emergence 2006 Grunt gameplay AI and voice pipeline. Phase564 authenticated original group-member loss announcements, and Phase558 authenticated the underlying `CAI_Sentence` mode and queue behavior. Phase565 follows three additional Grunt-specific virtual methods to the **real** original sentence tokens, state transitions and timing gates. No engine-generic physics/surface/models/props, Windows DLL boot, Android, or Quest VR work is included.

## Exact original binary and full function evidence

The original owner-private pair is SHA verified and kept out of this repository:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `server.dll` PE32 preferred image base `0x22000000`; source instructions verified in `.text` and full string literals in `.rdata`.

**Nine complete previously uncounted original x86 methods, 432 distinct original server bytes:**

| Original native VA | Bytes | Exact whole-method SHA-256 | Verified role |
| --- | ---: | --- | --- |
| `0x2235AE30` | **85** | `d4cf7a90c31cc3f6d7fb9412e5570cf975bbf130c9b70967bc4a7dcce32a68a1` | Grunt virtual `+0x5A8`, two event codes and flank sentence gate |
| `0x2235AE90` | **220** | `5256cbba36900560183a9203d0ac492a3f737abb3d028459defc187b6e07b9f8` | Grunt virtual `+0x598`, timed pain/taunt voice selector and flags |
| `0x2235AFF0` | **91** | `ec1585edc3137e2a5ff13d66729306b9ea340f3fa0fa37d7ddc5086187ac254c` | Grunt virtual `+0x58C`, `GRUNT_DIE` vs `GRUNT_MUTANTDEATH` |
| `0x22353FD0` | 6 | `b73c01b91523a4ea5ed232948f7e33d737622673f95edce26156ae6a8ccf1746` | Virtual `+0x7C0`: string `GRUNT_DIE` |
| `0x22353FE0` | 6 | `b25fb3090b44d9579a39a0c558fbe2206af2244a5bb5984340699cf0620a1cca` | Virtual `+0x7C4`: string `GRUNT_MUTANTDEATH` |
| `0x223540C0` | 6 | `abed7bd812cacd4b23cfe219e3ee543c4c14c30f80720d74bb1e04141d4d27cb` | Virtual `+0x7FC`: string `GRUNT_FLANK` |
| `0x22354170` | 6 | `e17d4c73112e6e6c8380d680b8c95c41e72022f80014f2cd6b27c3faf168ae83` | Virtual `+0x828`: string `GRUNT_PAIN` |
| `0x22354180` | 6 | `021b4ee4ca707a049660bc9e8ae29fc8bf11d6b852a522705aa529af78228896` | Virtual `+0x82C`: string `GRUNT_TAUNT` |
| `0x22354190` | 6 | `e17d4c73112e6e6c8380d680b8c95c41e72022f80014f2cd6b27c3faf168ae83` | Virtual `+0x830`: **also** `GRUNT_PAIN` |

Every code extent is complete and is followed by authentic `0xCC` padding, so neither a repeated 6-byte getter nor a partial function is double-counted. The six string getters are six distinct function addresses, including two *byte-identical* implementations of `GRUNT_PAIN` at different VAs. Their `.rdata` string pointers and full null-terminated strings are authenticated; those five tokens are **actual original game resources**, not Source SDK guesses.

Primary original `CNPC_MobGrunt` vtable begins `0x2249AAA4`, and the auditor verifies all nine slot-to-function associations (three decision methods, six getter methods), all six native indirect getter calls within the new methods, six original E8 rel32 direct calls, 40 opcode/field witnesses, and four float/double constants from original `.rdata`.

## Grunt virtual `+0x5A8` -> original `0x2235AE30`: event 0, event 1

The method branches on its one 32-bit numeric event argument:

- On **event `0`**, call the previously SHA-authenticated original Grunt assault-sentence helper `0x2235A3A0` (Phase537). This is an intentional side effect; it is not a numeric schedule return.
- On **event `1`**, obtain an x87 scalar via original `0x2209D410` using a pointer from the Grunt's `+0x8F8` object, member `+0x24`. Compare the returned scalar with **original `240.0f`** at native `0x223F8EFC`. A **strictly greater-than** finite result selects original Grunt virtual **`+0x7FC -> "GRUNT_FLANK"`**, and invokes original `0x220A35C0` on embedded sentence at **Grunt `+0xCC0`** with **argument `0`, mode `2`**. Below/equal threshold there is no sentence request.
- Other event numbers have no further action in this function.

The metric's source method and its physical units are not recovered here. Do not label it a distance, speed, or timer without evidence. Native x87 invalid/NaN behavior cannot be reconstructed faithfully just by a normal `float > 240` host expression, so invalid-float test inputs remain outside the modeled equivalence claim.

## Grunt virtual `+0x598` -> original `0x2235AE90`: pain / taunt

The original method is a 220-byte, source-bounded timed Grunt voice-selector that calls `0x220A35C0` with **argument `-1`, mode `0`**. It executes these steps in its native branch order:

1. If owner raw dword **`+0xBC & 0x10000000`**, return immediately.
2. Compare original engine float time at `*(0x22572190)+0x0C` with owner float deadline **`+0xC78`**. Only the **strictly later/ordered** path continues; equals or earlier return.
3. If owner `+0xBC & 0x08000000`, return immediately.
4. Initially select original virtual **`+0x828 -> GRUNT_PAIN`**.
5. Compute an original signed-integer ratio using **`fild [owner+0x9C]`** followed by **`fidiv [owner+0x98]`**, leaving the fraction in x87. These dwords are kept as raw `+0x9C` and `+0x98` pending type proof; the shape is *consistent* with a current/maximum ratio but the source names are not recovered.
6. If the high bit of owner field **`+0x988`** is not already set, compare the ratio with original **double `0.9`** at `0x223E9138`. A strictly higher ordinary finite value writes **`+0x988 |= 0x80000000`** and selects original virtual **`+0x82C -> GRUNT_TAUNT`**.
7. Otherwise, unless **`+0x988 & 0x40000000`** was already set, compare with original **double `0.25`** at `0x223DA478`. An ordinary finite ratio strictly below `0.25` writes **`+0x988 |= 0x40000000`** and selects original virtual **`+0x830 -> GRUNT_PAIN`**. Note the string **does not change** from the initial default when this flag is set; that side effect is still original and must be preserved.
8. Request the chosen sentence using the original `CAI_Sentence` member at `+0xCC0`, **argument `-1`, mode `0`**. Ignore native queue result. Then update owner deadline **`+0xC78 = original engine time + 1.0f`**, original literal at `0x223D22F8`, regardless of whether queueing succeeded.

This is a *requested announcement*, not proof that a sound actually plays. Original integer-division-by-zero/x87 exception behavior and nonfinite payloads are not modeled by the independent host C++17 reference, which returns a distinct `UnmodeledFloat` outcome for invalid cases. Floating-point rounding and comparisons in an eventual native-ABI port must be reconciled against retail x87 status words.

## Grunt virtual `+0x58C` -> original `0x2235AFF0`: death phrase selector

The authentic 91-byte method has three gates evaluated in this exact order:

1. If owner byte **`+0xBF & 0x10`** is set, return.
2. If owner byte **`+0xCB1 == 0`**, return.
3. If owner byte **`+0xC44 != 0`**, return.

If admitted, compare signed owner dword **`+0xC68`** to exactly `10` (`0x0A`):

- If *not equal*, call virtual **`+0x7C0 -> GRUNT_DIE`** and dispatch with **argument `-1`, mode `0`**.
- If *equal*, call virtual **`+0x7C4 -> GRUNT_MUTANTDEATH`** with the same sentence arguments.

The code does not name what raw owner `+0xC68` represents, so any suggestion that `10` is definitively the mutation enum is inference; the **original string** does unambiguously contain `MUTANTDEATH`. The routine does not implement death animation, removal or health damage; it only dispatches a sentence request.

## Source-oriented portable host model and test boundaries

- `include/sin_compat/original_grunt_reactions_phase565.hpp` uses an injected sentence/assault callback and a clearly separate host-only state, **not** original engine object memory offsets. It implements event selector, eligible finite pain/taunt gate with state writes/time, and death token selector.
- `tests/test_original_grunt_reactions_phase565.cpp` checks the event 0/1 split and strict 240 threshold; original `0.9` and `0.25` state transitions with exact flag writes; the original **1.0-second strictly-later** deadline; native repeated `GRUNT_PAIN` literal behind two getters; all three death gates and `+0xC68 == 10` branch; expected arguments and sentence modes. Boundary/NaN and zero divisor cases are explicitly unmodeled.
- `research/tools/audit_sin_original_grunt_reactions_phase565.py` checks both entire original DLL SHA256 identities, nine complete method hashes, nine primary Grunt vtable slot pointers, six original direct calls, six virtual calls, 40 instruction witnesses, all five native tokens, and four original float/double constants. Synthetic self-test refuses foreign binaries and all five-byte mutations to each original E8 instruction's opcode/rel32.

Owner-local original SHA audit: **PASS**. Python synthetic mutation test: **PASS**. Independent C++17 strict GCC/Clang: **PASS**. GCC AddressSanitizer/UndefinedBehaviorSanitizer: **PASS**. Standalone CTest: **2/2 PASS**. GitHub hosted workflows and original game runtime are separate evidence, not implied.

## Research next steps

1. Find original callsites for Grunt virtual `+0x598`, `+0x58C`, and `+0x5A8` to identify which original combat events trigger these functions. Do not infer event names from virtual offsets alone.
2. Follow native `0x2209D410` to establish the true source and unit of the event-1 metric.
3. Relate group-loss speech (Phase564) to pain/taunt and death routines through original event priorities and `CAI_Sentence` policy (Phase558), without claiming simultaneous playback or exact runtime arbitration.
4. Continue full recovery of the spatial target-choice branches in original 1,327-byte Grunt combat selector (Phase559) and original schedule-ID registration.

The original private commercial DLLs, machine-code dumps, map/model assets and executable patches are not redistributed. No complete decompiled client/server DLL or playable PC/Android/Quest title is claimed.