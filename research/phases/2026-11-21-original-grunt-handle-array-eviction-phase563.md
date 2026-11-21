# Phase 563: Original SiN 2006 Grunt group fixed-handle array, forced eviction and serial-checked pointer identity

## Scope and original provenance

Continues Phase562's genuine 120-byte group allocation, 16-entry join gate and owner-detach behavior. It resolves three previously opaque collection methods called by the original `0x220AA0D0` leave and `0x220AA1B0` join functions, plus two adjacent related operations. This is decompilation of original SiN-specific group/NPC code, not a reimplementation of generic Source collision, props, rendering, Android or Quest gameplay.

Authenticates the owner's *unchanged* original 2006 retail pair before any method checks:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; PE32 base `0x22000000`.
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Original private DLLs and raw native disassembly **are not included** in the repository or research package.

## Five authentic complete x86 methods

| Native server VA | Exact bytes | Whole-original SHA256 | Authenticated behavior |
| --- | ---: | --- | --- |
| `0x220A8610` | 64 | `4305455c6f88e006d53dda04c538316e784bf0863d3410fb903e0ea297e58479` | Compare resolved stored handle entity pointer with explicit pointer, return *inequality* |
| **`0x220A8670`** | **48** | `ee0e6c8a47b6655583a351927a51809764bf3b057e61d47366fa7b96b2bf32bb` | Delete index from inline handle vector, order-preserving shift and decrement count; **no owner notifications** |
| `0x220A9720` | 93 | `e052f8bb0ad326ec237e6016090f371910004fb3160759bc3952f20f72427190` | Iterate member handles, resolve valid generation and OR supplied bitmask into entity field `+0x988` |
| **`0x220A9790`** | **68** | `157624f5d89aa929c6323f5ca95ae352c09624477dfed4e3ee4f34d4a7ddb90e` | Append one 32-bit handle at prior count, increment count and return original appended index; no capacity or duplication guard |
| **`0x220A97E0`** | **129** | `2d84f0e2a9602983d23ae2b479c8ee197f27eb4ad8fdf1e8117038b2e13231c4` | Find first index with matching **resolved entity pointer**, not matching raw handle bits |

**Five new complete methods, 402 distinct authenticated original server bytes.** Auditor checks two direct calls to the native memory mover `0x223A5D10`, three previously proven Phase562 calls, and 28 field/instruction witnesses. The two exact methods at `0x220A8610` and `0x220A8670` end immediately before their respective next method without INT3 padding, so the auditor uses complete-method SHA, `ret 4`, and actual next entry bytes rather than finding the next `0xCC`.

## The original full-group eviction is NOT a normal leave

At exactly 16 members, Phase562's original `0x220AA1B0` invokes `0x220A8670` for the last element, then proceeds with append. Authentic `0x220A8670` contains only:

1. Read collection's 32-bit count at **array object `+0x40`** (in owning group, `group+0x48`).
2. Compute signed number of elements after requested index as `count - index - 1`.
3. When positive, call the original mover `0x223A5D10` with byte count `4 * remaining`, source `&slots[index+1]`, destination `&slots[index]`.
4. Unconditionally decrement collection count at `+0x40` and return using x86 `ret 4`.

There is **no owner vtable call, owner `+0x9C4` clear, owner `+0x9C8` clear, group-name update, or surviving-member notification** inside this exact original deletion helper. For forced removal of the last member, there is no move because `remaining == 0`, only the count decrement. This supports the precise statement that the *helper itself* does not notify or clear the evicted owner, and the Phase562 join caller calls this helper rather than the full leave path. It does not establish every other side effect elsewhere in the game lifecycle.

The old last slot's bits are not zeroed by removal; original append subsequently writes over that slot when adding the replacement member. This detail is useful when assessing transient stale group references.

## Append behavior at `0x220A9790`

The method receives the embedded fixed-array object, which resides at `group+0x08`. It reads `array+0x40` as prior count, increments it immediately, writes `array+0x44 = array` (original inline-storage marker), copies the caller-provided 32-bit handle into the slot at `array + prior_count*4`, and returns **prior count**. It retains a generic conditional move branch, but its computed number of elements to shift is algebraically zero for this append-at-end call: `(prior_count+1) - prior_count - 1 = 0`.

**There is no native overflow check or duplicate-handle suppression in this method.** It relies on callers such as original Phase562 join to enforce the 16-member path. Therefore, tests deliberately implement a bounded **host-only** overflow refusal rather than attributing it to the original 2006 executable.

