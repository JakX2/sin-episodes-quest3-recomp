# Phase 564: original Grunt group pointer virtuals, enemy-key reservation and member-loss speech

**Scope:** original *SiN Episodes: Emergence* 2006 `server.dll` game AI / Grunt group callback chain, continuing authenticated Phases 558–563. This is not a Source engine reimplementation, a native Windows replacement `server.dll`, or Android/Quest 3 launch work. Do not infer class names, member types, or executable ABI details that the original binary has not established.

## Binary identity and new full-function coverage

Original owner-private `SE1/bin.zip` was verified before disassembly or SHA checking. Original binaries remain private and are not committed:

- Exact original 2006 `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Exact original 2006 `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Native `server.dll` PE32 image base `0x22000000`, verified code section `.text`, data `.rdata`.

Ten **complete, previously uncounted native code bodies**, **376 distinct original `.text` bytes**:

| Original VA | Length | Whole-body SHA256 | Source-grounded role |
| --- | ---: | --- | --- |
| `0x22044670` | 13 | `a60a7112693b99216a257966994ffb15a47d318cdc5ac6420364ca2be6f622cd` | Grunt primary vtable `+0x690`: store group pointer to owner `+0x9C4` |
| `0x22044680` | 3 | `01cb47d078b4841b8408ec4fe278efa83115c6f6e101972987507d2b2b57dcf0` | Grunt vtable `+0x694`: Boolean zero result |
| `0x22044840` | 46 | `729aaf252765a75428d0bb7ebb70479acc072ab5de17dd78127326351d480ea2` | Grunt vtable `+0x154`: serial-checked entity handle `+0x8A0` resolver |
| `0x22054C80` | 28 | `417ce57aa7b372fd1d435a4bd9402fe42370c986cf61e990b938a08e927f27f2` | Owner `+0x9CC` signed inclusive selection-range test |
| `0x22054CA0` | 51 | `35eb22aff367f46e13187ef07092381ad94de0f67aeceb809c4e7a549cee001c` | Release old group slot using `+0x154`, then reset `+0x9CC` to `-1` |
| `0x220AAFF0` | 91 | `c727b891b3527e35fae31714120b19d54956f257f018011170469c953caa2e70` | Check and clear an existing reserved group-bitmap bit |
| `0x22353FB0` | 6 | `4b11cfc6b58b896a9e45635b0aecd2a2bd7f23c20643008098b368b8dbcc73a0` | Original constant getter returning `GRUNT_LAST_OF_SQUAD` |
| `0x22353FC0` | 6 | `31ead634b449a8c6cc177f46cf3641d6a22e9e5ccda4e208ce9a2cdc9e596308` | Original constant getter returning `GRUNT_MAN_DOWN` |
| `0x22358590` | 19 | `010287fed98c2f063bf7d10504dfca1c1dd6e1cec5595f1411a5f6b6676ab820` | Grunt virtual `+0x678`: byte `+0xB6` bit0 guard and base-tailcall |
| **`0x2235AF70`** | **113** | `43c5bf4a22da09311edc96258ac037095c155b7d988363b044801962a79a9794` | Grunt virtual `+0x750`: surviving member speech policy |

Original endpoint padding is `INT3` for all ten bodies, and the vtable guard `0x22358590` genuinely **tail-jumps** (`E9 rel32`) to `0x22030520`. The byte directly after its 19-byte body is native padding; treating the tail jump as a code flow into the following function would be incorrect.

The audit additionally authenticates **7 Grunt primary vtable entries** in native `.rdata` beginning at `0x2249AAA4`, **10 original indirect virtual callsite offsets**, **six original direct E8 calls within the ten bodies**, **two older-phase cross-call edges**, **42 instruction/offset witnesses**, two original `.rdata` string tokens and one relative tail jump. Prior body bytes are never counted again.

## Original `+0x690` resolves the group pointer assignment

The linked group join/leave in Phase 562 invokes a still-unknown owner virtual `+0x690`. The authenticated primary `CNPC_MobGrunt` vtable pointer at **`0x2249B134`** (`0x2249AAA4 + 0x690`) is **`0x22044670`**. This 13-byte native method reads its single stack parameter, writes it to raw owner dword **`+0x9C4`**, and returns with `ret 4`.

Thus on this original Grunt vtable:

- Phase562 join native callsite **`0x220AA230`** calls virtual `+0x690` with the new group, **directly storing the Grunt pointer to that group** before the group identity is written to owner `+0x9C8`.
- Phase562 normal leave native callsite **`0x220AA195`** calls virtual `+0x690` with zero, **directly clearing the Grunt group field** before owner `+0x9C8` is separately reset.
- Previous Phase560 owner-specific `0x22054D10` can also explicitly zero `+0x9C4` after full group leave.
- Phase563 **forced group capacity eviction** still uses `0x220A8670`, which by itself performs **no `+0x690` call**, so it must not be modeled as full leave.

