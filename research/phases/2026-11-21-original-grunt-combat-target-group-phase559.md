# Phase 559: authentic 2006 `CNPC_MobGrunt` prioritized combat selector and linked-entity eligibility

## Scope, not a port

Continues the original `CNPC_MobGrunt` virtual `+0x844` combat fallback from Phase 537, and the original `CAI_Sentence` modes 2/3 from Phase 558. Inspected **the private unchanged 2006 SiN Episodes `server.dll`**, not Half-Life 2 code or generic SDK source. No renderer, physics, prop loader, executable replacement DLL or Quest/Android runtime has been implemented.

Whole-original DLL identities, checked before all internal reads:

- `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original server x86 PE32 image base `0x22000000`; original `.text` checked for every body.

## Five independently authenticated original method extents

| Original 2006 VA | Exact body length | Whole-body SHA-256 | What the original control flow establishes |
| --- | ---: | --- | --- |
| `0x2235A560` | **1,327** (`0x52F`) | `99e554b194d86b725559e53c3b1526f7189a974547b745228b475d798e5cb43a` | Grunt combat preselection with short-circuit conditions, entity classes, handle gates, target/turret checks, optional nearby-entity selection, numeric schedule-like return paths |
| `0x22035250` | **46** | `4c90dde319ec23bacbf314d4c6a0691139ad2d9dd44d00e8bb139a3386c621b6` | Resolve Grunt-owned serial-checked entity handle at `+0x98C` |
| `0x220A9480` | **147** | `57e1c9be013f11f99645ffe137ad2d03624606d06202d0af9aed88903014aecc` | Linked collection count, optionally only live/eligible entries |
| `0x220A9210` | **230** | `926d34573d524ff0268466c64c61562c97af5df2e62cf482a032e5bc3e44d59b` | Return first eligible linked entity **only when eligible count >1** |
| `0x220AA2D0` | **78** | `3fb8bc6ec71d6b67bd8ca31893ea01520843b09ba405f3906590038da6c9cc1b` | Check candidate status and whether it equals the selected first eligible linked entity |

**Distinct authenticated original method bytes: 1,828 across five methods.** All boundaries have actual next `INT3` alignment. The 1,327-byte selector ends immediately **before `0x2235AA8F`**, not before the first `0xCC` byte visible in its bytes. At `0x2235A81C` the original x87 instruction `D8 CC` contains a legitimate `0xCC` ModR/M byte at `0x2235A81D`; blindly cutting at that byte would truncate the original function. This pitfall is explicitly tested.

Verification also authenticates **38 original internal E8 callsite/target pairs** (37 in the main selector; one from `0x220AA2D0` to `0x220A9210`), **four previously counted calling-site edges**, **61 opcode/field witnesses**, **five original classname literals**, **four real float/double constants**, and **11 native immediate-return witnesses**. The standard-looking `0x220F5700` character comparator (59-byte code body) is SHA-checked *only as supporting context*, and is **not** added to new gameplay method byte counts.

## 1. The main 1,327-byte Grunt routine is not just a Boolean function

The previously authenticated Grunt fallback at `0x2235D2C0` invokes this routine at exact callsite `0x2235D442 -> 0x2235A560` and then tests `EAX != 0` to suppress that fallback's own later schedule picks. **Internally the callee returns numeric IDs, not normalized true/false.** This does not prove that the parent forwards the numeric ID; the parent uses *truthiness* at the observed callsite.

The exact original return writes include:

| Original return VA(s) | Raw value | Original situation in this function |
| --- | --- | --- |
| `0x2235A632` | `0x70` | early selected-object flag/class branch |
| `0x2235A64D` | `0x66` | original condition `0x45` |
| `0x2235A694` | `0x72` | original condition `0x16` and valid serial-checked `+0xCB4` handle |
| `0x2235A6A2` | `0x27` | same condition and absent/stale/null `+0xCB4` handle |
| `0x2235A76A`, `0x2235A988`, `0x2235AA75` | `0x2A` | three distinct target/movement conditional paths |
| `0x2235A7A7` | `0x65` | target-related manager rejection plus `0x22054D40(...,0x0C)` |
| `0x2235A93F` | `0x29` | later original virtual `+0x79C` gate |
| `0x2235A95B` | `0x74` | candidate-selection action with `this+0xCBC` and `0x22029020` |
| `0x2235A99F` | `0x19` | later branch, accompanied by **write `this+0xC66=1`** |
| `0x2235AA85` | `0` | terminal zero path (`xor eax,eax`) |

These are **native numeric IDs**. Their exact original schedule names must be established from authentic registration before naming them. This report does not transplant Source 2013/Half-Life 2 enum labels onto SiN 2006.

### First verified decision segment (`0x2235A568..0x2235A6AF`)

The source-bounded branch order is:

1. Query shared condition **`0x2B`** using `0x220397C0`. Query condition **`0x0C`** only when `0x2B` was absent. If neither is set, skip the entire selected-object check.
2. If either is set, call `0x22035250(this)` to resolve the original `this+0x98C` entity handle. Null skips the selected-object checks.
3. For an actual object with signed dword `object+0x9C < 1000`, check whether it is `>0` and whether the separate original gate `0x22054D40(this,0x0E)` returns true. If so return **`0x70`**. `object+0x9C` is not assigned an invented type/semantic name.
4. Otherwise obtain `object+0x0C` or original shared empty string fallback, and compare it **byte-for-byte** via original `0x220F5700` against the strings **`func_physbox`**, **`prop_physics`**, **`func_breakable`**, **`func_door`**, in that order. Any exact match returns **`0x70`**. The comparator's own native machine code is case-sensitive, so changing these names to case-insensitive matches would not be faithful.
5. Query shared condition **`0x45`**; if true return **`0x66`**.
6. Query shared condition **`0x16`**; if true resolve the serialized handle `this+0xCB4` (value `0xFFFFFFFF` is the empty sentinel), using native handle table global `0x224F5620`. A valid serial and non-null pointer returns **`0x72`**; otherwise **`0x27`**.
7. When no earlier branch returned, the original code **continues into the remaining target/movement branches**. The independent host model returns a distinctly typed `Continue` status here, not an invented native return value.

The original `0x22035250` handle resolver checks `index = handle & 0xFFF`, `serial = handle >> 12`, the live table's serial at slot `+0x08`, and the object pointer at slot `+0x04`. The *independent* C++ model additionally enforces bounds on the table size. That host-only guard is **not attributed to the 2006 native implementation**.

### Later paths, still dependent on external unknowns

The remaining branches of the 1,327-byte original routine were authenticated **as a complete method**, and the following effects and control-flow checkpoints are recorded, but a complete executable replacement for every invoked engine virtual is not claimed:

- After the initial prefix, Grunt virtual `+0x154` obtains a target-like object. The routine checks target virtual `+0xD0` against **`6`** and compares a name/type reference to **`npc_turret_floor`** at original `0x22461D34` (direct pointer equality, otherwise a separate comparison helper `0x221387F0`). It uses another Grunt virtual `+0x670`, x87 call to `0x2206B5B0`, and engine-global time at `0x22572190+0x0C`. A strictly gated path returns `0x2A`; another target/manager check can return `0x65`.
- Original shared condition **`0x14`** splits two major branches. In its *present* path, an existing target, Grunt flag `+0xDB1`, an x87 **2,500.0f squared-distance** comparison at original `0x223DAF58`, and an enumeration helper at `0x220CDC10` contribute to a search for an acceptable nearby object. The search passes literal `500.0f` (`0x43FA0000`) and a maximum of **10** into an opaque engine helper. It evaluates `candidate+0x62C` bit checks, original casts/filters, x87 comparisons and collision/spatial callbacks; no claim is made that this search is only a generic radius query. If an acceptable candidate is found, it invokes `0x22029020` using member `this+0xCBC` and returns **`0x74`**.
- When no suitable candidate is obtained, original virtual `+0x79C` can return **`0x29`**. Later `+0x78C` and `0x22054D40(...,0x0C)` may select **`0x2A`**. Otherwise this path writes byte **`this+0xC66 = 1`** and returns **`0x19`**.
- The *absent condition `0x14`* branch checks a live target and absence of condition `0x0A`, performs several target/manager queries and x87 comparisons with exact native doubles `4.0`, `3.0` and `256.0`, then may return **`0x2A`** if `+0x78C` and `0x22054D40(...,0x0C)` allow it. Otherwise it returns `0`.

Original virtual method names, engine trace flags, helper `0x220CDC10`, external state invariants, x87 NaN behavior and game-specific meanings of numeric conditions are **still unresolved**. The original floating-point comparisons must not be casually collapsed into reordered C++ comparisons when claiming exact x86 behavior.

## 2. Linked group count, first-selection and membership chain

This is connected to **Phase 558's** sentence policy: its original mode-2 and mode-3 gates call `0x220A9480(group,1)` and require the result to be greater than 1. Mode 3 then passes the owner to `0x220AA2D0(group,owner)`.

### Native layout and gate

All three original routines use collection `this+0x08` as their first serialized-handle entry and signed `this+0x48` as the count. Entries are 4-byte handles. A live entry resolves through global native entity/handle table `0x224F5620` with low 12-bit slot index and high serial bits. It is **eligible** when:

```cpp
// Source-bounded policy, not the original native struct definition.
bool eligible(entity) {
    return entity != nullptr
        && (entity->raw_dword_0x204 != 0
            || embedded_component_virtual_0x2C(entity + 0xFC) != 0)
        && entity_virtual_0x694(entity) == 0;
}
```

**`0x220A9480`** returns raw `this+0x48` if its byte argument is zero. When the argument is nonzero, it scans every handle and returns **the number of eligible entries**. It does not filter/deduplicate the original array in place.

**`0x220A9210`** scans the array, remembers the **first** eligible entity, counts all ineligible entries, and at the end computes `eligible_count = total_count - rejected_count`. The original code executes `cmp eligible_count, 1; setle; sub 1; and first`, which yields:

```cpp
if (eligible_count <= 1) return nullptr;
return first_eligible_entity;  // only if eligible_count > 1
```

This is not the usual “return the sole eligible entity” helper. It requires *multiple* eligible entries and still selects the first. Duplicated handles are not explicitly rejected by these three routines; twice listing the same eligible entity can count as two entries. Whether **other** original code prevents duplicates during list construction is separate and unproven here.

**`0x220AA2D0`** rejects a null/non-eligible candidate, then calls `0x220A9210` on the owning linked group and returns true only if its result equals that candidate. An eligible candidate may therefore fail when only one eligible group entry exists, or when it is the second eligible entity in a group of more than one.

This has direct gameplay relevance to both **Grunt assault target choice** (original Phase 537 `0x220AA2D0` call) and **which Grunt sentence commands are accepted** (original Phase 558 `0x220AA2D0` gate). The linked group's exact Ritual C++ type name is not asserted, nor is the meaning of `entity+0x204` or virtual `+0x694`.

## 3. What the independent C++17 reference models (and does not)

`include/sin_compat/original_grunt_combat_phase559.hpp` provides:

- Safe serial/index handle lookup with a host-only bounds guard.
- The original eligible-entry predicate, counted/group-selected behavior, `0x220A9480` count mode, `0x220A9210` **strictly-more-than-one** gate, and `0x220AA2D0` candidate match.
- The exact four class comparisons and short-circuit **early prefix only** of `0x2235A560`, including original raw numeric results `0x70`, `0x66`, `0x72`, `0x27`.
- A distinct `Continue` sentinel for *unmodeled later original branches*, so callers cannot mistake test code for a full original-schedule implementation.

The reference is **not the retail game's C++ ABI**, not a decompiled binary source dump and not a runnable `server.dll`. It deliberately never supplies guessed semantics for late engine virtuals, path queries, target type meanings or x87 exceptions. The old shared `0x220F5700` byte comparator is SHA-checked only as evidence and is not counted as new gameplay-code coverage.

Independent `tests/test_original_grunt_combat_phase559.cpp` checks stale handles, null and serial mismatch, raw count versus eligible count, first-vs-later eligible entity, strict >1 semantics, duplication behavior, 4 exact classname comparisons, short-circuit condition ordering, raw integer `+0x9C` threshold, `+0xCB4` handle validity and the prefix's `Continue` outcome.

## 4. Reproducibility

**Owner-only original binary audit:**

```bash
python3 research/tools/audit_sin_original_grunt_combat_phase559.py --self-test
python3 research/tools/audit_sin_original_grunt_combat_phase559.py --bin-zip /private/SE1/bin.zip
```

**Independent original-free test:**

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG \
    -Iinclude tests/test_original_grunt_combat_phase559.cpp -o /tmp/sin_grunt_phase559
/tmp/sin_grunt_phase559
```

Owner-local exact-PE32 **PASS**: five full native SHA checks, 1,828 distinct original server bytes, 38 direct original call edges, four older xrefs rechecked without double-counting, 61 instruction witnesses, 5 original class strings, 4 float/double literals and 11 raw return writes. Mutation self-test **PASS** for every byte in all checked rel32 calls and invalid whole-DLL identity rejection. Local strict GCC and Clang **PASS**; ASan/UBSan **PASS**; standalone CTest **2/2 PASS**. GitHub CI is separate synthetic-original-free validation and is not a native game runtime test.

**Remaining original-code priorities:** resolve the Grunt's `0x22054D40` gate `0x0C/0x0E`, find genuine schedule-registration names for raw `0x70/0x66/0x72/0x27/0x65/0x29/0x74`, inspect the registered type and lifetime of the linked collection at owner `+0x9C4`, and reconstruct late x87/nearby-entity branches within `0x2235A560` to a full translation. Avoid spending work on HL2 already-proven rendering, physics, surface or prop pipelines.