# Phase 560: Original SiN 2006 Grunt linked-group admission, reservation and member iteration

## Evidence and scope

This phase connects Phase 559's `CNPC_MobGrunt` combat decision calls to the **original 2006 `server.dll`** group admission machinery. It also follows its adjacent group-ownership, identity and member-enumeration routines. It is a source-level recovery artifact, not a replacement Source game DLL, native Source runtime, sound engine, physics or VR port.

The owner-provided private `SE1/bin.zip` was independently opened and both full PE32 files were authenticated **before** studying these functions:

- `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, native image base `0x22000000`.
- `client.dll` SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

All below are exact original `.text` whole-function code extents, not invented names or borrowed Half-Life 2 logic.

| Original 2006 `server.dll` VA | Length | Complete method SHA-256 | Source-bounded role |
| --- | ---: | --- | --- |
| `0x22054CE0` | 47 | `ceffd4b720ba13adab1a5823d0be9085b058823227fb28b08de84d58dab1710e` | Lookup/fallback creation of group from registry `0x2254ADEC`, then owner attachment |
| `0x22054D10` | 33 | `db4185026c3fe22e4efb3bca4b8acc40858ce3cccf0161f874b1a8042ba2baea` | Release old linked group using `0x220AA0D0`, clear owner `+0x9C4` |
| **`0x22054D40`** | **64** | `cecfc5c20393a5acd51002f6d48d3dc4e055b1203bfa2efd7b44b4702287e8e5` | Group-controlled condition admission through Grunt virtual `+0x154` and `0x220AAEB0` |
| `0x22054D80` | 36 | `0ed1c827632ec634707dffe209a14580b7273305e34b124fbfb4910c9d007e12` | Optional owner `+0x9D0` identity versus third-parameter entity `+0x3AC` |
| `0x22054DB0` | 159 | `b8ef159948975464611f16dce765e38bc6740b60e66e274ba9e9aaa9225760a8` | Iterate eligible group entities and optionally dispatch per-candidate action |
| `0x22054E50` | 167 | `ecbcae173102f611af54359d90a772aaec9d0ada33544aab50c16ef978487b43` | Compare one converted key to owner value or count matching eligible group members |
| **`0x220AAEB0`** | **240** | `04b8c968551b4093eadfea1bb9768b0391e250547f8f13a1cd31e11eabb47ca0` | Inclusive integer-range admission, bitmap reservation, prior-index release and selected-index update |
| `0x220A9300` | 188 | `f01ab41f5a8a83dd597f93dbe89a0ce82317eeaaf12bfd8e746e49e982aae1ac` | Get first group member; optionally filter ineligible entities |
| `0x220A93C0` | 190 | `d9fd368ad8953b75accd68a184dac514a86243ea5b7ffea529f3e33d4d737a6a` | Increment cursor and get next member, with same optional eligibility filter |

**9 independently authenticated original native methods, 1,124 distinct code bytes.** Auditor also validates 20 original direct `E8 rel32` call edges, five previously authenticated Grunt caller edges, 36 short opcode/field witnesses, and two adjacent-function boundaries without padding. Neither the full original binary bytes nor raw disassembly is checked into GitHub.

### Boundary caveat: adjacent native functions

The full 64-byte method at `0x22054D40` has an exact native `ret 4` ending just before method `0x22054D80`, **without intermediate `INT3` padding**. Likewise the 240-byte method `0x220AAEB0` ends in `ret 16` immediately before the method at `0x220AAFA0`. The auditor uses whole-function SHA256, exact terminal return bytes and next-function entry witnesses rather than wrongly requiring alignment `0xCC` for these two bodies. For the remaining seven functions, immediate `0xCC` padding is verified.

## 1. What `CNPC_MobGrunt` condition helper `0x22054D40` does

The original checks Grunt owner `+0x9C4` for a null linked-group pointer. If absent, **it immediately returns `EAX=1`**. When present, it places the selector twice and the context address `owner+0x9CC` on the stack, then calls Grunt virtual `+0x154`. Phase564 authenticates that virtual as `0x22044840`, a **no-argument handle-to-entity-pointer resolver** reading owner `+0x8A0`. The resulting **entity pointer** is then passed as a fourth argument to original linked-group method `0x220AAEB0`, whose `ret 16` consumes the entire pending four-argument stack sequence. A zero `AL` from that method produces `EAX=0`; otherwise original `EAX=1`.

In source-bounded pseudocode (virtual prototype and direct stack-argument cleanup require more ABI analysis):

```cpp
bool Grunt_condition_22054D40(Grunt* grunt, int condition) {
    if (!grunt->raw_ptr_0x9C4) return true;
    auto* entity_key = original_grunt_virtual_0x154(grunt); // owner handle +0x8A0
    bool allowed = original_group_range_admission_220AAEB0(
        grunt->raw_ptr_0x9C4, entity_key, condition, condition,
        &grunt->raw_dword_0x9CC); // native writes persist on the owner
    return allowed;
}
```

**Phase564 correction:** the original virtual returns a serial-checked **entity pointer**, not an integer bitmap category. This is a source-style sketch of the observed native values; method names remain editorial, not recovered declarations. It does prove the group gate can reject an otherwise valid NPC condition. The following original Grunt callsites are cross-checked (without counting their bytes again): `0x2235A5B0`, `0x2235A760`, `0x2235A79D`, `0x2235A97C`, `0x2235AA6B`, each targeting `0x22054D40`. The previously authenticated caller arguments include `0x0E` and `0x0C`, but their exact original human-readable condition names remain unresolved.

## 2. Original `0x220AAEB0`: the group gate is a reservation algorithm

A separate complete 240-byte native method explains what the above call actually controls. Native method arguments include the returned entity pointer as an opaque **bitmap cache key**, a **signed inclusive index range**, and a pointer to a previously selected index. `0x220AA920` retrieves a bitmap for that entity key repeatedly. Its time-dependent cache handling is outside this phase and deliberately not simulated.

The actual successful/failed paths are:

1. Read the current selection through the supplied pointer.
2. If it lies in the inclusive requested range `[begin, end]`, **return true immediately**, without checking that the selection bit is marked.
3. If `begin > end`, reject.
4. Otherwise scan ascending indices, using the bitset returned from `0x220AA920`. **The first zero bit** is an available slot. A completely occupied range returns false.
5. If an old selection was not the `-1` sentinel, read its original bit; issue an original diagnostic when unexpectedly unmarked; then clear the old bit.
6. Set the new selection's bit, write the index through the caller's pointer (`owner+0x9CC` on the Grunt path), and return true.

A rough readable approximation (the native `0x220AA920` callback/cache, signed-overflow behavior, and error diagnostics are not implemented in this sketch):

```cpp
bool ReserveGroupRange(Group* group, Entity* entity_key,
                       int begin, int end, int* chosen) {
    if (*chosen >= begin && *chosen <= end) return true;
    for (int i = begin; i <= end; ++i) {
        if (!group->bitset_for_entity(entity_key).test(i)) {
            if (*chosen != -1) {
                if (!group->bitset_for_entity(entity_key).test(*chosen))
                    original_diagnostic();
                group->bitset_for_entity(entity_key).clear(*chosen);
            }
            group->bitset_for_entity(entity_key).set(i);
            *chosen = i;
            return true;
        }
    }
    return false;
}
```

**Newly clarified gameplay implication:** the raw `0x0C`/`0x0E` Grunt conditions are not simply static checked flags; the linked group may reserve access to a shared condition slot. A Grunt can therefore change admission results as other group members occupy/free bitmap slots. The exact mapping from index/bitmap to combat task vocabulary still requires reverse engineering, and this is not evidence of a specific line of dialogue or animation.

## 3. Linked-group membership and owner state

- **`0x22054CE0`:** calls native registry lookup `0x220A8050` with global `0x2254ADEC` and its single input. On a falsey result invokes `0x220AA810`; in either case dispatches native `0x220AA1B0` with the original Grunt owner. Semantic description: lookup-or-initialize followed by owner attachment. No claim that this creates a unique new C++ group object on every lookup miss.
- **`0x22054D10`:** if owner `+0x9C4` is non-null, calls original `0x220AA0D0` with `owner` and zero, then writes zero to `owner+0x9C4`. If already null, returns without touching the group.
- **`0x22054D80`:** when owner `+0x9D0` is non-null, returns false if the third stack argument is null or if its `+0x3AC` pointer differs from the owner field. Otherwise returns true. The exact semantic type of `+0x9D0` and `+0x3AC` is unresolved. It is a *pointer identity* comparison, not a general string comparison.
- **`0x22054DB0`:** if owner `+0x9C4` null, returns false. Otherwise iterates with `0x220A9300/0x220A93C0` in filtered mode. A candidate must have non-null `+0x94` and differ from the owner pointer. For candidates with bit `0x800` at `+0xB8`, invokes `0x2213FA60` before calling `0x22033F10` with original arguments including candidate origin at `+0x2BC`. A successful callback stops iteration and returns true; otherwise visits further members and eventually returns false. Source-level role of `0x22033F10` remains unresolved, so the C++ model only exposes a callback with matching order.
- **`0x22054E50`:** first calls helper `0x221F3E20` to convert its input to an opaque scalar. Without linked group, looks up owner-side object via `0x222AA640`, checks its `+0x0C` scalar for equality, and returns 0/1. With a group, iterates filtered members and **counts** all matches of their objects' `+0x0C` field, returning a non-Boolean total. The owner-side helper gets called *twice* on a matchable path, preserved as an unresolved side-effect caveat.

## 4. Verified filtered collection iteration: `0x220A9300` and `0x220A93C0`

These are not generic C++ container iterators; they operate on the original group object and use a 32-bit handle array at group `+0x08`, with count `+0x48`. Both return a native pointer or null while writing/updating an integer index through the passed cursor pointer.

With the **filter byte nonzero**, for each entry in array order:

1. Reject serialized handle `0xFFFFFFFF`.
2. Validate generation against global handle table `0x224F5620`, using handle low 12 bits as slot and the remaining high bits as generation.
3. Reject a null resolved entity pointer.
4. If entity `+0x204` is zero, require its embedded component at `+0xFC` virtual `+0x2C` to succeed.
5. Require the entity virtual `+0x694` to return **false**. A true return means skip it.
6. Return the eligible entity pointer and write the cursor index. The `next` function first increments the cursor.

With filter zero, the state/virtual checks are skipped, and the function resolves the raw handle at the current index only; it can return null rather than seek forward if that raw handle is invalid. The independent host tests make that distinction explicit. Invalid pointers and serials are supplied as synthetic host facts; the C++ model does not dereference original game memory.

## 5. Independent reference model and test boundary

`include/sin_compat/original_grunt_group_phase560.hpp` provides:

- A group-aware condition gate preserving immediate true on missing group and admission through the host-provided original `+0x154` and group method.
- An explicit **independent** bitmap-range allocator demonstrating first-free selection, inclusive boundaries, old-index release, and repeated admissibility.
- Filtered first/next entity iteration and counted matching scalars.
- Owner identity guard, group lookup/attach/detach sequence and member action iteration.

The host model adds checks against negative/huge range indices, represents live entities as typed synthetic data, and does not replicate `0x220AA920` cache/timer or the native `0x22054DB0` action's vector/argument-passing ABI. This is not a claim of exact MSVC C++ compatibility or real Windows module execution.

Synthetic tests cover no-group allowance, rejection, callback order, reservation reuse, no free slots, prior selection release, invalid old-bit warnings, independent categories, owner/candidate pointer match, group lookup/create/detach, filtered/unfiltered serial handle iteration, virtual eligibility gates, early action success, and metric count versus owner fallback.

### Reproduce

```bash
python3 research/tools/audit_sin_original_grunt_group_phase560.py --self-test
python3 research/tools/audit_sin_original_grunt_group_phase560.py --bin-zip /path/to/private/bin.zip

