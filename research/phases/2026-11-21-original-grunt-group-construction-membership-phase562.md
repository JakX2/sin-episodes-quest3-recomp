# Phase 562: Authentic original 2006 Grunt-linked group construction, join, leave and registry lifecycle

## Scope and original binary identity

This continues the SiN-specific **`CNPC_MobGrunt` owner `+0x9C4`** group work in Phases 559–561. The goal is to recover real server-side group membership and state changes before attempting replacement game-module or Android/Quest integration. It does not reimplement common Source physics, surface properties, model/prop loading, or rendering.

Owner-supplied unchanged original DLL pair, read only from the private `SE1/bin.zip`:

- `server.dll`: SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; PE32 base `0x22000000`.
- `client.dll`: SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

The auditor refuses other file identities before reading native method code. No proprietary binary or raw original disassembly is included in this repository.

## Original complete method extents

| Native server VA | Bytes | Whole-function SHA-256 | Recovered behavior |
| --- | ---: | --- | --- |
| `0x220AA6A0` | 99 | `838977b93fb2de5eaae51669e06519562ef484ecf7f0e9d783817be2cd2ff4b8` | Named group constructor; initializes member array, identifiers and timer/cache fields |
| `0x220AA710` | 90 | `7f9a809484db8d69b0213f9f3881beed3ee5abcf0844b457d04d25bffed62248` | Similar default group constructor with native default name |
| `0x220AA810` | 60 | `361b80733d736860c791353349994cd66cecc580f3813de6933b7b6706e6a7c6` | Allocate **0x78 bytes** for a group, construct, and optionally link it into a registry head |
| `0x220AA850` | 82 | `c68ad6c711e9bc018a03750f8c69fb8293e2ba6c547e780b82f735b8509bf90a` | Unlink a group from registry, release owned record storage and free the group allocation |
| **`0x220AA0D0`** | **219** | `39cc26906ddebebf3dcec5762cda698a71457dcd0140a493c51eade127776f65` | Remove member, optionally notify survivors, clear owner-group identity via owner virtual |
| **`0x220AA1B0`** | **273** | `26cbea4607e8daf71d0a6bc36920e34509b94e863aea48d2070215b5a3c3b061` | Add member to group, reject invalid state, handle previous group, enforce 16-entry path, notify callbacks |
| `0x220AA8C0` | 94 | `cf454aef73b3b7ceb1108ed5b60275ae142503b8c059d433f35644111e639a32` | Tree/index record insertion used by Phase561's bitmap refresh path |

**Seven new whole-method authentic x86 bodies totaling 917 distinct native bytes.** Additional original verification: **17 direct E8 CALL target pairs**, three prior-phase cross-edges and 33 short instruction/layout witnesses. Every method has verified native INT3 padding at its exact end. Previous methods and unmodified commercial binaries are not double counted or redistributed.

## Group allocation and original x86 class layout

The genuine original `0x220AA810` pushes literal `0x78` to allocator `0x2226E790`, then, if allocation succeeds, calls the original constructor `0x220AA6A0`. Thus this original class allocation is **120 bytes** in the 2006 x86 module, not a suggested host struct size in C++17 or ARM64.

The original named/default constructors establish these directly observed fields:

| Group field | Original initial value / role |
| --- | --- |
| `+0x04` | Native interned name or identity returned via `0x221F3DE0` |
| `+0x08` | Beginning of inline 32-bit member-handle collection |
| `+0x48` | Member count = 0 |
| `+0x58` | Unused index/handle marker = `0xFFFFFFFF` |
| `+0x5C` | Cached selected record pointer = 0 |
| `+0x60`, `+0x64`, `+0x68`, `+0x6C`, `+0x70` | Owned dynamic-record metadata = 0 |
| `+0x74` | Cache refresh deadline float = 0.0 |

The actual array insertion and element handle encoding still depend on previously traced group helper `0x220A9790`; **these offsets should not be memcpy'd as an object into a newer Source engine or Android**.

The original unregister/destruction `0x220AA850` follows the registry's linked list, unlinks a target group, calls `0x220A98B0` on the owned records at `group+0x60`, clears embedded collection state, and invokes the native group free helper `0x2226E800`. It does not prove how later engine lifecycle exceptions or concurrent callers would behave.

## Native member join: `0x220AA1B0`

This function receives a group as x86 `this` and a candidate owner entity pointer.

1. **Rejects a null owner** without modifying group storage.
2. **Rejects nonzero owner `+0x8C`** before joining; no symbolic bit meaning is invented.
3. Reads owner `+0x9C4`: if it already points to this group, return; if it points to another group, invokes original `0x220AA0D0` to detach the previous association with notification mode zero.
4. Checks original member count at group `+0x48` against **exactly 16 (`0x10`)**. At 16 it issues a native diagnostic with the group identity and invokes removal helper `0x220A8670` for the last array slot before proceeding. This is **not proof** that the owner object evicted this way gets the same notifications as a normal leave, since the helper's broader side effects are not recovered here.
5. Resolves the joining owner's original identity/handle via its virtual `+0x08`, and inserts that handle into group `+0x08` using original `0x220A9790`.
6. Invokes owner virtual **`+0x690`** with a group pointer, then writes original group identity at group `+0x04` into **owner `+0x9C8`**.
7. When the group member count is greater than one, follows an original related-entity traversal (`0x22060CC0`, owner virtual `+0x670`, `0x2206B7B0`, and `0x2206B130`) and dispatches owner virtual **`+0x678`**. The exact engine-side semantic names for these virtuals are not established; they are not simply renamed to familiar HL2 squad signals.