The adjacent original Grunt virtual **`+0x694 -> 0x22044680`** returns Boolean `false` via `xor al,al; ret`. In Phase559's original `0x220A9480` filtered group eligibility, a Grunt of this *exact* primary vtable therefore passes the virtual `+0x694` rejection stage; other eligibility conditions (native field `+0x204` and embedded component method `+0x2C`) still apply. This statement does **not** assign this result to every engine entity class.

## Important Phase 560 correction: vtable `+0x154` returns an ENTITY POINTER

The original primary Grunt vtable entry `+0x154` at **`0x2249ABF8`** is **`0x22044840`**, a **46-byte no-stack-argument serial-handle resolver**. It obtains the raw handle at **owner `+0x8A0`**, rejects `0xFFFFFFFF`, splits low 12 bits as the index and high bits as generation/serial, checks the native table at `0x224F5620`, and returns the live object pointer or zero.

This corrects an earlier provisional Phase560 C-like sketch that treated the virtual result as an integer `category` and showed the virtual as consuming condition and index parameters. At **`0x22054D40`**, the actual MSVC x86 call sequence pushes the context pointer `owner+0x9CC` and the numeric selector **twice** *before* the virtual, then invokes `+0x154` with **no stack cleanup** and pushes the returned entity pointer to native group **`0x220AAEB0`**. That group's `ret 0x10` cleans all four pending arguments, and the owner method's own `ret 4` consumes its selector. The source-bounded effective call is:

```cpp
// Corrected readable dataflow; not the original C++ declaration or calling ABI.
bool condition_admission(Grunt* owner, int numeric_selector) {
    if (!owner->raw_ptr_0x9C4) return true;
    auto* entity_key = original_virtual_154(owner); // owner+0x8A0 handle -> pointer
    return original_220AAEB0(owner->raw_ptr_0x9C4,
                             entity_key, numeric_selector,
                             numeric_selector, &owner->raw_dword_0x9CC);
}
```

The group bitmap-cache lookup at `0x220AA920` receives that pointer as a key when called from reservation/release paths. Its precise *conceptual* role (enemy, target, actor identity) should not be asserted without further native evidence. The structural type is now proven to be an entity pointer, **not** a numeric category or condition value. Phase561's refresh loop also calls a member's virtual `+0x154`, consistent with a key per related entity.

## New complete group selection release chain

- **`0x22054C80`** reads raw signed owner index `+0x9CC` and checks it is `>= first` and `<= last`. Its success/failure is Boolean and the two signed bounds are inclusive, without linked-group reads.
- **`0x22054CA0`** first checks owner `+0x9C4` for null. Without a group, it returns **without changing `+0x9CC`**. With a group, it reads current `+0x9CC`, invokes Grunt virtual `+0x154` to resolve the current entity key, passes key and index to native group `0x220AAFF0`, then sets owner `+0x9CC = -1`, even if the old selection was already `-1`.
- **`0x220AAFF0`** exits immediately for selected index `-1`. Otherwise it calls the original bitmap-cache helper `0x220AA920` *twice* for the same entity key. On the first call it tests the old index bit, emits the original diagnostic literal at `0x223EE848` if already unmarked, then on the second bitmap lookup **clears that bit regardless of the diagnostic condition**. Original bitmap index arithmetic is `signed index >> 5` and `(index & 31)` with no host bounds guard. Never reproduce native out-of-bounds writes in a portable reference.

Taken with the Phase560 admission method, this now ties group admission, slot reservation, bit release, and owner-index reset to the *same* entity-keyed bitmap. It does not establish full 30-second cache-compaction semantics beyond Phase561.

## Source-proven Grunt group speech callback: `0x2235AF70`

Phase562 identified a virtual `+0x750` invoked on *surviving* group members when full leave is requested with the **nonzero notification flag**. Native Grunt primary vtable `+0x750` resolves to `0x2235AF70`, whose complete 113-byte body has these conditions and outcomes:

1. If the surviving Grunt's owner byte **`+0xBF & 8`** is set, return immediately.
2. Otherwise, inspect the removed entity's **signed dword `+0x9C`**. If `>0`, return immediately. Do **not** automatically name this field `health` without original type evidence.
3. Call original `0x220A9480(group_at_owner+0x9C4, 1)` to count **eligible linked-group entities** (Phase559), *not* raw member-array length.
4. If the eligible count is **strictly less than 2**, call Grunt virtual **`+0x7B8`**, which is the original six-byte constant getter `0x22353FB0`, returning the **actual game token `GRUNT_LAST_OF_SQUAD`** at original `.rdata` `0x2248CDA8`. Call the previously reconstructed `CAI_Sentence` dispatcher `0x220A35C0` on Grunt `+0xCC0` with **argument `-1`, mode `1`**. Then call owner virtual **`+0x588` with `(0,0)` even if sentence queuing failed**.
5. If the eligible count is **2 or more**, use original virtual **`+0x7BC` -> `0x22353FC0`**, returning actual **`GRUNT_MAN_DOWN`** at original `.rdata` `0x2248CDBC`. Call sentence dispatcher with **argument `0`, mode `2`**. Do **not** call the separate owner `+0x588(0,0)` in this branch.
6. In both branches, original sentence-dispatch return is ignored. **A requested speech line does not prove that audio actually played.** Native preconditions for removed-owner pointer and linked group are assumed by the original, not safety-proven by this standalone method.

