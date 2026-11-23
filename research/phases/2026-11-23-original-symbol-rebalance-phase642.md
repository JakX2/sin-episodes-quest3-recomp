# Phase 642: source-authenticated original SiN red-black symbol-tree insertion fixup

## Objective and private retail provenance

Continue the original 2006 SiN Episodes: Emergence gameplay DLL symbol-registration path, **without duplicating Half-Life 2 physics, materials, surface properties, models, props, or rendering** (the owner's prior priority override). Previous source-authenticated stages:

- Phase 638: symbol forwarder `0x22282370`.
- Phase 639: symbol registrar `0x22282270` and slot acquisition `0x222810E0`.
- Phase 640: symbol-tree link `0x22281F30`.
- Phase 641: original left/right rotations `0x22281180`, `0x22281270`, plus sentinel-aware accessor `0x22280D40`.
- **Phase 642: entire original red-black insertion fixup `0x22281840`.**

Owner-supplied genuine original retail 2006 DLLs, not redistributable in repo:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

**Complete original method authenticated: 1,482 executable x86 bytes from VA `0x22281840`, original SHA256 `4ef6465536a6a442a842531f09f0d8f0a37f274be6403d540528b24059527ede`.** The method terminates with original `RET 4` at `0x22281E07`, followed by `INT3` padding. Native call convention is `__thiscall ECX=table` with one 32-bit slot-index argument, stored/compared as 16-bit `DI`.

The independent owner-only retail audit verifies both whole original DLLs, the **entire 1,482-byte body**, **four exact original rel32 rotation call sites**, **ten exact conditional/unconditional control-flow edges**, and **99 separate original opcode witnesses**, including root short-circuit, sentinel reads, the mirrored family branches, 16-bit color updates, the iteration backedge and final root blackening. Public CI --self-test checks invented bytes and fails closed if even one method byte or branch displacement changes.

## Decompiled structure at source level

Original fields, 16-bit little-endian WORDs, for each 16-byte slot record:
- `+0`: left child index
- `+2`: right child index
- `+4`: parent index
- `+6`: node color, **0=red / 1=black**
- remaining `+8..+15`: separate symbol token and mapped-ID data from Phases 638–639

Tree metadata:
- `table+0x04` fresh slot array pointer
- `table+0x10` root WORD index
- `0xFFFF` index selects original sentinel at absolute VA `0x22589138`
- shared sentinel initialization flag DWORD at `0x22589140`, bit 0.

**Native red-black insert fixup** (not generic deletion/rebalancing):

1. `0x22281848`: compare original low 16-bit input index to the root; if equal, jump directly to final root-blackening without initializing sentinel.
2. Otherwise enter the fixup loop. The original inlines the sentinel initialization/lookup sequence many times. Read the current node's parent and that parent's color. Stop when parent is black.
3. If parent is the **left** child of grandparent, uncle is grandparent.right. If uncle is red, change parent/uncle to black, grandparent to red, and retry from grandparent. Otherwise, if current is the parent's right child, first left-rotate parent, then recolor refreshed parent black, refreshed grandparent red and right-rotate grandparent.
4. If parent is the **right** child of grandparent, mirror the above: uncle is grandparent.left; red uncle recolors and moves up; black uncle optionally right-rotates parent before recoloring and left-rotating grandparent.
5. Repeat against the fresh root index until the loop exits.
6. Finally write **WORD black `1`** to current `[slot_array + 16*uint16(root) + 6]` at `0x22281E01`.

Four authenticated original rotation sites:
| Call VA | Original destination | Meaning |
|---|---|---|
| `0x22281B47` | `0x22281180` | left rotation of parent in triangle case |
| `0x22281BFF` | `0x22281270` | right rotation of grandparent in left-family straight fixup |
| `0x22281D23` | `0x22281270` | right rotation of parent in mirrored triangle |
| `0x22281DDB` | `0x22281180` | left rotation of grandparent in mirrored straight fixup |

**Important distinction:** the original code has repeated inlined sentinel initialization checks and register-dependent in-place read sequences. The portable source captures the **structural branch behavior** and original relevant field widths and side effects but delegates sentinel reads to the recovered Phase641 accessor. Therefore a complete *byte-for-byte equivalent trace of every redundant machine read* is **not claimed**. It also guards null/unmapped addresses, self-reference cycles (up to 65,536 steps), exceptions and callback reentrancy in the portable memory model, none of which are added behavior of the retail x86 function. The specific error/provenance `Result` structures are host-side instrumentation, not an original engine ABI.

## C++ composition and executable synthetic tests

New `Rebalance` class uses the actual compiled `Phase641::SentinelSlotAccessor` and `Phase641::RotateNode`, and integrates with the **actual compiled Phase640 LinkNode** through its original `rebalance_original_22281840` callback.

Tests exercise:
- Root insertion initial short circuit: blackens root without touching uninitialized sentinel globals.
- Mirrored triangle fixups requiring **two rotations** in each direction, plus both straight fixups.
- Left/right **red-uncle recoloring** without rotation.
- Continuous BST insertion with validation after **every insertion** of root-black, parent back-links, left/right BST ordering, red-parent restrictions, equal black height, no cycles, and exact visited-node count.
- Increasing, decreasing, ordered interleaved inputs, and **12 deterministic shuffled permutations of 127 unique keys**.
- Refusal on null table/slot array, injected write fault, thrown host exception, callback reentrancy, and deliberately corrupted cyclic tree.
- Local standalone C++17 compile/CTest and AddressSanitizer/UndefinedBehaviorSanitizer pass.

Synthetic test memory, IDs and tree keys are **fabricated**; they are not original retail gameplay traces. A green CTest or Android `dlopen` smoke does **not** prove a playable Windows binary replacement, Android in-level engine or Quest 3 game.

## Owner-only audit

`python research/tools/audit_original_symbol_rebalance_phase642.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

Public invented-byte self-test:

`python research/tools/audit_original_symbol_rebalance_phase642.py --self-test`

Original executable binaries remain owner-private.

## Next real engine gaps

Phase 642 closes the local registration **tree rebalance** reference from Phase640+641. The earlier Phase639 symbol-registration path still has two original opaque engine callbacks that must be addressed before actual source-faithful symbol-table integration:

- **`0x22280FB0`**: ~255-byte enumeration and stack argument rewriting.
- **`0x22280DD0`**: original growth/capacity helper, with subordinate dependencies.

Additional missing symbol/entity/NPC game logic and actual integration into original engine calls remain. Continue this original gameplay research, rather than revisiting HL2-equivalent physics/materials/models/rendering.