This changes actual NPC group membership, which in turn affects the linked group checked by Phase560's Grunt `0x22054D40` condition admission and the 30-second cache refresh from Phase561.

## Native member leave: `0x220AA0D0`

This function is called directly by Phase560 Grunt owner cleanup `0x22054D20 -> 0x220AA0D0` and by the join method when moving between groups.

1. Null candidate returns without changes.
2. Convert owner identity through native virtual `+0x08`, search the collection at group `+0x08` with native `0x220A97E0`.
3. Missing member produces an original diagnostic and returns. The function **does not** proceed with the normal owner-clear path after this miss.
4. Found member is removed from the fixed inline array by moving subsequent 32-bit handles, decrementing group member count. This order-preserving compaction is visible in the original byte instructions.
5. With the **optional nonzero notification flag**, iterate the remaining members by generation-checked handles; where present, invoke survivor virtual **`+0x750`** with the removed owner pointer. The ordinary Grunt destructor path passes zero for this flag, and must not be modeled as notifying survivors unconditionally.
6. Invoke removed owner's virtual **`+0x690`** with a null group pointer; clear owner field **`+0x9C8`** to zero.

The nearby Grunt helper at `0x22054D10` additionally clears owner **`+0x9C4`** after calling this leave routine. That two-level cleanup is why simply wiping the linked pointer at the start is not source equivalent.

## Record-index insertion context `0x220AA8C0`

Original refresh in Phase561 at `0x220AA9E0` calls exact helper `0x220AA8C0`, which is now authenticated as a complete 94-byte method. It calls 0x220A8A30, obtains an original 16-bit node index from 0x220A8BA0, passes metadata through 0x220AA350, increments original 16-bit field `+0x12`, then stores selected record data through a 12-byte-node layout.

Its entire native body is proven, but the full source-level semantics of underlying tree search/allocators are still unresolved. It is not misleadingly modeled as a generic `std::map` in the independent host code.

## Bounded C++17 host reconstruction

`include/sin_compat/original_grunt_group_lifecycle_phase562.hpp` defines a separate portable host-only group representation with the authenticated 120-byte allocation/16-member metadata as **constants**, not matching memory layout. The reference explicitly models:

- Named-group reuse or creation and no-live-member destruction policy (host-only safety);
- Rejected null/owner-state owner, and duplicate owner/group checks;
- Previous group detach before new join;
- Original fixed-member-limit path at 16;
- Owner virtual `+0x690` notifications in recorded order, owner identity field update and conditional multiple-member follow-up;
- Normal leave, order-preserving compaction, and optional survivor `+0x750` notifications.

**Crucial differences:** The host's forced-last-member eviction clears typed synthetic owner links for memory safety, which is *not attributed to original* `0x220A8670`. Likewise the host aborts on an inconsistent previous group reference rather than continuing with undefined native memory assumptions. The original related-entity graph and class virtuals remain callback abstractions, not an engine ABI.

### Reproduction

```bash
python3 research/tools/audit_sin_original_grunt_group_lifecycle_phase562.py --self-test
python3 research/tools/audit_sin_original_grunt_group_lifecycle_phase562.py --bin-zip /owner/private/bin.zip

c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude \
  tests/test_original_grunt_group_lifecycle_phase562.cpp -o /tmp/sin_group_lifecycle
/tmp/sin_group_lifecycle
```

Owner-private authentication **PASS**, 7 original method digests, 917 original server bytes, 17 verified direct call edges, 3 previous-phase xrefs and 33 opcode witnesses. Synthetic corruption selftest PASS (mutations of every E8 opcode/rel32 byte rejected, unsupported whole-file hashes rejected). Strict GCC and Clang C++17 PASS, GCC ASan/UBSan PASS, standalone CTest **2/2 PASS**.

## Next targets

1. Examine the parent/owner virtual **`+0x690`** to determine exactly where Grunt `+0x9C4` gets written and cleared and to connect the native group destructor semantics.
2. Identify group class name/RTTI rather than assigning an unproven Source SDK class.
3. Recover `0x220A9790`, `0x220A97E0`, and the fixed 16-slot collection helpers; verify handle uniqueness and forced eviction semantics.
4. Trace exact predicate context of member notification virtuals `+0x750`/`+0x678` and the action that follows grouping.
5. Connect Grunt schedule IDs to the original scheduler's registration tables and complete Phase559's late branch translation.

The original commercial DLL remains private. This phase is code recovery, not playable Windows/Android/Quest integration.