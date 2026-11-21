# Phase 558: exact original 2006 Grunt sentence dispatch, mode gate and queued-speech chain

**Scope:** decompilation of the original `server.dll` sentence-command subsystem
invoked by SiN's `CNPC_MobGrunt` combat fallback. This phase does not recreate
HL2 physics, prop/model rendering, engine audio, Android/VR integration, or
claim live game execution.

## Original binary authentication

The source of truth was the owner's original `SE1/bin.zip`, independently
retrieved through the linked project Drive and kept out of this repository.
The pair was checked before examining the original x86 control flow:

- `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Original `server.dll` PE32 preferred image base: `0x22000000`; all functions
  below are in original `.text`, with authentic `0xCC` alignment after the
  recorded method/data extents.

| Original server VA | Exact bytes | Complete method SHA-256 | Source-bounded role |
| --- | ---: | --- | --- |
| `0x220A32C0` | 167 | `acc55a05ec231b12ee39ff24f4446d6b02aa018b31536ff9cafad02858512f74` | conditional sentence diagnostic logger |
| `0x220A3370` | 156 | `514871b1c9c004298f641eb10bbf409693a614326956f83fcbad2a14f9b31427` | queued sentence state/owner callback |
| `0x220A3410` | **220** | `4d4e5ccf3f514e0c8295e5105b054444e2e06c9a31ac9a986b9ee857562c6cc4` | mode 0..3 policy with 4-entry native jump table |
| `0x220A34F0` | 203 | `6688daf8fd51c81d86a498d4d7de9e3e81f5f56034ad8e4aca23b327d35de24b` | queue sentence request and status diagnostics |
| `0x220A35C0` | 131 | `5d6bd2e685efd1123def77e7df7178a84a9acd184c73f8682f5cc0f199408653` | sentence mode/argument dispatcher |

**Totals:** five distinct complete native method extents, **877 new original
server bytes**, 13 independently checked internal direct `E8` CALLs, two
original outside/Grunt-side CALLs, 33 opcode/field witnesses, four native jump
table destinations, and five original diagnostic strings. All method bodies
were checked by whole-body SHA-256 in addition to targeted instruction
witnesses. The 16 bytes of the mode dispatch jump table at `0x220A34DC`
are correctly included in `0x220A3410`'s 220-byte interval. This table is
native data, not four additional machine-code functions.

### Source-linked Grunt owner

Phase 535 independently verifies the embedded sentence member
`CNPC_MobGrunt +0xCC0` as `CAI_Sentence<CNPC_GruntBase>` (through original RTTI).
The previously investigated original Grunt method at `0x2235A470` invokes
**this exact dispatcher** via the direct CALL at `0x2235A52D -> 0x220A35C0`.
The original shared wrapper at `0x220A367F` also calls it. These two xrefs
are checked, but the original caller bytes are **not counted as new methods**.
The method names below are semantic research labels, not recovered original
C++ symbol names or confirmed complete virtual prototypes.

## `0x220A3410`: mode-specific admission gate

The authentic x86 routine takes a mode word, uses `cmp ... 3` plus an
**unsigned** `ja` to admit out-of-range values, and a four-way jump table for
0,1,2,3. Its verified destinations are:

| Mode | Original branch entry | Distinct native behavior |
| --- | --- | --- |
| 0 | `0x220A34D6` | immediately admit, no owner dereference |
| 1 | `0x220A342B` | admit if owner AI state `+0x850` equals 3 **or** bit `0x02` is clear in owner flags `+0xB4` |
| 2 | `0x220A3448` | same owner-state/flag gate, **then** require non-null linked object at owner `+0x9C4` whose `0x220A9480(linked, 1)` result is strictly greater than 1 |
| 3 | `0x220A347F` | same owner-state/flag and linked-object count gates, **then** require `0x220AA2D0(linked, owner)` to return true |
| unsigned >3 | `0x220A34D6` | immediately admit (even a negative signed value is an unsigned out-of-range value in this function) |

For modes 1..3, the owner flag `+0xB4 & 2` is queried only if owner
state `+0x850 != 3`. Modes 0 and >3 do not call the related-object
subsystem. No invented meanings are assigned to the state `3`, the flag
bit, `+0x9C4`, or the `0x220AA2D0` relationship predicate.

C-like recovered branch sequence (original 0x220A3410):

```cpp
bool sentence_gate(OriginalSentence* s, uint32_t mode) {
    if (mode == 0 || mode > 3) return true;
    auto* owner = *(void**)((char*)s + 4);
    if (owner->raw_dword_0x850 != 3 && (owner->raw_dword_0xB4 & 2))
        return false;
    if (mode == 1) return true;
    auto* related = owner->raw_ptr_0x9C4;
    if (!related || original_220A9480(related, 1) <= 1) return false;
    if (mode == 2) return true;
    return original_220AA2D0(related, owner) != 0;
}
```

The above is readable pseudocode, not an executable MSVC object layout.
It is independently exercised by the new host-interface C++17 tests.

## `0x220A35C0`: gate -> invalidate -> queue -> postflag

Native byte/edge verification establishes the following order:

1. Call the preceding gate `0x220A3410` using the **third** stack argument
   (mode). If rejected, return signed `-1` and do **not** clear `this+0x0C`.
2. On admission set sentence object dword `+0x0C` to `0xFFFFFFFF`.
3. Read the **second** stack argument. If it is `-1`, directly call
   `0x220A34F0` to attempt a sentence queue request; return its value.
4. Otherwise call owner virtual byte `+0x584` with that argument. When
   false, emit the native diagnostic string **`CULL`** through `0x220A32C0`
   and return `-1`, without queueing or setting the owner flag.
5. When that virtual allows the argument, call `0x220A34F0`, then call
   owner virtual `+0x588` with the same argument and literal mask
   `0x40000000`. Return the queue method's original result, **including if
   the queue failed**. Owner update is therefore not conditional on success.

Source-like reconstruction:

```cpp
int sentence_dispatch(OriginalSentence* s,
                      const char* sentence,
                      int argument,
                      unsigned mode) {
    if (!sentence_gate(s, mode)) return -1;
    s->raw_dword_0x0C = -1;
    if (argument == -1) return sentence_queue_220A34F0(s, sentence);
    auto* owner = s->raw_owner_0x04;
    if (!owner->virtual_584(argument)) {
        sentence_log_220A32C0(s, "CULL", sentence);
        return -1;
    }
    const int id = sentence_queue_220A34F0(s, sentence);
    owner->virtual_588(argument, 0x40000000u);
    return id;
}
```

The first parameter is a native C string that may undergo further engine
processing in the separate queue method. This pseudocode is not an assertion
about the original `char*` lifetime, ownership, or engine request object.

## Other three complete methods

- **`0x220A34F0`, enqueue path:** copies an input NUL-terminated string
  into a **128-byte native stack region**, without any evident length bound
  in the original byte-copy loop. It optionally invokes owner virtual
  `+0x5B4` to transform the string, obtains owner linkage from `+0x16C`,
  calls native helper `0x221B3840` to create/queue a request, and returns
  the resulting request ID or `-1`. It sends the genuine diagnostics
  **`Speaking`** on success and **`BOGUS`** on failure, routed through
  `0x220A32C0`. The original stack copy has possible overflow behavior for
  overlong text; **do not replicate that unsafely** in an independent port.
- **`0x220A3370`, pending queue lifecycle:** checks this object's
  queued index at `+0x0C` for `-1`, performs an original x87 comparison of
  float `+0x10` with engine-global float `+0x0C` at `0x22572190`, and
  branches through original status-word parity tests. One path tests
  owner virtual `+0x584` using the stored argument at `+0x14`, invokes
  native delivery `0x221B37C0`, resolves engine virtual `+0x80`, logs
  **`Speaking [from QUEUE]`**, and calls owner virtual `+0x588` with a zero
  flag. Other paths either clear the queued ID or return without clearing.
  No simplified C++ floating comparison is claimed to reproduce the
  original x87 NaN/invalid exception behavior.
- **`0x220A32C0`, diagnostics:** selects native behavior using a global
  debug selector at `0x2254AC70` offset `+0x2C`; cases 0/1/2 have distinct
  outcomes, with case 1 going through imported call and case 2 through
  `0x220381B0` and additional engine linkage. Its original format string is
  **`SENTENCE [%d %.2f] %s: %s\n`**. This is a diagnostic helper, not
  a claim that the sentence command succeeded or that audio played.

The unverified engine buffer allocation, actual speech selection, audio
playback, native virtual signatures, and original object types are kept
outside the host reference instead of being filled with Half-Life 2 guesses.

## Independent C++17 host reference

`include/sin_compat/original_grunt_sentence_phase558.hpp` defines a
non-ABI `ISentenceHost` interface, the four-way mode gate and the
`0x220A35C0` command order. `tests/test_original_grunt_sentence_phase558.cpp`
checks state-3 short-circuit, flag rejection, related-object metric 1 vs 2,
relation predicate, unsigned out-of-range acceptance, rejected command's
state preservation, reset-before-queue, default vs non-default argument
paths, cull, and postqueue owner update **even when the queue returns -1**.
Original engine functions are not invoked. The independent host's
`queue_sentence` callback abstracts the separate original native
`0x220A34F0` work, including its unsafe fixed-buffer copy.

### Reproduce

```bash
python3 research/tools/audit_sin_original_grunt_sentence_phase558.py --self-test
python3 research/tools/audit_sin_original_grunt_sentence_phase558.py --bin-zip /private/SE1/bin.zip

g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude \
    tests/test_original_grunt_sentence_phase558.cpp -o /tmp/sin_sentence_test
/tmp/sin_sentence_test
```

Owner-local private SHA audit **PASS**: 5 complete methods / 877 original bytes
/ 13 direct internal calls / 2 external original call sites / 33 opcode
witnesses / original jump table / 5 literals. Original-free Python mutation
self-test **PASS**; strict GCC and Clang C++17 selftests **PASS**.
Hosted CI must *not* load or distribute the game binaries. Original-game
runtime proof has **not** been performed.

## Connected next research

The most valuable subsequent step is to identify the Grunt sentence
parameter keys and registered sentence IDs that flow into this command
pipeline, including what the original `0x220A9480` and `0x220AA2D0`
related-object gates mean. Preserve native logic/unknowns instead of
replacing them with Source SDK defaults. Separately, the original Grunt
schedule fallback continues to depend on `0x2235A560`, which is a much
larger control-flow routine and should be decompiled independently, rather
than guessing from its single Boolean use in the fallback.