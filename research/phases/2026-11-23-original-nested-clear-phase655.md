# Phase 655: original nested map reset, block teardown and name-view lookup

## Why these routines

Phase 654 reconstructs original red-black insertion balancing. This phase
recovers three original complementary methods in the same 2006
*SiN Episodes: Emergence* `server.dll` nested symbol/name-map implementation:
**recycling the 12-byte slot array after a clear, releasing the owned
string blocks, and looking up an already-existing string name**.

No work is performed on the already functional Half-Life 2-equivalent
physics, material/surface properties, models, props or renderer.

The unchanged owner-provided Drive `SE1/bin.zip` files match:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

## Whole native method provenance

| Native VA | Role | Bytes | Exact entire method SHA256 |
|---|---|---:|---|
| `0x2228EC50` | reset tree and rebuild free list | 132 | `be7e1d74e6cc2924a085a28c953618d1e8b21ac5da98245cc55cfa93634c21c6` |
| `0x2228F400` | reset and free each owned name block | 49 | `a2421243cd3fb6f5bb6a352ef57826558f6768eeb197757074f3e2202d28ec78` |
| `0x2228F3C0` | resolve name to an existing tree handle | 61 | `3f52a0896437df27f02f59ad5b20abfeb4a53403cef8d38577bb274713b459b5` |

All **242 original x86 executable bytes** are included in the entire-body
authentication. `0x2228EC50` and `0x2228F400` both end with native
`RET`; `0x2228F3C0` ends in `RET 8`. Each original next byte is
`INT3`. A private deterministic audit checks both original PE fingerprints,
the exact three method hashes, these three terminators and next-byte padding,
**16 exact original rel32 call edges** (including 13 original callers from
other original server functions), and six pinned original short branches.
Only independently derived C++ and a synthetic-only public audit are published.

## Native semantics, directly from 2006 disassembly

### `0x2228EC50`: rebuild free list without freeing node storage

Native `__thiscall ECX=map`, no stack arguments. Read the next-unused
16-bit index from `map+0x16`, subtract one **after zero extending to DWORD**.

- If next-unused == 0, skip every node write **and** skip lazy sentinel
  initialization.
- Otherwise initialize shared sentinel `0x2259012C` only if its flag bit0
  at `0x22590134` is clear, preserving the flag's high bits. Sentinel
  `WORD +0/+2/+4=FFFF`, color `WORD+6=1`.
- Starting with the **highest allocated index**, descending to zero,
  write `WORD[node+2]=previous_index` first and then
  `WORD[node+0]=self_index`, using the 12-byte node array at `[map+4]`.
  The first previous index is `FFFF`. This creates an ascending reusable
  list `0 → 1 → 2 → ... → next_unused-1 → FFFF`.
- Only after the node loop set `WORD[map+0x10]=FFFF` (root),
  `WORD[map+0x12]=0` (active count), and
  `WORD[map+0x14]=(next_unused ? 0 : FFFF)` (free head).
- **Preserve** `WORD[map+0x16]` next-unused index, capacity, node array
  allocation, key references, node colors and parent links. The nodes are
  freed by a *logical free list*, not by returning heap memory.

The original calculates the free-head selection using `NEG AX`,
`SBB EAX,EAX`, `AND EAX,FFFF0001` and `ADD EAX,FFFF`; the resulting
low WORD is exactly zero if the next-unused count is nonzero, and `FFFF`
when zero.

### `0x2228F400`: original name-block teardown

Native `__thiscall ECX=map`:

1. Always call `0x2228EC50` first, even when the block count is zero.
2. For signed `DWORD[map+0x28]` block count greater than zero, iterate
   the block pointers in `DWORD[map+0x1C]`, invoking the original
   memory manager free wrapper `0x2226E3E0` for **every** entry
   in order, including zero pointers. The free-call wrapper itself is
   recovered in earlier phases, but the actual engine heap is still a typed
   host operation.
3. Set **only** `DWORD[map+0x28]=0` after the calls. The block *pointer
   array* is neither cleared nor freed, and the node array is retained.

Unlike generic C++ destructors, this cleanup is deliberately asymmetric.

### `0x2228F3C0`: original non-allocating name-view lookup

Native `__thiscall ECX=map`, stack `(WORD* output, const char* name)`,
`RET 8`; returns the output pointer in EAX.

- For null name, write `FFFF` through output and return immediately:
  no lookup, map fallback write or sentinel touch.
- Otherwise store `name` in `DWORD[map+0x30]`, convert the original
  mutable second stack-argument DWORD into the temporary `FFFF/FFFF`
  string reference, and call previously reconstructed `0x2228EB40`
  with that temporary and comparator context = map.
- Write the resulting AX handle, including `FFFF` for absent name,
  to output and return the output pointer.
- **Unlike `0x2228F7A0`** this wrapper never inserts a missing name,
  copies a string or allocates a block.

The portable C++ reference accepts `scratch_ref` as a guest memory address
instead of mutating the real host stack word; this is deliberate, documented
ABI adaptation. Rejecting null pointers, bad guest addresses, host callbacks
and dangerous loops is defensive host behavior, not claimed original x86 code.

## Tests and integration

The new standalone C++17 tests verify zero/nonzero next-unused behavior,
exact **descending WORD right-then-left write order**, preserved key/color/
parent words, repeated reset, high-bit-preserving lazy sentinel writes,
partial committed writes when a later host callback fails, null guest slots,
negative signed block count, individual ordered free calls **including a
null entry**, non-atomic failure, null-name fast path, name fallback
temporary and existing-name lookup dispatch.

The compiled **Phase650→652→653→654→655 integration test** inserts nine
distinct numeric-key test records through real reference components:
parent search, capacity growth/relocation (3→6→12), red-black linking
and balancing, and native-style two-direction search. It invokes the
new non-allocating view wrapper with the actual compiled Phase650 search
and a typed invented comparator; then clears and frees two name blocks,
checks the exact nine-node ascending free list and preserved allocations,
and **re-inserts 30,20,10** by recycling actual slots via Phase653, forcing
a Phase654 rotation without triggering any further capacity growth.

The numeric-key comparator used by this combined test is synthetic and
does **not** claim engine validation of the game's original string
comparison ABI. Windows and Linux CI verify host-compiled source models,
not direct retail x86 execution or a live Source engine.

Public invented-byte audit:

```sh
python research/tools/audit_original_nested_clear_phase655.py --self-test
```

Owner-private retail audit (never publish binaries):

```sh
python research/tools/audit_original_nested_clear_phase655.py \
  --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL
```

## Remaining game milestone

Connect the reconstructed naming/entity registry to an actual replacement
`server.dll` / `client.dll` interface loaded by the original Windows
Source engine; measure original behavior in real maps, then adapt only
the missing SiN game behavior to Android/Quest 3. Synthetics, PE32
`LoadLibrary` compatibility and Android Bionic `dlopen` do not demonstrate
playable SiN or original game-process ABI equivalence.
