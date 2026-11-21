# Phase 570: original 2006 waypoint pool, chain disposal and identity-aware splice

**Scope:** recover genuine SiN Episodes: Emergence `server.dll` C++-compiled x86 functions underlying route candidate promotion (Phase569) and first-waypoint advancement (Phase568). This phase follows the original custom waypoint allocation pool and linked-list splice, not generic HL2 physics, props, model loading, pathfinding guesses or Android/Quest integration.

## Binary authenticity and method bounds

Original owner's **unmodified and private** `SE1/bin.zip` supplies both original DLLs. Before checking any native address, the auditor authenticates complete file hashes:

- Original 2006 `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` (SHA256).
- Original 2006 `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` (SHA256).
- Original `server.dll` PE32 image base: `0x22000000`.

| Original server VA | Authenticated complete bytes | Exact whole-method SHA256 | Evidence-bounded role |
| --- | ---: | --- | --- |
| **`0x220B2B10`** | **184** | `572d5112f9b4963c0ff9c6869ecb770f3e47ac4e32befd11f3f17a6ea31f2c94` | append/splice two waypoint chains, special matching-identifier reuse/free, reassign terminal flag |
| `0x220B2BD0` | 27 | `135f14e970afe12191b785d331ada22a7dac96d449d1d22ee34f13bd0371a93c` | invoke splice helper then replace holder first pointer |
| `0x220B2BF0` | 137 | `fcb01b8df91ea477cf9ff0fec1b47ab00c0c94c2f7b42f277ff7c488570266d4` | pool-allocate **0x30-byte waypoint**, initialize fields, splice with prior list, replace first pointer |
| **`0x220B2C80`** | **65** | `dcef9c71e6c6da6acab06761b58518ee4a274554c1644619a416a33c9b8fbd4b` | traverse from first, repair neighbor links and return **each node to original waypoint pool** |
| `0x220B2CD0` | 24 | `f7ddd3665e1027521969b2b09508780c27535cea6bb4b7095b40038e64c60a05` | wrapper accepting holder pointer, invoke chain disposal, then clear first pointer |

**Totals:** five nonoverlapping complete original function extents, **437 distinct native server code bytes**, seven direct original `E8 rel32` calls, one independent incoming cross-call from Phase569's native holder-clear wrapper, **40 targeted opcode/field witnesses**, five whole-function SHA256s, original method terminal RET and authentic subsequent `INT3` boundaries. A full function hash authenticates bytes not individually described by witnesses. Identifying a complete machine-code extent is **not** a claim that every branch has been translated into fully executable source-equivalent C++.

## `0x220B2C80`: actual forward-chain pool disposal

The code is definitive about the release order. With a null incoming first pointer it immediately returns. Otherwise it runs the following loop:

```cpp
// Pseudocode illustrating the source-bounded original native behavior.
void original_free_waypoint_chain(OriginalWaypoint* first) {
    for (OriginalWaypoint* cur = first; cur != nullptr; /* advanced below */) {
        OriginalWaypoint* next = cur->raw_forward_plus_0x28;
        if (next)
            next->raw_backward_plus_0x2C = cur->raw_backward_plus_0x2C;
        if (cur->raw_backward_plus_0x2C)
            cur->raw_backward_plus_0x2C->raw_forward_plus_0x28 = next;
        original_waypoint_pool_release_2226EF40(cur);
        cur = next;
    }
}
```

The native loop saves `next` **before** calling pool release, correctly avoiding a forward traversal through freed node data. It patches the `next.backward` and `previous.forward` links, then calls the **real game pool** at `0x2226EF40` with `ECX=0x2254AFF4`. The value is not a generic `delete` or `std::free` in the original. The host model provides a callback and does not actually free stack-owned regression objects.

The wrapper `0x220B2CD0` accepts a pointer to a first-node holder, calls `0x220B2C80(holder.first)`, and writes `holder.first = null`. Phase569's complete `0x220B2CF0` is a different calling convention/holder wrapper around this *same* core native disposal method. Thus the earlier route owner cleanup and new candidate construction really share the original waypoint node pool lifecycle.

Original graph handling is not robust to arbitrary invalid input, pointer cycles, inconsistent backward links, or allocator lifetime errors. The independent host reference rejects them with `UnsupportedGraph` rather than recreating native unsafe access.

## `0x220B2BF0`: authentic native 48-byte allocation and prepend

