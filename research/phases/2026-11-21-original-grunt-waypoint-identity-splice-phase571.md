# Phase 571: recover the original matching-ID waypoint splice and constructors

**Scope:** resolve the formerly unsupported identifier-equality case in 2006
`server.dll` method `0x220B2B10`, which is used by the original Grunt route
pipeline (Phases 567–570). Also authenticate four *new* neighboring complete
native methods used to construct and traverse waypoint records. This is
source-grounded SiN gameplay decompilation, **not** engine physics, prop
rendering, Source SDK inference or platform runtime integration.

## Original 2006 binary and precise byte accounting

Checked owner-private original `SE1/bin.zip` before reading internal instructions:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original server.dll preferred PE base `0x22000000`.

| Original native address | Exact extent | Whole-native SHA256 | Role |
| --- | ---: | --- | --- |
| `0x220B29F0` | **26 B** | `9aded3ff164add9dd409b2ee9acd656d9598f55ff0ef903a75f5c31f61c3bc4c` | Follow node `+0x28` to last node, return tail |
| `0x220B2A40` | **26 B** | `921a3f9d537e074580ceb7905ad98a4711f0742875c1c8f2280ffc1cee25517c` | Same tail traversal, starting from holder first |
| `0x220B2A60` | **70 B** | `48efdcc32e024ed3b76d009f2114c365f1e3265acd06218af1f0ee272cf0da94` | Default 48-byte waypoint constructor |
| `0x220B2AB0` | **84 B** | `d6d353f6e1f91498e5f0b95be0bc672eaa2ba2383a685a4f82842ada02aa668c` | Explicit 5-argument 48-byte waypoint constructor |
| `0x220B2B10` | 184 B, **revisited from Phase570** | `572d5112f9b4963c0ff9c6869ecb770f3e47ac4e32befd11f3f17a6ea31f2c94` | Existing verified splice; matched-ID control flow now reconstructed |

**New verified count:** four previously uncounted complete functions, **206
unique original code bytes**. The revisited 184 bytes from Phase570 are **not**
claimed as new bytes. All five whole-body SHA identities are checked. The four
new functions terminate in genuine RET/`ret 0x14`, followed by original
`INT3` boundary padding. The new generator crosscall
`0x220777E8 -> 0x220B2AB0` confirms the explicit constructor is used by
navigation virtual `+0x54` (whose own body is investigated separately).

## Exactly what the old identity-matched branch does

Original function `0x220B2B10(prefix, incoming)` scans the prefix through
`node+0x28` to its *last* node and clears terminal flag bit `0x08` in that
node's `+0x20` flags. It compares the prefix tail's **signed dword `+0x10`**
against sentinel `-1`. Only when it is not `-1` does it read incoming's
`+0x10` and compare identities.

The previously uncertain **equal-identity** native branch is now source-
translated using the exact original instruction order:

1. Save `incoming.next` (`+0x28`) as the successor. When non-null, assign
   `successor.previous = incoming.previous`.
2. When `incoming.previous` exists, set its `next = successor`. The old
   incoming node is now detached from its old neighbors.
3. Return the **incoming node** to the original custom waypoint pool by calling
   `0x2226EF40` using pool `0x2254AFF4`. The prefix tail is **not** released.
4. Set `prefix_tail.next = successor`. The prefix tail was found by walking to
   its previous end, so it had no successor before the operation.
5. If there is a successor, read its *possibly rewritten* previous pointer;
   when non-null, set that old predecessor's `next = nullptr` (this severs
   any residual preceding segment). Then set `successor.previous = prefix_tail`.
6. Find the final node through forward `+0x28` links and OR flag `0x08` into
   its `+0x20`. If no successor, mark **the prefix tail** as final instead.

In the **nonmatching** path (including the sentinel `-1`), the original keeps
`incoming`, detaches its former predecessor when there is one, assigns
`prefix_tail.next = incoming`, writes `incoming.previous = prefix_tail`, then
marks the resulting final node. For the `-1` prefix-tail sentinel, a null
incoming is accepted because the native branch does not read its identity.
For a **nonnegative (or otherwise not -1) prefix-tail identifier and a null
incoming**, the original attempts to dereference `incoming+0x10` and could
crash: the host reference rejects this as `UnsupportedNullNativePath` rather
than emulating the memory fault.