Human-readable faithful source sketch:

```cpp
void grunt_survivor_notification(Grunt* grunt, Entity* removed) {
    if (grunt->byte_0xBF & 8) return;
    if (removed->signed_dword_0x9C > 0) return;
    const int n = original_220A9480(grunt->group_0x9C4, /*filtered=*/1);
    if (n < 2) {
        const char* line = grunt->virtual_7B8(); // "GRUNT_LAST_OF_SQUAD"
        original_220A35C0(&grunt->sentence_0xCC0, line, -1, 1);
        grunt->virtual_588(0, 0); // unconditional after request
    } else {
        const char* line = grunt->virtual_7BC(); // "GRUNT_MAN_DOWN"
        original_220A35C0(&grunt->sentence_0xCC0, line, 0, 2);
    }
}
```

The source strings are immutable original sentence identifiers; the full content of speaker response/audio, queue success, and narrator/squad semantics have not been inferred. These authentic names explain an actual game behavior unavailable from the earlier generic group model.

## Grunt join callback and opacity boundary

The original Grunt virtual `+0x678` is `0x22358590`. It checks byte **`owner+0xB6 & 1`**. If true, it returns `AL=0` and removes **12 stack argument bytes**. Otherwise it **tail-jumps** to complete but separately unmodeled engine method `0x22030520` (that larger method is *not* counted as decompiled here). Group joining calls this virtual at original `0x220AA2A4`, and other Grunt behavior may call it. It is a real original short-circuit, not a claim that the base branch is automatically successful.

The independent host C++ reference delegates the generic method and does not attribute engine-side effects to this Grunt-specific wrapper.

## Audits, tests and limits

`research/tools/audit_sin_original_grunt_virtuals_phase564.py` runs only when explicitly provided `--bin-zip /private/bin.zip`. It refuses any client or server DLL SHA mismatch and verifies original method extents, endpoint padding/ret and jump destinations, vtable pointers, six rel32 calls, two earlier original edges, ten virtual-offset sites, 42 field/opcode witnesses and the two full literal strings. Original-free synthetic `--self-test` rejects mutations in **every opcode and rel32 byte** of all recorded direct E8 calls and the tail jump, and rejects foreign binary identities.

`include/sin_compat/original_grunt_virtuals_phase564.hpp` and `tests/test_original_grunt_virtuals_phase564.cpp` implement and test an independent C++17 host model for group pointer assignment, `+0x694` false result, serial-handle resolution with an injected safe resolver, signed-range check, corrected entity-key admission, release/reset ordering including missing-bit diagnostic and repeated bitmap access, the group-join flag gate, and both exact squad-speech paths including unconditional lower-count owner-mark on queue failure. All actual engine callbacks are opaque injected host methods; the original x86 object layout is **never** represented as a native portable struct.

Locally verified against the authenticated private original pair: **10 full methods / 376 distinct native bytes / 7 native Grunt vtable entries / 6 internal E8 edges / 2 previous xrefs / 10 virtual callsites / 42 opcode witnesses / 2 actual phrase strings / 1 original tail jump**. Synthetic audit mutation **PASS**. Strict GCC and Clang C++17 **PASS**; GCC AddressSanitizer and UndefinedBehaviorSanitizer **PASS**; standalone CTest **2/2 PASS**. Hosted GitHub CI, original-engine runtime tests, and branch integration must be reported separately, not conflated with this owner-local binary authentication.

## Next development work

1. Determine exactly when the original NPC calls `0x22054CA0` on target/condition transitions, and who persists the selected index across map/savegame boundaries.
2. Recover the original string-source dispatch and game-specific pronunciation to separate `CAI_Sentence` acceptance from actual playback.
3. Resolve the original Grunt `+0x154` entity-handle lifetime, including interactions with registration and the Phase559 combat fallback.
4. Reconstruct later `0x2235A560` spatial/target decisions and original schedule-ID registration before attempting full native replacement of the Grunt scheduler.

No original copyrighted DLLs, raw disassembly dumps, game assets, executable patching, model/prop code or port boot artifacts are shipped.