c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude \
  tests/test_original_grunt_group_phase560.cpp -o /tmp/sin_grunt_group
/tmp/sin_grunt_group
```

**Owner-original PE32 audit:** PASS, both original DLL SHA-256s, 9 unique complete native methods, 1,124 distinct original server bytes, 20 call edges, 5 prior Grunt cross-edges, 36 instruction/offset witnesses, two no-padding method boundaries. Python mutation self-test: PASS. GCC/Clang strict C++17: PASS. GCC ASan/UBSan: PASS. CMake/CTest: **2/2 PASS**. All hosted CI tests require only authored, noncommercial synthetic fixtures.

## Remaining decompilation targets

1. **`0x220AA920`** is the group bitmap accessor with time-dependent state/cache refresh. Its real class members and cache invalidation must be studied before treating the reference reservation bitmap as a substitute for original behavior.
2. Identify original registration names for the raw Grunt schedule IDs in Phase 559 instead of guessing shared Source SDK enum values.
3. Recover shared condition registry/meaning of `0x0C` and `0x0E` and native Grunt `+0x154` method signature.
4. Trace group class RTTI and constructor/destructor, including the true ownership of `+0x9C4` and member-array lifetime.
5. Independently translate the late target, x87 and nearby-entity branches of the 1,327-byte `0x2235A560` selector.

**This phase adds original gameplay-code facts, not a complete playable `server.dll`, Windows boot, Quest 3 or Android result.**