The equal-ID behavior is not a blind `append`: it **deletes exactly one incoming
node** and transfers only its successor onward. A middle-of-chain incoming
node's former predecessor ends with `next=null` after the routine's two-stage
link repair; this can appear redundant in the final state, but both writes are
visible in the original instructions. The callback invoked by the reference
can confirm the original detached-pointer state **at release time**.

### Readable branch sketch (not an original ABI implementation)

```cpp
Point* tail = forward_tail(prefix);
tail->flags &= ~0x08;
Point* attach = incoming;
if (tail->identity != -1 && tail->identity == incoming->identity) {
    attach = incoming->next;
    if (attach) attach->previous = incoming->previous;
    if (incoming->previous) incoming->previous->next = attach;
    original_pool_release(incoming);
}
tail->next = attach;
if (attach) {
    if (attach->previous) attach->previous->next = nullptr;
    attach->previous = tail;
}
(forward_tail(tail))->flags |= 0x08;
```

The native code has additional temporary pointer writes that are redundant
for well-formed *disjoint* forward chains, and it does not prevalidate malformed
input, overlapping lists or cycles. `include/sin_compat/original_grunt_waypoint_identity_phase571.hpp`
models safe valid-chain cases, preflights unsupported aliasing/malformed chains,
and preserves the documented release callback sequence. Its graph refusals
are **port safety**, not original engine behavior.

## Original 48-byte constructors: important last-write behavior

Original `0x220B2A60` and `0x220B2AB0` both first write `-1` to dwords
`+0x18` and `+0x1C`, but **later execute `rep stosd` over all 12 dwords
(48 bytes)**. As a result those two fields are actually **zero at function
return** unless another write occurs, despite their earlier temporary values.
The native order matters more than the superficial first assignment. The
independent C++ model explicitly clears them to zero; initial testing caught
and corrected its original use of a host `Point` default whose own two fields
were initialized to `-1` (not original). Both constructors set cached length
`+0x14` to float `-1.0f` via raw `0xBF800000`.

- `0x220B2A60`: after zeroing, copies three native-global default vector
  coordinates from `0x2251B1EC`, `0x2251B1F0`, `0x2251B1F4` into `+0x00..08`,
  sets `+0x10=-1`, leaves the links at `+0x28/+0x2C` null, and returns `this`.
- `0x220B2AB0`: five stack parameters supply vector pointer, dword `+0x0C`,
  numeric node kind `+0x24`, flags `+0x20`, and identity `+0x10`. The native
  method returns `this` and uses **`ret 0x14`** (callee cleans 20 bytes).
  The original navigation-route generator calls this constructor at
  `0x220777E8`, providing a strong cross-phase linkage to path generation.
- `0x220B29F0` and `0x220B2A40`: two distinct native traversal entries to
  the last forward-linked waypoint; one starts with a raw node, the other
  starts with a holder's first pointer. The body of `0x220B2A40` assumes a
  nonnull first pointer and has a `ret`, not `ret 4`.

## Reproduction

```bash
python3 research/tools/audit_sin_original_grunt_waypoint_identity_phase571.py --self-test
python3 research/tools/audit_sin_original_grunt_waypoint_identity_phase571.py --bin-zip /private/SE1/bin.zip

c++ -std=c++17 -UNDEBUG -Wall -Wextra -Wpedantic -Werror -Iinclude \
 tests/test_original_grunt_waypoint_identity_phase571.cpp -o /tmp/phase571
/tmp/phase571
```

The synthetic self-test verifies both genuine incoming native `E8 rel32` edges
and rejects every mutation of their opcode and displacement bytes. A forged
client/server pair is refused. The original audit checks four new whole-method
SHA256s plus prior Phase570's splice hash, **50 original opcode/branch-field
witnesses**, and the original call edges from the navigation constructor to
`0x220B2AB0` and the splice to the actual node pool release.

Strict GCC and Clang C++17 host tests cover matched head, matched tail, matched
interior, mismatch and old-predecessor severing, prefix chain length two,
sentinel `-1`, null dereference avoidance, alias/cycle rejection, terminal flag
transfer, release callback order and constructor last-writes. AddressSanitizer
and UndefinedBehaviorSanitizer are used to detect reference-only memory issues.

**Limitations:** This is a typed safe host model, not exact MSVC x86 ABI or
bitwise equivalent original memory behavior. Native allocation failure and
malformed input can fault. The original game code, commercial DLLs, full raw
assembly dump and proprietary assets are not redistributed. Source recovery
continues with navigation virtual `+0x54`, whose candidate-generation method
may call the explicit constructor above.