## Find behavior at `0x220A97E0`: a subtle null/null equality

The original code first resolves the candidate's encoded handle to an entity pointer using the global 16-byte record table at `0x224F5620`. The handle encoding uses low 12 bits as slot (`handle & 0xFFF`) and the remaining bits as serial (`handle >> 12`), validating the slot's current serial. It then iterates all original handles stored at `group+0x08`, also resolving each to an actual entity pointer, and **compares the resulting pointers**, not the native handle numbers. Returns first matching zero-based slot, or signed `-1` when none matches.

That means two distinct encoded handles can compare equal if they resolve to the same live entity. More surprisingly, when the supplied handle is stale and a group member handle is also stale, both are normalized to null and the original comparison can match **null == null**. The routine does not first reject null candidates. This is an authentic control-flow observation, not a suggestion to expose stale handles in a port. Host regressions explicitly preserve this case, while separately bounding the collection size for memory safety.

## Related methods

- **`0x220A8610`** resolves a stored encoded handle and returns whether that result **differs** from an explicit candidate entity pointer. Invalid/stale handles resolve to null. It returns zero when both resolve to null, another reason not to rewrite original handle queries as integer comparisons.
- **`0x220A9720`** walks the group's original member handle array (`group+0x08`, count at `group+0x48`), resolves valid 12-bit-slot/serial entity identities, and applies a bitwise OR of the caller's mask to each resolved entity's dword **`+0x988`**. Invalid/stale handles are skipped. The original loop does not deduplicate repeated references to the same entity, so its side effect is idempotent for the bits but can touch a duplicate entity multiple times. It does not make arbitrary external engine calls and should not be conflated with the optional survivor-notification virtual `+0x750` from the *full leave* path.

## Original direct xref verification

The previous Phase562 complete-method hashes are not counted twice. The Phase563 auditor independently checks these previously identified callers against exact original `E8 rel32` encodings:

| Prior native caller | Method | Context |
| --- | --- | --- |
| `0x220AA0F8` | `0x220A97E0` | Search member during **full leave** |
| `0x220AA209` | `0x220A8670` | Remove last member at capacity **16** |
| `0x220AA226` | `0x220A9790` | Append joining member's handle |

The two new-method direct `E8` calls are `0x220A8690 -> 0x223A5D10` and `0x220A97B6 -> 0x223A5D10`. The latter exists in the generic append's unreachable-for-normal-input conditional move branch, and must not be erased from source evidence even if not executed on the standard append path.

## Independent host model, tests and safety limits

`include/sin_compat/original_grunt_handle_array_phase563.hpp` is *not* a native x86 structure. It models documented behavior using a bounded 16-entry typed collection, an injected host resolver, and an injected entity flag writer:

- append returning previous count and deliberately **not deduplicating**;
- ordered deletion, no callbacks or clearing of spare slots;
- first resolved-pointer match, including original **null/null** equality and aliasing handles;
- bit-mask broadcast over all successfully resolved handles in order;
- original handle-resolved pointer inequality.

Tests cover the ordinary and deliberately subtle cases, plus host-only capacity and invalid-index guards. Host guards prevent out-of-range writes and are **not** falsely ascribed to the original code. The real engine's virtual callbacks, live serial-table lifetime, and original unexpected memory-failure behavior are outside the reference.

Reproduce on authorized original pair from repo root:

```bash
python3 research/tools/audit_sin_original_grunt_handle_array_phase563.py --self-test
python3 research/tools/audit_sin_original_grunt_handle_array_phase563.py --bin-zip /private/bin.zip

cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build -R 'sin_original_grunt_handle_array_phase563' --output-on-failure
```

Owner-original audit and synthetic mutation tests: **PASS**. Strict GCC/Clang C++17: **PASS**. GCC ASan/UBSan: **PASS**. Standalone 2/2 CTest: **PASS**.

## Continued gameplay priorities

The group implementation is now connected from creation through group-linking, 16-member replacement, order-preserving removal, conditional notification, group condition/admission, and 30-second bitmap refresh. Unresolved: identity and semantics of original group owner's vtable virtual `+0x690`, whether the evicted owner receives asynchronous notifications elsewhere, exact group RTTI/class name, condition-name registry for numeric IDs, and complete Grunt schedule-selector branch translation. None of this is a playable Windows/Android/Quest game DLL.