The original calls **`0x2226EE80(pool 0x2254AFF4, 0x30)`**, explicitly allocating a **0x30 / 48-byte** node. For successful allocation the code writes values corresponding to:

- Three initial vector components copied from an input vector.
- Raw `+0x0C=0`.
- Raw identity/parameter `+0x10=-1`, raw `+0x18=-1`, `+0x1C=-1`.
- Raw cached path length `+0x14=0xBF800000` (`-1.0f`).
- The supplied original numeric node kind `+0x24` and supplied flags `+0x20`.
- The original splice helper **`0x220B2B10(new_node, old_first)`**, followed by writing `holder.first = new_node`.

The node is a **new prefix** whose forward links can incorporate the existing old list. This matters: treating native allocation as an append-after-tail changes waypoint sequence order. The original allocation-failure path still attempts a native splice/holder update with a null new-node pointer; its exact failure handling is not promised to be safe. The host model refuses that case without fabricating success.

## `0x220B2B10`: identity-aware linked-list splice, not always append

The original function takes an introduced prefix list and another existing list; it walks to the end of the prefix and **clears terminal flag bit 8** of that last prefix node. It then reads the prefix last node's raw dword `+0x10` and performs a **signed `-1` sentinel check** followed by a comparison with the incoming list's raw `+0x10`.

- **Ordinary path:** if the prefix last node's ID is `-1` or does not match, it repairs the incoming list's previous link, assigns the incoming list to the prefix tail's `+0x28`, and sets the incoming first's `+0x2C` backlink. At completion it walks to the new final node and ORs terminal bit **`0x08`** into final flags `+0x20`. If there is no incoming list, it marks the prefix tail terminal instead.
- **Special matching-ID path:** original code unlinks the incoming node, calls pool release `0x2226EF40`, and splices an adjacent remaining segment into the prefix. The exact ownership and identity semantics for all graph topologies need deeper investigation. The native branch is *not* equivalent to plain append and is **not fully implemented** in the safe host reference.

The original wrapper `0x220B2BD0` calls that helper with a newly supplied prefix and the holder's old first, and **then** sets the holder to the supplied prefix. It does not by itself perform a fresh allocation.

The `0x220B2BF0` newly allocated prefix sets its ID `+0x10=-1`, which ordinarily selects the nonmatching path in the splice function. The original `0x220B2B10` can be used by other callers with nonnegative matching IDs, however, and must remain a separately documented semantic case in a source-equivalent port.

## Independent host reference, tests, provenance

- `include/sin_compat/original_grunt_waypoint_pool_phase570.hpp`: original-free, non-ABI, independently authored C++17 model. It includes a typed waypoint list, safe release callback, holder cleanup, the **supported nonmatching splice case**, and original 0x30-byte allocation field defaults. It returns `UnsupportedMatchedIdentity` instead of silently claiming to reproduce the special original matching-ID splice/free path.
- `tests/test_original_grunt_waypoint_pool_phase570.cpp`: tests ordered three-node first-to-last disposal callback, successor/predecessor repairs, pointer-holder clear, prepend order, terminal bit behavior, node field sentinels, identity-match refusal, null allocation refusal, and cycle rejection.
- **Original owner SHA audit PASS**, 5 full functions, 437 distinct bytes, 7 authentic direct calls, 1 crosscall, 40 instruction witnesses.
- Python original-free wrong-binary and mutation self-test **PASS**. Every byte of each witnessed original rel32 call is mutated independently and rejected.
- Strict GCC and Clang C++17 host reference **PASS**, warnings-as-errors; GCC ASan/UBSan **PASS**; standalone CMake/CTest **2/2 PASS**.

No original copyrighted DLL bytes, large disassembly dumps, game assets, proprietary original source or executable builds are committed. The original pool allocator's internal layout and exact matched-ID replacement remain unresolved. Source-level research and portable test code are distinct from an engine-loadable original DLL.

## Remaining source-first tasks

1. Recover the *actual allocator* behind pool `0x2254AFF4` and the original native implementations `0x2226EE80` / `0x2226EF40`, instead of guessing heap semantics.
2. Fully unwind the matched-ID branch in `0x220B2B10`, reproduce all original pointer/ownership cases with tests, and then integrate it into the route-promotion model.
3. Follow navigation virtual `+0x54` and Grunt event-1 producer to find the original generator of candidate flanking routes.
4. Continue prioritizing `server.dll` and `client.dll` gameplay code over generic HL2 systems and platform adaptation.